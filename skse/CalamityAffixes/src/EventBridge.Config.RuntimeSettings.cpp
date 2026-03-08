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
		_runtimeSettings.userSettingsPersist = {};
		_runtimeSettings.userSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();

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
			const bool hadLegacyPlayerHookOverride = runtime.contains("allowPlayerHealthDamageHook");
			_runtimeSettings.enabled = runtime.value("enabled", _runtimeSettings.enabled);
			_loot.debugLog = runtime.value("debugNotifications", _loot.debugLog);
			_runtimeSettings.combatDebugLog = runtime.value("debugCombat", _runtimeSettings.combatDebugLog);
			_loot.dotTagSafetyAutoDisable = runtime.value("dotSafetyAutoDisable", _loot.dotTagSafetyAutoDisable);
			_runtimeSettings.disableCombatEvidenceLease = runtime.value("disableCombatEvidenceLease", _runtimeSettings.disableCombatEvidenceLease);
			_runtimeSettings.disableHealthDamageRouting = runtime.value("disableHealthDamageRouting", _runtimeSettings.disableHealthDamageRouting);
			_runtimeSettings.allowPlayerHealthDamageHook = true;
			_runtimeSettings.disablePassiveSuffixSpells = runtime.value("disablePassiveSuffixSpells", _runtimeSettings.disablePassiveSuffixSpells);
			_runtimeSettings.disableTrapSystemTick = runtime.value("disableTrapSystemTick", _runtimeSettings.disableTrapSystemTick);
			_runtimeSettings.disableTrapCasts = runtime.value("disableTrapCasts", _runtimeSettings.disableTrapCasts);
			_runtimeSettings.forceStopAlarmPulse = runtime.value("forceStopAlarmPulse", _runtimeSettings.forceStopAlarmPulse);

			const double validationIntervalSeconds =
				runtime.value("validationIntervalSeconds", static_cast<double>(_equipResync.intervalMs) / 1000.0);
			const double procChanceMultiplier =
				runtime.value("procChanceMultiplier", static_cast<double>(_runtimeSettings.procChanceMult));
			const double runewordFragmentChancePercent =
				runtime.value("runewordFragmentChancePercent", static_cast<double>(_loot.runewordFragmentChancePercent));
			const double reforgeOrbChancePercent =
				runtime.value("reforgeOrbChancePercent", static_cast<double>(_loot.reforgeOrbChancePercent));
			const bool allowNonHostileFirstHitProc = runtime.value(
				"allowNonHostileFirstHitProc",
				_runtimeSettings.allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed));

			const float clampedValidationIntervalSeconds =
				std::clamp(static_cast<float>(validationIntervalSeconds), 0.0f, 30.0f);
			_equipResync.intervalMs =
				(clampedValidationIntervalSeconds <= 0.0f)
					? 0u
					: static_cast<std::uint64_t>(clampedValidationIntervalSeconds * 1000.0f);
			_equipResync.nextAtMs = 0;

			_runtimeSettings.procChanceMult = std::clamp(static_cast<float>(procChanceMultiplier), 0.0f, 3.0f);
			_loot.runewordFragmentChancePercent = std::clamp(static_cast<float>(runewordFragmentChancePercent), 0.0f, 100.0f);
			_loot.reforgeOrbChancePercent = std::clamp(static_cast<float>(reforgeOrbChancePercent), 0.0f, 100.0f);
			_runtimeSettings.allowNonHostilePlayerOwnedOutgoingProcs.store(allowNonHostileFirstHitProc, std::memory_order_relaxed);

			if (hadLegacyPlayerHookOverride) {
				SKSE::log::warn(
					"CalamityAffixes: runtime override 'allowPlayerHealthDamageHook' is ignored; player incoming hits now use the HandleHealthDamage hook by default.");
			}
		} catch (const nlohmann::json::exception& e) {
			SKSE::log::warn(
				"CalamityAffixes: JSON error reading user runtime settings; using defaults. ({})",
				e.what());
		}
		SyncCurrencyDropModeState("ApplyRuntimeUserSettingsOverrides");

		if (!_loot.dotTagSafetyAutoDisable) {
			_combatState.dotTagSafetySuppressed = false;
		}

		spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
		spdlog::flush_on(spdlog::level::warn);

		SKSE::log::info(
			"CalamityAffixes: runtime overrides loaded from {} (enabled={}, procMult={}, runeFrag={}%, reforgeOrb={}%, runtimeCurrencyDropsEnabled={}, debugCombat={}, disableCombatEvidenceLease={}, disableHealthDamageRouting={}, allowPlayerHealthDamageHook={}, disablePassiveSuffixSpells={}, disableTrapSystemTick={}, disableTrapCasts={}, forceStopAlarmPulse={}).",
			std::string(kUserSettingsRelativePath),
			_runtimeSettings.enabled,
			_runtimeSettings.procChanceMult,
			_loot.runewordFragmentChancePercent,
			_loot.reforgeOrbChancePercent,
			_loot.runtimeCurrencyDropsEnabled,
			_runtimeSettings.combatDebugLog,
			_runtimeSettings.disableCombatEvidenceLease,
			_runtimeSettings.disableHealthDamageRouting,
			_runtimeSettings.allowPlayerHealthDamageHook,
			_runtimeSettings.disablePassiveSuffixSpells,
			_runtimeSettings.disableTrapSystemTick,
			_runtimeSettings.disableTrapCasts,
			_runtimeSettings.forceStopAlarmPulse);

		_runtimeSettings.userSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();
	}

	nlohmann::json EventBridge::BuildRuntimeUserSettingsJson() const
	{
		nlohmann::json runtime = nlohmann::json::object();
		runtime["enabled"] = _runtimeSettings.enabled;
		runtime["debugNotifications"] = _loot.debugLog;
		runtime["debugCombat"] = _runtimeSettings.combatDebugLog;
		runtime["validationIntervalSeconds"] = static_cast<double>(_equipResync.intervalMs) / 1000.0;
		runtime["procChanceMultiplier"] = _runtimeSettings.procChanceMult;
		runtime["runewordFragmentChancePercent"] = _loot.runewordFragmentChancePercent;
		runtime["reforgeOrbChancePercent"] = _loot.reforgeOrbChancePercent;
		runtime["dotSafetyAutoDisable"] = _loot.dotTagSafetyAutoDisable;
		runtime["allowNonHostileFirstHitProc"] =
			_runtimeSettings.allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed);
		runtime["disableCombatEvidenceLease"] = _runtimeSettings.disableCombatEvidenceLease;
		runtime["disableHealthDamageRouting"] = _runtimeSettings.disableHealthDamageRouting;
		runtime["allowPlayerHealthDamageHook"] = _runtimeSettings.allowPlayerHealthDamageHook;
		runtime["disablePassiveSuffixSpells"] = _runtimeSettings.disablePassiveSuffixSpells;
		runtime["disableTrapSystemTick"] = _runtimeSettings.disableTrapSystemTick;
		runtime["disableTrapCasts"] = _runtimeSettings.disableTrapCasts;
		runtime["forceStopAlarmPulse"] = _runtimeSettings.forceStopAlarmPulse;
		return runtime;
	}

	std::string EventBridge::BuildRuntimeUserSettingsPayload() const
	{
		return BuildRuntimeUserSettingsJson().dump();
	}

	void EventBridge::MarkRuntimeUserSettingsDirty()
	{
		(void)RuntimeUserSettingsDebounce::MarkDirty(
			_runtimeSettings.userSettingsPersist,
			BuildRuntimeUserSettingsPayload(),
			std::chrono::steady_clock::now(),
			kRuntimeUserSettingsPersistDebounce);
	}

	void EventBridge::QueueRuntimeUserSettingsPersist()
	{
		MarkRuntimeUserSettingsDirty();
		MaybeFlushRuntimeUserSettings(std::chrono::steady_clock::now(), true);
	}

	void EventBridge::MaybeFlushRuntimeUserSettings(std::chrono::steady_clock::time_point a_now, bool a_force)
	{
		if (!RuntimeUserSettingsDebounce::ShouldFlush(_runtimeSettings.userSettingsPersist, a_now, a_force)) {
			return;
		}

		if (_runtimeSettings.userSettingsPersist.pendingPayload.empty()) {
			_runtimeSettings.userSettingsPersist.pendingPayload = BuildRuntimeUserSettingsPayload();
		}
		if (_runtimeSettings.userSettingsPersist.pendingPayload == _runtimeSettings.userSettingsPersist.lastPersistedPayload) {
			RuntimeUserSettingsDebounce::MarkPersistSuccess(_runtimeSettings.userSettingsPersist);
			return;
		}

		if (PersistRuntimeUserSettings()) {
			_runtimeSettings.userSettingsPersist.lastPersistedPayload = _runtimeSettings.userSettingsPersist.pendingPayload;
			RuntimeUserSettingsDebounce::MarkPersistSuccess(_runtimeSettings.userSettingsPersist);
		} else {
			RuntimeUserSettingsDebounce::MarkPersistFailure(
				_runtimeSettings.userSettingsPersist,
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
