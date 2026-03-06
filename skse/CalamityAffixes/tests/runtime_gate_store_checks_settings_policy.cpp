#include "runtime_gate_store_checks_common.h"

#include "CalamityAffixes/CombatRuntimeState.h"
#include "CalamityAffixes/LootRuntimeState.h"
#include "CalamityAffixes/RunewordUiContracts.h"

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

		bool CheckCombatRuntimeStateResetBehavior()
		{
			using namespace std::chrono;

			CalamityAffixes::CombatRuntimeState state{};
			state.recentOwnerHitAt.emplace(0x14u, steady_clock::time_point{ milliseconds(100) });
			state.recentOwnerKillAt.emplace(0x15u, steady_clock::time_point{ milliseconds(200) });
			state.recentOwnerIncomingHitAt.emplace(0x16u, steady_clock::time_point{ milliseconds(300) });
			state.outgoingHitPerTargetLastAt.emplace(0x17u, steady_clock::time_point{ milliseconds(400) });
			state.playerCombatEvidenceExpiresAt = steady_clock::time_point{ milliseconds(500) };
			state.lastHealthDamageSignature = 0xBEEFull;
			state.lastHealthDamageSignatureAt = steady_clock::time_point{ milliseconds(600) };
			state.triggerProcBudgetWindowStartMs = 77u;
			state.triggerProcBudgetConsumed = 3u;
			state.castOnCritNextAllowed = steady_clock::time_point{ milliseconds(700) };
			state.castOnCritCycleCursor = 5u;

			state.Reset();

			if (!state.recentOwnerHitAt.empty() ||
				!state.recentOwnerKillAt.empty() ||
				!state.recentOwnerIncomingHitAt.empty() ||
				!state.outgoingHitPerTargetLastAt.empty()) {
				std::cerr << "combat_runtime_state_reset: expected recent-combat maps to be cleared\n";
				return false;
			}
			if (state.playerCombatEvidenceExpiresAt.time_since_epoch().count() != 0 ||
				state.lastHealthDamageSignature != 0u ||
				state.lastHealthDamageSignatureAt.time_since_epoch().count() != 0 ||
				state.triggerProcBudgetWindowStartMs != 0u ||
				state.triggerProcBudgetConsumed != 0u ||
				state.castOnCritNextAllowed.time_since_epoch().count() != 0 ||
				state.castOnCritCycleCursor != 0u) {
				std::cerr << "combat_runtime_state_reset: expected scalar runtime state to reset to defaults\n";
				return false;
			}

			return true;
		}

		bool CheckCombatRuntimeStateExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path repoRoot = testFile.parent_path().parent_path();
			const fs::path eventBridgeHeader = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";
			const fs::path combatRuntimeHeader = repoRoot / "include" / "CalamityAffixes" / "CombatRuntimeState.h";
			const fs::path resetFile = repoRoot / "src" / "EventBridge.Config.Reset.cpp";
			const fs::path serializationLoadFile = repoRoot / "src" / "EventBridge.Serialization.Load.cpp";
			const fs::path serializationLifecycleFile = repoRoot / "src" / "EventBridge.Serialization.Lifecycle.cpp";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto eventBridgeHeaderText = loadText(eventBridgeHeader);
			const auto combatRuntimeHeaderText = loadText(combatRuntimeHeader);
			const auto resetText = loadText(resetFile);
			const auto serializationLoadText = loadText(serializationLoadFile);
			const auto serializationLifecycleText = loadText(serializationLifecycleFile);
			if (!eventBridgeHeaderText.has_value() || !combatRuntimeHeaderText.has_value() ||
				!resetText.has_value() || !serializationLoadText.has_value() || !serializationLifecycleText.has_value()) {
				std::cerr << "combat_runtime_state_extraction: failed to load source files\n";
				return false;
			}

			if (eventBridgeHeaderText->find("CombatRuntimeState _combatState{};") == std::string::npos ||
				eventBridgeHeaderText->find("CombatRuntimeState _combatRuntime{};") != std::string::npos ||
				eventBridgeHeaderText->find("_perTargetCooldownStore") != std::string::npos ||
				eventBridgeHeaderText->find("_nonHostileFirstHitGate") != std::string::npos ||
				eventBridgeHeaderText->find("_dotCooldowns") != std::string::npos ||
				eventBridgeHeaderText->find("_procDepth") != std::string::npos ||
				eventBridgeHeaderText->find("_lastPapyrusHit") != std::string::npos ||
				eventBridgeHeaderText->find("_lowHealthTriggerConsumed") != std::string::npos) {
				std::cerr << "combat_runtime_state_extraction: EventBridge.h still owns combat transient fields directly\n";
				return false;
			}

			if (combatRuntimeHeaderText->find("PerTargetCooldownStore perTargetCooldownStore{};") == std::string::npos ||
				combatRuntimeHeaderText->find("NonHostileFirstHitGate nonHostileFirstHitGate{};") == std::string::npos ||
				combatRuntimeHeaderText->find("std::atomic<std::uint32_t> procDepth{ 0 };") == std::string::npos ||
				combatRuntimeHeaderText->find("LastHitKey lastPapyrusHit{};") == std::string::npos ||
				combatRuntimeHeaderText->find("void ResetTransientState() noexcept") == std::string::npos) {
				std::cerr << "combat_runtime_state_extraction: CombatRuntimeState.h is missing extracted combat ownership or reset helper\n";
				return false;
			}

			for (const auto* source : { &*resetText, &*serializationLoadText, &*serializationLifecycleText }) {
				if (source->find("_combatState.ResetTransientState();") == std::string::npos) {
					std::cerr << "combat_runtime_state_extraction: combat reset helper must be used in reset/load/revert paths\n";
					return false;
				}
			}

			return true;
		}

		bool CheckLootRuntimeStateResetBehavior()
		{
			CalamityAffixes::LootRuntimeState state{};
			state.previewAffixes.emplace(0x10001u, CalamityAffixes::InstanceAffixSlots{});
			state.previewRecent.push_back(0x10001u);
			state.evaluatedInstances.insert(0x10002u);
			state.evaluatedRecent.push_back(0x10002u);
			state.evaluatedInsertionsSincePrune = 3u;
			state.currencyRollLedger.emplace(0x20003u, 7u);
			state.currencyRollLedgerRecent.push_back(0x20003u);
			state.lootChanceEligibleFailStreak = 4u;
			state.runewordFragmentFailStreak = 5u;
			state.reforgeOrbFailStreak = 6u;
			state.activeSlotPenalty.push_back(0.5f);
			state.playerContainerStash[{ 0x14u, 0x15u }] = 2;

			state.ResetForConfigReload();

			if (!state.previewAffixes.empty() ||
				!state.previewRecent.empty() ||
				state.lootChanceEligibleFailStreak != 0u ||
				state.runewordFragmentFailStreak != 0u ||
				state.reforgeOrbFailStreak != 0u ||
				!state.activeSlotPenalty.empty() ||
				!state.playerContainerStash.empty()) {
				std::cerr << "loot_runtime_state_reset: config-reload reset must clear preview/pity/slot-penalty/stash state\n";
				return false;
			}
			if (state.evaluatedInstances.empty() || state.currencyRollLedger.empty()) {
				std::cerr << "loot_runtime_state_reset: config-reload reset must preserve save-backed evaluated/ledger state\n";
				return false;
			}

			state.ResetForLoadOrRevert();
			if (!state.evaluatedInstances.empty() ||
				!state.evaluatedRecent.empty() ||
				state.evaluatedInsertionsSincePrune != 0u ||
				!state.currencyRollLedger.empty() ||
				!state.currencyRollLedgerRecent.empty()) {
				std::cerr << "loot_runtime_state_reset: load/revert reset must clear evaluated/ledger state\n";
				return false;
			}

			return true;
		}

		bool CheckLootRuntimeStateExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path repoRoot = testFile.parent_path().parent_path();
			const fs::path eventBridgeHeader = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";
			const fs::path lootRuntimeHeader = repoRoot / "include" / "CalamityAffixes" / "LootRuntimeState.h";
			const fs::path resetFile = repoRoot / "src" / "EventBridge.Config.Reset.cpp";
			const fs::path serializationLoadFile = repoRoot / "src" / "EventBridge.Serialization.Load.cpp";
			const fs::path serializationLifecycleFile = repoRoot / "src" / "EventBridge.Serialization.Lifecycle.cpp";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto eventBridgeHeaderText = loadText(eventBridgeHeader);
			const auto lootRuntimeHeaderText = loadText(lootRuntimeHeader);
			const auto resetText = loadText(resetFile);
			const auto serializationLoadText = loadText(serializationLoadFile);
			const auto serializationLifecycleText = loadText(serializationLifecycleFile);
			if (!eventBridgeHeaderText.has_value() || !lootRuntimeHeaderText.has_value() ||
				!resetText.has_value() || !serializationLoadText.has_value() || !serializationLifecycleText.has_value()) {
				std::cerr << "loot_runtime_state_extraction: failed to load source files\n";
				return false;
			}

			if (eventBridgeHeaderText->find("#include \"CalamityAffixes/LootRuntimeState.h\"") == std::string::npos ||
				eventBridgeHeaderText->find("LootRuntimeState _lootState{};") == std::string::npos ||
				eventBridgeHeaderText->find("_lootPrefixSharedBag") != std::string::npos ||
				eventBridgeHeaderText->find("_lootPreviewAffixes") != std::string::npos ||
				eventBridgeHeaderText->find("_lootCurrencyRollLedger") != std::string::npos ||
				eventBridgeHeaderText->find("_lootRerollGuard") != std::string::npos ||
				eventBridgeHeaderText->find("_playerContainerStash") != std::string::npos) {
				std::cerr << "loot_runtime_state_extraction: EventBridge.h still owns loot runtime fields directly\n";
				return false;
			}

			if (lootRuntimeHeaderText->find("LootShuffleBagState prefixSharedBag{};") == std::string::npos ||
				lootRuntimeHeaderText->find("std::unordered_map<std::uint64_t, InstanceAffixSlots> previewAffixes{};") == std::string::npos ||
				lootRuntimeHeaderText->find("std::unordered_set<std::uint64_t> evaluatedInstances{};") == std::string::npos ||
				lootRuntimeHeaderText->find("std::unordered_map<std::uint64_t, std::uint32_t> currencyRollLedger{};") == std::string::npos ||
				lootRuntimeHeaderText->find("LootRerollGuard rerollGuard{};") == std::string::npos ||
				lootRuntimeHeaderText->find("void ResetForConfigReload() noexcept") == std::string::npos ||
				lootRuntimeHeaderText->find("void ResetForLoadOrRevert() noexcept") == std::string::npos) {
				std::cerr << "loot_runtime_state_extraction: LootRuntimeState.h is missing extracted loot ownership or reset helpers\n";
				return false;
			}

			if (resetText->find("_lootState.ResetForConfigReload();") == std::string::npos ||
				serializationLoadText->find("_lootState.ResetForLoadOrRevert();") == std::string::npos ||
				serializationLifecycleText->find("_lootState.ResetForLoadOrRevert();") == std::string::npos) {
				std::cerr << "loot_runtime_state_extraction: reset/load/revert paths must use LootRuntimeState helpers\n";
				return false;
			}

			return true;
		}

		bool CheckConfigReloadTransientRuntimeResetPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path resetFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.Reset.cpp";

			std::ifstream in(resetFile);
			if (!in.is_open()) {
				std::cerr << "config_reload_transient_reset: failed to open reset source: " << resetFile << "\n";
				return false;
			}

			const std::string source(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());

			const std::array<std::string_view, 1> requiredPatterns{
				"_combatState.ResetTransientState();",
			};

			for (const auto needle : requiredPatterns) {
				if (source.find(needle) == std::string::npos) {
					std::cerr << "config_reload_transient_reset: missing reset pattern in config reload path: " << needle << "\n";
					return false;
				}
			}

			return true;
		}

		bool CheckRunewordUiContractDefaults()
		{
			CalamityAffixes::RunewordBaseInventoryEntry base{};
			CalamityAffixes::RunewordRecipeEntry recipe{};
			CalamityAffixes::RunewordPanelState panel{};
			CalamityAffixes::OperationResult result{};

			if (base.instanceKey != 0u || base.selected ||
				recipe.recipeToken != 0u || recipe.selected ||
				panel.hasBase || panel.hasRecipe || panel.isComplete ||
				panel.insertedRunes != 0u || panel.totalRunes != 0u ||
				panel.canInsert ||
				result.success) {
				std::cerr << "runeword_ui_contract_defaults: expected DTO defaults to remain zero-initialized\n";
				return false;
			}

			panel.requiredRunes.push_back({ .runeName = "El", .required = 1u, .owned = 0u });
			recipe.effectSummaryText = "summary";
			recipe.effectDetailText = "detail";
			result.message = "ok";
			if (panel.requiredRunes.size() != 1u ||
				recipe.effectSummaryText != "summary" ||
				recipe.effectDetailText != "detail" ||
				result.message != "ok") {
				std::cerr << "runeword_ui_contract_defaults: expected DTO fields to remain usable as runtime contracts\n";
				return false;
			}

			return true;
		}
	}
