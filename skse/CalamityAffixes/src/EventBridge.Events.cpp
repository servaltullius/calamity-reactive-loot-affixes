#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/HitDataUtil.h"

#include <memory>
#include <type_traits>
#include <utility>

namespace CalamityAffixes
{
	template <class Event>
	RE::BSEventNotifyControl EventBridge::DispatchEngineEvent(
		const Event* a_event,
		RE::BSEventNotifyControl (EventBridge::*a_handler)(const Event*, EventStateLock&, const EngineEventContext&))
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Capture the epoch BEFORE copying engine references. A load/revert racing
		// with snapshot creation must not relabel old-world input as new work.
		const auto generation = _eventDispatcher.CaptureGeneration();
		EngineEventContext context{
			.observedAt = std::chrono::steady_clock::now(),
			.procOrigin = ScopedProcDepth::IsActiveOnCurrentThread(_combatState)
		};
		auto event = *a_event;
		if constexpr (std::is_same_v<Event, RE::TESHitEvent>) {
			// TESHitEvent's NiPointers own cause/target; HitData's actor/source
			// handles and attackData NiPointer survive deferral too. Never consult
			// a later hit's flags/damage when consuming an earlier queued event.
			auto* targetRef = SanitizeObjectPointer(event.target.get());
			auto* target = targetRef ? targetRef->template As<RE::Actor>() : nullptr;
			if (const auto* hitData = HitDataUtil::GetLastHitData(target)) {
				context.hitData = *hitData;
				// This transient engine command is not used by affix dispatch.
				context.hitData->VATSCommand = nullptr;
			}
		} else if constexpr (std::is_same_v<Event, SKSE::ModCallbackEvent>) {
			// The handler uses only owning BSFixedStrings and numArg. Do not retain
			// the unused, non-owning sender pointer across a task boundary.
			event.sender = nullptr;
		}

		// No callback borrows a TES*Event*, Actor*, ActiveEffect*, or stack
		// HitData. NiPointers keep reference events alive; container events use
		// a generation-aware ObjectRefHandle; other payload fields are values.
		auto work = std::make_unique<detail::DeferredEventDispatcher::Work>(
			[this, event = std::move(event), context = std::move(context), a_handler](EventStateLock& a_lock) {
				std::optional<ScopedProcDepth> originGuard;
				if constexpr (!std::is_same_v<Event, RE::TESHitEvent> && !std::is_same_v<Event, RE::TESDeathEvent>) {
					if (context.procOrigin && !ScopedProcDepth::IsActiveOnCurrentThread(_combatState)) {
						// Hit rejects its proc-origin input outright; Death restores
						// its guard locally so it ends BEFORE terminal lock.unlock().
						// Other handlers keep state locked through return. In particular
						// DoT observation/ICD still runs, but its proc is suppressed.
						originGuard.emplace(_combatState);
					}
				}
				(void)(this->*a_handler)(&event, a_lock, context);
			});
		(void)_eventDispatcher.Submit(
			generation,
			std::move(work),
			_stateMutex,
			[this](auto a_generation) { return ScheduleEngineEventDrain(a_generation); });
		return RE::BSEventNotifyControl::kContinue;
	}

	bool EventBridge::ScheduleEngineEventDrain(detail::DeferredEventDispatcher::Generation a_generation)
	{
		if (auto* tasks = SKSE::GetTaskInterface()) {
			tasks->AddTask([this, a_generation]() {
				_eventDispatcher.Drain(
					a_generation,
					_stateMutex,
					[this](auto a_nextGeneration) { return ScheduleEngineEventDrain(a_nextGeneration); });
			});
			return true;
		}
		// Never fall back to waiting on state inside an engine callback. The
		// inbox retains work, and subsequent admission retries scheduling.
		SKSE::log::error("CalamityAffixes: event task interface unavailable; pending events retained for retry.");
		return false;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*)
	{
		return DispatchEngineEvent(a_event, &EventBridge::HandleHitEvent);
	}
	RE::BSEventNotifyControl EventBridge::ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		return DispatchEngineEvent(a_event, &EventBridge::HandleDeathEvent);
	}
	RE::BSEventNotifyControl EventBridge::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		return DispatchEngineEvent(a_event, &EventBridge::HandleEquipEvent);
	}
	RE::BSEventNotifyControl EventBridge::ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*)
	{
		return DispatchEngineEvent(a_event, &EventBridge::HandleActivateEvent);
	}
	RE::BSEventNotifyControl EventBridge::ProcessEvent(const RE::TESMagicEffectApplyEvent* a_event, RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*)
	{
		return DispatchEngineEvent(a_event, &EventBridge::HandleMagicEffectApplyEvent);
	}
	RE::BSEventNotifyControl EventBridge::ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*)
	{
		return DispatchEngineEvent(a_event, &EventBridge::HandleContainerChangedEvent);
	}
	RE::BSEventNotifyControl EventBridge::ProcessEvent(const RE::TESUniqueIDChangeEvent* a_event, RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*)
	{
		return DispatchEngineEvent(a_event, &EventBridge::HandleUniqueIDChangeEvent);
	}
	RE::BSEventNotifyControl EventBridge::ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		return DispatchEngineEvent(a_event, &EventBridge::HandleModCallbackEvent);
	}
}
