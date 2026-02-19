#pragma once

#include <array>
#include <string_view>

namespace CalamityAffixes::RuntimeContract
{
	inline constexpr std::string_view kRuntimeContractRelativePath = "Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json";
	inline constexpr std::string_view kFieldSupportedTriggers = "supportedTriggers";
	inline constexpr std::string_view kFieldSupportedActionTypes = "supportedActionTypes";
	inline constexpr std::string_view kFieldRunewordCatalog = "runewordCatalog";
	inline constexpr std::string_view kFieldRunewordRuneWeights = "runewordRuneWeights";
	inline constexpr std::string_view kFieldRunewordId = "id";
	inline constexpr std::string_view kFieldRunewordName = "name";
	inline constexpr std::string_view kFieldRunewordRunes = "runes";
	inline constexpr std::string_view kFieldRunewordResultAffixId = "resultAffixId";
	inline constexpr std::string_view kFieldRunewordRecommendedBase = "recommendedBase";
	inline constexpr std::string_view kFieldRuneWeightRune = "rune";
	inline constexpr std::string_view kFieldRuneWeightWeight = "weight";
	inline constexpr std::string_view kRecommendedBaseWeapon = "Weapon";
	inline constexpr std::string_view kRecommendedBaseArmor = "Armor";

	inline constexpr std::string_view kTriggerHit = "Hit";
	inline constexpr std::string_view kTriggerIncomingHit = "IncomingHit";
	inline constexpr std::string_view kTriggerDotApply = "DotApply";
	inline constexpr std::string_view kTriggerKill = "Kill";
	inline constexpr std::string_view kTriggerLowHealth = "LowHealth";

	inline constexpr std::string_view kActionDebugNotify = "DebugNotify";
	inline constexpr std::string_view kActionCastSpell = "CastSpell";
	inline constexpr std::string_view kActionCastSpellAdaptiveElement = "CastSpellAdaptiveElement";
	inline constexpr std::string_view kActionCastOnCrit = "CastOnCrit";
	inline constexpr std::string_view kActionConvertDamage = "ConvertDamage";
	inline constexpr std::string_view kActionMindOverMatter = "MindOverMatter";
	inline constexpr std::string_view kActionArchmage = "Archmage";
	inline constexpr std::string_view kActionCorpseExplosion = "CorpseExplosion";
	inline constexpr std::string_view kActionSummonCorpseExplosion = "SummonCorpseExplosion";
	inline constexpr std::string_view kActionSpawnTrap = "SpawnTrap";

	inline constexpr std::array<std::string_view, 5> kSupportedTriggers{
		kTriggerHit,
		kTriggerIncomingHit,
		kTriggerDotApply,
		kTriggerKill,
		kTriggerLowHealth
	};

	inline constexpr std::array<std::string_view, 10> kSupportedActionTypes{
		kActionDebugNotify,
		kActionCastSpell,
		kActionCastSpellAdaptiveElement,
		kActionCastOnCrit,
		kActionConvertDamage,
		kActionMindOverMatter,
		kActionArchmage,
		kActionCorpseExplosion,
		kActionSummonCorpseExplosion,
		kActionSpawnTrap
	};

	[[nodiscard]] inline bool IsSupportedTrigger(std::string_view a_trigger)
	{
		for (const auto candidate : kSupportedTriggers) {
			if (candidate == a_trigger) {
				return true;
			}
		}
		return false;
	}

	[[nodiscard]] inline bool IsSupportedActionType(std::string_view a_actionType)
	{
		for (const auto candidate : kSupportedActionTypes) {
			if (candidate == a_actionType) {
				return true;
			}
		}
		return false;
	}
}
