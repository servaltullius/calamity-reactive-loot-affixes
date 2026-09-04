#include "CalamityAffixes/DeferredEventDispatcher.h"
#include "CalamityAffixes/CombatRuntimeState.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
	using Dispatcher = CalamityAffixes::detail::DeferredEventDispatcher;
	using Generation = Dispatcher::Generation;
	using namespace std::chrono_literals;

	[[noreturn]] void TimedOut(const char* a_context)
	{
		std::cerr << "deferred_event_dispatcher: timeout: " << a_context << std::endl;
		// A broken lock order must fail, not hang while future/thread destructors
		// try to join the intentionally blocked regression threads.
		std::_Exit(EXIT_FAILURE);
	}

	void Check(bool a_condition, const char* a_message)
	{
		if (!a_condition) {
			throw std::runtime_error(a_message);
		}
	}

	template <class T>
	T Await(std::future<T>& a_future, const char* a_context)
	{
		if (a_future.wait_for(5s) != std::future_status::ready) {
			TimedOut(a_context);
		}
		return a_future.get();
	}

	class Gate
	{
	public:
		Gate() : _future(_promise.get_future().share()) {}
		void Open() { _promise.set_value(); }
		void Wait(const char* a_context) const
		{
			if (_future.wait_for(5s) != std::future_status::ready) {
				TimedOut(a_context);
			}
		}

	private:
		std::promise<void> _promise;
		std::shared_future<void> _future;
	};

	class TaskQueue
	{
	public:
		bool operator()(Generation a_generation)
		{
			++attempts;
			if (!accept.load()) {
				return false;
			}
			const std::scoped_lock lock(_mutex);
			_tasks.push_back(a_generation);
			return true;
		}

		std::optional<Generation> Pop()
		{
			const std::scoped_lock lock(_mutex);
			if (_tasks.empty()) {
				return std::nullopt;
			}
			const auto next = _tasks.front();
			_tasks.pop_front();
			return next;
		}

		std::size_t Size()
		{
			const std::scoped_lock lock(_mutex);
			return _tasks.size();
		}

		std::atomic_bool accept{ true };
		std::atomic_uint attempts{ 0u };

	private:
		std::mutex _mutex;
		std::deque<Generation> _tasks;
	};

	template <class D = Dispatcher, class F>
	std::unique_ptr<typename D::Work> MakeWork(F&& a_work)
	{
		return std::make_unique<typename D::Work>(std::forward<F>(a_work));
	}

	template <class D>
	void QueueContended(
		D& a_dispatcher,
		typename D::StateMutex& a_state,
		TaskQueue& a_tasks,
		std::unique_ptr<typename D::Work> a_work)
	{
		typename D::StateLock lock(a_state);
		const auto generation = a_dispatcher.CaptureGeneration();
		auto producer = std::async(std::launch::async, [&, work = std::move(a_work)]() mutable {
			return a_dispatcher.Submit(generation, std::move(work), a_state, a_tasks);
		});
		Check(Await(producer, "contended ingress waited for state"), "current work was rejected");
	}

	template <class D>
	void DrainAll(D& a_dispatcher, typename D::StateMutex& a_state, TaskQueue& a_tasks)
	{
		for (std::size_t count = 0u; count < 10000u; ++count) {
			const auto next = a_tasks.Pop();
			if (!next) {
				return;
			}
			a_dispatcher.Drain(*next, a_state, a_tasks);
		}
		throw std::runtime_error("drain never became idle");
	}

	void CheckImmediateAndRecursiveDispatch()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		std::vector<int> order;
		const auto generation = dispatcher.CaptureGeneration();
		Check(dispatcher.Submit(generation, MakeWork([&](Dispatcher::StateLock& lock) {
			Check(lock.owns_lock(), "immediate work did not own state");
			order.push_back(1);
			Check(dispatcher.Submit(generation, MakeWork([&](Dispatcher::StateLock& nestedLock) {
				Check(nestedLock.owns_lock(), "recursive work did not own state");
				order.push_back(2);
			}), state, tasks), "recursive work was rejected");
			order.push_back(3);
		}), state, tasks), "immediate work was rejected");
		Check(order == std::vector<int>{ 1, 2, 3 }, "uncontended recursion became deferred");
		Check(tasks.Size() == 0u, "uncontended work unnecessarily scheduled a task");

		{
			Dispatcher::StateLock lock(state);
			dispatcher.Invalidate();
		}
		Check(!dispatcher.Submit(generation, MakeWork([&](auto&) { order.push_back(4); }), state, tasks),
			"stale submission was admitted");
		Check(order.size() == 3u, "stale submission ran");
	}

	void CheckReportedLockInversion()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		std::mutex engine;
		Gate engineHeld;
		Gate stateHeld;
		std::atomic_uint ran{ 0u };
		const auto generation = dispatcher.CaptureGeneration();

		auto damageThread = std::async(std::launch::async, [&] {
			Dispatcher::StateLock lock(state);
			stateHeld.Open();
			engineHeld.Wait("engine lock was not acquired");
			const std::scoped_lock engineLock(engine);
		});
		auto eventThread = std::async(std::launch::async, [&] {
			const std::scoped_lock engineLock(engine);
			engineHeld.Open();
			stateHeld.Wait("health-damage state lock was not acquired");
			Check(dispatcher.Submit(generation, MakeWork([&](auto&) { ++ran; }), state, tasks),
				"engine-lock callback was rejected");
			Check(ran == 0u, "contended callback executed under the engine lock");
		});
		Await(eventThread, "engine-lock callback blocked on the state owner (AB-BA)");
		Await(damageThread, "state owner never acquired the released engine lock");
		DrainAll(dispatcher, state, tasks);
		Check(ran == 1u, "contended event was lost or duplicated");
	}

	void CheckDrainDoesNotHoldQueueAcrossEngineCalls()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		std::mutex engine;
		Gate engineHeld;
		Gate workEntered;
		std::atomic_uint ran{ 0u };
		const auto generation = dispatcher.CaptureGeneration();
		QueueContended(dispatcher, state, tasks, MakeWork([&](auto&) {
			workEntered.Open();
			engineHeld.Wait("drain test engine lock was not acquired");
			const std::scoped_lock lock(engine);
			++ran;
		}));
		const auto task = tasks.Pop();
		Check(task.has_value(), "contended work was not scheduled");

		auto eventThread = std::async(std::launch::async, [&] {
			const std::scoped_lock engineLock(engine);
			engineHeld.Open();
			workEntered.Wait("drain work never began");
			Check(dispatcher.Submit(generation, MakeWork([&](auto&) { ++ran; }), state, tasks),
				"event during drain was rejected");
		});
		auto drainThread = std::async(std::launch::async, [&] { dispatcher.Drain(*task, state, tasks); });
		Await(eventThread, "drain held its queue mutex while waiting for the engine");
		Await(drainThread, "drain failed to resume after event-lock callback returned");
		DrainAll(dispatcher, state, tasks);
		Check(ran == 2u, "enqueue during drain was lost or duplicated");
	}

	void CheckFifoAcrossTerminalUnlockAndReentry()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		Gate inlineUnlocked;
		Gate finishInline;
		std::vector<int> order;
		const auto generation = dispatcher.CaptureGeneration();

		auto first = std::async(std::launch::async, [&] {
			return dispatcher.Submit(generation, MakeWork([&](Dispatcher::StateLock& lock) {
				order.push_back(1);
				lock.unlock();
				inlineUnlocked.Open();
				finishInline.Wait("terminal inline work was not released");
			}), state, tasks);
		});
		inlineUnlocked.Wait("inline work did not unlock state");
		Check(dispatcher.Submit(generation, MakeWork([&](auto&) {
			order.push_back(2);
			Check(dispatcher.Submit(generation, MakeWork([&](auto&) { order.push_back(4); }), state, tasks),
				"queued reentry was rejected");
		}), state, tasks), "second event was rejected");
		Check(dispatcher.Submit(generation, MakeWork([&](auto&) { order.push_back(3); }), state, tasks),
			"third event was rejected");
		Check(order == std::vector<int>{ 1 }, "new thread overtook terminal-unlocked inline work");
		Check(tasks.Size() == 0u, "queue was scheduled before outer inline work finished");
		finishInline.Open();
		Check(Await(first, "inline work never completed"), "first event was rejected");
		DrainAll(dispatcher, state, tasks);
		Check(order == std::vector<int>{ 1, 2, 3, 4 }, "in-flight or recursive events overtook the FIFO");
	}

	void CheckBoundedBatchAndDuplicateDrain()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		const auto generation = dispatcher.CaptureGeneration();
		std::vector<unsigned> order;
		{
			Dispatcher::StateLock lock(state);
			auto producer = std::async(std::launch::async, [&] {
				for (unsigned i = 0u; i < 300u; ++i) {
					Check(dispatcher.Submit(generation, MakeWork([&, i](Dispatcher::StateLock& eventLock) {
						order.push_back(i);
						if (i == 0u) {
							eventLock.unlock();
							// A duplicate scheduled callback must not start a second
							// batch while the first batch is still in progress.
							dispatcher.Drain(generation, state, tasks);
						}
					}), state, tasks), "batch event was rejected");
				}
			});
			Await(producer, "batch producer blocked on state");
		}
		Check(tasks.Size() == 1u, "queue scheduled duplicate initial tasks");
		dispatcher.Drain(*tasks.Pop(), state, tasks);
		Check(order.size() == Dispatcher::kMaxBatchSize, "drain exceeded or lost its bounded batch");
		Check(tasks.Size() == 1u, "remaining batch did not have exactly one continuation");
		Check(dispatcher.Submit(generation, MakeWork([&](auto&) { order.push_back(300u); }), state, tasks),
			"post-batch event was rejected");
		Check(order.size() == Dispatcher::kMaxBatchSize, "new submission overtook a scheduled continuation");
		DrainAll(dispatcher, state, tasks);
		Check(order.size() == 301u, "batch processing lost or duplicated an event");
		for (unsigned i = 0u; i < order.size(); ++i) {
			Check(order[i] == i, "FIFO order changed across a batch boundary");
		}
	}

	void CheckConcurrentExactlyOnce()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		const auto generation = dispatcher.CaptureGeneration();
		constexpr unsigned kProducers = 8u;
		constexpr unsigned kPerProducer = 150u;
		std::vector<std::atomic_uint> observed(kProducers * kPerProducer);
		std::vector<std::future<void>> producers;
		{
			Dispatcher::StateLock lock(state);
			for (unsigned producer = 0u; producer < kProducers; ++producer) {
				producers.push_back(std::async(std::launch::async, [&, producer] {
					for (unsigned i = 0u; i < kPerProducer; ++i) {
						const auto id = producer * kPerProducer + i;
						Check(dispatcher.Submit(generation, MakeWork([&, id](auto&) { ++observed[id]; }), state, tasks),
							"concurrent event was rejected");
					}
				}));
			}
			for (auto& producer : producers) {
				Await(producer, "concurrent producer blocked on state");
			}
		}
		Check(tasks.Size() == 1u, "concurrent producers scheduled duplicate tasks");
		DrainAll(dispatcher, state, tasks);
		for (const auto& count : observed) {
			Check(count == 1u, "concurrent FIFO lost or duplicated a snapshot");
		}
	}

	void CheckPayloadLifetimeAndReentrantDestruction()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		unsigned destroyed = 0u;
		unsigned ran = 0u;
		unsigned teardownEvents = 0u;
		auto makeSnapshot = [&] {
			return std::shared_ptr<int>(new int(7), [&](int* value) {
				delete value;
				++destroyed;
				Check(dispatcher.Submit(dispatcher.CaptureGeneration(), MakeWork([&](auto&) { ++teardownEvents; }), state, tasks),
					"payload teardown reentry was rejected");
			});
		};

		auto snapshot = makeSnapshot();
		std::weak_ptr<int> weak = snapshot;
		QueueContended(dispatcher, state, tasks, MakeWork([&, snapshot = std::move(snapshot)](auto&) { ran += *snapshot; }));
		Check(!weak.expired() && destroyed == 0u, "queued snapshot was not owned by the dispatcher");
		{
			Dispatcher::StateLock lock(state);
			dispatcher.Invalidate();
		}
		Check(weak.expired() && destroyed == 1u, "invalidate retained a retired snapshot");
		Check(ran == 0u && teardownEvents == 1u, "retired snapshot executed or teardown could not reenter");
		DrainAll(dispatcher, state, tasks);

		snapshot = makeSnapshot();
		weak = snapshot;
		QueueContended(dispatcher, state, tasks, MakeWork([&, snapshot = std::move(snapshot)](auto&) { ran += *snapshot; }));
		DrainAll(dispatcher, state, tasks);
		Check(weak.expired() && destroyed == 2u && ran == 7u && teardownEvents == 2u,
			"drained snapshot lifetime or reentrant teardown regressed");
	}

	class ObservedRecursiveMutex
	{
	public:
		void lock()
		{
			if (beforeBlockingLock) {
				beforeBlockingLock();
			}
			_mutex.lock();
		}
		bool try_lock() { return _mutex.try_lock(); }
		void unlock() { _mutex.unlock(); }
		std::function<void()> beforeBlockingLock;

	private:
		std::recursive_mutex _mutex;
	};

	void CheckGenerationAfterStateLockWait()
	{
		using D = CalamityAffixes::detail::BasicDeferredEventDispatcher<ObservedRecursiveMutex>;
		D dispatcher;
		D::StateMutex state;
		TaskQueue tasks;
		unsigned ran = 0u;
		QueueContended(dispatcher, state, tasks, MakeWork<D>([&](auto&) { ++ran; }));
		const auto generation = *tasks.Pop();
		D::StateLock heldState(state);
		Gate drainTryingState;
		state.beforeBlockingLock = [&] { drainTryingState.Open(); };
		auto drain = std::async(std::launch::async, [&] { dispatcher.Drain(generation, state, tasks); });
		drainTryingState.Wait("drain never attempted its blocking state lock");
		// The old task has separated its batch and reached lock(), but this
		// thread still owns state. Any epoch check before lock() is now stale.
		dispatcher.Invalidate();
		heldState.unlock();
		Await(drain, "invalidated drain did not finish after state was released");
		Check(ran == 0u, "epoch was not checked after acquiring state");
	}

	void CheckStaleTaskDoesNotDisturbNewGeneration()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		std::vector<int> order;
		QueueContended(dispatcher, state, tasks, MakeWork([&](auto&) { order.push_back(0); }));
		const auto oldTask = *tasks.Pop();
		{
			Dispatcher::StateLock lock(state);
			dispatcher.Invalidate();
		}
		QueueContended(dispatcher, state, tasks, MakeWork([&](auto&) { order.push_back(1); }));
		const auto newTask = *tasks.Pop();
		dispatcher.Drain(oldTask, state, tasks);
		Check(dispatcher.Submit(dispatcher.CaptureGeneration(), MakeWork([&](auto&) { order.push_back(2); }), state, tasks),
			"new-generation event was rejected");
		Check(order.empty() && tasks.Size() == 0u, "stale task reset the new generation's scheduled claim");
		dispatcher.Drain(newTask, state, tasks);
		Check(order == std::vector<int>{ 1, 2 }, "stale work replayed or new generation FIFO changed");
	}

	void CheckSchedulerFailureRecovery()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		std::vector<int> order;
		tasks.accept = false;
		QueueContended(dispatcher, state, tasks, MakeWork([&](auto&) { order.push_back(1); }));
		Check(tasks.Size() == 0u && tasks.attempts == 1u, "scheduler failure was not reported");
		tasks.accept = true;
		Check(dispatcher.Submit(dispatcher.CaptureGeneration(), MakeWork([&](auto&) { order.push_back(2); }), state, tasks),
			"retry submission was rejected");
		Check(order.empty(), "scheduler failure allowed later work to overtake retained work");
		Check(tasks.Size() == 1u && tasks.attempts == 2u, "later event did not retry failed scheduling");
		DrainAll(dispatcher, state, tasks);
		Check(order == std::vector<int>{ 1, 2 }, "scheduler recovery lost or reordered events");
	}

	void CheckExceptionPreservesPendingWork()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		std::vector<int> order;
		const auto generation = dispatcher.CaptureGeneration();
		QueueContended(dispatcher, state, tasks, MakeWork([&](auto&) {
			order.push_back(1);
			Check(dispatcher.Submit(generation, MakeWork([&](auto&) { order.push_back(3); }), state, tasks),
				"exception-path reentry was rejected");
			throw std::runtime_error("intentional work failure");
		}));
		Check(dispatcher.Submit(generation, MakeWork([&](auto&) { order.push_back(2); }), state, tasks),
			"second exception-path event was rejected");
		bool threw = false;
		try {
			dispatcher.Drain(*tasks.Pop(), state, tasks);
		} catch (const std::runtime_error&) {
			threw = true;
		}
		Check(threw, "work exception was hidden");
		DrainAll(dispatcher, state, tasks);
		Check(order == std::vector<int>{ 1, 2, 3 }, "exception lost or reordered unstarted work");
	}

	void CheckQueuedProcOriginIsRestoredWithoutContaminatingOtherThreads()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		CalamityAffixes::CombatRuntimeState combat;
		unsigned observations = 0u;
		unsigned suppressed = 0u;
		unsigned allowed = 0u;
		const auto generation = dispatcher.CaptureGeneration();
		auto workForOrigin = [&](bool a_procOrigin) {
			return MakeWork([&, a_procOrigin](auto&) {
				std::optional<CalamityAffixes::ScopedProcDepth> originGuard;
				if (a_procOrigin && !CalamityAffixes::ScopedProcDepth::IsActiveOnCurrentThread(combat)) {
					originGuard.emplace(combat);
				}
				++observations;
				if (combat.procDepth.load() > 0u) {
					++suppressed;
				} else {
					++allowed;
				}
			});
		};
		{
			Dispatcher::StateLock lock(state);
			const CalamityAffixes::ScopedProcDepth originalProc(combat);
			// This callback is on another thread while the shared depth is 1.
			// It creates the backlog, but must not inherit that other thread's
			// origin and suppress its own legitimate DoT proc when drained.
			auto independentEvent = std::async(std::launch::async, [&] {
				const bool origin = CalamityAffixes::ScopedProcDepth::IsActiveOnCurrentThread(combat);
				Check(!origin && combat.procDepth.load() == 1u, "proc origin leaked to another thread");
				return dispatcher.Submit(generation, workForOrigin(origin), state, tasks);
			});
			Check(Await(independentEvent, "independent proc event blocked"), "independent event was rejected");
			const bool ownOrigin = CalamityAffixes::ScopedProcDepth::IsActiveOnCurrentThread(combat);
			Check(ownOrigin, "same-thread proc origin was not captured");
			Check(dispatcher.Submit(generation, workForOrigin(ownOrigin), state, tasks), "own proc event was rejected");
		}
		Check(combat.procDepth.load() == 0u, "original proc depth leaked before deferred execution");
		DrainAll(dispatcher, state, tasks);
		Check(observations == 2u && suppressed == 1u && allowed == 1u,
			"queued origin lost observation or created/suppressed the wrong proc");
		Check(combat.procDepth.load() == 0u && !CalamityAffixes::ScopedProcDepth::IsActiveOnCurrentThread(combat),
			"restored proc origin leaked after draining");
	}

	void CheckTerminalUnlockDropsRestoredProcGuardBeforeReset()
	{
		Dispatcher dispatcher;
		Dispatcher::StateMutex state;
		TaskQueue tasks;
		CalamityAffixes::CombatRuntimeState combat;
		Gate terminalUnlocked;
		Gate nextGenerationActive;
		Gate finishNextGeneration;
		QueueContended(dispatcher, state, tasks, MakeWork([&](Dispatcher::StateLock& lock) {
			std::optional<CalamityAffixes::ScopedProcDepth> restoredOrigin;
			restoredOrigin.emplace(combat);
			Check(combat.procDepth.load() == 1u, "restored terminal proc guard did not activate");
			// Death's terminal engine notification releases state. Its restored
			// guard must end first, so an old destructor cannot decrement a new
			// generation's guard after another thread resets the state.
			restoredOrigin.reset();
			lock.unlock();
			terminalUnlocked.Open();
			nextGenerationActive.Wait("reset did not begin during terminal engine work");
			Check(!CalamityAffixes::ScopedProcDepth::IsActiveOnCurrentThread(combat),
				"terminal engine work retained its old thread-local guard");
		}));
		const auto task = *tasks.Pop();
		auto resetThread = std::async(std::launch::async, [&] {
			terminalUnlocked.Wait("terminal event did not release state");
			Dispatcher::StateLock lock(state);
			dispatcher.Invalidate();
			combat.ResetTransientState();
			const CalamityAffixes::ScopedProcDepth newGenerationGuard(combat);
			nextGenerationActive.Open();
			finishNextGeneration.Wait("old drain did not finish while new generation was active");
			Check(combat.procDepth.load() == 1u, "old terminal guard decremented the new generation's depth");
		});
		auto drainThread = std::async(std::launch::async, [&] { dispatcher.Drain(task, state, tasks); });
		Await(drainThread, "terminal-unlocked old drain did not complete");
		Check(combat.procDepth.load() == 1u, "old work altered a new generation guard");
		finishNextGeneration.Open();
		Await(resetThread, "reset thread could not finish its new generation scope");
		Check(combat.procDepth.load() == 0u, "new generation guard did not unwind");
	}
}

int main()
{
	try {
		CheckImmediateAndRecursiveDispatch();
		CheckReportedLockInversion();
		CheckDrainDoesNotHoldQueueAcrossEngineCalls();
		CheckFifoAcrossTerminalUnlockAndReentry();
		CheckBoundedBatchAndDuplicateDrain();
		CheckConcurrentExactlyOnce();
		CheckPayloadLifetimeAndReentrantDestruction();
		CheckGenerationAfterStateLockWait();
		CheckStaleTaskDoesNotDisturbNewGeneration();
		CheckSchedulerFailureRecovery();
		CheckExceptionPreservesPendingWork();
		CheckQueuedProcOriginIsRestoredWithoutContaminatingOtherThreads();
		CheckTerminalUnlockDropsRestoredProcGuardBeforeReset();
		std::cout << "deferred_event_dispatcher: 13 behavioral/concurrency checks passed\n";
		return EXIT_SUCCESS;
	} catch (const std::exception& error) {
		std::cerr << "deferred_event_dispatcher: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
