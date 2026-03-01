#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PlayerOwnership.h"
#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/PrismaTooltip.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

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
		if (eventName == kUiTogglePanelEvent) {
			PrismaTooltip::ToggleControlPanel();
			RE::DebugNotification("Calamity: Prisma panel toggled");
			return RE::BSEventNotifyControl::kContinue;
		}

		if (eventName == kUiSetPanelEvent) {
			const bool open = a_event->numArg > 0.5f;
			PrismaTooltip::SetControlPanelOpen(open);
			RE::DebugNotification(open ? "Calamity: Prisma panel ON" : "Calamity: Prisma panel OFF");
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!_configLoaded) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const char* filterRaw = a_event->strArg.c_str();
		const std::string_view affixFilter = (filterRaw && *filterRaw) ? std::string_view(filterRaw) : std::string_view{};
		const auto queueRuntimeUserSettingsPersist = [&]() {
			MarkRuntimeUserSettingsDirty();
			MaybeFlushRuntimeUserSettings(std::chrono::steady_clock::now(), true);
		};
		constexpr auto kMcmChanceNotificationCooldown = std::chrono::milliseconds(1500);
		static auto s_lastRunewordChanceNotificationAt = std::chrono::steady_clock::time_point{};
		static auto s_lastReforgeChanceNotificationAt = std::chrono::steady_clock::time_point{};
		const auto shouldEmitChanceNotification =
			[&](std::chrono::steady_clock::time_point& a_lastNotificationAt) {
				if (a_lastNotificationAt.time_since_epoch().count() == 0) {
					a_lastNotificationAt = now;
					return true;
				}
				if ((now - a_lastNotificationAt) < kMcmChanceNotificationCooldown) {
					return false;
				}
				a_lastNotificationAt = now;
				return true;
			};

			if (eventName == kMcmSetEnabledEvent) {
				_runtimeEnabled = (a_event->numArg > 0.5f);
				queueRuntimeUserSettingsPersist();
				RE::DebugNotification(_runtimeEnabled ? "Calamity: enabled" : "Calamity: disabled");
				return RE::BSEventNotifyControl::kContinue;
			}

				if (eventName == kMcmSetDebugNotificationsEvent) {
					_loot.debugLog = (a_event->numArg > 0.5f);
					spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
					spdlog::flush_on(spdlog::level::warn);
					queueRuntimeUserSettingsPersist();
					RE::DebugNotification(_loot.debugLog ? "Calamity: debug notifications ON" : "Calamity: debug notifications OFF");
					return RE::BSEventNotifyControl::kContinue;
				}

			if (eventName == kMcmSetValidationIntervalEvent) {
				const float seconds = std::clamp(a_event->numArg, 0.0f, 30.0f);
				_equipResync.intervalMs = (seconds <= 0.0f) ? 0u : static_cast<std::uint64_t>(seconds * 1000.0f);
				_equipResync.nextAtMs = 0;
				queueRuntimeUserSettingsPersist();
				if (seconds <= 0.0f) {
					RE::DebugNotification("Calamity: periodic validation OFF");
				} else {
					std::string note = "Calamity: validation interval ";
					note += std::to_string(static_cast<int>(seconds));
					note += "s";
					RE::DebugNotification(note.c_str());
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetProcChanceMultEvent) {
				_runtimeProcChanceMult = std::clamp(a_event->numArg, 0.0f, 3.0f);
				queueRuntimeUserSettingsPersist();
				std::string note = "Calamity: proc x";
				note += std::to_string(_runtimeProcChanceMult);
				RE::DebugNotification(note.c_str());
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetRunewordFragmentChanceEvent) {
				const float previousRunewordChance = _loot.runewordFragmentChancePercent;
				_loot.runewordFragmentChancePercent = std::clamp(a_event->numArg, 0.0f, 100.0f);
				const float runewordChanceDelta = _loot.runewordFragmentChancePercent - previousRunewordChance;
				const bool runewordChanceChanged = runewordChanceDelta > 0.001f || runewordChanceDelta < -0.001f;
				if (runewordChanceChanged) {
					SyncCurrencyDropModeState("MCM.SetRunewordFragmentChance");
					queueRuntimeUserSettingsPersist();
					if (shouldEmitChanceNotification(s_lastRunewordChanceNotificationAt)) {
						std::string note = "Calamity: runeword fragment chance ";
						note += std::to_string(_loot.runewordFragmentChancePercent);
						note += "%";
						RE::DebugNotification(note.c_str());
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetReforgeOrbChanceEvent) {
				const float previousReforgeChance = _loot.reforgeOrbChancePercent;
				_loot.reforgeOrbChancePercent = std::clamp(a_event->numArg, 0.0f, 100.0f);
				const float reforgeChanceDelta = _loot.reforgeOrbChancePercent - previousReforgeChance;
				const bool reforgeChanceChanged = reforgeChanceDelta > 0.001f || reforgeChanceDelta < -0.001f;
				if (reforgeChanceChanged) {
					SyncCurrencyDropModeState("MCM.SetReforgeOrbChance");
					queueRuntimeUserSettingsPersist();
					if (shouldEmitChanceNotification(s_lastReforgeChanceNotificationAt)) {
						std::string note = "Calamity: reforge orb chance ";
						note += std::to_string(_loot.reforgeOrbChancePercent);
						note += "%";
						RE::DebugNotification(note.c_str());
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetDotSafetyAutoDisableEvent) {
				_loot.dotTagSafetyAutoDisable = (a_event->numArg > 0.5f);
				queueRuntimeUserSettingsPersist();
				if (!_loot.dotTagSafetyAutoDisable) {
					_dotTagSafetySuppressed = false;
					RE::DebugNotification("Calamity: DotApply auto-disable OFF (warn only)");
				} else {
					if (_loot.dotTagSafetyUniqueEffectThreshold > 0u &&
						_dotObservedMagicEffects.size() > _loot.dotTagSafetyUniqueEffectThreshold) {
						_dotTagSafetySuppressed = true;
						RE::DebugNotification("Calamity: DotApply auto-disabled (safety)");
					} else {
						RE::DebugNotification("Calamity: DotApply auto-disable ON");
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetAllowNonHostileFirstHitProcEvent) {
				const bool enabled = (a_event->numArg > 0.5f);
				_allowNonHostilePlayerOwnedOutgoingProcs.store(enabled, std::memory_order_relaxed);
				_nonHostileFirstHitGate.Clear();
				queueRuntimeUserSettingsPersist();
				RE::DebugNotification(enabled ?
					"Calamity: non-hostile first-hit proc ON" :
					"Calamity: non-hostile first-hit proc OFF");
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSpawnTestItemEvent) {
				auto* player = RE::PlayerCharacter::GetSingleton();
				auto* ironSword = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00012EB7);
				if (player && ironSword) {
					player->AddObjectToContainer(ironSword, nullptr, 1, nullptr);
					RE::DebugNotification("Calamity: spawned test item (Iron Sword)");
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kRunewordGrantStarterOrbsEvent) {
				// One-time starter grant: skip if the player already owns any reforge orbs.
				auto* player = RE::PlayerCharacter::GetSingleton();
				auto* orb = player
					? RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_Misc_ReforgeOrb")
					: nullptr;
				if (player && orb && player->GetItemCount(orb) > 0) {
					RE::DebugNotification("Reforge Orbs: already owned.");
					return RE::BSEventNotifyControl::kContinue;
				}

				const auto amount = (a_event->numArg > 0.0f) ? static_cast<std::uint32_t>(a_event->numArg) : 3u;
				GrantReforgeOrbs(amount);
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!_runtimeEnabled) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kManualModeCycleNextEvent) {
				CycleManualModeForEquippedAffixes(+1, affixFilter);
			} else if (eventName == kManualModeCyclePrevEvent) {
				CycleManualModeForEquippedAffixes(-1, affixFilter);
			} else if (eventName == kRunewordBaseNextEvent) {
				CycleRunewordBase(+1);
			} else if (eventName == kRunewordBasePrevEvent) {
				CycleRunewordBase(-1);
			} else if (eventName == kRunewordRecipeNextEvent) {
				CycleRunewordRecipe(+1);
			} else if (eventName == kRunewordRecipePrevEvent) {
				CycleRunewordRecipe(-1);
			} else if (eventName == kRunewordInsertRuneEvent) {
				InsertRunewordRuneIntoSelectedBase();
			} else if (eventName == kRunewordStatusEvent) {
				LogRunewordStatus();
			} else if (eventName == kRunewordGrantNextRuneEvent) {
				const auto amount = (a_event->numArg > 0.0f) ? static_cast<std::uint32_t>(a_event->numArg) : 1u;
				GrantNextRequiredRuneFragment(amount);
			} else if (eventName == kRunewordGrantRecipeSetEvent) {
				const auto amount = (a_event->numArg > 0.0f) ? static_cast<std::uint32_t>(a_event->numArg) : 1u;
				GrantCurrentRecipeRuneSet(amount);
			}

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
		if (!_configLoaded || !_runtimeEnabled) {
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
