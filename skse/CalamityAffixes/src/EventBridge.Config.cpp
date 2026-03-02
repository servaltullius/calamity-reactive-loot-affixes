#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RunewordContractSnapshot.h"
#include "CalamityAffixes/RuntimeContract.h"
#include "CalamityAffixes/RuntimePaths.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_set>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		const nlohmann::json kEmptyAffixArray = nlohmann::json::array();

		void ValidateRuntimeContractSnapshot()
		{
			const auto contractPath = RuntimePaths::ResolveRuntimeRelativePath(RuntimeContract::kRuntimeContractRelativePath);

			std::ifstream in(contractPath, std::ios::binary);
			if (!in.is_open()) {
				SKSE::log::warn(
					"CalamityAffixes: runtime contract snapshot missing at {}. Re-run generator to refresh contract artifacts.",
					contractPath.string());
				return;
			}

			nlohmann::json contract = nlohmann::json::object();
			try {
				in >> contract;
			} catch (const std::exception& e) {
				SKSE::log::warn(
					"CalamityAffixes: failed to parse runtime contract snapshot {} ({}).",
					contractPath.string(),
					e.what());
				return;
			}

			auto validateArray = [&](std::string_view a_key, const auto& a_expected, const auto& a_isSupported) {
				const auto it = contract.find(std::string(a_key));
				if (it == contract.end() || !it->is_array()) {
					SKSE::log::warn(
						"CalamityAffixes: runtime contract snapshot field '{}' is missing/invalid in {}.",
						a_key,
						contractPath.string());
					return false;
				}

				std::unordered_set<std::string> actual{};
				for (const auto& entry : *it) {
					if (!entry.is_string()) {
						SKSE::log::warn(
							"CalamityAffixes: runtime contract snapshot field '{}' contains non-string entries in {}.",
							a_key,
							contractPath.string());
						return false;
					}
					const auto value = entry.get<std::string>();
					if (!a_isSupported(value)) {
						SKSE::log::warn(
							"CalamityAffixes: runtime contract snapshot field '{}' contains unsupported value '{}' in {}.",
							a_key,
							value,
							contractPath.string());
						return false;
					}
					actual.insert(value);
				}

				for (const auto expected : a_expected) {
					if (actual.find(std::string(expected)) == actual.end()) {
						SKSE::log::warn(
							"CalamityAffixes: runtime contract snapshot field '{}' is missing expected value '{}' in {}.",
							a_key,
							expected,
							contractPath.string());
						return false;
					}
				}
				return true;
			};

			const bool triggersOk = validateArray(
				RuntimeContract::kFieldSupportedTriggers,
				RuntimeContract::kSupportedTriggers,
				[](std::string_view value) { return RuntimeContract::IsSupportedTrigger(value); });
			const bool actionsOk = validateArray(
				RuntimeContract::kFieldSupportedActionTypes,
				RuntimeContract::kSupportedActionTypes,
				[](std::string_view value) { return RuntimeContract::IsSupportedActionType(value); });

			RunewordContractSnapshot runewordContractSnapshot{};
			const bool runewordOk = LoadRunewordContractSnapshotFromRuntime(
				RuntimeContract::kRuntimeContractRelativePath,
				runewordContractSnapshot);

			if (triggersOk && actionsOk && runewordOk) {
				SKSE::log::info(
					"CalamityAffixes: runtime contract snapshot validated at {}.",
					contractPath.string());
			}
		}
	}

	void EventBridge::LoadConfig()
	{
		ResetRuntimeStateForConfigReload();
		InitializeRunewordCatalog();

		nlohmann::json j;
		if (!LoadRuntimeConfigJson(j)) {
			return;
		}
		ValidateRuntimeContractSnapshot();

		ApplyLootConfigFromJson(j);
		ApplyRuntimeUserSettingsOverrides();

		const auto* affixes = ResolveAffixArray(j);
		if (!affixes) {
			SKSE::log::error("CalamityAffixes: invalid runtime config schema (keywords.affixes).");
			return;
		}

		auto* handler = RE::TESDataHandler::GetSingleton();
		ParseConfiguredAffixesFromJson(*affixes, handler);

		IndexConfiguredAffixes();
		SynthesizeRunewordRuntimeAffixes();
		RebuildSharedLootPools();

		_activeCounts.assign(_affixes.size(), 0);

		SanitizeRunewordState();
		SanitizeAllTrackedLootInstancesForCurrentLootRules("LoadConfig.postIndex");
		_configLoaded = true;

		RebuildActiveCounts();

		SKSE::log::info(
			"CalamityAffixes: runtime config loaded (affixes={}, prefixWeapon={}, prefixArmor={}, suffixWeapon={}, suffixArmor={}, lootChance={}%, runeFragChance={}%, reforgeOrbChance={}%, runtimeCurrencyDropsEnabled={}, sourceMult(corpse/container/boss/world)={:.2f}/{:.2f}/{:.2f}/{:.2f}, triggerBudget={}/{}, trapCastBudgetPerTick={}).",
			_affixes.size(),
			_lootWeaponAffixes.size(), _lootArmorAffixes.size(),
			_lootWeaponSuffixes.size(), _lootArmorSuffixes.size(),
			_loot.chancePercent,
			_loot.runewordFragmentChancePercent,
			_loot.reforgeOrbChancePercent,
			_loot.runtimeCurrencyDropsEnabled,
			_loot.lootSourceChanceMultCorpse,
			_loot.lootSourceChanceMultContainer,
			_loot.lootSourceChanceMultBossContainer,
			_loot.lootSourceChanceMultWorld,
			_loot.triggerProcBudgetPerWindow,
			_loot.triggerProcBudgetWindowMs,
			_loot.trapCastBudgetPerTick);

		SKSE::log::info(
			"CalamityAffixes: special action indices (convert={}, castOnCrit={}, mindOverMatter={}, archmage={}, corpseExplosion={}).",
			_convertAffixIndices.size(),
			_castOnCritAffixIndices.size(),
			_mindOverMatterAffixIndices.size(),
			_archmageAffixIndices.size(),
			_corpseExplosionAffixIndices.size());

		if (_affixes.empty()) {
			SKSE::log::error("CalamityAffixes: no affixes loaded. Is the generated CalamityAffixes plugin enabled?");
		}
	}

	const nlohmann::json* EventBridge::ResolveAffixArray(const nlohmann::json& a_configRoot) const
	{
		const auto keywordsIt = a_configRoot.find("keywords");
		if (keywordsIt == a_configRoot.end()) {
			return &kEmptyAffixArray;
		}
		if (!keywordsIt->is_object()) {
			SKSE::log::error("CalamityAffixes: runtime config field 'keywords' must be an object.");
			return nullptr;
		}

		const auto affixesIt = keywordsIt->find("affixes");
		if (affixesIt == keywordsIt->end()) {
			return &kEmptyAffixArray;
		}
		if (!affixesIt->is_array()) {
			SKSE::log::error("CalamityAffixes: runtime config field 'keywords.affixes' must be an array.");
			return nullptr;
		}

		return &(*affixesIt);
	}

}
