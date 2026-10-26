#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PlayerHealthDamageHookPolicy.h"
#include "CalamityAffixes/RuntimePaths.h"
#include "CalamityAffixes/UserSettingsMigration.h"
#include "CalamityAffixes/UserSettingsPersistence.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		constexpr std::string_view kMcmUserSettingsIniRelativePath = "Data/MCM/Settings/CalamityAffixes.ini";

		[[nodiscard]] std::optional<double> ParseIniNumber(std::string_view a_text)
		{
			double value = 0.0;
			const auto* end = a_text.data() + a_text.size();
			const auto [ptr, ec] = std::from_chars(a_text.data(), end, value);
			if (ec != std::errc{} || ptr != end) {
				return std::nullopt;
			}
			return value;
		}
	}

	bool EventBridge::SeedRuntimeUserSettingsFromMcmIni()
	{
		// Only the player's own MCM file (MCM Helper writes it on change), never the
		// shipped defaults under MCM/Config.
		const auto path = RuntimePaths::ResolveRuntimeRelativePath(kMcmUserSettingsIniRelativePath);
		std::ifstream in(path, std::ios::binary);
		if (!in.is_open()) {
			return false;
		}
		const std::string text{ std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };

		std::uint32_t applied = 0u;
		auto number = [&](std::string_view a_key) -> std::optional<double> {
			const auto raw = UserSettingsMigration::FindIniValue(text, "General", a_key);
			auto value = raw.empty() ? std::nullopt : ParseIniNumber(raw);
			if (value) {
				++applied;
			}
			return value;
		};
		if (const auto v = number("bEnabled")) {
			_runtimeSettings.enabled.store(*v != 0.0, std::memory_order_relaxed);
		}
		if (const auto v = number("fValidationInterval")) {
			const float seconds = std::clamp(static_cast<float>(*v), 0.0f, 30.0f);
			_equipResync.intervalMs = seconds <= 0.0f ? 0u : static_cast<std::uint64_t>(seconds * 1000.0f);
			_equipResync.nextAtMs = 0;
		}
		if (const auto v = number("fProcChanceMult")) {
			_runtimeSettings.procChanceMult = std::clamp(static_cast<float>(*v), 0.0f, 3.0f);
		}
		if (const auto v = number("fRunewordFragmentChancePercent")) {
			_loot.runewordFragmentChancePercent = std::clamp(static_cast<float>(*v), 0.0f, 100.0f);
		}
		if (const auto v = number("fReforgeOrbChancePercent")) {
			_loot.reforgeOrbChancePercent = std::clamp(static_cast<float>(*v), 0.0f, 100.0f);
		}
		if (const auto v = number("fIdentifyScrollChancePercent")) {
			_loot.identifyScrollChancePercent = std::clamp(static_cast<float>(*v), 0.0f, 100.0f);
		}
		if (const auto v = number("fScouringOrbChancePercent")) {
			_loot.scouringOrbChancePercent = std::clamp(static_cast<float>(*v), 0.0f, 100.0f);
		}
		if (const auto v = number("bDotSafetyAutoDisable")) {
			_loot.dotTagSafetyAutoDisable = *v != 0.0;
		}
		if (const auto v = number("bAllowNonHostileFirstHitProc")) {
			_runtimeSettings.allowNonHostilePlayerOwnedOutgoingProcs.store(*v != 0.0, std::memory_order_relaxed);
		}
		if (applied == 0u) {
			return false;
		}
		SKSE::log::info(
			"CalamityAffixes: {} is missing; took {} settings from {}.",
			std::string(kUserSettingsRelativePath),
			applied,
			std::string(kMcmUserSettingsIniRelativePath));
		return true;
	}

	void EventBridge::ApplyRuntimeUserSettingsOverrides()
	{
		_runtimeSettings.userSettingsPersist = {};
		_runtimeSettings.userSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();

		nlohmann::json root;
		if (!UserSettingsPersistence::LoadJsonObject(kUserSettingsRelativePath, root)) {
			// The runtime file is gone (an MO2 overwrite clean-up) but the MCM still
			// shows the player's choices: run with those instead of the defaults,
			// and write them down so the next start reads the file again.
			std::error_code ec;
			const bool missing = !std::filesystem::exists(RuntimePaths::ResolveRuntimeRelativePath(kUserSettingsRelativePath), ec) && !ec;
			if (missing && SeedRuntimeUserSettingsFromMcmIni()) {
				SyncCurrencyDropModeState("SeedRuntimeUserSettingsFromMcmIni");
				(void)PersistRuntimeUserSettings();
				_runtimeSettings.userSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();
			}
			return;
		}
		int storedVersion = 0;
		if (const auto versionIt = root.find("version"); versionIt != root.end() && versionIt->is_number_integer()) {
			storedVersion = versionIt->get<int>();
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
			const bool hasDebugHudNotifications = runtime.contains("debugHudNotifications");
			const bool hasDebugVerboseLogging = runtime.contains("debugVerboseLogging");
			const bool useLegacyDebugNotifications =
				!hasDebugHudNotifications &&
				!hasDebugVerboseLogging &&
				runtime.contains("debugNotifications");
			const bool legacyDebugNotifications =
				useLegacyDebugNotifications
					? runtime.value("debugNotifications", _loot.debugLog)
					: false;
			_runtimeSettings.enabled.store(
				runtime.value(
					"enabled",
					_runtimeSettings.enabled.load(std::memory_order_relaxed)),
				std::memory_order_relaxed);
			if (hasDebugHudNotifications) {
				_loot.debugHudNotifications = runtime.value("debugHudNotifications", _loot.debugHudNotifications);
			} else if (useLegacyDebugNotifications) {
				_loot.debugHudNotifications = legacyDebugNotifications;
			}
			if (hasDebugVerboseLogging) {
				_loot.debugLog = runtime.value("debugVerboseLogging", _loot.debugLog);
			} else if (useLegacyDebugNotifications) {
				_loot.debugLog = legacyDebugNotifications;
			}
			_runtimeSettings.combatDebugLog = runtime.value("debugCombat", _runtimeSettings.combatDebugLog);
			_loot.dotTagSafetyAutoDisable = runtime.value("dotSafetyAutoDisable", _loot.dotTagSafetyAutoDisable);
			_runtimeSettings.disableCombatEvidenceLease = runtime.value("disableCombatEvidenceLease", _runtimeSettings.disableCombatEvidenceLease);
			_runtimeSettings.disableHealthDamageRouting = runtime.value("disableHealthDamageRouting", _runtimeSettings.disableHealthDamageRouting);
			// hadLegacyPlayerHookOverride is intentionally NOT passed here: it only
			// drives the deprecation warning below. The hook is unconditional.
			_runtimeSettings.allowPlayerHealthDamageHook = detail::ResolvePlayerHealthDamageHookEnabled();
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
			const double identifyScrollChancePercent =
				runtime.value("identifyScrollChancePercent", static_cast<double>(_loot.identifyScrollChancePercent));
			// One-time, by file version: a 2 the player picks after the bump stays 2.
			const double scouringOrbChancePercent = UserSettingsMigration::MigrateScouringOrbChancePercent(
				storedVersion,
				runtime.value("scouringOrbChancePercent", static_cast<double>(_loot.scouringOrbChancePercent)));
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
			_loot.identifyScrollChancePercent = std::clamp(static_cast<float>(identifyScrollChancePercent), 0.0f, 100.0f);
			_loot.scouringOrbChancePercent = std::clamp(static_cast<float>(scouringOrbChancePercent), 0.0f, 100.0f);
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

		ApplyVerboseLoggingLevel();
		spdlog::flush_on(spdlog::level::warn);

		SKSE::log::info(
			"CalamityAffixes: runtime overrides loaded from {} (enabled={}, procMult={}, runeFrag={}%, reforgeOrb={}%, identifyScroll={}%, scouringOrb={}%, broadRuntimeCurrencyDropsEnabled={}, corpseDeathCurrencyDropsEnabled={}, debugHud={}, debugVerbose={}, debugCombat={}, disableCombatEvidenceLease={}, disableHealthDamageRouting={}, allowPlayerHealthDamageHook={}, disablePassiveSuffixSpells={}, disableTrapSystemTick={}, disableTrapCasts={}, forceStopAlarmPulse={}).",
			std::string(kUserSettingsRelativePath),
			_runtimeSettings.enabled.load(std::memory_order_relaxed),
			_runtimeSettings.procChanceMult,
			_loot.runewordFragmentChancePercent,
			_loot.reforgeOrbChancePercent,
			_loot.identifyScrollChancePercent,
			_loot.scouringOrbChancePercent,
			_loot.runtimeCurrencyDropsEnabled,
			_loot.runtimeCorpseDeathCurrencyDropsEnabled,
			_loot.debugHudNotifications,
			_loot.debugLog,
			_runtimeSettings.combatDebugLog,
			_runtimeSettings.disableCombatEvidenceLease,
			_runtimeSettings.disableHealthDamageRouting,
			_runtimeSettings.allowPlayerHealthDamageHook,
			_runtimeSettings.disablePassiveSuffixSpells,
			_runtimeSettings.disableTrapSystemTick,
			_runtimeSettings.disableTrapCasts,
			_runtimeSettings.forceStopAlarmPulse);

		// Write the migrated values (and the new version) back right away, so the
		// migration runs once.
		if (storedVersion < UserSettingsMigration::kCurrentVersion) {
			(void)PersistRuntimeUserSettings();
		}
		_runtimeSettings.userSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();
	}

	nlohmann::json EventBridge::BuildRuntimeUserSettingsJson() const
	{
		nlohmann::json runtime = nlohmann::json::object();
		runtime["enabled"] = _runtimeSettings.enabled.load(std::memory_order_relaxed);
		runtime["debugHudNotifications"] = _loot.debugHudNotifications;
		runtime["debugVerboseLogging"] = _loot.debugLog;
		runtime["debugCombat"] = _runtimeSettings.combatDebugLog;
		runtime["validationIntervalSeconds"] = static_cast<double>(_equipResync.intervalMs) / 1000.0;
		runtime["procChanceMultiplier"] = _runtimeSettings.procChanceMult;
		runtime["runewordFragmentChancePercent"] = _loot.runewordFragmentChancePercent;
		runtime["reforgeOrbChancePercent"] = _loot.reforgeOrbChancePercent;
		runtime["identifyScrollChancePercent"] = _loot.identifyScrollChancePercent;
		runtime["scouringOrbChancePercent"] = _loot.scouringOrbChancePercent;
		runtime["dotSafetyAutoDisable"] = _loot.dotTagSafetyAutoDisable;
		runtime["allowNonHostileFirstHitProc"] =
			_runtimeSettings.allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed);
		runtime["disableCombatEvidenceLease"] = _runtimeSettings.disableCombatEvidenceLease;
		runtime["disableHealthDamageRouting"] = _runtimeSettings.disableHealthDamageRouting;
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
				root["version"] = UserSettingsMigration::kCurrentVersion;
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
