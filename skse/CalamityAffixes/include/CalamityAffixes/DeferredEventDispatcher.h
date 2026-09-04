#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace CalamityAffixes::detail
{
	// Engine event sources can call sinks while holding their own event lock.
	// Such a sink must never wait for the state mutex: another thread can own
	// that mutex while entering the same event source through an engine call.
	//
	// A contended submission joins one FIFO, including submissions made while
	// an earlier batch is executing. Uncontended same-thread recursion retains
	// its synchronous behavior until there is a backlog. Queue ownership is
	// never held across state-lock waits, work, scheduling, or payload teardown.
	// The mutex parameter is a testable BasicLockable seam; the product alias
	// below uses the existing std::recursive_mutex without replacing its lock.
	template <class Mutex>
	class BasicDeferredEventDispatcher
	{
	public:
		using StateMutex = Mutex;
		using StateLock = std::unique_lock<StateMutex>;
		using Work = std::function<void(StateLock&)>;
		using Generation = std::uint64_t;
		static constexpr std::size_t kMaxBatchSize = 128u;

		[[nodiscard]] Generation CaptureGeneration() const noexcept
		{
			return _generation.load(std::memory_order_acquire);
		}

		[[nodiscard]] bool IsCurrent(Generation a_generation) const noexcept
		{
			return CaptureGeneration() == a_generation;
		}

		// Scheduler(generation) returns true only if a Drain task was accepted.
		// False retains the FIFO for the next submission to retry. It must not
		// report failure after accepting/invoking a task. StateLock may be
		// explicitly unlocked by Work for a terminal engine event notification.
		template <class Scheduler>
		bool Submit(
			Generation a_generation,
			std::unique_ptr<Work> a_work,
			StateMutex& a_stateMutex,
			Scheduler&& a_scheduler)
		{
			if (!a_work || !*a_work || !IsCurrent(a_generation)) {
				return false;
			}

			StateLock stateLock(a_stateMutex, std::try_to_lock);
			bool runInline = false;
			bool schedule = false;
			{
				const std::scoped_lock queueLock(_queueMutex);
				if (!IsCurrent(a_generation)) {
					return false;
				}

				const auto thread = std::this_thread::get_id();
				runInline = stateLock.owns_lock() && _pending.empty() && !_scheduled && !_draining &&
					(_inlineDepth == 0u || _inlineOwner == thread);
				if (runInline) {
					_inlineOwner = thread;
					++_inlineDepth;
				} else {
					_pending.push_back(std::move(a_work));
					if (!_scheduled && _inlineDepth == 0u) {
						_scheduled = true;
						schedule = true;
					}
				}
			}

			if (!runInline) {
				if (stateLock.owns_lock()) {
					stateLock.unlock();
				}
				if (schedule) {
					Schedule(a_generation, a_scheduler);
				}
				return true;
			}

			std::exception_ptr error;
			try {
				(*a_work)(stateLock);
			} catch (...) {
				error = std::current_exception();
			}
			if (stateLock.owns_lock()) {
				stateLock.unlock();
			}
			a_work.reset();
			FinishInline(a_generation, a_scheduler);
			if (error) {
				std::rethrow_exception(error);
			}
			return true;
		}

		template <class Scheduler>
		void Drain(Generation a_generation, StateMutex& a_stateMutex, Scheduler&& a_scheduler)
		{
			std::array<std::unique_ptr<Work>, kMaxBatchSize> batch{};
			std::size_t count = 0u;
			{
				const std::scoped_lock queueLock(_queueMutex);
				if (!IsCurrent(a_generation) || !_scheduled || _draining || _inlineDepth != 0u) {
					return;
				}
				_draining = true;
				while (count < batch.size() && !_pending.empty()) {
					batch[count++] = std::move(_pending.front());
					_pending.pop_front();
				}
			}

			for (std::size_t i = 0u; i < count; ++i) {
				std::exception_ptr error;
				{
					StateLock stateLock(a_stateMutex);
					// Invalidation can happen while this task waits for state.
					// Checking only before lock() would replay a previous save.
					if (IsCurrent(a_generation)) {
						try {
							(*batch[i])(stateLock);
						} catch (...) {
							error = std::current_exception();
						}
					}
				}
				batch[i].reset();
				if (error) {
					// The failing work was invoked once; preserve unstarted work
					// ahead of events admitted during its execution.
					{
						const std::scoped_lock queueLock(_queueMutex);
						if (IsCurrent(a_generation)) {
							for (auto remaining = count; remaining > i + 1u; --remaining) {
								_pending.push_front(std::move(batch[remaining - 1u]));
							}
						}
					}
					FinishDrain(a_generation, a_scheduler);
					std::rethrow_exception(error);
				}
			}
			FinishDrain(a_generation, a_scheduler);
		}

		// Lifecycle/config callers must own the same state mutex used by
		// Submit/Drain when invalidating and replacing shared state. This
		// serializes reset with execution AFTER its final generation check;
		// generation alone cannot cancel a callback already executing.
		void Invalidate()
		{
			std::deque<std::unique_ptr<Work>> retired;
			{
				const std::scoped_lock queueLock(_queueMutex);
				_generation.fetch_add(1u, std::memory_order_acq_rel);
				retired.swap(_pending);
				_scheduled = false;
				_draining = false;
				_inlineDepth = 0u;
				_inlineOwner = {};
			}
			// Captured engine handles and smart references may run destructors
			// which re-enter this dispatcher. Destroy them outside the queue lock.
		}

	private:
		template <class Scheduler>
		void Schedule(Generation a_generation, Scheduler& a_scheduler)
		{
			bool accepted = false;
			try {
				accepted = a_scheduler(a_generation);
			} catch (...) {
				ReleaseFailedSchedule(a_generation);
				throw;
			}
			if (!accepted) {
				ReleaseFailedSchedule(a_generation);
			}
		}

		void ReleaseFailedSchedule(Generation a_generation)
		{
			const std::scoped_lock queueLock(_queueMutex);
			if (IsCurrent(a_generation)) {
				_scheduled = false;
			}
		}

		template <class Scheduler>
		void FinishInline(Generation a_generation, Scheduler& a_scheduler)
		{
			bool schedule = false;
			{
				const std::scoped_lock queueLock(_queueMutex);
				if (!IsCurrent(a_generation)) {
					return;
				}
				if (--_inlineDepth == 0u) {
					_inlineOwner = {};
					if (!_pending.empty() && !_scheduled) {
						_scheduled = true;
						schedule = true;
					}
				}
			}
			if (schedule) {
				Schedule(a_generation, a_scheduler);
			}
		}

		template <class Scheduler>
		void FinishDrain(Generation a_generation, Scheduler& a_scheduler)
		{
			bool schedule = false;
			{
				const std::scoped_lock queueLock(_queueMutex);
				if (!IsCurrent(a_generation)) {
					return;
				}
				_draining = false;
				schedule = !_pending.empty();
				_scheduled = schedule;
			}
			if (schedule) {
				Schedule(a_generation, a_scheduler);
			}
		}

		std::atomic<Generation> _generation{ 1u };
		std::mutex _queueMutex;
		std::deque<std::unique_ptr<Work>> _pending;
		std::thread::id _inlineOwner{};
		std::size_t _inlineDepth{ 0u };
		bool _scheduled{ false };
		bool _draining{ false };
	};

	using DeferredEventDispatcher = BasicDeferredEventDispatcher<std::recursive_mutex>;
}
