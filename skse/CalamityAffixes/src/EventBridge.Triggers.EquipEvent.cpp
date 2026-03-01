#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PointerSafety.h"

#include <chrono>

namespace CalamityAffixes
{
	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESEquipEvent* a_event,
		RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!_configLoaded || !_runtimeEnabled || _affixes.empty()) {
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
