#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PlayerOwnership.h"
#include <string_view>

namespace CalamityAffixes
{
	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const SKSE::ModCallbackEvent* a_event,
		RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const char* eventNameRaw = a_event->eventName.c_str();
		if (!eventNameRaw || !*eventNameRaw) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const std::string_view eventName(eventNameRaw);
		if (HandleUiModCallback(eventName, a_event->numArg)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!_configLoaded) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const char* filterRaw = a_event->strArg.c_str();
		const std::string_view affixFilter = (filterRaw && *filterRaw) ? std::string_view(filterRaw) : std::string_view{};
		if (HandleRuntimeSettingsModCallback(eventName, a_event->numArg, now)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (HandleDebugUtilityModCallback(eventName, a_event->numArg)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!_runtimeSettings.enabled) {
			return RE::BSEventNotifyControl::kContinue;
		}

		(void)HandleRuntimeGameplayModCallback(eventName, a_event->numArg, affixFilter);

		return RE::BSEventNotifyControl::kContinue;
	}

	bool EventBridge::IsPlayerOwned(RE::Actor* a_actor)
	{
		return IsPlayerOwnedActor(a_actor);
	}

	RE::Actor* EventBridge::GetPlayerOwner(RE::Actor* a_actor)
	{
		return ResolvePlayerOwnerActor(a_actor);
	}

	void EventBridge::SendModEvent(std::string_view a_eventName, RE::TESForm* a_sender)
	{
		auto* source = SKSE::GetModCallbackEventSource();
		if (!source) {
			return;
		}

		SKSE::ModCallbackEvent event{
			RE::BSFixedString(a_eventName.data()),
			RE::BSFixedString(""),
			0.0f,
			a_sender
		};

		source->SendEvent(&event);
	}

	void EventBridge::MaybeResyncEquippedAffixes(std::chrono::steady_clock::time_point a_now)
	{
		if (!_configLoaded || !_runtimeSettings.enabled) {
			return;
		}

		if (_equipResync.intervalMs == 0u) {
			return;
		}

		const auto nowMs =
			static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(a_now.time_since_epoch()).count());

		if (!_equipResync.ShouldRun(nowMs)) {
			return;
		}

		RebuildActiveCounts();
	}

}
