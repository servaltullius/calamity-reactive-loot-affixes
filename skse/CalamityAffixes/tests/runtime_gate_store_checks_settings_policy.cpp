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
			const fs::path eventBridgeHeaderFile = testFile.parent_path().parent_path() / "include" / "CalamityAffixes" / "EventBridge.h";
			const fs::path eventBridgeConfigFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.cpp";
			const fs::path triggerEventsFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Triggers.Events.cpp";
			const fs::path prismaSettingsFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.SettingsLayout.inl";
			const fs::path persistenceHeaderFile = testFile.parent_path().parent_path() / "include" / "CalamityAffixes" / "UserSettingsPersistence.h";
			const fs::path persistenceSourceFile = testFile.parent_path().parent_path() / "src" / "UserSettingsPersistence.cpp";
			const fs::path runtimePathsHeaderFile = testFile.parent_path().parent_path() / "include" / "CalamityAffixes" / "RuntimePaths.h";
			const fs::path runtimePathsSourceFile = testFile.parent_path().parent_path() / "src" / "RuntimePaths.cpp";
			const fs::path debounceHeaderFile = testFile.parent_path().parent_path() / "include" / "CalamityAffixes" / "RuntimeUserSettingsDebounce.h";
			const fs::path cmakeFile = testFile.parent_path().parent_path() / "CMakeLists.txt";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto headerText = loadText(eventBridgeHeaderFile);
			const auto configText = loadText(eventBridgeConfigFile);
			const auto triggerText = loadText(triggerEventsFile);
			const auto prismaText = loadText(prismaSettingsFile);
			const auto persistenceHeaderText = loadText(persistenceHeaderFile);
			const auto persistenceSourceText = loadText(persistenceSourceFile);
			const auto runtimePathsHeaderText = loadText(runtimePathsHeaderFile);
			const auto runtimePathsSourceText = loadText(runtimePathsSourceFile);
			const auto debounceHeaderText = loadText(debounceHeaderFile);
			const auto cmakeText = loadText(cmakeFile);
			if (!headerText.has_value() || !configText.has_value() || !triggerText.has_value() ||
				!prismaText.has_value() || !persistenceHeaderText.has_value() ||
				!persistenceSourceText.has_value() || !runtimePathsHeaderText.has_value() ||
				!runtimePathsSourceText.has_value() || !debounceHeaderText.has_value() || !cmakeText.has_value()) {
				std::cerr << "external_user_settings_persistence: required source file missing\n";
				return false;
			}

			if (headerText->find("kUserSettingsRelativePath = \"Data/SKSE/Plugins/CalamityAffixes/user_settings.json\"") == std::string::npos ||
				headerText->find("kRuntimeContractRelativePath = \"Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json\"") == std::string::npos ||
				headerText->find("void ApplyRuntimeUserSettingsOverrides();") == std::string::npos ||
				headerText->find("bool PersistRuntimeUserSettings();") == std::string::npos ||
				headerText->find("void MarkRuntimeUserSettingsDirty();") == std::string::npos ||
				headerText->find("void MaybeFlushRuntimeUserSettings(") == std::string::npos ||
				headerText->find("RuntimeUserSettingsDebounce.h") == std::string::npos) {
				std::cerr << "external_user_settings_persistence: EventBridge user-settings declarations are missing\n";
				return false;
			}

			if (configText->find("ApplyRuntimeUserSettingsOverrides();") == std::string::npos ||
				configText->find("ValidateRuntimeContractSnapshot();") == std::string::npos ||
				configText->find("void EventBridge::ApplyRuntimeUserSettingsOverrides()") == std::string::npos ||
				configText->find("std::string EventBridge::BuildRuntimeUserSettingsPayload() const") == std::string::npos ||
				configText->find("void EventBridge::MarkRuntimeUserSettingsDirty()") == std::string::npos ||
				configText->find("void EventBridge::MaybeFlushRuntimeUserSettings(") == std::string::npos ||
				configText->find("bool EventBridge::PersistRuntimeUserSettings()") == std::string::npos ||
				configText->find("UserSettingsPersistence::LoadJsonObject(kUserSettingsRelativePath, root)") == std::string::npos ||
				configText->find("UserSettingsPersistence::UpdateJsonObject(") == std::string::npos ||
				configText->find("RuntimeContract::kTriggerHit") == std::string::npos ||
				configText->find("RuntimeContract::kActionCastOnCrit") == std::string::npos ||
				configText->find("RuntimeUserSettingsDebounce::MarkDirty(") == std::string::npos ||
				configText->find("RuntimeUserSettingsDebounce::ShouldFlush(") == std::string::npos) {
				std::cerr << "external_user_settings_persistence: EventBridge runtime load/save wiring is missing\n";
				return false;
			}

			const auto hasPersistenceCallWithinEventBlock = [&](std::string_view a_eventToken) {
				const auto eventPos = triggerText->find(std::string("eventName == ") + std::string(a_eventToken));
				if (eventPos == std::string::npos) {
					return false;
				}
				const auto blockReturnPos = triggerText->find("return RE::BSEventNotifyControl::kContinue;", eventPos);
				if (blockReturnPos == std::string::npos) {
					return false;
				}
				const auto persistPos = triggerText->find("queueRuntimeUserSettingsPersist();", eventPos);
				return persistPos != std::string::npos && persistPos < blockReturnPos;
			};

			const std::array<std::string_view, 8> requiredPersistedEvents{
				"kMcmSetEnabledEvent",
				"kMcmSetDebugNotificationsEvent",
				"kMcmSetValidationIntervalEvent",
				"kMcmSetProcChanceMultEvent",
				"kMcmSetRunewordFragmentChanceEvent",
				"kMcmSetReforgeOrbChanceEvent",
				"kMcmSetDotSafetyAutoDisableEvent",
				"kMcmSetAllowNonHostileFirstHitProcEvent"
			};
			for (const auto eventToken : requiredPersistedEvents) {
				if (!hasPersistenceCallWithinEventBlock(eventToken)) {
					std::cerr << "external_user_settings_persistence: missing persistence call in " << eventToken << " block\n";
					return false;
				}
			}
			if (triggerText->find("MarkRuntimeUserSettingsDirty();") == std::string::npos ||
				triggerText->find("MaybeFlushRuntimeUserSettings(") == std::string::npos ||
				triggerText->find("MaybeFlushRuntimeUserSettings(std::chrono::steady_clock::now(), true);") == std::string::npos) {
				std::cerr << "external_user_settings_persistence: runtime settings debounce flush wiring is missing\n";
				return false;
			}

			if (prismaText->find("LoadPanelHotkeyFromUserSettings()") == std::string::npos ||
				prismaText->find("std::numeric_limits<std::uint32_t>::max()") == std::string::npos ||
				prismaText->find("PersistPanelHotkeyToUserSettings(resolvedKey);") == std::string::npos ||
				prismaText->find("RestorePanelHotkeyFromUserSettings();") == std::string::npos ||
				prismaText->find("LoadUiLanguageModeFromUserSettings()") == std::string::npos ||
				prismaText->find("PersistUiLanguageModeToUserSettings(resolvedMode);") == std::string::npos ||
				prismaText->find("RestoreUiLanguageModeFromUserSettings();") == std::string::npos) {
				std::cerr << "external_user_settings_persistence: Prisma user-settings fallback/persist wiring is missing\n";
				return false;
			}

			if (persistenceHeaderText->find("namespace CalamityAffixes::UserSettingsPersistence") == std::string::npos ||
				persistenceHeaderText->find("LoadJsonObject(") == std::string::npos ||
				persistenceHeaderText->find("UpdateJsonObject(") == std::string::npos ||
				persistenceSourceText->find("std::scoped_lock lk{ g_userSettingsIoLock };") == std::string::npos ||
				persistenceSourceText->find("FlushFileBuffers") == std::string::npos ||
				persistenceSourceText->find("MoveFileExW") == std::string::npos ||
				persistenceSourceText->find("RuntimePaths::ResolveRuntimeRelativePath(") == std::string::npos ||
				persistenceSourceText->find("QuarantineUnreadableJsonFileUnlocked") == std::string::npos) {
				std::cerr << "external_user_settings_persistence: shared persistence module is missing\n";
				return false;
			}

			if (runtimePathsHeaderText->find("namespace CalamityAffixes::RuntimePaths") == std::string::npos ||
				runtimePathsHeaderText->find("GetRuntimeDirectory()") == std::string::npos ||
				runtimePathsHeaderText->find("ResolveRuntimeRelativePath(") == std::string::npos ||
				runtimePathsSourceText->find("GetModuleFileNameW") == std::string::npos ||
				runtimePathsSourceText->find("std::filesystem::current_path()") == std::string::npos) {
				std::cerr << "external_user_settings_persistence: runtime path resolver module is missing\n";
				return false;
			}

			if (debounceHeaderText->find("struct State") == std::string::npos ||
				debounceHeaderText->find("inline bool MarkDirty(") == std::string::npos ||
				debounceHeaderText->find("inline bool ShouldFlush(") == std::string::npos ||
				debounceHeaderText->find("inline void MarkPersistFailure(") == std::string::npos) {
				std::cerr << "external_user_settings_persistence: runtime debounce state helper is missing\n";
				return false;
			}

			if (cmakeText->find("include/CalamityAffixes/UserSettingsPersistence.h") == std::string::npos ||
				cmakeText->find("include/CalamityAffixes/RuntimeContract.h") == std::string::npos ||
				cmakeText->find("include/CalamityAffixes/RuntimePaths.h") == std::string::npos ||
				cmakeText->find("include/CalamityAffixes/RuntimeUserSettingsDebounce.h") == std::string::npos ||
				cmakeText->find("src/RuntimePaths.cpp") == std::string::npos ||
				cmakeText->find("src/UserSettingsPersistence.cpp") == std::string::npos) {
				std::cerr << "external_user_settings_persistence: build integration is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRuntimeUserSettingsRoundTripFieldPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path eventBridgeConfigFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.cpp";

			std::ifstream in(eventBridgeConfigFile);
			if (!in.is_open()) {
				std::cerr << "runtime_user_settings_round_trip: missing EventBridge.Config.cpp\n";
				return false;
			}
			const std::string configText(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());

			if (configText.find("std::string EventBridge::BuildRuntimeUserSettingsPayload() const") == std::string::npos ||
				configText.find("bool EventBridge::PersistRuntimeUserSettings()") == std::string::npos ||
				configText.find("void EventBridge::ApplyRuntimeUserSettingsOverrides()") == std::string::npos) {
				std::cerr << "runtime_user_settings_round_trip: runtime user-settings functions missing\n";
				return false;
			}

			const std::array<std::string_view, 8> runtimeKeys{
				"enabled",
				"debugNotifications",
				"validationIntervalSeconds",
				"procChanceMultiplier",
				"runewordFragmentChancePercent",
				"reforgeOrbChancePercent",
				"dotSafetyAutoDisable",
				"allowNonHostileFirstHitProc"
			};

			const auto hasNearAnchor = [&configText](std::size_t a_pos, std::string_view a_anchor, std::size_t a_window) {
				if (a_pos == std::string::npos) {
					return false;
				}
				const auto windowStart = (a_pos > a_window) ? (a_pos - a_window) : 0;
				const auto anchorPos = configText.rfind(a_anchor, a_pos);
				return anchorPos != std::string::npos && anchorPos >= windowStart;
			};

			for (const auto key : runtimeKeys) {
				const std::string persistPattern = std::string("runtime[\"") + std::string(key) + "\"] =";
				if (configText.find(persistPattern) == std::string::npos) {
					std::cerr << "runtime_user_settings_round_trip: missing persist payload field '" << key << "'\n";
					return false;
				}

				const std::string quotedKey = std::string("\"") + std::string(key) + "\"";
				bool loadFieldFound = false;
				std::size_t keyPos = configText.find(quotedKey);
				while (keyPos != std::string::npos) {
					if (hasNearAnchor(keyPos, "runtime.value(", 256)) {
						loadFieldFound = true;
						break;
					}
					keyPos = configText.find(quotedKey, keyPos + quotedKey.size());
				}

				if (!loadFieldFound) {
					std::cerr << "runtime_user_settings_round_trip: missing load override field '" << key << "'\n";
					return false;
				}
			}

			return true;
		}
}
