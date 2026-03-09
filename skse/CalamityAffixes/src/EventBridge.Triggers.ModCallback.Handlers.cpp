#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PrismaTooltip.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		// MCM passes boolean settings as float 1.0/0.0; threshold for truthy conversion.
		constexpr float kMcmBoolThreshold = 0.5f;
		constexpr auto kMcmChanceNotificationCooldown = std::chrono::milliseconds(1500);

		bool ShouldEmitChanceNotification(
			std::chrono::steady_clock::time_point& a_lastNotificationAt,
			std::chrono::steady_clock::time_point a_now)
		{
			if (a_lastNotificationAt.time_since_epoch().count() == 0) {
				a_lastNotificationAt = a_now;
				return true;
			}
			if ((a_now - a_lastNotificationAt) < kMcmChanceNotificationCooldown) {
				return false;
			}
			a_lastNotificationAt = a_now;
			return true;
		}
	}

	bool EventBridge::HandleUiModCallback(std::string_view a_eventName, float a_numArg)
	{
		if (a_eventName == kUiTogglePanelEvent) {
			PrismaTooltip::ToggleControlPanel();
			EmitHudNotification("Calamity: Prisma panel toggled");
			return true;
		}

		if (a_eventName == kUiSetPanelEvent) {
			const bool open = a_numArg > kMcmBoolThreshold;
			PrismaTooltip::SetControlPanelOpen(open);
			EmitHudNotification(open ? "Calamity: Prisma panel ON" : "Calamity: Prisma panel OFF");
			return true;
		}

		return false;
	}

	bool EventBridge::HandleRuntimeSettingsModCallback(
		std::string_view a_eventName,
		float a_numArg,
		std::chrono::steady_clock::time_point a_now)
	{
		static auto s_lastRunewordChanceNotificationAt = std::chrono::steady_clock::time_point{};
		static auto s_lastReforgeChanceNotificationAt = std::chrono::steady_clock::time_point{};

		if (a_eventName == kMcmSetEnabledEvent) {
			_runtimeSettings.enabled = (a_numArg > kMcmBoolThreshold);
			QueueRuntimeUserSettingsPersist();
			EmitHudNotification(_runtimeSettings.enabled ? "Calamity: enabled" : "Calamity: disabled");
			return true;
		}

		if (a_eventName == kLegacyMcmSetDebugNotificationsEvent) {
			const bool enabled = (a_numArg > kMcmBoolThreshold);
			_loot.debugHudNotifications = enabled;
			_loot.debugLog = enabled;
			ApplyVerboseLoggingLevel();
			spdlog::flush_on(spdlog::level::warn);
			QueueRuntimeUserSettingsPersist();
			EmitHudNotification(enabled ? "Calamity: debug HUD/log ON (legacy)" : "Calamity: debug HUD/log OFF (legacy)");
			return true;
		}

		if (a_eventName == kMcmSetDebugHudNotificationsEvent) {
			_loot.debugHudNotifications = (a_numArg > kMcmBoolThreshold);
			QueueRuntimeUserSettingsPersist();
			EmitHudNotification(_loot.debugHudNotifications ? "Calamity: debug HUD ON" : "Calamity: debug HUD OFF");
			return true;
		}

		if (a_eventName == kMcmSetDebugVerboseLoggingEvent) {
			_loot.debugLog = (a_numArg > kMcmBoolThreshold);
			ApplyVerboseLoggingLevel();
			spdlog::flush_on(spdlog::level::warn);
			QueueRuntimeUserSettingsPersist();
			EmitHudNotification(_loot.debugLog ? "Calamity: debug log ON" : "Calamity: debug log OFF");
			return true;
		}

		if (a_eventName == kMcmSetValidationIntervalEvent) {
			const float seconds = std::clamp(a_numArg, 0.0f, 30.0f);
			_equipResync.intervalMs = (seconds <= 0.0f) ? 0u : static_cast<std::uint64_t>(seconds * 1000.0f);
			_equipResync.nextAtMs = 0;
			QueueRuntimeUserSettingsPersist();
			if (seconds <= 0.0f) {
				EmitHudNotification("Calamity: periodic validation OFF");
			} else {
				std::string note = "Calamity: validation interval ";
				note += std::to_string(static_cast<int>(seconds));
				note += "s";
				EmitHudNotification(note.c_str());
			}
			return true;
		}

		if (a_eventName == kMcmSetProcChanceMultEvent) {
			_runtimeSettings.procChanceMult = std::clamp(a_numArg, 0.0f, 3.0f);
			QueueRuntimeUserSettingsPersist();
			std::string note = "Calamity: proc x";
			note += std::to_string(_runtimeSettings.procChanceMult);
			EmitHudNotification(note.c_str());
			return true;
		}

		if (a_eventName == kMcmSetRunewordFragmentChanceEvent) {
			const float previousRunewordChance = _loot.runewordFragmentChancePercent;
			_loot.runewordFragmentChancePercent = std::clamp(a_numArg, 0.0f, 100.0f);
			const float runewordChanceDelta = _loot.runewordFragmentChancePercent - previousRunewordChance;
			const bool runewordChanceChanged = runewordChanceDelta > 0.001f || runewordChanceDelta < -0.001f;
			if (runewordChanceChanged) {
				SyncCurrencyDropModeState("MCM.SetRunewordFragmentChance");
				QueueRuntimeUserSettingsPersist();
				if (ShouldEmitChanceNotification(s_lastRunewordChanceNotificationAt, a_now)) {
					std::string note = "Calamity: runeword fragment chance ";
					note += std::to_string(_loot.runewordFragmentChancePercent);
					note += "%";
					EmitHudNotification(note.c_str());
				}
			}
			return true;
		}

		if (a_eventName == kMcmSetReforgeOrbChanceEvent) {
			const float previousReforgeChance = _loot.reforgeOrbChancePercent;
			_loot.reforgeOrbChancePercent = std::clamp(a_numArg, 0.0f, 100.0f);
			const float reforgeChanceDelta = _loot.reforgeOrbChancePercent - previousReforgeChance;
			const bool reforgeChanceChanged = reforgeChanceDelta > 0.001f || reforgeChanceDelta < -0.001f;
			if (reforgeChanceChanged) {
				SyncCurrencyDropModeState("MCM.SetReforgeOrbChance");
				QueueRuntimeUserSettingsPersist();
				if (ShouldEmitChanceNotification(s_lastReforgeChanceNotificationAt, a_now)) {
					std::string note = "Calamity: reforge orb chance ";
					note += std::to_string(_loot.reforgeOrbChancePercent);
					note += "%";
					EmitHudNotification(note.c_str());
				}
			}
			return true;
		}

		if (a_eventName == kMcmSetDotSafetyAutoDisableEvent) {
			_loot.dotTagSafetyAutoDisable = (a_numArg > kMcmBoolThreshold);
			QueueRuntimeUserSettingsPersist();
			if (!_loot.dotTagSafetyAutoDisable) {
				_combatState.dotTagSafetySuppressed = false;
				EmitHudNotification("Calamity: DotApply auto-disable OFF (warn only)");
			} else {
				if (_loot.dotTagSafetyUniqueEffectThreshold > 0u &&
					_combatState.dotObservedMagicEffects.size() > _loot.dotTagSafetyUniqueEffectThreshold) {
					_combatState.dotTagSafetySuppressed = true;
					EmitHudNotification("Calamity: DotApply auto-disabled (safety)");
				} else {
					EmitHudNotification("Calamity: DotApply auto-disable ON");
				}
			}
			return true;
		}

		if (a_eventName == kMcmSetAllowNonHostileFirstHitProcEvent) {
			const bool enabled = (a_numArg > kMcmBoolThreshold);
			_runtimeSettings.allowNonHostilePlayerOwnedOutgoingProcs.store(enabled, std::memory_order_relaxed);
			_combatState.nonHostileFirstHitGate.Clear();
			QueueRuntimeUserSettingsPersist();
			EmitHudNotification(enabled ?
				"Calamity: non-hostile first-hit proc ON" :
				"Calamity: non-hostile first-hit proc OFF");
			return true;
		}

		return false;
	}

	bool EventBridge::HandleDebugUtilityModCallback(std::string_view a_eventName, float a_numArg)
	{
		if (a_eventName == kMcmSpawnTestItemEvent) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* ironSword = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00012EB7);
			if (player && ironSword) {
				player->AddObjectToContainer(ironSword, nullptr, 1, nullptr);
				EmitHudNotification("Calamity: spawned test item (Iron Sword)");
			}
			return true;
		}

		if (a_eventName == kRunewordGrantStarterOrbsEvent) {
			// One-time starter grant: skip if the player already owns any reforge orbs.
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* orb = player
				? RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_Misc_ReforgeOrb")
				: nullptr;
			if (player && orb && player->GetItemCount(orb) > 0) {
				EmitHudNotification("Reforge Orbs: already owned.");
				return true;
			}

			const auto amount = (a_numArg > 0.0f) ? static_cast<std::uint32_t>(a_numArg) : 3u;
			GrantReforgeOrbs(amount);
			return true;
		}

		if (a_eventName == kMcmForceRebuildEvent) {
			auto* rebuildPlayer = RE::PlayerCharacter::GetSingleton();
			if (rebuildPlayer) {
				for (auto* spell : _appliedPassiveSpells) {
					rebuildPlayer->RemoveSpell(spell);
				}
			}
			_appliedPassiveSpells.clear();
			RebuildActiveCounts();
			std::uint32_t activeCount = 0;
			for (std::size_t i = 0; i < _activeCounts.size(); ++i) {
				if (_activeCounts[i] > 0) {
					++activeCount;
				}
			}
			std::string note = "Calamity: rebuilt (";
			note += std::to_string(activeCount);
			note += " active, ";
			note += std::to_string(_appliedPassiveSpells.size());
			note += " passive)";
			EmitHudNotification(note.c_str());
			SKSE::log::info("CalamityAffixes: force-rebuild — {} active affixes, {} passive spells applied.",
				activeCount, _appliedPassiveSpells.size());
			return true;
		}

		if (a_eventName == kMcmGrantRecoveryPackEvent) {
			auto* recoveryPlayer = RE::PlayerCharacter::GetSingleton();
			if (recoveryPlayer) {
				GrantReforgeOrbs(5u);
				if (_runewordState.runeNameByToken.empty()) {
					InitializeRunewordCatalog();
				}
				std::uint32_t fragmentsGranted = 0u;
				for (const auto& [runeToken, runeName] : _runewordState.runeNameByToken) {
					if (runeToken == 0u || runeName.empty()) {
						continue;
					}
					const auto given = RunewordDetail::GrantRunewordFragments(
						recoveryPlayer, _runewordState.runeNameByToken, runeToken, 1u);
					fragmentsGranted += given;
				}
				std::string note = "Calamity: recovery pack (5 orbs, ";
				note += std::to_string(fragmentsGranted);
				note += " fragments)";
				EmitHudNotification(note.c_str());
				SKSE::log::info("CalamityAffixes: granted recovery pack — 5 orbs, {} fragment types.", fragmentsGranted);
			}
			return true;
		}

		return false;
	}

	bool EventBridge::HandleRuntimeGameplayModCallback(
		std::string_view a_eventName,
		float a_numArg,
		std::string_view a_affixIdFilter)
	{
		if (a_eventName == kManualModeCycleNextEvent) {
			CycleManualModeForEquippedAffixes(+1, a_affixIdFilter);
			return true;
		}
		if (a_eventName == kManualModeCyclePrevEvent) {
			CycleManualModeForEquippedAffixes(-1, a_affixIdFilter);
			return true;
		}
		if (a_eventName == kRunewordBaseNextEvent) {
			CycleRunewordBase(+1);
			return true;
		}
		if (a_eventName == kRunewordBasePrevEvent) {
			CycleRunewordBase(-1);
			return true;
		}
		if (a_eventName == kRunewordRecipeNextEvent) {
			CycleRunewordRecipe(+1);
			return true;
		}
		if (a_eventName == kRunewordRecipePrevEvent) {
			CycleRunewordRecipe(-1);
			return true;
		}
		if (a_eventName == kRunewordInsertRuneEvent) {
			InsertRunewordRuneIntoSelectedBase();
			return true;
		}
		if (a_eventName == kRunewordStatusEvent) {
			LogRunewordStatus();
			return true;
		}
		if (a_eventName == kRunewordGrantNextRuneEvent) {
			const auto amount = (a_numArg > 0.0f) ? static_cast<std::uint32_t>(a_numArg) : 1u;
			GrantNextRequiredRuneFragment(amount);
			return true;
		}
		if (a_eventName == kRunewordGrantRecipeSetEvent) {
			const auto amount = (a_numArg > 0.0f) ? static_cast<std::uint32_t>(a_numArg) : 1u;
			GrantCurrentRecipeRuneSet(amount);
			return true;
		}

		return false;
	}
}
