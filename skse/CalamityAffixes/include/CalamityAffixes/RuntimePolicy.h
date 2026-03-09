#pragma once

#include <array>
#include <string_view>

namespace CalamityAffixes::RuntimePolicy
{
	inline constexpr std::string_view kRuntimeConfigRelativePath = "Data/SKSE/Plugins/CalamityAffixes/affixes.json";
	inline constexpr std::string_view kRuntimeContractRelativePath = "Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json";
	inline constexpr std::string_view kUserSettingsRelativePath = "Data/SKSE/Plugins/CalamityAffixes/user_settings.json";

	inline constexpr bool kAllowPickupRandomAffixAssignment = false;
	inline constexpr bool kAllowLegacyPickupAffixRollBranch = false;
	inline constexpr bool kAllowPickupCurrencyRollFromContainerSources = false;
	inline constexpr bool kAllowWorldPickupCurrencyRoll = false;
	inline constexpr bool kAllowCorpseActivationRuntimeCurrencyRollInHybridMode = false;

	inline constexpr std::string_view kMcmSetEnabledEvent = "CalamityAffixes_MCM_SetEnabled";
	inline constexpr std::string_view kLegacyMcmSetDebugNotificationsEvent = "CalamityAffixes_MCM_SetDebugNotifications";
	inline constexpr std::string_view kMcmSetDebugHudNotificationsEvent = "CalamityAffixes_MCM_SetDebugHudNotifications";
	inline constexpr std::string_view kMcmSetDebugVerboseLoggingEvent = "CalamityAffixes_MCM_SetDebugVerboseLogging";
	inline constexpr std::string_view kMcmSetValidationIntervalEvent = "CalamityAffixes_MCM_SetValidationInterval";
	inline constexpr std::string_view kMcmSetProcChanceMultEvent = "CalamityAffixes_MCM_SetProcChanceMult";
	inline constexpr std::string_view kMcmSetRunewordFragmentChanceEvent = "CalamityAffixes_MCM_SetRunewordFragmentChance";
	inline constexpr std::string_view kMcmSetReforgeOrbChanceEvent = "CalamityAffixes_MCM_SetReforgeOrbChance";
	inline constexpr std::string_view kMcmSetDotSafetyAutoDisableEvent = "CalamityAffixes_MCM_SetDotSafetyAutoDisable";
	inline constexpr std::string_view kMcmSetAllowNonHostileFirstHitProcEvent = "CalamityAffixes_MCM_SetAllowNonHostileFirstHitProc";

	inline constexpr std::array<std::string_view, 9> kPersistedRuntimeUserSettingEventNames{
		kMcmSetEnabledEvent,
		kMcmSetDebugHudNotificationsEvent,
		kMcmSetDebugVerboseLoggingEvent,
		kMcmSetValidationIntervalEvent,
		kMcmSetProcChanceMultEvent,
		kMcmSetRunewordFragmentChanceEvent,
		kMcmSetReforgeOrbChanceEvent,
		kMcmSetDotSafetyAutoDisableEvent,
		kMcmSetAllowNonHostileFirstHitProcEvent
	};

	inline constexpr std::array<std::string_view, 9> kRuntimeUserSettingKeys{
		"enabled",
		"debugHudNotifications",
		"debugVerboseLogging",
		"validationIntervalSeconds",
		"procChanceMultiplier",
		"runewordFragmentChancePercent",
		"reforgeOrbChancePercent",
		"dotSafetyAutoDisable",
		"allowNonHostileFirstHitProc"
	};

	[[nodiscard]] constexpr bool IsPersistedRuntimeUserSettingEvent(std::string_view a_eventName) noexcept
	{
		for (const auto name : kPersistedRuntimeUserSettingEventNames) {
			if (name == a_eventName) {
				return true;
			}
		}
		return false;
	}
}
