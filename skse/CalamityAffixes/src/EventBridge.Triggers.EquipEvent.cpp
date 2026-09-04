#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PointerSafety.h"

#include <chrono>

namespace CalamityAffixes
{
	RE::BSEventNotifyControl EventBridge::HandleEquipEvent(
		const RE::TESEquipEvent* a_event,
		EventStateLock&,
		const EngineEventContext& a_context)
	{
		const auto now = a_context.observedAt;
		MaybeFlushRuntimeUserSettings(now, false);

		if (!_configLoaded || !_runtimeSettings.enabled.load(std::memory_order_relaxed) || _affixRuntimeState.affixes.empty()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!a_event || !a_event->actor) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto actorHolder = a_event->actor;
		auto* actorRef = actorHolder ? SanitizeObjectPointer(actorHolder.get()) : nullptr;
		auto* actor = actorRef ? actorRef->As<RE::Actor>() : nullptr;
		if (!actor || !actor->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* item = RE::TESForm::LookupByID<RE::TESForm>(a_event->baseObject);
		if (!item) {
			return RE::BSEventNotifyControl::kContinue;
		}

		RebuildActiveCounts();
		return RE::BSEventNotifyControl::kContinue;
	}

}
