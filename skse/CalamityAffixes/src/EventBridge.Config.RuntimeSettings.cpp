#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/UserSettingsPersistence.h"

#include <algorithm>
#include <string>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
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

		try {
			const auto& runtime = *runtimeIt;
			_runtimeEnabled = runtime.value("enabled", _runtimeEnabled);
			_loot.debugLog = runtime.value("debugNotifications", _loot.debugLog);
			_combatDebugLog = runtime.value("debugCombat", _combatDebugLog);
			_loot.dotTagSafetyAutoDisable = runtime.value("dotSafetyAutoDisable", _loot.dotTagSafetyAutoDisable);
			_disableCombatEvidenceLease = runtime.value("disableCombatEvidenceLease", _disableCombatEvidenceLease);
			_disableHealthDamageRouting = runtime.value("disableHealthDamageRouting", _disableHealthDamageRouting);
			_disablePassiveSuffixSpells = runtime.value("disablePassiveSuffixSpells", _disablePassiveSuffixSpells);
			_disableTrapSystemTick = runtime.value("disableTrapSystemTick", _disableTrapSystemTick);
			_disableTrapCasts = runtime.value("disableTrapCasts", _disableTrapCasts);
			_forceStopAlarmPulse = runtime.value("forceStopAlarmPulse", _forceStopAlarmPulse);

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
		} catch (const nlohmann::json::exception& e) {
			SKSE::log::warn(
				"CalamityAffixes: JSON error reading user runtime settings; using defaults. ({})",
				e.what());
		}
		SyncCurrencyDropModeState("ApplyRuntimeUserSettingsOverrides");

		if (!_loot.dotTagSafetyAutoDisable) {
			_dotTagSafetySuppressed = false;
		}

		spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
		spdlog::flush_on(spdlog::level::warn);

		SKSE::log::info(
			"CalamityAffixes: runtime overrides loaded from {} (enabled={}, procMult={}, runeFrag={}%, reforgeOrb={}%, runtimeCurrencyDropsEnabled={}, debugCombat={}, disableCombatEvidenceLease={}, disableHealthDamageRouting={}, disablePassiveSuffixSpells={}, disableTrapSystemTick={}, disableTrapCasts={}, forceStopAlarmPulse={}).",
			std::string(kUserSettingsRelativePath),
			_runtimeEnabled,
			_runtimeProcChanceMult,
			_loot.runewordFragmentChancePercent,
			_loot.reforgeOrbChancePercent,
			_loot.runtimeCurrencyDropsEnabled,
			_combatDebugLog,
			_disableCombatEvidenceLease,
			_disableHealthDamageRouting,
			_disablePassiveSuffixSpells,
			_disableTrapSystemTick,
			_disableTrapCasts,
			_forceStopAlarmPulse);

		_runtimeUserSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();
	}

	nlohmann::json EventBridge::BuildRuntimeUserSettingsJson() const
	{
		nlohmann::json runtime = nlohmann::json::object();
		runtime["enabled"] = _runtimeEnabled;
		runtime["debugNotifications"] = _loot.debugLog;
		runtime["debugCombat"] = _combatDebugLog;
		runtime["validationIntervalSeconds"] = static_cast<double>(_equipResync.intervalMs) / 1000.0;
		runtime["procChanceMultiplier"] = _runtimeProcChanceMult;
		runtime["runewordFragmentChancePercent"] = _loot.runewordFragmentChancePercent;
		runtime["reforgeOrbChancePercent"] = _loot.reforgeOrbChancePercent;
		runtime["dotSafetyAutoDisable"] = _loot.dotTagSafetyAutoDisable;
		runtime["allowNonHostileFirstHitProc"] =
			_allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed);
		runtime["disableCombatEvidenceLease"] = _disableCombatEvidenceLease;
		runtime["disableHealthDamageRouting"] = _disableHealthDamageRouting;
		runtime["disablePassiveSuffixSpells"] = _disablePassiveSuffixSpells;
		runtime["disableTrapSystemTick"] = _disableTrapSystemTick;
		runtime["disableTrapCasts"] = _disableTrapCasts;
		runtime["forceStopAlarmPulse"] = _forceStopAlarmPulse;
		return runtime;
	}

	std::string EventBridge::BuildRuntimeUserSettingsPayload() const
	{
		return BuildRuntimeUserSettingsJson().dump();
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
		const auto runtimeSnapshot = BuildRuntimeUserSettingsJson();
		const bool updated = UserSettingsPersistence::UpdateJsonObject(
			kUserSettingsRelativePath,
			[&runtimeSnapshot](nlohmann::json& root) {
				root["version"] = 1;
				auto& runtime = root["runtime"];
				if (!runtime.is_object()) {
					runtime = nlohmann::json::object();
				}
				for (auto it = runtimeSnapshot.begin(); it != runtimeSnapshot.end(); ++it) {
					runtime[it.key()] = it.value();
				}
			});

		if (!updated) {
			SKSE::log::warn(
				"CalamityAffixes: failed to persist runtime user settings to {}.",
				std::string(kUserSettingsRelativePath));
		}
		return updated;
	}
}
