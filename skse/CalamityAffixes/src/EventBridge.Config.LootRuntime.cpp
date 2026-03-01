#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootEligibility.h"
#include "CalamityAffixes/RuntimePaths.h"
#include "EventBridge.Config.Shared.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		using ConfigShared::Trim;

		[[nodiscard]] std::string ToLowerAscii(std::string_view a_text)
		{
			std::string out;
			out.reserve(a_text.size());
			for (char c : a_text) {
				out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
			}
			return out;
		}

		[[nodiscard]] std::optional<bool> ParseLootNameMarkerPositionIsTrailing(std::string_view a_text)
		{
			const auto normalized = ToLowerAscii(Trim(a_text));
			if (normalized == "leading") {
				return false;
			}
			if (normalized == "trailing") {
				return true;
			}
			return std::nullopt;
		}
	}

	bool EventBridge::LoadRuntimeConfigJson(nlohmann::json& a_outJson) const
	{
		const auto configPath = RuntimePaths::ResolveRuntimeRelativePath(kRuntimeConfigRelativePath);

		SKSE::log::info("CalamityAffixes: loading runtime config from: {}", configPath.string());

		std::ifstream in(configPath);
		if (!in.is_open()) {
			SKSE::log::warn("CalamityAffixes: runtime config not found: {}", configPath.string());
			return false;
		}

		try {
			in >> a_outJson;
		} catch (const std::exception& e) {
			SKSE::log::error("CalamityAffixes: failed to parse runtime config (affixes.json): {}", e.what());
			return false;
		}

		return true;
	}

	void EventBridge::ApplyLootConfigFromJson(const nlohmann::json& a_configRoot)
	{
		_loot.armorEditorIdDenyContains = detail::MakeDefaultLootArmorEditorIdDenyContains();
		_loot.bossContainerEditorIdAllowContains = detail::MakeDefaultBossContainerEditorIdAllowContains();
		_loot.bossContainerEditorIdDenyContains.clear();
		_loot.nameMarkerPosition = LootNameMarkerPosition::kTrailing;
		_loot.currencyDropMode = CurrencyDropMode::kHybrid;
		_loot.runtimeCurrencyDropsEnabled = true;
		const auto& loot = a_configRoot.value("loot", nlohmann::json::object());
		if (loot.is_object()) {
			_loot.chancePercent = loot.value("chancePercent", 0.0f);
			_loot.runewordFragmentChancePercent =
				static_cast<float>(loot.value("runewordFragmentChancePercent", static_cast<double>(_loot.runewordFragmentChancePercent)));
			_loot.reforgeOrbChancePercent =
				static_cast<float>(loot.value("reforgeOrbChancePercent", static_cast<double>(_loot.reforgeOrbChancePercent)));
			if (const auto modeIt = loot.find("currencyDropMode"); modeIt != loot.end()) {
				if (modeIt->is_string()) {
					const auto modeText = modeIt->get<std::string>();
					const auto normalized = ToLowerAscii(Trim(modeText));
					if (normalized != "hybrid") {
						SKSE::log::warn(
							"CalamityAffixes: loot.currencyDropMode='{}' is deprecated. Hybrid-only drop mode is enforced.",
							modeText);
					}
				} else {
					SKSE::log::warn(
						"CalamityAffixes: loot.currencyDropMode must be string 'hybrid'. Hybrid-only drop mode is enforced.");
				}
			}
			_loot.currencyDropMode = CurrencyDropMode::kHybrid;
			_loot.runtimeCurrencyDropsEnabled = true;
			_loot.lootSourceChanceMultCorpse =
				static_cast<float>(loot.value("lootSourceChanceMultCorpse", static_cast<double>(_loot.lootSourceChanceMultCorpse)));
			_loot.lootSourceChanceMultContainer =
				static_cast<float>(loot.value("lootSourceChanceMultContainer", static_cast<double>(_loot.lootSourceChanceMultContainer)));
			_loot.lootSourceChanceMultBossContainer =
				static_cast<float>(loot.value("lootSourceChanceMultBossContainer", static_cast<double>(_loot.lootSourceChanceMultBossContainer)));
			_loot.lootSourceChanceMultWorld =
				static_cast<float>(loot.value("lootSourceChanceMultWorld", static_cast<double>(_loot.lootSourceChanceMultWorld)));
			_loot.renameItem = loot.value("renameItem", false);
			_loot.sharedPool = loot.value("sharedPool", false);
			_loot.debugLog = loot.value("debugLog", false);
			_loot.dotTagSafetyAutoDisable = loot.value("dotTagSafetyAutoDisable", _loot.dotTagSafetyAutoDisable);
			const double dotTagSafetyThreshold =
				loot.value("dotTagSafetyUniqueEffectThreshold", static_cast<double>(_loot.dotTagSafetyUniqueEffectThreshold));
			const double trapGlobalMaxActive = loot.value("trapGlobalMaxActive", static_cast<double>(_loot.trapGlobalMaxActive));
			const double trapCastBudgetPerTick = loot.value("trapCastBudgetPerTick", static_cast<double>(_loot.trapCastBudgetPerTick));
			const double triggerProcBudgetPerWindow = loot.value("triggerProcBudgetPerWindow", static_cast<double>(_loot.triggerProcBudgetPerWindow));
			const double triggerProcBudgetWindowMs = loot.value("triggerProcBudgetWindowMs", static_cast<double>(_loot.triggerProcBudgetWindowMs));
			_loot.cleanupInvalidLegacyAffixes = loot.value("cleanupInvalidLegacyAffixes", _loot.cleanupInvalidLegacyAffixes);
			_loot.stripTrackedSuffixSlots = loot.value("stripTrackedSuffixSlots", _loot.stripTrackedSuffixSlots);
			_loot.nameFormat = loot.value("nameFormat", std::string{ "{base} [{affix}]" });
			if (const auto markerPosIt = loot.find("nameMarkerPosition"); markerPosIt != loot.end()) {
				if (markerPosIt->is_string()) {
					if (const auto isTrailing = ParseLootNameMarkerPositionIsTrailing(markerPosIt->get<std::string>()); isTrailing.has_value()) {
						_loot.nameMarkerPosition = *isTrailing ? LootNameMarkerPosition::kTrailing : LootNameMarkerPosition::kLeading;
					} else {
						SKSE::log::warn(
							"CalamityAffixes: invalid loot.nameMarkerPosition '{}'; expected 'leading' or 'trailing'. Falling back to trailing.",
							markerPosIt->get<std::string>());
						_loot.nameMarkerPosition = LootNameMarkerPosition::kTrailing;
					}
				} else {
					SKSE::log::warn(
						"CalamityAffixes: loot.nameMarkerPosition must be a string ('leading' or 'trailing'). Falling back to trailing.");
					_loot.nameMarkerPosition = LootNameMarkerPosition::kTrailing;
				}
			}

			if (const auto denyIt = loot.find("armorEditorIdDenyContains"); denyIt != loot.end()) {
				if (denyIt->is_array()) {
					_loot.armorEditorIdDenyContains.clear();
					for (const auto& raw : *denyIt) {
						if (!raw.is_string()) {
							continue;
						}
						std::string markerRaw = raw.get<std::string>();
						const auto marker = Trim(markerRaw);
						if (!marker.empty()) {
							_loot.armorEditorIdDenyContains.emplace_back(marker);
						}
					}
				} else {
					SKSE::log::warn("CalamityAffixes: loot.armorEditorIdDenyContains must be an array of strings.");
				}
			}

			if (const auto allowIt = loot.find("bossContainerEditorIdAllowContains"); allowIt != loot.end()) {
				if (allowIt->is_array()) {
					_loot.bossContainerEditorIdAllowContains.clear();
					for (const auto& raw : *allowIt) {
						if (!raw.is_string()) {
							continue;
						}
						std::string markerRaw = raw.get<std::string>();
						const auto marker = Trim(markerRaw);
						if (!marker.empty()) {
							_loot.bossContainerEditorIdAllowContains.emplace_back(marker);
						}
					}
				} else {
					SKSE::log::warn("CalamityAffixes: loot.bossContainerEditorIdAllowContains must be an array of strings.");
				}
			}

			if (const auto denyBossIt = loot.find("bossContainerEditorIdDenyContains"); denyBossIt != loot.end()) {
				if (denyBossIt->is_array()) {
					_loot.bossContainerEditorIdDenyContains.clear();
					for (const auto& raw : *denyBossIt) {
						if (!raw.is_string()) {
							continue;
						}
						std::string markerRaw = raw.get<std::string>();
						const auto marker = Trim(markerRaw);
						if (!marker.empty()) {
							_loot.bossContainerEditorIdDenyContains.emplace_back(marker);
						}
					}
				} else {
					SKSE::log::warn("CalamityAffixes: loot.bossContainerEditorIdDenyContains must be an array of strings.");
				}
			}

			if (_loot.chancePercent < 0.0f) {
				_loot.chancePercent = 0.0f;
			} else if (_loot.chancePercent > 100.0f) {
				_loot.chancePercent = 100.0f;
			}

			if (_loot.runewordFragmentChancePercent < 0.0f) {
				_loot.runewordFragmentChancePercent = 0.0f;
			} else if (_loot.runewordFragmentChancePercent > 100.0f) {
				_loot.runewordFragmentChancePercent = 100.0f;
			}

			if (_loot.reforgeOrbChancePercent < 0.0f) {
				_loot.reforgeOrbChancePercent = 0.0f;
			} else if (_loot.reforgeOrbChancePercent > 100.0f) {
				_loot.reforgeOrbChancePercent = 100.0f;
			}

			_loot.configuredRunewordFragmentChancePercent = _loot.runewordFragmentChancePercent;
			_loot.configuredReforgeOrbChancePercent = _loot.reforgeOrbChancePercent;

			_loot.lootSourceChanceMultCorpse = std::clamp(_loot.lootSourceChanceMultCorpse, 0.0f, 5.0f);
			_loot.lootSourceChanceMultContainer = std::clamp(_loot.lootSourceChanceMultContainer, 0.0f, 5.0f);
			_loot.lootSourceChanceMultBossContainer = std::clamp(_loot.lootSourceChanceMultBossContainer, 0.0f, 5.0f);
			_loot.lootSourceChanceMultWorld = std::clamp(_loot.lootSourceChanceMultWorld, 0.0f, 5.0f);

			if (dotTagSafetyThreshold <= 0.0) {
				_loot.dotTagSafetyUniqueEffectThreshold = 0u;
			} else {
				const double clamped = std::clamp(dotTagSafetyThreshold, 8.0, 4096.0);
				_loot.dotTagSafetyUniqueEffectThreshold = static_cast<std::uint32_t>(clamped);
			}

			if (trapGlobalMaxActive <= 0.0) {
				_loot.trapGlobalMaxActive = 0u;
			} else {
				const double clamped = std::clamp(trapGlobalMaxActive, 1.0, 4096.0);
				_loot.trapGlobalMaxActive = static_cast<std::uint32_t>(clamped);
			}

			if (trapCastBudgetPerTick <= 0.0) {
				_loot.trapCastBudgetPerTick = 0u;
			} else {
				const double clamped = std::clamp(trapCastBudgetPerTick, 1.0, 256.0);
				_loot.trapCastBudgetPerTick = static_cast<std::uint32_t>(clamped);
			}

			if (triggerProcBudgetPerWindow <= 0.0) {
				_loot.triggerProcBudgetPerWindow = 0u;
			} else {
				const double clamped = std::clamp(triggerProcBudgetPerWindow, 1.0, 512.0);
				_loot.triggerProcBudgetPerWindow = static_cast<std::uint32_t>(clamped);
			}

			if (triggerProcBudgetWindowMs <= 0.0) {
				_loot.triggerProcBudgetWindowMs = 0u;
			} else {
				const double clamped = std::clamp(triggerProcBudgetWindowMs, 1.0, 5000.0);
				_loot.triggerProcBudgetWindowMs = static_cast<std::uint32_t>(clamped);
			}
		}

		SyncCurrencyDropModeState("ApplyLootConfigFromJson");

		spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
		spdlog::flush_on(spdlog::level::info);
	}

	void EventBridge::SyncCurrencyDropModeState(std::string_view a_contextTag)
	{
		// Currency drop mode is intentionally fixed to hybrid.
		_loot.currencyDropMode = CurrencyDropMode::kHybrid;
		_loot.runtimeCurrencyDropsEnabled = true;

		// SPID-owned corpse authority: leveled-list chanceNone values come from the ESP
		// (set by Generator) and are NOT modified at runtime. MCM drop-chance settings
		// only affect container-source runtime rolls.

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: {} currency mode synced (mode={}, runtimeEnabled={}, configuredRune={}%, configuredReforge={}%, currentRune={}%, currentReforge={}%).",
				a_contextTag,
				"hybrid",
				_loot.runtimeCurrencyDropsEnabled,
				_loot.configuredRunewordFragmentChancePercent,
				_loot.configuredReforgeOrbChancePercent,
				_loot.runewordFragmentChancePercent,
				_loot.reforgeOrbChancePercent);
		}
	}
}
