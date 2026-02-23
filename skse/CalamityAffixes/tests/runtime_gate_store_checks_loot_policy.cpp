#include "runtime_gate_store_checks_common.h"

namespace RuntimeGateStoreChecks
{
	bool CheckRunewordTooltipOverlayPolicy()
	{
		if (CalamityAffixes::ShouldShowRunewordTooltipInItemOverlay()) {
			std::cerr << "tooltip_policy: runeword text must stay panel-only\n";
			return false;
		}

		return true;
	}

	bool CheckLootPreviewRuntimePolicy()
	{
		if (CalamityAffixes::ShouldEnableSyntheticLootPreviewTooltip()) {
			std::cerr << "loot_preview_policy: synthetic preview rollout must remain disabled by policy helper\n";
			return false;
		}

		if (CalamityAffixes::RuntimePolicy::kAllowPickupRandomAffixAssignment ||
			CalamityAffixes::RuntimePolicy::kAllowLegacyPickupAffixRollBranch ||
			CalamityAffixes::RuntimePolicy::kAllowWorldPickupCurrencyRoll ||
			CalamityAffixes::RuntimePolicy::kAllowPickupCurrencyRollFromContainerSources) {
			std::cerr << "loot_preview_policy: pickup runtime policy must remain activation-only without legacy/random branches\n";
			return false;
		}

		if (CalamityAffixes::RuntimePolicy::kAllowCorpseActivationRuntimeCurrencyRollInHybridMode) {
			std::cerr << "loot_preview_policy: hybrid corpse activation runtime rolls must stay disabled\n";
			return false;
		}

		return true;
	}

	bool CheckLootCurrencyLedgerSerializationPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path headerFile = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";
		const fs::path assignFile = repoRoot / "src" / "EventBridge.Loot.Assign.cpp";
		const fs::path serializationFile = repoRoot / "src" / "EventBridge.Serialization.cpp";
		const fs::path triggerEventsFile = repoRoot / "src" / "EventBridge.Triggers.Events.cpp";

		const auto worldKeyDay10 = CalamityAffixes::detail::BuildLootCurrencyLedgerKey(
			CalamityAffixes::detail::LootCurrencySourceTier::kWorld,
			0x00000000u,
			0x00012EB7u,
			0x0042u,
			10u);
		const auto worldKeyDay11 = CalamityAffixes::detail::BuildLootCurrencyLedgerKey(
			CalamityAffixes::detail::LootCurrencySourceTier::kWorld,
			0x00000000u,
			0x00012EB7u,
			0x0042u,
			11u);
		const auto corpseKeyDay10 = CalamityAffixes::detail::BuildLootCurrencyLedgerKey(
			CalamityAffixes::detail::LootCurrencySourceTier::kCorpse,
			0x000ABC01u,
			0u,
			0u,
			10u);
		if (worldKeyDay10 == 0u || worldKeyDay10 == worldKeyDay11 || worldKeyDay10 == corpseKeyDay10) {
			std::cerr << "loot_currency_ledger_policy: key builder must produce non-zero and source/day-distinct keys\n";
			return false;
		}
		if (!CalamityAffixes::detail::IsLootCurrencyLedgerExpired(5u, 35u, 30u) ||
			CalamityAffixes::detail::IsLootCurrencyLedgerExpired(6u, 35u, 30u) ||
			CalamityAffixes::detail::IsLootCurrencyLedgerExpired(10u, 9u, 30u) ||
			CalamityAffixes::detail::IsLootCurrencyLedgerExpired(1u, 100u, 0u)) {
			std::cerr << "loot_currency_ledger_policy: expiry helper behavior is incorrect\n";
			return false;
		}

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto headerText = loadText(headerFile);
		if (!headerText.has_value()) {
			std::cerr << "loot_currency_ledger_policy: failed to open header source: " << headerFile << "\n";
			return false;
		}
		if (headerText->find("kSerializationRecordLootCurrencyLedger") == std::string::npos ||
			headerText->find("kLootCurrencyLedgerSerializationVersion") == std::string::npos ||
			headerText->find("_lootCurrencyRollLedger") == std::string::npos ||
			headerText->find("_lootCurrencyRollLedgerRecent") == std::string::npos) {
			std::cerr << "loot_currency_ledger_policy: ledger storage/constants missing from EventBridge.h\n";
			return false;
		}

			const auto assignText = loadText(assignFile);
			if (!assignText.has_value()) {
				std::cerr << "loot_currency_ledger_policy: failed to open assign source: " << assignFile << "\n";
				return false;
			}
			if (assignText->find("TryBeginLootCurrencyLedgerRoll(") == std::string::npos ||
				assignText->find("FinalizeLootCurrencyLedgerRoll(") == std::string::npos ||
				assignText->find("detail::IsLootCurrencyLedgerExpired(") == std::string::npos ||
				assignText->find("_lootCurrencyRollLedger.emplace(") == std::string::npos ||
				assignText->find("_lootCurrencyRollLedger.erase(oldest)") == std::string::npos) {
				std::cerr << "loot_currency_ledger_policy: source-keyed ledger behavior missing in EventBridge.Loot.Assign.cpp\n";
				return false;
			}

		const auto serializationText = loadText(serializationFile);
		if (!serializationText.has_value()) {
			std::cerr << "loot_currency_ledger_policy: failed to open serialization source: " << serializationFile << "\n";
			return false;
		}
		if (serializationText->find("kSerializationRecordLootCurrencyLedger") == std::string::npos ||
			serializationText->find("kLootCurrencyLedgerSerializationVersion") == std::string::npos ||
			serializationText->find("_lootCurrencyRollLedger.size()") == std::string::npos ||
			serializationText->find("_lootCurrencyRollLedger.emplace(key, dayStamp)") == std::string::npos ||
			serializationText->find("_lootCurrencyRollLedger.clear()") == std::string::npos ||
			serializationText->find("_lootCurrencyRollLedgerRecent.clear()") == std::string::npos) {
			std::cerr << "loot_currency_ledger_policy: save/load/revert ledger handling missing in EventBridge.Serialization.cpp\n";
			return false;
		}

		const auto triggerText = loadText(triggerEventsFile);
		if (!triggerText.has_value()) {
			std::cerr << "loot_currency_ledger_policy: failed to open trigger-events source: " << triggerEventsFile << "\n";
			return false;
		}
			const bool hasDirectActivationRollPath =
				triggerText->find("TryRollRunewordFragmentToken(") != std::string::npos &&
				triggerText->find("TryRollReforgeOrbGrant(") != std::string::npos &&
				triggerText->find("TryPlaceLootCurrencyItem(") != std::string::npos;
			const bool hasSharedActivationRollHelper =
				triggerText->find("ExecuteCurrencyDropRolls(") != std::string::npos;
			const bool hasSourceTierResolverCall =
				triggerText->find("ResolveActivatedLootCurrencySourceTier(") != std::string::npos ||
				triggerText->find("detail::ResolveLootCurrencySourceTier(") != std::string::npos;
			const bool hasSpidOnlyCorpseSkipLog =
				triggerText->find("activation corpse currency roll skipped (SPID-owned corpse policy in hybrid mode)") != std::string::npos;
			const bool hasLegacyTaggedCorpseSkipLog =
				triggerText->find("SPID-tagged corpse in hybrid mode") != std::string::npos;
			const bool hasLegacyUntaggedCorpseSkipLog =
				triggerText->find("untagged corpse in hybrid mode; SPID-only corpse policy") != std::string::npos;
			const bool hasLegacyCorpseFallbackLog =
				triggerText->find("activation corpse currency roll fallback (untagged corpse in hybrid mode)") != std::string::npos;
			if (triggerText->find("const RE::TESActivateEvent* a_event") == std::string::npos ||
				!hasSourceTierResolverCall ||
				triggerText->find("HasCurrencyDeathDistributionTag(") != std::string::npos ||
				!hasSpidOnlyCorpseSkipLog ||
				hasLegacyTaggedCorpseSkipLog ||
				hasLegacyUntaggedCorpseSkipLog ||
				hasLegacyCorpseFallbackLog ||
				(!hasDirectActivationRollPath && !hasSharedActivationRollHelper) ||
				(triggerText->find("detail::IsLootCurrencyLedgerExpired(") == std::string::npos &&
					triggerText->find("TryBeginLootCurrencyLedgerRoll(") == std::string::npos)) {
				std::cerr << "loot_currency_ledger_policy: activation-time corpse/container currency roll path missing in EventBridge.Triggers.Events.cpp\n";
				return false;
		}

		return true;
	}

	bool CheckSerializationTransientRuntimeResetPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path serializationFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Serialization.cpp";

		std::ifstream in(serializationFile);
		if (!in.is_open()) {
			std::cerr << "serialization_transient_reset: failed to open source file: " << serializationFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		const auto countOccurrences = [&](std::string_view needle) -> std::size_t {
			std::size_t count = 0;
			std::size_t pos = 0;
			while ((pos = source.find(needle, pos)) != std::string::npos) {
				++count;
				pos += needle.size();
			}
			return count;
		};

		const std::array<std::string_view, 8> requiredBothLoadAndRevert{
			"_activeCounts.clear();",
			"_activeHitTriggerAffixIndices.clear();",
			"_dotCooldowns.clear();",
			"_perTargetCooldownStore.Clear();",
			"_nonHostileFirstHitGate.Clear();",
			"_recentOwnerHitAt.clear();",
			"_lowHealthTriggerConsumed.clear();",
			"_triggerProcBudgetWindowStartMs = 0u;"
		};

		for (const auto needle : requiredBothLoadAndRevert) {
			if (countOccurrences(needle) < 2) {
				std::cerr << "serialization_transient_reset: expected Load/Revert reset guard missing for pattern: " << needle << "\n";
				return false;
			}
		}

		return true;
	}

	bool CheckLootRerollExploitGuardPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path lootFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.cpp";

		std::ifstream in(lootFile);
		if (!in.is_open()) {
			std::cerr << "loot_reroll_exploit_guard: failed to open loot source: " << lootFile << "\n";
			return false;
		}

		std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("IsLootSourceCorpseChestOrWorld(") == std::string::npos ||
			source.find("_lootRerollGuard.ConsumeIfPlayerDropPickup(") == std::string::npos ||
			source.find("_playerContainerStash[stashKey] += a_event->itemCount;") == std::string::npos ||
			source.find("skipping loot roll (player dropped + re-picked)") == std::string::npos ||
			source.find("skipping loot roll (player stashed + retrieved)") == std::string::npos) {
			std::cerr << "loot_reroll_exploit_guard: anti-reroll pickup guards are missing\n";
			return false;
		}

		return true;
	}

	bool CheckLootChanceMcmCleanupPolicy()
	{
		if (CalamityAffixes::RuntimePolicy::IsPersistedRuntimeUserSettingEvent("CalamityAffixes_MCM_SetLootChance")) {
			std::cerr << "loot_chance_mcm_cleanup: deprecated loot chance event must not be in persisted runtime-setting events\n";
			return false;
		}

		return true;
	}

	bool CheckMcmDropChanceRuntimeBridgePolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path().parent_path().parent_path();
		const fs::path configJsonFile = repoRoot / "Data" / "MCM" / "Config" / "CalamityAffixes" / "config.json";
		const fs::path mcmConfigScriptFile = repoRoot / "Data" / "Scripts" / "Source" / "CalamityAffixes_MCMConfig.psc";
		const fs::path modeControlScriptFile = repoRoot / "Data" / "Scripts" / "Source" / "CalamityAffixes_ModeControl.psc";
		const fs::path triggerEventsFile = repoRoot / "skse" / "CalamityAffixes" / "src" / "EventBridge.Triggers.Events.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto configJsonText = loadText(configJsonFile);
		if (!configJsonText.has_value()) {
			std::cerr << "mcm_drop_chance_bridge: failed to open MCM config: " << configJsonFile << "\n";
			return false;
		}
		if (configJsonText->find("\"id\": \"fRunewordFragmentChancePercent:General\"") == std::string::npos ||
			configJsonText->find("\"function\": \"SetRunewordFragmentChancePercent\"") == std::string::npos ||
			configJsonText->find("\"id\": \"fReforgeOrbChancePercent:General\"") == std::string::npos ||
			configJsonText->find("\"function\": \"SetReforgeOrbChancePercent\"") == std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: MCM slider/action wiring is missing\n";
			return false;
		}
		const auto countOccurrences = [](std::string_view haystack, std::string_view needle) -> std::size_t {
			std::size_t count = 0;
			std::size_t pos = 0;
			while ((pos = haystack.find(needle, pos)) != std::string_view::npos) {
				++count;
				pos += needle.size();
			}
			return count;
		};
		if (countOccurrences(*configJsonText, "\"formatString\": \"{1}%\"") < 2 ||
			configJsonText->find("\"formatString\": \"%.1f%%\"") != std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: chance slider formatString must use MCM Helper token syntax ({1}%).\n";
			return false;
		}

		const auto mcmConfigText = loadText(mcmConfigScriptFile);
		if (!mcmConfigText.has_value()) {
			std::cerr << "mcm_drop_chance_bridge: failed to open MCM script: " << mcmConfigScriptFile << "\n";
			return false;
		}
		if (mcmConfigText->find("Event OnSettingChange(string a_ID)") == std::string::npos ||
			mcmConfigText->find("a_ID == RunewordFragmentChanceSettingName || a_ID == ReforgeOrbChanceSettingName") == std::string::npos ||
			mcmConfigText->find("float runewordChance = GetModSettingFloat(RunewordFragmentChanceSettingName)") == std::string::npos ||
			mcmConfigText->find("runewordChance = RunewordFragmentChanceDefault") == std::string::npos ||
			mcmConfigText->find("float reforgeChance = GetModSettingFloat(ReforgeOrbChanceSettingName)") == std::string::npos ||
			mcmConfigText->find("reforgeChance = ReforgeOrbChanceDefault") == std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: MCM setting-change/sentinel fallback guard is missing\n";
			return false;
		}

		const auto modeControlText = loadText(modeControlScriptFile);
		if (!modeControlText.has_value()) {
			std::cerr << "mcm_drop_chance_bridge: failed to open mode-control script: " << modeControlScriptFile << "\n";
			return false;
		}
		if (modeControlText->find("Emit(\"CalamityAffixes_MCM_SetRunewordFragmentChance\"") == std::string::npos ||
			modeControlText->find("Emit(\"CalamityAffixes_MCM_SetReforgeOrbChance\"") == std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: mode-control mod-event bridge is missing\n";
			return false;
		}

		if (!CalamityAffixes::RuntimePolicy::IsPersistedRuntimeUserSettingEvent("CalamityAffixes_MCM_SetRunewordFragmentChance") ||
			!CalamityAffixes::RuntimePolicy::IsPersistedRuntimeUserSettingEvent("CalamityAffixes_MCM_SetReforgeOrbChance")) {
			std::cerr << "mcm_drop_chance_bridge: runtime policy event constants are missing\n";
			return false;
		}

		const auto triggerText = loadText(triggerEventsFile);
		if (!triggerText.has_value()) {
			std::cerr << "mcm_drop_chance_bridge: failed to open trigger source: " << triggerEventsFile << "\n";
			return false;
		}
		if (triggerText->find("eventName == kMcmSetRunewordFragmentChanceEvent") == std::string::npos ||
			triggerText->find("_loot.runewordFragmentChancePercent = std::clamp(a_event->numArg, 0.0f, 100.0f);") == std::string::npos ||
			triggerText->find("eventName == kMcmSetReforgeOrbChanceEvent") == std::string::npos ||
			triggerText->find("_loot.reforgeOrbChancePercent = std::clamp(a_event->numArg, 0.0f, 100.0f);") == std::string::npos ||
			triggerText->find("previousRunewordChance") == std::string::npos ||
			triggerText->find("previousReforgeChance") == std::string::npos ||
			triggerText->find("shouldEmitChanceNotification(") == std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: trigger event handlers are missing\n";
			return false;
		}

		return true;
	}

}
