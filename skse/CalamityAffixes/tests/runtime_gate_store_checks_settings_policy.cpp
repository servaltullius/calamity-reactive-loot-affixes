#include "runtime_gate_store_checks_common.h"

namespace RuntimeGateStoreChecks
{
		bool CheckRuntimeUserSettingsDebounceBehavior()
		{
			using namespace std::chrono;

			CalamityAffixes::RuntimeUserSettingsDebounce::State state{};
			state.lastPersistedPayload = R"({"enabled":true})";
			const auto base = steady_clock::time_point{ milliseconds(1000) };
			const auto debounce = milliseconds(250);

			if (CalamityAffixes::RuntimeUserSettingsDebounce::MarkDirty(
					state,
					state.lastPersistedPayload,
					base,
					debounce)) {
				std::cerr << "runtime_settings_debounce: identical payload should not mark state dirty\n";
				return false;
			}
			if (state.dirty || !state.pendingPayload.empty()) {
				std::cerr << "runtime_settings_debounce: identical payload should clear pending state\n";
				return false;
			}

			if (!CalamityAffixes::RuntimeUserSettingsDebounce::MarkDirty(
					state,
					R"({"enabled":false})",
					base,
					debounce)) {
				std::cerr << "runtime_settings_debounce: changed payload should mark state dirty\n";
				return false;
			}
			if (!state.dirty || state.pendingPayload.empty()) {
				std::cerr << "runtime_settings_debounce: changed payload should be queued\n";
				return false;
			}

			if (CalamityAffixes::RuntimeUserSettingsDebounce::ShouldFlush(state, base + milliseconds(200), false)) {
				std::cerr << "runtime_settings_debounce: flush should be deferred before debounce expires\n";
				return false;
			}
			if (!CalamityAffixes::RuntimeUserSettingsDebounce::ShouldFlush(state, base + milliseconds(250), false)) {
				std::cerr << "runtime_settings_debounce: flush should be allowed at debounce boundary\n";
				return false;
			}
			if (!CalamityAffixes::RuntimeUserSettingsDebounce::ShouldFlush(state, base + milliseconds(1), true)) {
				std::cerr << "runtime_settings_debounce: forced flush should bypass debounce\n";
				return false;
			}

			CalamityAffixes::RuntimeUserSettingsDebounce::MarkPersistFailure(state, base + milliseconds(260), debounce);
			if (CalamityAffixes::RuntimeUserSettingsDebounce::ShouldFlush(state, base + milliseconds(300), false)) {
				std::cerr << "runtime_settings_debounce: failed persist should re-arm debounce window\n";
				return false;
			}
			if (!CalamityAffixes::RuntimeUserSettingsDebounce::ShouldFlush(state, base + milliseconds(520), false)) {
				std::cerr << "runtime_settings_debounce: retry flush should be allowed after re-armed debounce\n";
				return false;
			}

			state.lastPersistedPayload = state.pendingPayload;
			CalamityAffixes::RuntimeUserSettingsDebounce::MarkPersistSuccess(state);
			if (state.dirty || !state.pendingPayload.empty()) {
				std::cerr << "runtime_settings_debounce: successful persist should clear pending state\n";
				return false;
			}

			return true;
		}

		bool CheckExternalUserSettingsPersistencePolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path projectRoot = testFile.parent_path().parent_path();
			const fs::path includeRoot = projectRoot / "include" / "CalamityAffixes";
			const fs::path srcRoot = projectRoot / "src";

			if (CalamityAffixes::RuntimePolicy::kUserSettingsRelativePath !=
					"Data/SKSE/Plugins/CalamityAffixes/user_settings.json" ||
				CalamityAffixes::RuntimePolicy::kRuntimeContractRelativePath !=
					"Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json") {
				std::cerr << "external_user_settings_persistence: runtime path contract changed unexpectedly\n";
				return false;
			}

			for (const auto eventName : CalamityAffixes::RuntimePolicy::kPersistedRuntimeUserSettingEventNames) {
				if (!CalamityAffixes::RuntimePolicy::IsPersistedRuntimeUserSettingEvent(eventName)) {
					std::cerr << "external_user_settings_persistence: persisted runtime-setting event lookup failed: " << eventName << "\n";
					return false;
				}
			}
			if (CalamityAffixes::RuntimePolicy::IsPersistedRuntimeUserSettingEvent("CalamityAffixes_MCM_SetLootChance")) {
				std::cerr << "external_user_settings_persistence: deprecated loot chance event must not be persisted\n";
				return false;
			}

			const auto hasDuplicates = [](const auto& names) {
				for (std::size_t i = 0; i < names.size(); ++i) {
					for (std::size_t j = i + 1; j < names.size(); ++j) {
						if (names[i] == names[j]) {
							return true;
						}
					}
				}
				return false;
			};
			if (hasDuplicates(CalamityAffixes::RuntimePolicy::kPersistedRuntimeUserSettingEventNames)) {
				std::cerr << "external_user_settings_persistence: duplicate runtime-setting persisted event names found\n";
				return false;
			}
			if (hasDuplicates(CalamityAffixes::RuntimePolicy::kRuntimeUserSettingKeys)) {
				std::cerr << "external_user_settings_persistence: duplicate runtime-setting keys found\n";
				return false;
			}

			const std::array<fs::path, 6> requiredFiles{
				includeRoot / "UserSettingsPersistence.h",
				includeRoot / "RuntimePaths.h",
				includeRoot / "RuntimeUserSettingsDebounce.h",
				srcRoot / "UserSettingsPersistence.cpp",
				srcRoot / "RuntimePaths.cpp",
				srcRoot / "EventBridge.Config.RuntimeSettings.cpp"
			};
			for (const auto& path : requiredFiles) {
				if (!fs::exists(path)) {
					std::cerr << "external_user_settings_persistence: required module file missing: " << path << "\n";
					return false;
				}
			}

			return true;
		}

		bool CheckRuntimeUserSettingsRoundTripFieldPolicy()
		{
			const auto& runtimeKeys = CalamityAffixes::RuntimePolicy::kRuntimeUserSettingKeys;
			if (runtimeKeys.size() != 8u) {
				std::cerr << "runtime_user_settings_round_trip: runtime user-setting key count changed unexpectedly\n";
				return false;
			}
			for (const auto key : runtimeKeys) {
				if (key.empty()) {
					std::cerr << "runtime_user_settings_round_trip: runtime user-setting key must not be empty\n";
					return false;
				}
			}
			for (std::size_t i = 0; i < runtimeKeys.size(); ++i) {
				for (std::size_t j = i + 1; j < runtimeKeys.size(); ++j) {
					if (runtimeKeys[i] == runtimeKeys[j]) {
						std::cerr << "runtime_user_settings_round_trip: duplicate runtime user-setting key found: "
						          << runtimeKeys[i] << "\n";
						return false;
					}
				}
			}
			for (const auto key : runtimeKeys) {
				if (key == "lootChancePercent") {
					std::cerr << "runtime_user_settings_round_trip: deprecated lootChancePercent key must stay removed\n";
					return false;
				}
			}

			return true;
		}

		bool CheckEventBridgeStateMutexReentrancyPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path eventBridgeHeader = testFile.parent_path().parent_path() / "include" / "CalamityAffixes" / "EventBridge.h";

			std::ifstream in(eventBridgeHeader);
			if (!in.is_open()) {
				std::cerr << "eventbridge_state_mutex_reentrancy: failed to open header: " << eventBridgeHeader << "\n";
				return false;
			}

			const std::string source(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());

			if (source.find("std::recursive_mutex _stateMutex;") == std::string::npos) {
				std::cerr << "eventbridge_state_mutex_reentrancy: EventBridge state mutex must remain recursive for re-entrant mod callback dispatch\n";
				return false;
			}

			return true;
		}
}
