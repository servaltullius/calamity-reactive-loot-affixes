#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PointerSafety.h"

#include <chrono>


namespace CalamityAffixes
{
	static_assert(!RuntimePolicy::kAllowActivationCurrencyRoll);

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESActivateEvent* a_event,
		RE::BSTEventSource<RE::TESActivateEvent>*)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!_configLoaded || !_runtimeSettings.enabled || !a_event || !a_event->actionRef || !a_event->objectActivated) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* actionRef = SanitizeObjectPointer(a_event->actionRef.get());
		auto* sourceRef = SanitizeObjectPointer(a_event->objectActivated.get());
		auto* actionActor = actionRef ? actionRef->As<RE::Actor>() : nullptr;
		if (!actionActor || !actionActor->IsPlayerRef() || !sourceRef) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (_runtimeSettings.combatDebugLog) {
			auto* baseObject = sourceRef->GetBaseObject();
			auto* combatController = actionActor->GetActorRuntimeData().combatController;
			const auto runtimeTargetHandle = actionActor->GetActorRuntimeData().currentCombatTarget.native_handle();
			const auto controllerTargetHandle = combatController ? combatController->targetHandle.native_handle() : 0u;
			const bool murderAlarmBit = actionActor->GetActorRuntimeData().boolBits.any(RE::Actor::BOOL_BITS::kMurderAlarm);
			const bool controllerStarted = combatController ? combatController->startedCombat : false;
			const bool controllerStopped = combatController ? combatController->stoppedCombat : false;
			const std::string_view baseEditorId =
				(baseObject && baseObject->GetFormEditorID())
					? std::string_view(baseObject->GetFormEditorID())
					: std::string_view{};
			SKSE::log::info(
				"CalamityAffixes: activation probe (targetRef={}({:08X}), base={}({:08X}), baseEditorId={}, inCombat={}, runtimeTarget={:08X}, controllerTarget={:08X}, murderAlarm={}, ctrlStarted={}, ctrlStopped={}).",
				sourceRef->GetName(),
				sourceRef->GetFormID(),
				baseObject ? baseObject->GetName() : "<none>",
				baseObject ? baseObject->GetFormID() : 0u,
				baseEditorId,
				actionActor->IsInCombat(),
				runtimeTargetHandle,
				controllerTargetHandle,
				murderAlarmBit,
				controllerStarted,
				controllerStopped);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

}
