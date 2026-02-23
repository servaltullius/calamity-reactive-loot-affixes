#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootEligibility.h"
#include "CalamityAffixes/RuntimePaths.h"
#include "CalamityAffixes/UserSettingsPersistence.h"
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
		} catch (const std::exception&) {
			SKSE::log::error("CalamityAffixes: failed to parse runtime config (affixes.json).");
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
		spdlog::flush_on(spdlog::level::warn);
	}

	void EventBridge::ApplyRuntimeUserSettingsOverrides()
	{
		_runtimeUserSettingsPersist = {};
		_runtimeUserSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();

		nlohmann::json root;
		if (!UserSettingsPersistence::LoadJsonObject(kUserSettingsRelativePath, root)) {
			return;
		}

		if (!root.is_object()) {
			SKSE::log::warn(
				"CalamityAffixes: user settings root is not an object ({}).",
				std::string(kUserSettingsRelativePath));
			return;
		}

		const auto runtimeIt = root.find("runtime");
		if (runtimeIt == root.end()) {
			return;
		}
		if (!runtimeIt->is_object()) {
			SKSE::log::warn(
				"CalamityAffixes: user settings runtime section is invalid ({}).",
				std::string(kUserSettingsRelativePath));
			return;
		}

		const auto& runtime = *runtimeIt;
		_runtimeEnabled = runtime.value("enabled", _runtimeEnabled);
		_loot.debugLog = runtime.value("debugNotifications", _loot.debugLog);
		_loot.dotTagSafetyAutoDisable = runtime.value("dotSafetyAutoDisable", _loot.dotTagSafetyAutoDisable);

		const double validationIntervalSeconds =
			runtime.value("validationIntervalSeconds", static_cast<double>(_equipResync.intervalMs) / 1000.0);
		const double procChanceMultiplier =
			runtime.value("procChanceMultiplier", static_cast<double>(_runtimeProcChanceMult));
		const double runewordFragmentChancePercent =
			runtime.value("runewordFragmentChancePercent", static_cast<double>(_loot.runewordFragmentChancePercent));
		const double reforgeOrbChancePercent =
			runtime.value("reforgeOrbChancePercent", static_cast<double>(_loot.reforgeOrbChancePercent));
		const bool allowNonHostileFirstHitProc = runtime.value(
			"allowNonHostileFirstHitProc",
			_allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed));

		const float clampedValidationIntervalSeconds =
			std::clamp(static_cast<float>(validationIntervalSeconds), 0.0f, 30.0f);
		_equipResync.intervalMs =
			(clampedValidationIntervalSeconds <= 0.0f)
				? 0u
				: static_cast<std::uint64_t>(clampedValidationIntervalSeconds * 1000.0f);
		_equipResync.nextAtMs = 0;

		_runtimeProcChanceMult = std::clamp(static_cast<float>(procChanceMultiplier), 0.0f, 3.0f);
		_loot.runewordFragmentChancePercent = std::clamp(static_cast<float>(runewordFragmentChancePercent), 0.0f, 100.0f);
		_loot.reforgeOrbChancePercent = std::clamp(static_cast<float>(reforgeOrbChancePercent), 0.0f, 100.0f);
		_allowNonHostilePlayerOwnedOutgoingProcs.store(allowNonHostileFirstHitProc, std::memory_order_relaxed);
		SyncCurrencyDropModeState("ApplyRuntimeUserSettingsOverrides");

		if (!_loot.dotTagSafetyAutoDisable) {
			_dotTagSafetySuppressed = false;
		}

		spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
		spdlog::flush_on(spdlog::level::warn);

		SKSE::log::info(
			"CalamityAffixes: runtime overrides loaded from {} (enabled={}, procMult={}, runeFrag={}%, reforgeOrb={}%, runtimeCurrencyDropsEnabled={}).",
			std::string(kUserSettingsRelativePath),
			_runtimeEnabled,
			_runtimeProcChanceMult,
			_loot.runewordFragmentChancePercent,
			_loot.reforgeOrbChancePercent,
			_loot.runtimeCurrencyDropsEnabled);

		_runtimeUserSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();
	}

	std::int8_t EventBridge::ToLeveledListChanceNonePercent(float a_dropChancePercent)
	{
		const auto clampedDrop = std::clamp(a_dropChancePercent, 0.0f, 100.0f);
		const auto chanceNoneRounded = static_cast<long>(std::lround(100.0 - static_cast<double>(clampedDrop)));
		const auto clampedChanceNone = std::clamp(chanceNoneRounded, 0l, 100l);
		return static_cast<std::int8_t>(clampedChanceNone);
	}

	void EventBridge::SyncLeveledListCurrencyDropChances(std::string_view a_contextTag) const
	{
		auto* runewordDropList = RE::TESForm::LookupByEditorID<RE::TESLevItem>("CAFF_LItem_RunewordFragmentDrops");
		auto* reforgeDropList = RE::TESForm::LookupByEditorID<RE::TESLevItem>("CAFF_LItem_ReforgeOrbDrops");
		if (!runewordDropList || !reforgeDropList) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: {} leveled-list currency sync skipped (runewordList={}, reforgeList={}).",
					a_contextTag,
					runewordDropList != nullptr,
					reforgeDropList != nullptr);
			}
			return;
		}

		const float runewordDropChance = _loot.runewordFragmentChancePercent;
		const float reforgeDropChance = _loot.reforgeOrbChancePercent;

		runewordDropList->chanceNone = ToLeveledListChanceNonePercent(runewordDropChance);
		reforgeDropList->chanceNone = ToLeveledListChanceNonePercent(reforgeDropChance);

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: {} leveled-list currency chances synced (runewordDrop={}%, reforgeDrop={}%).",
				a_contextTag,
				runewordDropChance,
				reforgeDropChance);
		}
	}

	void EventBridge::SyncCurrencyDropModeState(std::string_view a_contextTag)
	{
		// Currency drop mode is intentionally fixed to hybrid.
		_loot.currencyDropMode = CurrencyDropMode::kHybrid;
		_loot.runtimeCurrencyDropsEnabled = true;

		// Keep CAFF_LItem_* drop chances in sync for SPID Item corpse distribution
		// and any legacy leveled-list consumers.
		SyncLeveledListCurrencyDropChances(a_contextTag);

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

	std::string EventBridge::BuildRuntimeUserSettingsPayload() const
	{
		nlohmann::json runtime = nlohmann::json::object();
		runtime["enabled"] = _runtimeEnabled;
		runtime["debugNotifications"] = _loot.debugLog;
		runtime["validationIntervalSeconds"] = static_cast<double>(_equipResync.intervalMs) / 1000.0;
		runtime["procChanceMultiplier"] = _runtimeProcChanceMult;
		runtime["runewordFragmentChancePercent"] = _loot.runewordFragmentChancePercent;
		runtime["reforgeOrbChancePercent"] = _loot.reforgeOrbChancePercent;
		runtime["dotSafetyAutoDisable"] = _loot.dotTagSafetyAutoDisable;
		runtime["allowNonHostileFirstHitProc"] =
			_allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed);
		return runtime.dump();
	}

	void EventBridge::MarkRuntimeUserSettingsDirty()
	{
		(void)RuntimeUserSettingsDebounce::MarkDirty(
			_runtimeUserSettingsPersist,
			BuildRuntimeUserSettingsPayload(),
			std::chrono::steady_clock::now(),
			kRuntimeUserSettingsPersistDebounce);
	}

	void EventBridge::MaybeFlushRuntimeUserSettings(std::chrono::steady_clock::time_point a_now, bool a_force)
	{
		if (!RuntimeUserSettingsDebounce::ShouldFlush(_runtimeUserSettingsPersist, a_now, a_force)) {
			return;
		}

		if (_runtimeUserSettingsPersist.pendingPayload.empty()) {
			_runtimeUserSettingsPersist.pendingPayload = BuildRuntimeUserSettingsPayload();
		}
		if (_runtimeUserSettingsPersist.pendingPayload == _runtimeUserSettingsPersist.lastPersistedPayload) {
			RuntimeUserSettingsDebounce::MarkPersistSuccess(_runtimeUserSettingsPersist);
			return;
		}

		if (PersistRuntimeUserSettings()) {
			_runtimeUserSettingsPersist.lastPersistedPayload = _runtimeUserSettingsPersist.pendingPayload;
			RuntimeUserSettingsDebounce::MarkPersistSuccess(_runtimeUserSettingsPersist);
		} else {
			RuntimeUserSettingsDebounce::MarkPersistFailure(
				_runtimeUserSettingsPersist,
				a_now,
				kRuntimeUserSettingsPersistDebounce);
		}
	}

	bool EventBridge::PersistRuntimeUserSettings()
	{
		const bool updated = UserSettingsPersistence::UpdateJsonObject(
			kUserSettingsRelativePath,
			[&](nlohmann::json& root) {
				root["version"] = 1;
				auto& runtime = root["runtime"];
				if (!runtime.is_object()) {
					runtime = nlohmann::json::object();
				}

				runtime["enabled"] = _runtimeEnabled;
				runtime["debugNotifications"] = _loot.debugLog;
				runtime["validationIntervalSeconds"] = static_cast<double>(_equipResync.intervalMs) / 1000.0;
				runtime["procChanceMultiplier"] = _runtimeProcChanceMult;
				runtime["runewordFragmentChancePercent"] = _loot.runewordFragmentChancePercent;
				runtime["reforgeOrbChancePercent"] = _loot.reforgeOrbChancePercent;
				runtime["dotSafetyAutoDisable"] = _loot.dotTagSafetyAutoDisable;
				runtime["allowNonHostileFirstHitProc"] =
					_allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed);
			});

		if (!updated) {
			SKSE::log::warn(
				"CalamityAffixes: failed to persist runtime user settings to {}.",
				std::string(kUserSettingsRelativePath));
		}
		return updated;
	}
}
