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
		const fs::path lootRuntimeHeaderFile = repoRoot / "include" / "CalamityAffixes" / "LootRuntimeState.h";
		const fs::path constantsInlFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.Constants.inl";
		const fs::path lootServiceFile = repoRoot / "src" / "EventBridge.Loot.Service.cpp";
		const fs::path serializationSaveFile = repoRoot / "src" / "EventBridge.Serialization.Save.cpp";
		const fs::path serializationLoadFile = repoRoot / "src" / "EventBridge.Serialization.Load.cpp";
		const fs::path serializationLifecycleFile = repoRoot / "src" / "EventBridge.Serialization.Lifecycle.cpp";
		const fs::path triggerEventsFile = repoRoot / "src" / "EventBridge.Triggers.ActivateEvent.cpp";

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
		const auto lootRuntimeHeaderText = loadText(lootRuntimeHeaderFile);
		if (!headerText.has_value() || !lootRuntimeHeaderText.has_value()) {
			std::cerr << "loot_currency_ledger_policy: failed to open header source: " << headerFile << "\n";
			return false;
		}
		// Constants may live in EventBridge.h itself or in the extracted detail/EventBridge.Constants.inl,
		// while loot runtime ownership lives in LootRuntimeState.h.
		std::string headerAndConstants = *headerText + *lootRuntimeHeaderText;
		if (const auto constantsText = loadText(constantsInlFile); constantsText.has_value()) {
			headerAndConstants += *constantsText;
		}
		if (headerAndConstants.find("kSerializationRecordLootCurrencyLedger") == std::string::npos ||
			headerAndConstants.find("kLootCurrencyLedgerSerializationVersion") == std::string::npos ||
			headerAndConstants.find("LootRuntimeState _lootState{};") == std::string::npos ||
			headerAndConstants.find("currencyRollLedger{}") == std::string::npos ||
			headerAndConstants.find("currencyRollLedgerRecent{}") == std::string::npos) {
			std::cerr << "loot_currency_ledger_policy: ledger storage/constants missing from extracted runtime ownership headers\n";
			return false;
		}

			const auto serviceText = loadText(lootServiceFile);
			if (!serviceText.has_value()) {
				std::cerr << "loot_currency_ledger_policy: failed to open loot service source: " << lootServiceFile << "\n";
				return false;
			}
			if (serviceText->find("TryBeginLootCurrencyLedgerRoll(") == std::string::npos ||
				serviceText->find("FinalizeLootCurrencyLedgerRoll(") == std::string::npos ||
				serviceText->find("detail::IsLootCurrencyLedgerExpired(") == std::string::npos ||
				serviceText->find("_lootState.currencyRollLedger.emplace(") == std::string::npos ||
				serviceText->find("_lootState.currencyRollLedger.erase(oldest)") == std::string::npos) {
				std::cerr << "loot_currency_ledger_policy: source-keyed ledger behavior missing in EventBridge.Loot.Service.cpp\n";
				return false;
			}

		// Serialization is split across Save/Load/Lifecycle — concatenate all three.
		std::string serializationText;
		for (const auto& sf : { serializationSaveFile, serializationLoadFile, serializationLifecycleFile }) {
			const auto part = loadText(sf);
			if (!part.has_value()) {
				std::cerr << "loot_currency_ledger_policy: failed to open serialization source: " << sf << "\n";
				return false;
			}
			serializationText += *part;
		}
		if (serializationText.find("kSerializationRecordLootCurrencyLedger") == std::string::npos ||
			serializationText.find("kLootCurrencyLedgerSerializationVersion") == std::string::npos ||
			serializationText.find("_lootState.currencyRollLedger.size()") == std::string::npos ||
			serializationText.find("_lootState.currencyRollLedger.emplace(key, dayStamp)") == std::string::npos ||
			serializationText.find("_lootState.ResetForLoadOrRevert();") == std::string::npos ||
			serializationText.find("_lootState.currencyRollLedgerRecent.clear()") == std::string::npos) {
			std::cerr << "loot_currency_ledger_policy: save/load/revert ledger handling missing in serialization sources\n";
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
				std::cerr << "loot_currency_ledger_policy: activation-time corpse/container currency roll path missing in EventBridge.Triggers.ActivateEvent.cpp\n";
				return false;
		}

		return true;
	}

	bool CheckLootServiceExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path cmakeFile = repoRoot / "CMakeLists.txt";
		const fs::path assignFile = repoRoot / "src" / "EventBridge.Loot.Assign.cpp";
		const fs::path serviceFile = repoRoot / "src" / "EventBridge.Loot.Service.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto cmakeText = loadText(cmakeFile);
		const auto assignText = loadText(assignFile);
		const auto serviceText = loadText(serviceFile);
		if (!cmakeText.has_value() || !assignText.has_value() || !serviceText.has_value()) {
			std::cerr << "loot_service_extraction: failed to load source files\n";
			return false;
		}

		if (cmakeText->find("src/EventBridge.Loot.Service.cpp") == std::string::npos ||
			assignText->find("void EventBridge::EnsureMultiAffixDisplayName(") == std::string::npos ||
			assignText->find("void EventBridge::CleanupInvalidLootInstance(") == std::string::npos ||
			assignText->find("void EventBridge::SanitizeAllTrackedLootInstancesForCurrentLootRules(") == std::string::npos ||
			assignText->find("EventBridge::CurrencyRollExecutionResult EventBridge::ExecuteCurrencyDropRolls(") != std::string::npos ||
			assignText->find("bool EventBridge::RollLootChanceGateForEligibleInstance()") != std::string::npos ||
			assignText->find("bool EventBridge::IsLootObjectEligibleForAffixes(") != std::string::npos ||
			serviceText->find("EventBridge::CurrencyRollExecutionResult EventBridge::ExecuteCurrencyDropRolls(") == std::string::npos ||
			serviceText->find("bool EventBridge::RollLootChanceGateForEligibleInstance()") == std::string::npos ||
			serviceText->find("bool EventBridge::IsLootObjectEligibleForAffixes(") == std::string::npos ||
			serviceText->find("bool EventBridge::TryBeginLootCurrencyLedgerRoll(") == std::string::npos ||
			serviceText->find("void EventBridge::ProcessLootAcquired(") == std::string::npos) {
			std::cerr << "loot_service_extraction: loot service/mutation responsibilities are not cleanly separated\n";
			return false;
		}

		return true;
	}

	bool CheckLootDisplayNameExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path assignFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Assign.cpp";

		std::ifstream in(assignFile);
		if (!in.is_open()) {
			std::cerr << "loot_display_name_extraction: failed to open assign source: " << assignFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		const auto resolvePos = source.find("std::string EventBridge::ResolveStoredLootDisplayBaseName(");
		const auto stripPos = source.find("std::string EventBridge::StripKnownLootAffixTags(");
		const auto markerPos = source.find("std::string EventBridge::BuildLootNameMarker(");
		const auto ensurePos = source.find("void EventBridge::EnsureMultiAffixDisplayName(");
		const auto resolveCallPos = source.find("ResolveStoredLootDisplayBaseName(a_entry, a_xList, &storedCustomName)", ensurePos);
		const auto markerCallPos = source.find("BuildLootNameMarker(a_slots.count)", ensurePos);

		if (resolvePos == std::string::npos ||
			stripPos == std::string::npos ||
			markerPos == std::string::npos ||
			ensurePos == std::string::npos ||
			resolveCallPos == std::string::npos ||
			markerCallPos == std::string::npos) {
			std::cerr << "loot_display_name_extraction: expected display-name helpers or calls are missing\n";
			return false;
		}

		if (!(resolvePos < stripPos && stripPos < markerPos && markerPos < ensurePos &&
			ensurePos < resolveCallPos && resolveCallPos < markerCallPos)) {
			std::cerr << "loot_display_name_extraction: helper extraction order regressed\n";
			return false;
		}

		if (source.find("auto stripKnownAffixTags = [&](std::string_view a_name)", ensurePos) != std::string::npos ||
			source.find("std::string marker;", ensurePos) != std::string::npos) {
			std::cerr << "loot_display_name_extraction: EnsureMultiAffixDisplayName regained inline parsing/marker state\n";
			return false;
		}

		return true;
	}

	bool CheckLootTrackedSanitizeExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path assignFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Assign.cpp";

		std::ifstream in(assignFile);
		if (!in.is_open()) {
			std::cerr << "loot_tracked_sanitize_extraction: failed to open assign source: " << assignFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		const auto perInstancePos = source.find("bool EventBridge::SanitizeTrackedLootInstanceForCurrentLootRules(");
		const auto summaryPos = source.find("void EventBridge::LogTrackedLootSanitizationSummary(");
		const auto aggregatePos = source.find("void EventBridge::SanitizeAllTrackedLootInstancesForCurrentLootRules(");
		const auto helperCallPos = source.find("SanitizeTrackedLootInstanceForCurrentLootRules(it, a_context, sanitizedInstances, erasedInstances)", aggregatePos);
		const auto summaryCallPos = source.find("LogTrackedLootSanitizationSummary(a_context, sanitizedInstances, erasedInstances);", aggregatePos);

		if (perInstancePos == std::string::npos ||
			summaryPos == std::string::npos ||
			aggregatePos == std::string::npos ||
			helperCallPos == std::string::npos ||
			summaryCallPos == std::string::npos) {
			std::cerr << "loot_tracked_sanitize_extraction: expected tracked-loot sanitize helpers or calls are missing\n";
			return false;
		}

		if (!(perInstancePos < summaryPos && summaryPos < aggregatePos &&
			aggregatePos < helperCallPos && helperCallPos < summaryCallPos)) {
			std::cerr << "loot_tracked_sanitize_extraction: tracked-loot sanitize helper order regressed\n";
			return false;
		}

		if (source.find("ForgetLootEvaluatedInstance(it->first);", aggregatePos) != std::string::npos ||
			source.find("_instanceAffixes.erase(it);", aggregatePos) != std::string::npos ||
			source.find("sanitized tracked loot instances (context={}, changed={}, erased={}).", aggregatePos) != std::string::npos) {
			std::cerr << "loot_tracked_sanitize_extraction: aggregate sanitize function regained inline erase/logging responsibilities\n";
			return false;
		}

		return true;
	}

	bool CheckLootSlotSanitizeHelperExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path assignFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Assign.cpp";

		std::ifstream in(assignFile);
		if (!in.is_open()) {
			std::cerr << "loot_slot_sanitize_helper_extraction: failed to open assign source: " << assignFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		const auto keepPos = source.find("bool EventBridge::ShouldKeepTrackedLootAffixToken(");
		const auto equalPos = source.find("bool EventBridge::InstanceAffixSlotsEqual(");
		const auto applyPos = source.find("void EventBridge::ApplySanitizedInstanceAffixSlots(");
		const auto sanitizePos = source.find("bool EventBridge::SanitizeInstanceAffixSlotsForCurrentLootRules(");
		const auto keepCallPos = source.find("return ShouldKeepTrackedLootAffixToken(a_token);", sanitizePos);
		const auto equalCallPos = source.find("if (InstanceAffixSlotsEqual(a_slots, sanitized)) {", sanitizePos);
		const auto applyCallPos = source.find("ApplySanitizedInstanceAffixSlots(a_instanceKey, a_slots, sanitized, removedTokens, removedCount);", sanitizePos);

		if (keepPos == std::string::npos ||
			equalPos == std::string::npos ||
			applyPos == std::string::npos ||
			sanitizePos == std::string::npos ||
			keepCallPos == std::string::npos ||
			equalCallPos == std::string::npos ||
			applyCallPos == std::string::npos) {
			std::cerr << "loot_slot_sanitize_helper_extraction: expected sanitize helpers or calls are missing\n";
			return false;
		}

		if (!(keepPos < equalPos && equalPos < applyPos && applyPos < sanitizePos &&
			sanitizePos < keepCallPos && keepCallPos < equalCallPos && equalCallPos < applyCallPos)) {
			std::cerr << "loot_slot_sanitize_helper_extraction: sanitize helper order regressed\n";
			return false;
		}

		if (source.find("const auto idxIt = _affixRegistry.affixIndexByToken.find(a_token);", sanitizePos) != std::string::npos ||
			source.find("for (std::uint8_t i = 0; i < a_slots.count; ++i)", sanitizePos) != std::string::npos ||
			source.find("_instanceStates.erase(MakeInstanceStateKey(a_instanceKey, removedToken));", sanitizePos) != std::string::npos) {
			std::cerr << "loot_slot_sanitize_helper_extraction: sanitize function regained inline token/equality/apply responsibilities\n";
			return false;
		}

		return true;
	}

	bool CheckSerializationTransientRuntimeResetPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path serializationLoadFile = repoRoot / "src" / "EventBridge.Serialization.Load.cpp";
		const fs::path serializationLifecycleFile = repoRoot / "src" / "EventBridge.Serialization.Lifecycle.cpp";

		auto loadFile = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		// Patterns must appear in both Load and Revert (now split across two files).
		std::string source;
		for (const auto& sf : { serializationLoadFile, serializationLifecycleFile }) {
			const auto part = loadFile(sf);
			if (!part.has_value()) {
				std::cerr << "serialization_transient_reset: failed to open source file: " << sf << "\n";
				return false;
			}
			source += *part;
		}

		const auto countOccurrences = [&](std::string_view needle) -> std::size_t {
			std::size_t count = 0;
			std::size_t pos = 0;
			while ((pos = source.find(needle, pos)) != std::string::npos) {
				++count;
				pos += needle.size();
			}
			return count;
		};

		const std::array<std::string_view, 3> requiredBothLoadAndRevert{
			"_activeCounts.clear();",
			"_activeHitTriggerAffixIndices.clear();",
			"_combatState.ResetTransientState();",
		};

		for (const auto needle : requiredBothLoadAndRevert) {
			if (countOccurrences(needle) < 2) {
				std::cerr << "serialization_transient_reset: expected Load/Revert reset guard missing for pattern: " << needle << "\n";
				return false;
			}
		}

		return true;
	}

	bool CheckSuffixProcChanceParsingPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path parsingFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.AffixParsing.cpp";

		std::ifstream in(parsingFile);
		if (!in.is_open()) {
			std::cerr << "suffix_proc_chance_parsing: failed to open parsing source: " << parsingFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("a_out.procChancePct = 0.0f;") == std::string::npos) {
			std::cerr << "suffix_proc_chance_parsing: suffix proc chance must stay fixed at 0.0f\n";
			return false;
		}

		if (source.find("a_out.procChancePct = (a_kidChancePct > 0.0f) ? a_kidChancePct : 1.0f;") != std::string::npos) {
			std::cerr << "suffix_proc_chance_parsing: legacy kid.chance -> procChancePct coupling must remain removed\n";
			return false;
		}

		if (source.find("a_out.lootWeight = a_outKidChancePct;") == std::string::npos) {
			std::cerr << "suffix_proc_chance_parsing: kid chance -> lootWeight mapping must remain present\n";
			return false;
		}

		return true;
	}

	bool CheckSerializationDrainSafetyPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path serializationLoadFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Serialization.Load.cpp";

		std::ifstream in(serializationLoadFile);
		if (!in.is_open()) {
			std::cerr << "serialization_drain_safety: failed to open serialization source: " << serializationLoadFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("constexpr std::uint32_t kMaxDrainBytes = 10'000'000u;") == std::string::npos ||
			source.find("bool DrainRecordBytes(") == std::string::npos ||
			source.find("std::array<std::uint8_t, 4096> sink{};") == std::string::npos ||
			source.find("DrainRecordBytes(a_intfc, remaining, \"partial-record-recovery\");") == std::string::npos ||
			source.find("DrainRecordBytes(a_intfc, length, \"unsupported-iaxf-version\");") == std::string::npos ||
			source.find("DrainRecordBytes(a_intfc, length, \"unknown-record\");") == std::string::npos) {
			std::cerr << "serialization_drain_safety: bounded drain path guards are missing\n";
			return false;
		}

		if (source.find("std::vector<std::uint8_t> sink(length);") != std::string::npos ||
			source.find("std::vector<std::uint8_t> sink(remaining);") != std::string::npos) {
			std::cerr << "serialization_drain_safety: legacy length-sized drain allocation must not exist\n";
			return false;
		}

		return true;
	}

	bool CheckSpecialActionProcSafetyPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path specialFile = repoRoot / "src" / "EventBridge.Actions.Special.cpp";
		const fs::path archmageFile = repoRoot / "src" / "EventBridge.Actions.Archmage.cpp";

		auto loadFile = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		// Special + Archmage are split but share proc safety patterns.
		std::string source;
		for (const auto& sf : { specialFile, archmageFile }) {
			const auto part = loadFile(sf);
			if (!part.has_value()) {
				std::cerr << "special_action_proc_safety: failed to open source: " << sf << "\n";
				return false;
			}
			source += *part;
		}

		if (source.find("ResolveSpecialActionProcChancePct") == std::string::npos ||
			source.find("RollProcChance") == std::string::npos ||
			source.find("PassesLuckyHitGate(affix, Trigger::kHit, a_hitData, now)") == std::string::npos ||
			source.find("if (_combatState.procDepth > 0)") == std::string::npos ||
			source.find("sourceEditorId.starts_with(\"CAFF_\")") == std::string::npos ||
			source.find("ResolveArchmageResourceUsage(") == std::string::npos ||
			source.find("selection.bestExtraDamage = extraDamage;") == std::string::npos ||
			source.find("selection.bestExtraCost = extraCost;") == std::string::npos ||
			source.find("ExecuteArchmageCast(") == std::string::npos) {
			std::cerr << "special_action_proc_safety: special-action proc/lucky-hit/recursion guards are missing\n";
			return false;
		}

		return true;
	}

	bool CheckCorpseExplosionBudgetSafetyPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path corpseFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Actions.Corpse.cpp";

		std::ifstream in(corpseFile);
		if (!in.is_open()) {
			std::cerr << "corpse_explosion_budget_safety: failed to open corpse-actions source: " << corpseFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		const auto budgetPos = source.find("if (!TryConsumeCorpseExplosionBudget(");
		const auto icdPos = source.find("bestAffix.nextAllowed = now + bestAffix.icd;");
		if (budgetPos == std::string::npos ||
			source.find("CorpseExplosionBudgetDenyReason::kDuplicateCorpse") == std::string::npos ||
			source.find("CorpseExplosionBudgetDenyReason::kRateLimited") == std::string::npos ||
			source.find("CorpseExplosionBudgetDenyReason::kChainDepthExceeded") == std::string::npos ||
			source.find("seenCorpses.size() > 512u") == std::string::npos ||
			source.find("kCorpseExplosionDefaultMaxTargets = 48u") == std::string::npos ||
			source.find("RollProcChance(_rng, _rngMutex, chancePct)") == std::string::npos ||
			source.find("selection.bestUsesPerTargetIcd") == std::string::npos ||
			source.find("if (selection.bestUsesPerTargetIcd && targetsHit > 0u)") == std::string::npos ||
			icdPos == std::string::npos ||
			icdPos < budgetPos) {
			std::cerr << "corpse_explosion_budget_safety: corpse budget/rate-limit/target-cap guards are missing\n";
			return false;
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
			source.find("_lootState.rerollGuard.ConsumeIfPlayerDropPickup(") == std::string::npos ||
			source.find("_lootState.playerContainerStash[stashKey] += a_event->itemCount;") == std::string::npos ||
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
			const fs::path triggerDispatchFile = repoRoot / "skse" / "CalamityAffixes" / "src" / "EventBridge.Triggers.ModCallback.cpp";
			const fs::path triggerHandlersFile = repoRoot / "skse" / "CalamityAffixes" / "src" / "EventBridge.Triggers.ModCallback.Handlers.cpp";

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

			std::string triggerTextCombined;
			for (const auto& sf : { triggerDispatchFile, triggerHandlersFile }) {
				const auto part = loadText(sf);
				if (!part.has_value()) {
					std::cerr << "mcm_drop_chance_bridge: failed to open trigger source: " << sf << "\n";
					return false;
				}
				triggerTextCombined += *part;
			}
			if (triggerTextCombined.find("eventName == kMcmSetRunewordFragmentChanceEvent") == std::string::npos ||
				triggerTextCombined.find("_loot.runewordFragmentChancePercent = std::clamp(a_numArg, 0.0f, 100.0f);") == std::string::npos ||
				triggerTextCombined.find("eventName == kMcmSetReforgeOrbChanceEvent") == std::string::npos ||
				triggerTextCombined.find("_loot.reforgeOrbChancePercent = std::clamp(a_numArg, 0.0f, 100.0f);") == std::string::npos ||
				triggerTextCombined.find("previousRunewordChance") == std::string::npos ||
				triggerTextCombined.find("previousReforgeChance") == std::string::npos ||
				triggerTextCombined.find("ShouldEmitChanceNotification(") == std::string::npos) {
				std::cerr << "mcm_drop_chance_bridge: trigger event handlers are missing\n";
				return false;
			}

		return true;
	}

}
