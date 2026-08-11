#include "runtime_gate_store_checks_common.h"

#include "CalamityAffixes/PluginLogging.h"
#include "CalamityAffixes/SuffixFamilySelection.h"

#include <stdexcept>

namespace RuntimeGateStoreChecks
{
	bool CheckEquippedBuildSummaryPolicy()
	{
		using namespace CalamityAffixes::detail;
		if (!ResolveEquippedBuildSummaryReady(true, false, false, false) ||
			ResolveEquippedBuildSummaryReady(true, true, false, true) ||
			!ResolveEquippedBuildSummaryReady(true, true, true, true)) {
			std::cerr << "equipped_build_summary: ready-state policy drifted\n";
			return false;
		}

		const auto disabledPassiveSpell = ResolveEquippedBuildPassiveContributionState({
			.hasPassiveSpell = true,
			.passiveSpellsDisabled = true,
		});
		const auto suppressedCrit = ResolveEquippedBuildPassiveContributionState({
			.hasCritContribution = true,
			.suffixFamilySuppressed = true,
		});
		const auto suppressedScroll = ResolveEquippedBuildPassiveContributionState({
			.hasScrollContribution = true,
			.suffixFamilySuppressed = true,
		});
		if (!disabledPassiveSpell.hasPassiveContribution ||
			disabledPassiveSpell.passiveContributionActive ||
			!disabledPassiveSpell.passiveSpellDisabled ||
			suppressedCrit.passiveContributionActive ||
			!suppressedScroll.passiveContributionActive) {
			std::cerr << "equipped_build_summary: passive contribution state drifted\n";
			return false;
		}

		const EquippedBuildPolicyInput offense{
			.trigger = EquippedBuildTriggerKind::kDotApply,
			.slot = EquippedBuildSlotKind::kPrefix,
			.procLane = EquippedBuildProcLane::kStandard,
			.configuredProcChancePct = 30.0f,
		};
		if (!ShouldShowEquippedBuildEntry(offense) ||
			!HasEquippedBuildProcRoll(offense) ||
			ResolveEquippedBuildGroup(offense) != EquippedBuildGroup::kOffense ||
			ResolveEquippedBuildTriggerKey(offense) != EquippedBuildTriggerKey::kDotApply) {
			std::cerr << "equipped_build_summary: offense proc classification drifted\n";
			return false;
		}

		const EquippedBuildPolicyInput defense{
			.trigger = EquippedBuildTriggerKind::kLowHealth,
			.slot = EquippedBuildSlotKind::kPrefix,
			.procLane = EquippedBuildProcLane::kSpecial,
			.configuredProcChancePct = 45.0f,
		};
		if (ResolveEquippedBuildGroup(defense) != EquippedBuildGroup::kDefense ||
			ResolveEquippedBuildTriggerKey(defense) != EquippedBuildTriggerKey::kLowHealth) {
			std::cerr << "equipped_build_summary: defense classification drifted\n";
			return false;
		}

		const EquippedBuildPolicyInput hiddenDebug{
			.trigger = EquippedBuildTriggerKind::kHit,
			.slot = EquippedBuildSlotKind::kPrefix,
			.procLane = EquippedBuildProcLane::kNone,
			.isDebugNotify = true,
		};
		const auto visibleDebug = EquippedBuildPolicyInput{
			.trigger = EquippedBuildTriggerKind::kHit,
			.slot = EquippedBuildSlotKind::kSuffix,
			.procLane = EquippedBuildProcLane::kNone,
			.hasPassiveContribution = true,
			.isDebugNotify = true,
		};
		if (ShouldShowEquippedBuildEntry(hiddenDebug) ||
			!ShouldShowEquippedBuildEntry(visibleDebug) ||
			ResolveEquippedBuildGroup(visibleDebug) != EquippedBuildGroup::kPassive) {
			std::cerr << "equipped_build_summary: debug-helper visibility drifted\n";
			return false;
		}

		const auto hybridRuneword = EquippedBuildPolicyInput{
			.trigger = EquippedBuildTriggerKind::kHit,
			.slot = EquippedBuildSlotKind::kRuneword,
			.procLane = EquippedBuildProcLane::kSpecial,
			.hasPassiveContribution = true,
			.configuredProcChancePct = 35.0f,
		};
		if (!HasEquippedBuildProcRoll(hybridRuneword) ||
			IsEquippedBuildPassive(hybridRuneword) ||
			ResolveEquippedBuildGroup(hybridRuneword) != EquippedBuildGroup::kOffense) {
			std::cerr << "equipped_build_summary: hybrid proc/passive classification drifted\n";
			return false;
		}

		const auto hitLucky = EquippedBuildPolicyInput{
			.trigger = EquippedBuildTriggerKind::kHit,
			.luckyHitChancePct = 20.0f,
		};
		const auto killLucky = EquippedBuildPolicyInput{
			.trigger = EquippedBuildTriggerKind::kKill,
			.luckyHitChancePct = 20.0f,
		};
		if (!HasEquippedBuildLuckyHitGate(hitLucky) || HasEquippedBuildLuckyHitGate(killLucky)) {
			std::cerr << "equipped_build_summary: lucky-hit applicability drifted\n";
			return false;
		}

		if (DescribeEquippedBuildGroup(EquippedBuildGroup::kKill) != "kill" ||
			DescribeEquippedBuildTriggerKey(EquippedBuildTriggerKey::kPassive) != "passive" ||
			DescribeEquippedBuildSlotKind(EquippedBuildSlotKind::kRuneword) != "runeword" ||
			DescribeEquippedBuildSuffixState(
				ResolveEquippedBuildSuffixState(true, true, false)) != "suppressed") {
			std::cerr << "equipped_build_summary: wire keys drifted\n";
			return false;
		}

		return true;
	}

	bool CheckNonHostileFirstHitGate()
	{
		using namespace std::chrono;

		CalamityAffixes::NonHostileFirstHitGate gate{};
		const auto now = steady_clock::now();

		// First non-hostile hit on a fresh pair is allowed.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now)) {
			std::cerr << "gate: expected first non-hostile hit to be allowed\n";
			return false;
		}

		// Reentry within the short window stays allowed.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(10))) {
			std::cerr << "gate: expected reentry within window to be allowed\n";
			return false;
		}

		// Reentry beyond the window is denied.
		if (gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(30))) {
			std::cerr << "gate: expected reentry after window to be denied\n";
			return false;
		}

		// Hostile transition clears and denies this hit.
		if (gate.Resolve(0x14u, 0x1234u, true, true, false, now + milliseconds(200))) {
			std::cerr << "gate: expected hostile transition to be denied\n";
			return false;
		}

		// After hostile clear, first non-hostile hit can be allowed again.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(250))) {
			std::cerr << "gate: expected first non-hostile hit after clear to be allowed\n";
			return false;
		}

		// Guard rails.
		if (gate.Resolve(0x14u, 0x2234u, false, false, false, now)) {
			std::cerr << "gate: expected disabled setting to deny\n";
			return false;
		}
		if (gate.Resolve(0x14u, 0x3234u, true, false, true, now)) {
			std::cerr << "gate: expected player target to deny\n";
			return false;
		}

		return true;
	}

	bool CheckPerTargetCooldownStore()
	{
		using namespace std::chrono;

		CalamityAffixes::PerTargetCooldownStore store{};
		const auto now = steady_clock::now();

		// Fresh key is not blocked.
		if (store.IsBlocked(0xA5u, 0x99u, now)) {
			std::cerr << "store: expected fresh key to be unblocked\n";
			return false;
		}

		// Commit establishes blocking interval.
		store.Commit(0xA5u, 0x99u, milliseconds(200), now);
		if (!store.IsBlocked(0xA5u, 0x99u, now + milliseconds(100))) {
			std::cerr << "store: expected key to be blocked before ICD expiry\n";
			return false;
		}
		if (store.IsBlocked(0xA5u, 0x99u, now + milliseconds(200))) {
			std::cerr << "store: expected key to unblock at ICD boundary\n";
			return false;
		}

		// Invalid commit inputs must not create blocking state.
		store.Commit(0u, 0x99u, milliseconds(200), now);
		if (store.IsBlocked(0u, 0x99u, now + milliseconds(1))) {
			std::cerr << "store: expected zero-token commit to be ignored\n";
			return false;
		}

		// Clear removes existing state.
		store.Clear();
		if (store.IsBlocked(0xA5u, 0x99u, now + milliseconds(1))) {
			std::cerr << "store: expected clear to remove cooldown state\n";
			return false;
		}

		return true;
	}

	bool CheckHandleHealthDamageVfuncIndexPolicy()
	{
		if (CalamityAffixes::Hooks::HandleHealthDamageVfuncIndexForRuntime(false) != 0x104u) {
			std::cerr << "hooks: expected non-VR HandleHealthDamage index to be 0x104\n";
			return false;
		}

		if (CalamityAffixes::Hooks::HandleHealthDamageVfuncIndexForRuntime(true) != 0x106u) {
			std::cerr << "hooks: expected VR HandleHealthDamage index to be 0x106\n";
			return false;
		}

		return true;
	}

	bool CheckHooksDispatchExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path cmakeFile = repoRoot / "CMakeLists.txt";
		const fs::path hooksFile = repoRoot / "src" / "Hooks.cpp";
		const fs::path hooksDispatchHeaderFile = repoRoot / "src" / "Hooks.Dispatch.h";
		const fs::path hooksDispatchFile = repoRoot / "src" / "Hooks.Dispatch.cpp";

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
		const auto hooksText = loadText(hooksFile);
		const auto hooksDispatchHeaderText = loadText(hooksDispatchHeaderFile);
		const auto hooksDispatchText = loadText(hooksDispatchFile);
		if (!cmakeText.has_value() || !hooksText.has_value() || !hooksDispatchHeaderText.has_value() ||
			!hooksDispatchText.has_value()) {
			std::cerr << "hooks_dispatch_extraction: failed to load source files\n";
			return false;
		}

		if (cmakeText->find("src/Hooks.Dispatch.cpp") == std::string::npos ||
			hooksText->find("#include \"Hooks.Dispatch.h\"") == std::string::npos ||
			hooksText->find("!bridge->IsRuntimeEnabled()") == std::string::npos ||
			hooksText->find("detail::AdjustDamageAndEvaluateSpecials(") == std::string::npos ||
			hooksText->find("detail::SchedulePostHealthDamageActions(") == std::string::npos ||
			hooksText->find("detail::ClearDispatchRuntimeState();") == std::string::npos ||
			hooksText->find("PlayCastOnCritProcFeedbackSfx(") != std::string::npos ||
			hooksText->find("ExecutePostHealthDamageActions(") != std::string::npos ||
			hooksDispatchHeaderText->find("struct DamageAdjustmentResult") == std::string::npos ||
			hooksDispatchHeaderText->find("void SchedulePostHealthDamageActions(") == std::string::npos ||
			hooksDispatchText->find("PlayCastOnCritProcFeedbackSfx(") == std::string::npos ||
			hooksDispatchText->find("ExecutePostHealthDamageActions(") == std::string::npos ||
			hooksDispatchText->find("thread_local bool g_inProcDispatch = false;") == std::string::npos ||
			hooksDispatchText->find("MakeProcDispatchSignature(") == std::string::npos ||
			hooksDispatchText->find("record.signature == dispatchSignature") == std::string::npos ||
			hooksDispatchText->find("kExactDuplicateCallbackWindow = std::chrono::milliseconds(50)") == std::string::npos ||
			hooksDispatchText->find("hasRecord && elapsed < kTimeCooldown") != std::string::npos) {
			std::cerr << "hooks_dispatch_extraction: hook plumbing and dispatch helpers are not cleanly separated\n";
			return false;
		}

		return true;
	}

	bool CheckPluginLoggingExceptionSafetyPolicy()
	{
		std::size_t errorCalls = 0u;
		std::string lastError;
		const bool shouldFailSafely = CalamityAffixes::ConfigurePluginLogger(
			[]() {
				throw std::runtime_error("expected sink failure");
			},
			[&](std::string_view a_reason) {
				++errorCalls;
				lastError.assign(a_reason);
			});
		if (shouldFailSafely) {
			std::cerr << "plugin_logging_exception_safety: expected throwing sink factory to return false\n";
			return false;
		}
		if (errorCalls != 1u || lastError.find("expected sink failure") == std::string::npos) {
			std::cerr << "plugin_logging_exception_safety: expected error handler to receive sink failure exactly once\n";
			return false;
		}

		std::size_t installCalls = 0u;
		const bool shouldSucceed = CalamityAffixes::ConfigurePluginLogger(
			[&]() {
				++installCalls;
			},
			[](std::string_view) {});
		if (!shouldSucceed || installCalls != 1u) {
			std::cerr << "plugin_logging_exception_safety: expected valid sink factory to install logger once\n";
			return false;
		}

		return true;
	}

	bool CheckRebuildActiveCountsLoggingPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path sourceFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Triggers.ActiveCounts.cpp";

		std::ifstream in(sourceFile);
		if (!in.is_open()) {
			std::cerr << "rebuild_active_counts_logging: failed to open source file: " << sourceFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("if (_loot.debugLog)") == std::string::npos ||
			source.find("SKSE::log::debug(") == std::string::npos ||
			source.find("\"CalamityAffixes: RebuildActiveCounts —") == std::string::npos ||
			source.find("Always log rebuild summary at INFO level") != std::string::npos ||
			source.find("SKSE::log::info(\n\t\t\t\t\t\"CalamityAffixes: RebuildActiveCounts —") != std::string::npos ||
			source.find("SKSE::log::info(\"CalamityAffixes: RebuildActiveCounts —") != std::string::npos) {
			std::cerr << "rebuild_active_counts_logging: expected summary logging to stay debug-only\n";
			return false;
		}

		return true;
	}

	bool CheckRebuildActiveCountsExtractionPolicy()
	{
		using CalamityAffixes::detail::IsAffixFamilyAvailable;
		using CalamityAffixes::detail::PassiveSpellReconcileInput;
		using CalamityAffixes::detail::PassiveSpellReconcileAction;
		using CalamityAffixes::detail::RecordSelectedAffixFamily;
		using CalamityAffixes::detail::ResolvePassiveSpellReconcileAction;
		using CalamityAffixes::detail::ShouldAccumulateFamilylessSuffixValue;
		using CalamityAffixes::detail::ShouldDeferTieredSuffixFamily;
		using CalamityAffixes::detail::SuffixFamilyBestCandidate;
		using CalamityAffixes::detail::SuffixFamilyClassification;

		// Exercise the production policies that RebuildActiveCounts delegates to.
		// These assertions survive harmless statement rewrites in EventBridge while
		// still proving the tier winner and passive reconciliation outcomes.
		SuffixFamilyBestCandidate tieredBest;
		tieredBest.Consider("suffix_assassin_t1", 4u);
		tieredBest.Consider("suffix_assassin_t3", 9u);
		tieredBest.Consider("suffix_assassin_t2", 1u);
		if (!tieredBest.selected || tieredBest.id != "suffix_assassin_t3" || tieredBest.index != 9u) {
			std::cerr << "rebuild_active_counts: highest suffix tier did not win\n";
			return false;
		}

		SuffixFamilyBestCandidate stableTieBreak;
		stableTieBreak.Consider("legacy_family_late", 7u);
		stableTieBreak.Consider("legacy_family_early", 2u);
		if (!stableTieBreak.selected || stableTieBreak.index != 2u) {
			std::cerr << "rebuild_active_counts: same-rank suffix did not use config order\n";
			return false;
		}

		struct SuffixClassificationCase
		{
			SuffixFamilyClassification classification;
			bool shouldDefer;
			bool shouldAccumulate;
		};
		constexpr std::array suffixClassifications{
			SuffixClassificationCase{ { .isSuffix = false, .hasFamily = false }, false, false },
			SuffixClassificationCase{ { .isSuffix = false, .hasFamily = true }, false, false },
			SuffixClassificationCase{ { .isSuffix = true, .hasFamily = false }, false, true },
			SuffixClassificationCase{ { .isSuffix = true, .hasFamily = true }, true, false },
		};
		for (const auto& testCase : suffixClassifications) {
			if (ShouldDeferTieredSuffixFamily(testCase.classification) != testCase.shouldDefer ||
				ShouldAccumulateFamilylessSuffixValue(testCase.classification) != testCase.shouldAccumulate) {
				std::cerr << "rebuild_active_counts: suffix family deferral policy drifted\n";
				return false;
			}
		}

		std::vector<std::string> selectedFamilies;
		RecordSelectedAffixFamily(selectedFamilies, {});
		RecordSelectedAffixFamily(selectedFamilies, "assassin");
		RecordSelectedAffixFamily(selectedFamilies, "vitality");
		if (selectedFamilies != std::vector<std::string>{ "assassin", "vitality" } ||
			!IsAffixFamilyAvailable(selectedFamilies, {}) ||
			!IsAffixFamilyAvailable(selectedFamilies, "resistance") ||
			IsAffixFamilyAvailable(selectedFamilies, "assassin")) {
			std::cerr << "rebuild_active_counts: affix family duplicate exclusion drifted\n";
			return false;
		}

		struct PassiveCase
		{
			bool desired;
			bool present;
			bool disabled;
			bool refreshRequested;
			PassiveSpellReconcileAction expected;
		};
		constexpr std::array passiveCases{
			PassiveCase{ false, false, false, false, PassiveSpellReconcileAction::kKeep },
			PassiveCase{ false, true, false, false, PassiveSpellReconcileAction::kRemove },
			PassiveCase{ true, false, false, false, PassiveSpellReconcileAction::kAdd },
			PassiveCase{ true, true, false, false, PassiveSpellReconcileAction::kKeep },
			PassiveCase{ true, true, false, true, PassiveSpellReconcileAction::kRefresh },
			PassiveCase{ true, true, true, true, PassiveSpellReconcileAction::kRemove },
		};
		for (const auto& testCase : passiveCases) {
			if (ResolvePassiveSpellReconcileAction(PassiveSpellReconcileInput{
					.desired = testCase.desired,
					.present = testCase.present,
					.passivesDisabled = testCase.disabled,
					.refreshRequested = testCase.refreshRequested,
				}) != testCase.expected) {
				std::cerr << "rebuild_active_counts: passive reconciliation decision drifted\n";
				return false;
			}
		}

		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path sourceFile = repoRoot / "src" / "EventBridge.Triggers.ActiveCounts.cpp";
		const fs::path mainFile = repoRoot / "src" / "main.cpp";
		const fs::path trapsFile = repoRoot / "src" / "EventBridge.Traps.cpp";
		const fs::path affixParsingFile = repoRoot / "src" / "EventBridge.Config.AffixParsing.cpp";
		const fs::path typesFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.Types.inl";
		const fs::path slotRollFile = repoRoot / "src" / "EventBridge.Loot.AffixSlotRoll.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto sourceText = loadText(sourceFile);
		const auto mainText = loadText(mainFile);
		const auto trapsText = loadText(trapsFile);
		const auto affixParsingText = loadText(affixParsingFile);
		const auto typesText = loadText(typesFile);
		const auto slotRollText = loadText(slotRollFile);
		if (!sourceText.has_value() || !mainText.has_value() || !trapsText.has_value() ||
			!affixParsingText.has_value() || !typesText.has_value() || !slotRollText.has_value()) {
			std::cerr << "rebuild_active_counts_extraction: failed to load source files\n";
			return false;
		}

		const auto extractFunctionBody = [](const std::string& a_text, std::string_view a_signature)
			-> std::optional<std::string_view> {
			const auto signaturePos = a_text.find(a_signature);
			if (signaturePos == std::string::npos) {
				return std::nullopt;
			}
			const auto openBrace = a_text.find('{', signaturePos + a_signature.size());
			if (openBrace == std::string::npos) {
				return std::nullopt;
			}

			std::size_t depth = 0u;
			for (std::size_t i = openBrace; i < a_text.size(); ++i) {
				if (a_text[i] == '{') {
					++depth;
				} else if (a_text[i] == '}') {
					if (--depth == 0u) {
						return std::string_view(a_text).substr(openBrace + 1u, i - openBrace - 1u);
					}
				}
			}
			return std::nullopt;
		};

		const auto deactivateBody = extractFunctionBody(*sourceText, "void EventBridge::DeactivateRuntimeState()");
		const auto refreshBody = extractFunctionBody(*sourceText, "void EventBridge::RefreshInventoryInstanceActiveState(");
		const auto accumulateBody = extractFunctionBody(*sourceText, "void EventBridge::AccumulateEquippedAffixState(");
		const auto collectBody = extractFunctionBody(*sourceText, "void EventBridge::CollectBestSuffixFamilyState(");
		const auto passiveBody = extractFunctionBody(*sourceText, "void EventBridge::ApplyDesiredPassiveSpells(");
		const auto rebuildBody = extractFunctionBody(*sourceText, "void EventBridge::RebuildActiveCounts(");
		const auto tickTrapsBody = extractFunctionBody(*trapsText, "void EventBridge::TickTraps()");
		const auto previewBody = extractFunctionBody(*slotRollText, "std::optional<InstanceAffixSlots> EventBridge::BuildLootPreviewAffixSlots(");
		if (!deactivateBody || !refreshBody || !accumulateBody || !collectBody || !passiveBody || !rebuildBody ||
			!tickTrapsBody || !previewBody) {
			std::cerr << "rebuild_active_counts_extraction: focused helper body is missing\n";
			return false;
		}
		if (refreshBody->find("AccumulateEquippedAffixState") == std::string::npos) {
			std::cerr << "rebuild_active_counts_extraction: equipped instance refresh no longer reaches accumulation\n";
			return false;
		}

		const auto rebuildReset = rebuildBody->find("ResetActiveCountsStateForRebuild");
		const auto rebuildRefresh = rebuildBody->find("RefreshInventoryInstanceActiveState");
		const auto rebuildCollect = rebuildBody->find("CollectBestSuffixFamilyState");
		const auto rebuildIndex = rebuildBody->find("RebuildActiveTriggerIndexCaches");
		const auto rebuildApply = rebuildBody->find("ApplyDesiredPassiveSpells");
		const auto rebuildSummary = rebuildBody->find("LogRebuildActiveCountsDebugSummary");
		if (rebuildBody->find("_configLoaded") == std::string::npos ||
			rebuildBody->find("_runtimeSettings.enabled") == std::string::npos ||
			rebuildBody->find("DeactivateRuntimeState") == std::string::npos ||
			rebuildBody->find("equippedTokenCacheReady") == std::string::npos ||
			rebuildReset == std::string::npos || rebuildRefresh == std::string::npos ||
			rebuildCollect == std::string::npos || rebuildIndex == std::string::npos ||
			rebuildApply == std::string::npos || rebuildSummary == std::string::npos ||
			!(rebuildReset < rebuildRefresh && rebuildRefresh < rebuildCollect &&
				rebuildCollect < rebuildIndex && rebuildIndex < rebuildApply && rebuildApply < rebuildSummary)) {
			std::cerr << "rebuild_active_counts_extraction: rebuild orchestration order drifted\n";
			return false;
		}

		const auto deactivateReset = deactivateBody->find("ResetActiveCountsStateForRebuild");
		const auto deactivatePassives = deactivateBody->find("ApplyDesiredPassiveSpells");
		const auto deactivateTraps = deactivateBody->find("ClearTrapRuntimeState");
		const auto deactivateCombat = deactivateBody->find("ResetTransientState");
		const auto deactivateHooks = deactivateBody->find("Hooks::ClearRuntimeState");
		if (deactivateBody->find("appliedPassiveSpells") == std::string::npos ||
			deactivateBody->find("nextAllowed") == std::string::npos ||
			deactivateReset == std::string::npos || deactivatePassives == std::string::npos ||
			deactivateTraps == std::string::npos || deactivateCombat == std::string::npos ||
			deactivateHooks == std::string::npos ||
			!(deactivateReset < deactivatePassives && deactivatePassives < deactivateTraps &&
				deactivateTraps < deactivateCombat && deactivateCombat < deactivateHooks)) {
			std::cerr << "rebuild_active_counts_extraction: deactivate cleanup order drifted\n";
			return false;
		}

		const auto familyCrit = accumulateBody->find("activeCritDamageBonusPct");
		if (accumulateBody->find("CountProcPenaltySlots") == std::string::npos ||
			accumulateBody->find("equippedInstanceKeysByToken") == std::string::npos ||
			accumulateBody->find("activeCounts") == std::string::npos ||
			accumulateBody->find("passiveSpell") == std::string::npos ||
			accumulateBody->find("ShouldDeferTieredSuffixFamily") == std::string::npos ||
			accumulateBody->find("ShouldAccumulateFamilylessSuffixValue") == std::string::npos ||
			familyCrit == std::string::npos ||
			accumulateBody->find("activeCritDamageBonusPct", familyCrit + 1u) != std::string::npos ||
			accumulateBody->find("activeSlotPenalty") == std::string::npos ||
			collectBody->find("SuffixFamilyBestCandidate") == std::string::npos ||
			collectBody->find("activeCounts") == std::string::npos ||
			collectBody->find("activeCritDamageBonusPct") == std::string::npos ||
			collectBody->find("disablePassiveSuffixSpells") == std::string::npos ||
			collectBody->find("passiveSpell") == std::string::npos) {
			std::cerr << "rebuild_active_counts_extraction: suffix accumulation structure drifted\n";
			return false;
		}

		if (passiveBody->find("appliedPassiveSpells") == std::string::npos ||
			passiveBody->find("refreshPassiveSpellOnPostLoad") == std::string::npos ||
			passiveBody->find("PassiveSpellReconcileInput") == std::string::npos ||
			passiveBody->find("ResolvePassiveSpellReconcileAction") == std::string::npos ||
			passiveBody->find("PassiveSpellReconcileAction::kAdd") == std::string::npos ||
			passiveBody->find("PassiveSpellReconcileAction::kRemove") == std::string::npos ||
			passiveBody->find("PassiveSpellReconcileAction::kRefresh") == std::string::npos ||
			passiveBody->find("AddSpell") == std::string::npos ||
			passiveBody->find("RemoveSpell") == std::string::npos ||
			passiveBody->find("PlayActionFeedback") == std::string::npos) {
			std::cerr << "rebuild_active_counts_extraction: passive application structure drifted\n";
			return false;
		}

		const auto hooksInstall = mainText->find("Hooks::Install");
		const auto trapsInstall = mainText->find("TrapSystem::Install");
		const auto enabledStatus = mainText->find("IsRuntimeEnabled");
		const auto trapEnabledGate = tickTrapsBody->find("_runtimeSettings.enabled");
		const auto trapTickDisableGate = tickTrapsBody->find("disableTrapSystemTick");
		const auto prefixFamilyPolicy = previewBody->find("IsAffixFamilyAvailable");
		const auto prefixFamilyRecord = previewBody->find("RecordSelectedAffixFamily");
		const auto suffixFamilySelection = previewBody->find("chosenFamilies");
		if (hooksInstall == std::string::npos || trapsInstall == std::string::npos ||
			enabledStatus == std::string::npos || !(hooksInstall < enabledStatus && trapsInstall < enabledStatus) ||
			trapEnabledGate == std::string::npos || trapTickDisableGate == std::string::npos ||
			trapEnabledGate >= trapTickDisableGate ||
			affixParsingText->find("refreshPassiveSpellOnPostLoad") == std::string::npos ||
			typesText->find("refreshPassiveSpellOnPostLoad") == std::string::npos ||
			previewBody->find("chosenPrefixIndices") == std::string::npos ||
			previewBody->find("chosenPrefixFamilies") == std::string::npos ||
			prefixFamilyPolicy == std::string::npos || prefixFamilyRecord == std::string::npos ||
			suffixFamilySelection == std::string::npos ||
			prefixFamilyPolicy >= suffixFamilySelection || prefixFamilyRecord >= suffixFamilySelection) {
			std::cerr << "rebuild_active_counts_extraction: surrounding runtime structure drifted\n";
			return false;
		}

		return true;
	}

	bool CheckHealthDamageSignatureWindowPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path sourceFile = repoRoot / "src" / "EventBridge.Triggers.HealthDamage.Routing.cpp";
		const fs::path triggerGuardsFile = repoRoot / "include" / "CalamityAffixes" / "TriggerGuards.h";

		std::ifstream in(sourceFile);
		if (!in.is_open()) {
			std::cerr << "health_damage_signature_window: failed to open source file: " << sourceFile << "\n";
			return false;
		}
		std::ifstream guardsIn(triggerGuardsFile);
		if (!guardsIn.is_open()) {
			std::cerr << "health_damage_signature_window: failed to open helper file: " << triggerGuardsFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		const std::string guards(
			(std::istreambuf_iterator<char>(guardsIn)),
			std::istreambuf_iterator<char>());

		if (guards.find("constexpr bool ShouldSuppressDuplicateHealthDamageSignature(") == std::string::npos ||
			guards.find("constexpr bool ShouldSuppressHealthDamageStaleLeak(") == std::string::npos ||
			source.find("const auto sig = MakeHealthDamageSignature") == std::string::npos ||
			source.find("ShouldSuppressDuplicateHealthDamageSignature(") == std::string::npos ||
			source.find("ShouldSuppressHealthDamageStaleLeak(expectedDealt, absDamage)") == std::string::npos ||
			source.find("kOutgoingPerTargetWindow") != std::string::npos ||
			source.find("outgoingHitPerTargetLastAt") != std::string::npos) {
			std::cerr << "health_damage_signature_window: expected routing guards to be extracted and used from the dedicated routing file\n";
			return false;
		}

		return true;
	}

	bool CheckHealthDamageGuardHelperFlow()
	{
		struct FakeHealthDamageState
		{
			std::uint64_t lastSignature{ 0u };
			std::uint64_t lastSignatureAtMs{ 0u };
		};

		struct FakeHealthDamageEvent
		{
			std::uint64_t signature{ 0u };
			std::uint64_t nowMs{ 0u };
			float expectedDealt{ 0.0f };
			float absDamage{ 0.0f };
		};

		auto shouldRoute = [](FakeHealthDamageState& a_state, const FakeHealthDamageEvent& a_event) {
			if (CalamityAffixes::ShouldSuppressDuplicateHealthDamageSignature(
				a_event.signature,
				a_state.lastSignature,
				a_event.nowMs,
				a_state.lastSignatureAtMs,
				100u)) {
				return false;
			}
			if (CalamityAffixes::ShouldSuppressHealthDamageStaleLeak(a_event.expectedDealt, a_event.absDamage)) {
				return false;
			}
			a_state.lastSignature = a_event.signature;
			a_state.lastSignatureAtMs = a_event.nowMs;
			return true;
		};

		{
			FakeHealthDamageState state{};
			if (!shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xAAu, .nowMs = 100u, .expectedDealt = 18.0f, .absDamage = 18.0f })) {
				std::cerr << "health_damage_guard_flow: expected first fake routed hit to pass\n";
				return false;
			}
			if (shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xAAu, .nowMs = 150u, .expectedDealt = 18.0f, .absDamage = 18.0f })) {
				std::cerr << "health_damage_guard_flow: expected duplicate signature inside stale window to be suppressed\n";
				return false;
			}
			if (!shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xAAu, .nowMs = 200u, .expectedDealt = 18.0f, .absDamage = 18.0f })) {
				std::cerr << "health_damage_guard_flow: expected duplicate signature at stale window boundary to pass\n";
				return false;
			}
		}

		{
			FakeHealthDamageState state{};
			if (shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xBBu, .nowMs = 300u, .expectedDealt = 20.0f, .absDamage = 4.0f })) {
				std::cerr << "health_damage_guard_flow: expected stale-leak fake event to be suppressed\n";
				return false;
			}
			if (state.lastSignature != 0u) {
				std::cerr << "health_damage_guard_flow: expected suppressed stale-leak event to leave fake state untouched\n";
				return false;
			}
		}

		{
			FakeHealthDamageState state{};
			if (!shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xC1u, .nowMs = 1000u, .expectedDealt = 14.0f, .absDamage = 14.0f })) {
				std::cerr << "health_damage_guard_flow: expected initial fake event to pass\n";
				return false;
			}
			if (!shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xC2u, .nowMs = 1001u, .expectedDealt = 14.0f, .absDamage = 14.0f })) {
				std::cerr << "health_damage_guard_flow: expected distinct rapid-hit signature to pass\n";
				return false;
			}
		}

		return true;
	}

	bool CheckTesHitFallbackSourceValidationPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path sourceFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Triggers.HitEvent.cpp";

		std::ifstream in(sourceFile);
		if (!in.is_open()) {
			std::cerr << "tes_hit_fallback_source_validation: failed to open source file: " << sourceFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		// What this still pins is structural and cheap to keep true across
		// refactors: the fallback consults the shared commit policy, honours the
		// proc-recursion guard, and resets duplicate tracking when the data is
		// not yet committed (otherwise the follow-up event carrying the real
		// data gets suppressed as a duplicate and the proc is lost).
		//
		// The decision itself -- which combinations of (hasHitData,
		// matchesActors, hasHitLikeSource) may drive a proc -- used to be
		// asserted here by pinning the exact call text of each HitDataUtil
		// helper.  That blocked any rewording of the condition while proving
		// nothing about the outcome, so it now lives in
		// IsCommittedFallbackHitData and is covered by
		// tests/test_tes_hit_fallback_policy.cpp.
		if (source.find("if (_combatState.procDepth > 0)") == std::string::npos ||
			source.find("detail::IsCommittedFallbackHitData(") == std::string::npos ||
			source.find("if (!hasCommittedHitData)") == std::string::npos ||
			source.find("Trigger::kIncomingHit") == std::string::npos ||
			source.find("Trigger::kLowHealth") == std::string::npos) {
			std::cerr << "tes_hit_fallback_source_validation: TESHitEvent fallback must validate committed hit-like source data\n";
			return false;
		}

		return true;
	}

	bool CheckBloomTrapProcFeedbackPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path feedbackHeaderFile = repoRoot / "include" / "CalamityAffixes" / "ProcFeedback.h";
		const fs::path eventBridgeTypesFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.Types.inl";
		const fs::path trapActionFile = repoRoot / "src" / "EventBridge.Actions.Trap.cpp";
		const fs::path trapFeedbackFile = repoRoot / "src" / "EventBridge.Actions.TrapFeedback.cpp";
		const fs::path trapConfigFile = repoRoot / "src" / "EventBridge.Config.AffixParsing.cpp";
		const fs::path trapsFile = repoRoot / "src" / "EventBridge.Traps.cpp";
		const fs::path mainFile = repoRoot / "src" / "main.cpp";
		const fs::path lifecycleFile = repoRoot / "src" / "EventBridge.Serialization.Lifecycle.cpp";
		const fs::path saveFile = repoRoot / "src" / "EventBridge.Serialization.Save.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto feedbackHeaderText = loadText(feedbackHeaderFile);
		const auto eventBridgeTypesText = loadText(eventBridgeTypesFile);
		const auto trapActionText = loadText(trapActionFile);
		const auto trapFeedbackText = loadText(trapFeedbackFile);
		const auto trapConfigText = loadText(trapConfigFile);
		const auto trapsText = loadText(trapsFile);
		const auto mainText = loadText(mainFile);
		const auto lifecycleText = loadText(lifecycleFile);
		const auto saveText = loadText(saveFile);
		if (!feedbackHeaderText.has_value() || !eventBridgeTypesText.has_value() ||
			!trapActionText.has_value() || !trapFeedbackText.has_value() ||
			!trapConfigText.has_value() || !trapsText.has_value() || !mainText.has_value() ||
			!lifecycleText.has_value() || !saveText.has_value()) {
			std::cerr << "bloom_trap_proc_feedback: failed to load source files\n";
			return false;
		}

		if (feedbackHeaderText->find("kBloomSpellEditorIdPrefix = \"CAFF_SPEL_DOT_BLOOM_\"") == std::string::npos ||
			feedbackHeaderText->find("inline void PlayBloomProcFeedback(") == std::string::npos ||
			feedbackHeaderText->find("inline std::string_view ResolveBloomProcDebugLabel(") == std::string::npos ||
			eventBridgeTypesText->find("std::chrono::steady_clock::time_point playerOwnedCombatCleanupExpiresAt{};") == std::string::npos ||
			eventBridgeTypesText->find("playerOwnedCombatCleanupExpiresAt = {};") == std::string::npos ||
			trapActionText->find("#include \"CalamityAffixes/ProcFeedback.h\"") == std::string::npos ||
			trapActionText->find("std::clamp(armDelaySeconds, 0.18f, 0.75f)") == std::string::npos ||
			trapActionText->find("_loot.debugHudNotifications && ProcFeedback::IsBloomProcSpell(a_action.spell)") == std::string::npos ||
			trapActionText->find("EmitDebugHudNotification(note.c_str());") == std::string::npos ||
			trapActionText->find("Calamity: {} planted") == std::string::npos ||
			trapActionText->find("Calamity: {} skipped ({})") == std::string::npos ||
			trapActionText->find("SelectSpawnTrapTarget(a_action, a_owner, a_target, a_hitData, spawnTarget, &failureReason)") == std::string::npos ||
			trapsText->find("#include \"CalamityAffixes/ProcFeedback.h\"") == std::string::npos ||
			trapsText->find("ProcFeedback::PlayBloomProcFeedback(triggeredTarget, trapSnapshot.spell, 0.12f, false);") == std::string::npos ||
			trapsText->find("_loot.debugHudNotifications && ProcFeedback::IsBloomProcSpell(trapSnapshot.spell)") == std::string::npos ||
			trapsText->find("EmitDebugHudNotification(note.c_str());") == std::string::npos ||
			trapsText->find("Calamity: {} burst") == std::string::npos ||
			trapsText->find("playerOwnedCombatCleanupExpiresAt") == std::string::npos ||
			trapsText->find("if (cleanupLeaseExpiresAt.time_since_epoch().count() == 0 || now > cleanupLeaseExpiresAt)") == std::string::npos ||
			trapsText->find("if (IsPlayerOwned(owner))") == std::string::npos) {
			std::cerr << "bloom_trap_proc_feedback: expected bloom trap proc feedback helper to stay wired at spawn and trigger time\n";
			return false;
		}

		if (eventBridgeTypesText->find("RE::TESObjectSTAT* markerWorldObject{ nullptr };") == std::string::npos ||
			eventBridgeTypesText->find("RE::ObjectRefHandle markerReference{};") == std::string::npos ||
			eventBridgeTypesText->find("RE::NiPointer<RE::TESObjectREFR> markerReferenceOwner{};") == std::string::npos ||
			eventBridgeTypesText->find("pendingMarkerCleanup") == std::string::npos ||
			trapConfigText->find("markerWorldObjectEditorId") == std::string::npos ||
			trapConfigText->find("LookupFormFromSpec<RE::TESObjectSTAT>") == std::string::npos ||
			trapFeedbackText->find("CreateReferenceAtLocation(") == std::string::npos ||
			trapFeedbackText->find("CanSpawnPlacedTrapMarker(") == std::string::npos ||
			trapFeedbackText->find("ShouldReusePlacedTrapMarker(") == std::string::npos ||
			trapFeedbackText->find("ProcessPendingTrapMarkerCleanup(") == std::string::npos ||
			mainText->find("SKSE::MessagingInterface::kSaveGame") == std::string::npos ||
			mainText->find("OnPreSaveGame()") == std::string::npos ||
			lifecycleText->find("void EventBridge::OnPreSaveGame()") == std::string::npos ||
			saveText->find("ClearTrapRuntimeState(") != std::string::npos) {
			std::cerr << "trap_world_marker_contract: expected scriptless temporary-reference spawn, reuse, cap, and cleanup wiring\n";
			return false;
		}

		return true;
	}

	bool CheckConfigLoadPipelineExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path privateApiFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.PrivateApi.inl";
		const fs::path configFile = repoRoot / "src" / "EventBridge.Config.cpp";
		const fs::path pipelineFile = repoRoot / "src" / "EventBridge.Config.LoadPipeline.cpp";
		const fs::path cmakeFile = repoRoot / "CMakeLists.txt";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto privateApiText = loadText(privateApiFile);
		const auto configText = loadText(configFile);
		const auto pipelineText = loadText(pipelineFile);
		const auto cmakeText = loadText(cmakeFile);
		if (!privateApiText.has_value() || !configText.has_value() || !cmakeText.has_value()) {
			std::cerr << "config_load_pipeline_extraction: failed to load source files\n";
			return false;
		}
		if (!pipelineText.has_value()) {
			std::cerr << "config_load_pipeline_extraction: missing pipeline source file: " << pipelineFile << "\n";
			return false;
		}

		if (privateApiText->find("void BuildConfigDerivedAffixState(const nlohmann::json& a_affixes, RE::TESDataHandler* a_handler);") == std::string::npos ||
			cmakeText->find("src/EventBridge.Config.LoadPipeline.cpp") == std::string::npos ||
			configText->find("BuildConfigDerivedAffixState(*affixes, handler);") == std::string::npos ||
			configText->find("ParseConfiguredAffixesFromJson(*affixes, handler);") != std::string::npos ||
			configText->find("IndexConfiguredAffixes();") != std::string::npos ||
			configText->find("SynthesizeRunewordRuntimeAffixes();") != std::string::npos ||
			configText->find("RebuildSharedLootPools();") != std::string::npos ||
			configText->find("_affixRuntimeState.activeCounts.assign(_affixRuntimeState.affixes.size(), 0);") != std::string::npos ||
			pipelineText->find("ParseConfiguredAffixesFromJson(a_affixes, a_handler);") == std::string::npos ||
			pipelineText->find("IndexConfiguredAffixes();") == std::string::npos ||
			pipelineText->find("SynthesizeRunewordRuntimeAffixes();") == std::string::npos ||
			pipelineText->find("RebuildSharedLootPools();") == std::string::npos ||
			pipelineText->find("_affixRuntimeState.activeCounts.assign(_affixRuntimeState.affixes.size(), 0);") == std::string::npos) {
			std::cerr << "config_load_pipeline_extraction: config load pipeline extraction is incomplete\n";
			return false;
		}

		return true;
	}

	bool CheckHybridCurrencyDropPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path sourceFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.LootRuntime.cpp";

		std::ifstream in(sourceFile);
		if (!in.is_open()) {
			std::cerr << "corpse_currency_drop_policy: failed to open source file: " << sourceFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("detail::ResolveCorpseDeathOnlyCurrencyDropPolicy()") == std::string::npos ||
			source.find("only eligible hostile deaths use the isolated corpse path") == std::string::npos ||
			source.find("Container activation, pickup, world placement, and new SPID distribution stay off") == std::string::npos ||
			source.find("SKSE death event (corpse inventory only)") == std::string::npos) {
			std::cerr << "corpse_currency_drop_policy: expected legacy hybrid token with hostile-corpse-only SKSE authority\n";
			return false;
		}

		return true;
	}

	bool CheckAffixSpecialActionStateExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path eventBridgeHeaderFile = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";
		const fs::path stateGroupsFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.StateGroups.inl";
		const fs::path specialActionHeaderFile = repoRoot / "include" / "CalamityAffixes" / "AffixSpecialActionState.h";
		const fs::path indexingFile = repoRoot / "src" / "EventBridge.Config.IndexingShared.cpp";
		const fs::path resetFile = repoRoot / "src" / "EventBridge.Config.Reset.cpp";
		const fs::path configFile = repoRoot / "src" / "EventBridge.Config.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto eventBridgeHeaderText = loadText(eventBridgeHeaderFile);
		const auto stateGroupsText = loadText(stateGroupsFile);
		const auto specialActionHeaderText = loadText(specialActionHeaderFile);
		const auto indexingText = loadText(indexingFile);
		const auto resetText = loadText(resetFile);
		const auto configText = loadText(configFile);
		if (!eventBridgeHeaderText.has_value() || !stateGroupsText.has_value() ||
			!indexingText.has_value() || !resetText.has_value() || !configText.has_value()) {
			std::cerr << "affix_special_action_state_extraction: failed to load source files\n";
			return false;
		}
		if (!specialActionHeaderText.has_value()) {
			std::cerr << "affix_special_action_state_extraction: missing special action header file: " << specialActionHeaderFile << "\n";
			return false;
		}

		if (eventBridgeHeaderText->find("#include \"CalamityAffixes/AffixSpecialActionState.h\"") == std::string::npos ||
			eventBridgeHeaderText->find("#include \"detail/EventBridge.StateGroups.inl\"") == std::string::npos ||
			eventBridgeHeaderText->find("AffixSpecialActionState _affixSpecialActions{};") == std::string::npos ||
			eventBridgeHeaderText->find("AffixRuntimeCacheState _affixRuntimeState{};") == std::string::npos ||
			eventBridgeHeaderText->find("InstanceTrackingState _instanceTrackingState{};") == std::string::npos ||
			eventBridgeHeaderText->find("std::vector<AffixRuntime>& _affixes") != std::string::npos ||
			eventBridgeHeaderText->find("std::unordered_map<std::uint64_t, InstanceAffixSlots>& _instanceAffixes") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _castOnCritAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _convertAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _mindOverMatterAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _archmageAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _corpseExplosionAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _summonCorpseExplosionAffixIndices;") != std::string::npos ||
			stateGroupsText->find("struct AffixRuntimeCacheState") == std::string::npos ||
			stateGroupsText->find("std::vector<AffixRuntime> affixes{};") == std::string::npos ||
			stateGroupsText->find("std::vector<std::size_t> activeHitTriggerAffixIndices{};") == std::string::npos ||
			stateGroupsText->find("struct InstanceTrackingState") == std::string::npos ||
			stateGroupsText->find("std::unordered_map<std::uint64_t, InstanceAffixSlots> instanceAffixes{};") == std::string::npos ||
			specialActionHeaderText->find("castOnCritAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("convertAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("mindOverMatterAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("archmageAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("corpseExplosionAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("summonCorpseExplosionAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.castOnCritAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.convertAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.mindOverMatterAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.archmageAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.corpseExplosionAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.summonCorpseExplosionAffixIndices") == std::string::npos ||
			resetText->find("_affixSpecialActions = {};") == std::string::npos ||
			configText->find("_affixSpecialActions.convertAffixIndices.size()") == std::string::npos ||
			configText->find("_affixSpecialActions.castOnCritAffixIndices.size()") == std::string::npos ||
			configText->find("_affixSpecialActions.mindOverMatterAffixIndices.size()") == std::string::npos ||
			configText->find("_affixSpecialActions.archmageAffixIndices.size()") == std::string::npos ||
			configText->find("_affixSpecialActions.corpseExplosionAffixIndices.size()") == std::string::npos) {
			std::cerr << "affix_special_action_state_extraction: special action extraction is incomplete\n";
			return false;
		}

		return true;
	}

	bool CheckTriggerProcPolicyExtraction()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path repoRoot = testFile.parent_path().parent_path();
			const fs::path privateApiFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.PrivateApi.inl";
			const fs::path triggersDispatchFile = repoRoot / "src" / "EventBridge.Triggers.cpp";
			const fs::path triggersRuntimeFile = repoRoot / "src" / "EventBridge.Triggers.Runtime.cpp";
			const fs::path policyFile = repoRoot / "src" / "EventBridge.Triggers.Policy.cpp";
			const fs::path cmakeFile = repoRoot / "CMakeLists.txt";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
			};

			const auto privateApiText = loadText(privateApiFile);
			const auto triggersDispatchText = loadText(triggersDispatchFile);
			const auto triggersRuntimeText = loadText(triggersRuntimeFile);
			const auto policyText = loadText(policyFile);
			const auto cmakeText = loadText(cmakeFile);
			if (!privateApiText.has_value() || !triggersDispatchText.has_value() || !triggersRuntimeText.has_value() ||
				!policyText.has_value() || !cmakeText.has_value()) {
				std::cerr << "trigger_proc_policy_extraction: failed to load source files\n";
				return false;
			}

			const std::string triggersText = *triggersDispatchText + *triggersRuntimeText;
			const auto trapEligibilityPos = triggersDispatchText->find("if (affix.action.type == ActionType::kSpawnTrap)");
			const auto chanceRollPos = triggersDispatchText->find("RollTriggerProcChance(chance)");
			const auto runtimeCommitPos = triggersDispatchText->find("CommitTriggerProcRuntime(affix, perTargetKey, usesPerTargetIcd, chance, a_now);");

			if (cmakeText->find("src/EventBridge.Triggers.Policy.cpp") == std::string::npos ||
				cmakeText->find("src/EventBridge.Triggers.Runtime.cpp") == std::string::npos ||
				privateApiText->find("bool PassesTriggerProcPreconditions(") == std::string::npos ||
				privateApiText->find("float ResolveTriggerProcChancePct(") == std::string::npos ||
				privateApiText->find("bool RollTriggerProcChance(float a_chancePct);") == std::string::npos ||
			privateApiText->find("void CommitTriggerProcRuntime(") == std::string::npos ||
				triggersText.find("PassesTriggerProcPreconditions(") == std::string::npos ||
				triggersText.find("ResolveTriggerProcChancePct(affix, a_affixIndex)") == std::string::npos ||
				triggersText.find("RollTriggerProcChance(chance)") == std::string::npos ||
				trapEligibilityPos == std::string::npos ||
				chanceRollPos == std::string::npos ||
				runtimeCommitPos == std::string::npos ||
				trapEligibilityPos > chanceRollPos ||
				trapEligibilityPos > runtimeCommitPos ||
				triggersText.find("CommitTriggerProcRuntime(affix, perTargetKey, usesPerTargetIcd, chance, a_now);") == std::string::npos ||
				triggersText.find("affix.procChancePct * _runtimeSettings.procChanceMult") != std::string::npos ||
				triggersText.find("ResolveTriggerProcCooldownMs(") != std::string::npos ||
			policyText->find("bool EventBridge::PassesTriggerProcPreconditions(") == std::string::npos ||
			policyText->find("float EventBridge::ResolveTriggerProcChancePct(") == std::string::npos ||
			policyText->find("bool EventBridge::RollTriggerProcChance(") == std::string::npos ||
			policyText->find("void EventBridge::CommitTriggerProcRuntime(") == std::string::npos) {
			std::cerr << "trigger_proc_policy_extraction: ProcessTrigger policy extraction is incomplete\n";
			return false;
		}

		return true;
	}

		bool CheckProcessTriggerExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path repoRoot = testFile.parent_path().parent_path();
			const fs::path privateApiFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.PrivateApi.inl";
			const fs::path snapshotFile = repoRoot / "include" / "CalamityAffixes" / "LowHealthTriggerSnapshot.h";
			const fs::path triggersFile = repoRoot / "src" / "EventBridge.Triggers.cpp";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto privateApiText = loadText(privateApiFile);
		const auto snapshotText = loadText(snapshotFile);
		const auto triggersText = loadText(triggersFile);
		if (!privateApiText.has_value() || !snapshotText.has_value() || !triggersText.has_value()) {
			std::cerr << "process_trigger_extraction: failed to load source files\n";
			return false;
		}

		if (privateApiText->find("bool CanProcessTriggerDispatch(") == std::string::npos ||
			privateApiText->find("bool TryProcessTriggerAffix(") == std::string::npos ||
			privateApiText->find("void FinalizeTriggerDispatch(") == std::string::npos ||
			snapshotText->find("struct LowHealthTriggerSnapshot") == std::string::npos ||
			snapshotText->find("constexpr LowHealthTriggerSnapshot BuildLowHealthTriggerSnapshot(") == std::string::npos ||
			triggersText->find("#include \"CalamityAffixes/LowHealthTriggerSnapshot.h\"") == std::string::npos ||
			triggersText->find("const auto lowHealthSnapshot = BuildLowHealthTriggerSnapshot(") == std::string::npos ||
			// Pin that the dispatch gate is still called, not how its arguments
			// happen to be spelled.  The re-entrancy contract it carries (the
			// caller iterates its own copy of the index list) is covered
			// behaviourally by CheckTriggerDispatchSnapshot* instead.
			triggersText->find("CanProcessTriggerDispatch(") == std::string::npos ||
			triggersText->find("TryProcessTriggerAffix(") == std::string::npos ||
			triggersText->find("FinalizeTriggerDispatch(") == std::string::npos) {
			std::cerr << "process_trigger_extraction: expected ProcessTrigger flow to stay decomposed into trigger helpers\n";
			return false;
		}

		return true;
	}

	bool CheckUniformLootRollSelection()
	{
		std::mt19937 rng{ 0xCAFFu };

		{
			const std::vector<std::size_t> empty{};
			if (CalamityAffixes::detail::SelectUniformEligibleLootIndex(rng, empty).has_value()) {
				std::cerr << "loot_select: expected empty candidate set to return nullopt\n";
				return false;
			}
		}

		{
			const std::vector<std::size_t> single{ 42u };
			const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndex(rng, single);
			if (!picked.has_value() || *picked != 42u) {
				std::cerr << "loot_select: expected single candidate set to return that element\n";
				return false;
			}
		}

		const std::vector<std::size_t> candidates{ 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u };
		std::array<std::size_t, 8> counts{};
		constexpr std::size_t kDraws = 100000u;

		for (std::size_t i = 0; i < kDraws; ++i) {
			const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndex(rng, candidates);
			if (!picked.has_value() || *picked >= counts.size()) {
				std::cerr << "loot_select: invalid pick while sampling uniform distribution\n";
				return false;
			}
			counts[*picked] += 1u;
		}

		const double expected = static_cast<double>(kDraws) / static_cast<double>(counts.size());
		const double tolerance = expected * 0.05;  // wide enough to avoid flake; tight enough to catch weighting regressions.
		for (std::size_t i = 0; i < counts.size(); ++i) {
			const double diff = std::abs(static_cast<double>(counts[i]) - expected);
			if (diff > tolerance) {
				std::cerr << "loot_select: bucket " << i << " outside tolerance (count="
				          << counts[i] << ", expected=" << expected << ", tolerance=" << tolerance << ")\n";
				return false;
			}
		}

		const auto [minIt, maxIt] = std::minmax_element(counts.begin(), counts.end());
		if (static_cast<double>(*maxIt - *minIt) > expected * 0.09) {
			std::cerr << "loot_select: spread too wide (min=" << *minIt << ", max=" << *maxIt << ")\n";
			return false;
		}

		return true;
	}

	bool CheckShuffleBagLootRollSelection()
	{
		std::mt19937 rng{ 0xBA6Au };
		const std::vector<std::size_t> pool{ 0u, 1u, 2u, 3u, 4u, 5u };
		std::vector<std::size_t> bag{};
		std::size_t cursor = 0u;

		// Full-eligible draws should behave as "without replacement" per cycle.
		for (int cycle = 0; cycle < 4; ++cycle) {
			std::array<std::size_t, 6> seen{};
			for (std::size_t draw = 0; draw < pool.size(); ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					pool,
					bag,
					cursor,
					[](std::size_t) { return true; });
				if (!picked.has_value() || *picked >= seen.size()) {
					std::cerr << "shuffle_bag: invalid draw in full-eligible cycle\n";
					return false;
				}
				seen[*picked] += 1u;
			}
			for (std::size_t i = 0; i < seen.size(); ++i) {
				if (seen[i] != 1u) {
					std::cerr << "shuffle_bag: expected exactly one hit per bucket in a cycle (bucket="
					          << i << ", count=" << seen[i] << ")\n";
					return false;
				}
			}
		}

		// Temporary ineligibility must not consume that entry from the bag.
		bag.clear();
		cursor = 0u;

		const auto first = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
			rng,
			pool,
			bag,
			cursor,
			[](std::size_t) { return true; });
		if (!first.has_value()) {
			std::cerr << "shuffle_bag: expected first draw to succeed\n";
			return false;
		}

		if (cursor >= bag.size()) {
			std::cerr << "shuffle_bag: invalid cursor after first draw\n";
			return false;
		}

		const auto temporarilyExcluded = bag[cursor];
		const auto second = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
			rng,
			pool,
			bag,
			cursor,
			[temporarilyExcluded](std::size_t idx) { return idx != temporarilyExcluded; });
		if (!second.has_value()) {
			std::cerr << "shuffle_bag: expected second draw to succeed with one temporary exclusion\n";
			return false;
		}

		const auto third = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
			rng,
			pool,
			bag,
			cursor,
			[](std::size_t) { return true; });
		if (!third.has_value()) {
			std::cerr << "shuffle_bag: expected third draw to succeed\n";
			return false;
		}
		if (*third != temporarilyExcluded) {
			std::cerr << "shuffle_bag: temporarily excluded entry was consumed unexpectedly\n";
			return false;
		}

		return true;
	}

	bool CheckWeightedShuffleBagLootRollSelection()
	{
		std::mt19937 rng{ 0xC0FFEEu };
		const std::vector<std::size_t> pool{ 0u, 1u, 2u, 3u };
		const std::array<float, 4> weights{ 8.0f, 4.0f, 2.0f, 1.0f };
		std::array<std::size_t, 4> counts{};

		std::vector<std::size_t> bag{};
		std::size_t cursor = 0u;
		constexpr std::size_t kDraws = 200000u;

		// Long-run frequency should track configured weights.
		for (std::size_t draw = 0; draw < kDraws; ++draw) {
			const auto picked = CalamityAffixes::detail::SelectWeightedEligibleLootIndexWithShuffleBag(
				rng,
				pool,
				bag,
				cursor,
				[](std::size_t) { return true; },
				[&](std::size_t idx) {
					if (idx >= weights.size()) {
						return 0.0f;
					}
					return weights[idx];
				});
			if (!picked.has_value() || *picked >= counts.size()) {
				std::cerr << "weighted_shuffle_bag: invalid weighted draw\n";
				return false;
			}
			counts[*picked] += 1u;
		}

		const double weightSum = 15.0;
		const std::array<double, 4> expected{
			8.0 / weightSum,
			4.0 / weightSum,
			2.0 / weightSum,
			1.0 / weightSum
		};

		for (std::size_t i = 0; i < counts.size(); ++i) {
			const double observed = static_cast<double>(counts[i]) / static_cast<double>(kDraws);
			if (std::abs(observed - expected[i]) > 0.015) {
				std::cerr << "weighted_shuffle_bag: distribution drift on bucket " << i
				          << " (observed=" << observed << ", expected=" << expected[i] << ")\n";
				return false;
			}
		}

		// Zero-weight entries should never win while a positive-weight option exists.
		const std::array<float, 4> zeroHeavy{ 10.0f, 0.0f, 0.0f, 0.0f };
		for (std::size_t draw = 0; draw < 1000u; ++draw) {
			const auto picked = CalamityAffixes::detail::SelectWeightedEligibleLootIndexWithShuffleBag(
				rng,
				pool,
				bag,
				cursor,
				[](std::size_t) { return true; },
				[&](std::size_t idx) {
					if (idx >= zeroHeavy.size()) {
						return 0.0f;
					}
					return zeroHeavy[idx];
				});
			if (!picked.has_value() || *picked != 0u) {
				std::cerr << "weighted_shuffle_bag: zero-weight candidate selected unexpectedly\n";
				return false;
			}
		}

		// All-zero weights must still produce a valid eligible pick (uniform fallback).
		const std::array<float, 4> allZero{ 0.0f, 0.0f, 0.0f, 0.0f };
		for (std::size_t draw = 0; draw < 1000u; ++draw) {
			const auto picked = CalamityAffixes::detail::SelectWeightedEligibleLootIndexWithShuffleBag(
				rng,
				pool,
				bag,
				cursor,
				[](std::size_t) { return true; },
				[&](std::size_t idx) {
					if (idx >= allZero.size()) {
						return 0.0f;
					}
					return allZero[idx];
				});
			if (!picked.has_value() || *picked >= pool.size()) {
				std::cerr << "weighted_shuffle_bag: all-zero fallback failed\n";
				return false;
			}
		}

		return true;
	}

	bool CheckFixedWindowBudget()
	{
		std::uint64_t windowStartMs = 0u;
		std::uint32_t consumed = 0u;

		// Disabled budget settings should always pass.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 0u, 5u, windowStartMs, consumed)) {
			std::cerr << "budget: expected windowMs=0 to disable budget gate\n";
			return false;
		}
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 50u, 0u, windowStartMs, consumed)) {
			std::cerr << "budget: expected maxPerWindow=0 to disable budget gate\n";
			return false;
		}

		windowStartMs = 0u;
		consumed = 0u;

		// First two consumes pass, third in same window fails.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected first consume to pass\n";
			return false;
		}
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(120u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected second consume to pass within same window\n";
			return false;
		}
		if (CalamityAffixes::TryConsumeFixedWindowBudget(130u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected third consume in same window to fail\n";
			return false;
		}

		// New window should reset budget.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(151u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected consume to pass after window rollover\n";
			return false;
		}

		// Time rewind should also reset window safely.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(40u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected consume to pass after timestamp rewind reset\n";
			return false;
		}

		// Budget=1 allows exactly one consume, then blocks.
		windowStartMs = 0u;
		consumed = 0u;
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 50u, 1u, windowStartMs, consumed)) {
			std::cerr << "budget: expected single-budget first consume to pass\n";
			return false;
		}
		if (CalamityAffixes::TryConsumeFixedWindowBudget(110u, 50u, 1u, windowStartMs, consumed)) {
			std::cerr << "budget: expected single-budget second consume to fail\n";
			return false;
		}

		// Both windowMs=0 and maxPerWindow=0 at the same time still passes.
		windowStartMs = 0u;
		consumed = 0u;
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 0u, 0u, windowStartMs, consumed)) {
			std::cerr << "budget: expected both-zero settings to pass\n";
			return false;
		}

		return true;
	}

	bool CheckImmediateHealthReadback()
	{
		int readCount = 0;
		int castCount = 0;
		const auto disabled = CalamityAffixes::ObserveImmediateHealthChange(
			false,
			[&]() -> std::optional<float> {
				++readCount;
				return 100.0f;
			},
			[&]() { ++castCount; });
		if (readCount != 0 || castCount != 1 || disabled.healthChange.has_value()) {
			std::cerr << "health_readback: disabled observation must cast once without reading\n";
			return false;
		}

		std::vector<int> order;
		readCount = 0;
		castCount = 0;
		const auto damage = CalamityAffixes::ObserveImmediateHealthChange(
			true,
			[&]() -> std::optional<float> {
				order.push_back(readCount == 0 ? 1 : 3);
				return readCount++ == 0 ? 100.0f : 86.425f;
			},
			[&]() {
				order.push_back(2);
				++castCount;
			});
		if (order != std::vector<int>{ 1, 2, 3 } || castCount != 1 ||
			!damage.healthChange || std::abs(*damage.healthChange - (-13.575f)) > 0.001f) {
			std::cerr << "health_readback: damage ordering or delta is incorrect\n";
			return false;
		}

		readCount = 0;
		const auto healing = CalamityAffixes::ObserveImmediateHealthChange(
			true,
			[&]() -> std::optional<float> { return readCount++ == 0 ? 50.0f : 60.0f; },
			[]() {});
		if (!healing.healthChange || *healing.healthChange != 10.0f) {
			std::cerr << "health_readback: healing delta must stay positive\n";
			return false;
		}

		const auto unchanged = CalamityAffixes::ObserveImmediateHealthChange(
			true,
			[]() -> std::optional<float> { return 100.0f; },
			[]() {});
		if (!unchanged.healthChange || *unchanged.healthChange != 0.0f) {
			std::cerr << "health_readback: a sampled zero delta must remain distinguishable\n";
			return false;
		}

		readCount = 0;
		const auto unavailable = CalamityAffixes::ObserveImmediateHealthChange(
			true,
			[&]() -> std::optional<float> {
				return readCount++ == 0 ? std::optional<float>{} : std::optional<float>{ 100.0f };
			},
			[]() {});
		if (unavailable.healthChange.has_value()) {
			std::cerr << "health_readback: missing samples must not fabricate a delta\n";
			return false;
		}

		readCount = 0;
		const auto nonFinite = CalamityAffixes::ObserveImmediateHealthChange(
			true,
			[&]() -> std::optional<float> {
				return readCount++ == 0 ?
					std::optional<float>{ std::numeric_limits<float>::infinity() } :
					std::optional<float>{ 100.0f };
			},
			[]() {});
		if (nonFinite.healthBefore.has_value() || nonFinite.healthChange.has_value()) {
			std::cerr << "health_readback: non-finite samples must be rejected\n";
			return false;
		}

		return true;
	}

	bool CheckRecentlyAndLuckyHitGuards()
	{
		// Recently window semantics.
		if (!CalamityAffixes::IsWithinRecentlyWindowMs(1200u, 1000u, 250u)) {
			std::cerr << "recently: expected event inside window to pass\n";
			return false;
		}
		if (CalamityAffixes::IsWithinRecentlyWindowMs(1301u, 1000u, 250u)) {
			std::cerr << "recently: expected event outside window to fail\n";
			return false;
		}
		if (!CalamityAffixes::IsOutsideRecentlyWindowMs(1300u, 1000u, 250u)) {
			std::cerr << "recently: expected not-hit-recently gate to pass for old hit\n";
			return false;
		}
		if (CalamityAffixes::IsOutsideRecentlyWindowMs(1199u, 1000u, 250u)) {
			std::cerr << "recently: expected not-hit-recently gate to fail for fresh hit\n";
			return false;
		}

		// Lucky-hit effective chance envelope.
		if (CalamityAffixes::ResolveLuckyHitEffectiveChancePct(25.0f, 0.5f) != 12.5f) {
			std::cerr << "lucky_hit: expected 25% * 0.5 = 12.5%\n";
			return false;
		}
		if (CalamityAffixes::ResolveLuckyHitEffectiveChancePct(80.0f, 2.0f) != 100.0f) {
			std::cerr << "lucky_hit: expected clamp to 100%\n";
			return false;
		}

		// Runtime-like stochastic sanity check for lucky-hit chance.
		std::mt19937 rng{ 0x1A2Bu };
		std::uniform_real_distribution<float> dist(0.0f, 100.0f);

		const float effectiveChance = CalamityAffixes::ResolveLuckyHitEffectiveChancePct(25.0f, 0.5f);  // 12.5%
		constexpr std::size_t kTrials = 120000u;
		std::size_t hits = 0u;
		for (std::size_t i = 0; i < kTrials; ++i) {
			if (dist(rng) < effectiveChance) {
				++hits;
			}
		}

		const double observedPct = static_cast<double>(hits) * 100.0 / static_cast<double>(kTrials);
		const double expectedPct = 12.5;
		const double tolerancePct = 0.8;  // wide enough to avoid flakes, tight enough to catch regressions.
		if (std::abs(observedPct - expectedPct) > tolerancePct) {
			std::cerr << "lucky_hit: stochastic distribution drifted (observed="
			          << observedPct << "%, expected=" << expectedPct << "%)\n";
			return false;
		}

		return true;
	}

	bool CheckShuffleBagSanitizeAndRollConstraints()
	{
		struct MockAffix
		{
			float effectiveLootWeight{ 0.0f };
			bool isSuffix{ false };
			std::string family{};
		};

		std::mt19937 rng{ 0x5EEDu };
		std::vector<MockAffix> affixes{
			{ 1.0f, false, "" },      // 0 weapon prefix
			{ 1.0f, false, "" },      // 1 weapon prefix
			{ 0.0f, false, "" },      // 2 weapon prefix (disabled)
			{ 1.0f, false, "" },      // 3 armor prefix
			{ 1.0f, false, "" },      // 4 armor prefix
			{ 1.0f, true, "life" },   // 5 suffix
			{ 1.0f, true, "mana" },   // 6 suffix
			{ 1.0f, true, "life" },   // 7 suffix
		};

		const std::vector<std::size_t> weaponPrefixes{ 0u, 1u, 2u };
		const std::vector<std::size_t> armorPrefixes{ 3u, 4u };
		const std::vector<std::size_t> sharedPrefixes{ 0u, 1u, 2u, 3u, 4u };
		const std::vector<std::size_t> sharedSuffixes{ 5u, 6u, 7u };

		std::vector<std::size_t> prefixWeaponBag{};
		std::size_t prefixWeaponCursor = 0u;
		std::vector<std::size_t> prefixSharedBag{};
		std::size_t prefixSharedCursor = 0u;
		std::vector<std::size_t> suffixSharedBag{};
		std::size_t suffixSharedCursor = 0u;

		{
			std::vector<std::size_t> dirtyBag{ 99u, 2u, 2u, 1u };
			std::size_t dirtyCursor = 99u;
			CalamityAffixes::detail::SanitizeLootShuffleBagOrder(sharedPrefixes, dirtyBag, dirtyCursor);
			const std::vector<std::size_t> expected{ 2u, 1u, 0u, 3u, 4u };
			if (dirtyBag != expected || dirtyCursor != expected.size()) {
				std::cerr << "shuffle_bag: sanitize did not normalize bag as expected\n";
				return false;
			}
		}

		{
			if (CalamityAffixes::detail::kStandardReforgeOrbCost != 1u ||
				CalamityAffixes::detail::kLockedReforgeOrbCost != 2u) {
				std::cerr << "reforge: standard/locked orb costs drifted from 1/2\n";
				return false;
			}
			if (CalamityAffixes::detail::CanLockRegularAffixForReforge(1u) ||
				!CalamityAffixes::detail::CanLockRegularAffixForReforge(2u)) {
				std::cerr << "reforge: locked mode must require at least two regular affixes\n";
				return false;
			}
			if (!CalamityAffixes::detail::IsExpectedLockedReforgeInstance(0x100u, 0x100u) ||
				CalamityAffixes::detail::IsExpectedLockedReforgeInstance(0x100u, 0x200u) ||
				CalamityAffixes::detail::IsExpectedLockedReforgeInstance(0u, 0u)) {
				std::cerr << "reforge: locked command must stay bound to the selected instance\n";
				return false;
			}
			if (CalamityAffixes::detail::ResolveReforgeTargetAffixCount(0u) != 1u) {
				std::cerr << "reforge: zero-affix bases should reroll with one target slot\n";
				return false;
			}
			if (CalamityAffixes::detail::ResolveReforgeTargetAffixCount(7u) !=
				static_cast<std::uint8_t>(CalamityAffixes::kMaxRegularAffixesPerItem)) {
				std::cerr << "reforge: corrupted high affix counts should clamp to max slots\n";
				return false;
			}

			const auto zero = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(0u);
			if (zero.prefixTarget != 0u || zero.suffixTarget != 0u) {
				std::cerr << "shuffle_bag: target=0 composition should be (0/0)\n";
				return false;
			}
			const auto one = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(1u);
			if (one.prefixTarget != 1u || one.suffixTarget != 0u) {
				std::cerr << "shuffle_bag: target=1 composition should be (1/0)\n";
				return false;
			}
			const auto two = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(2u);
			if (two.prefixTarget != 1u || two.suffixTarget != 1u) {
				std::cerr << "shuffle_bag: target=2 composition should be (1P/1S)\n";
				return false;
			}
			const auto three = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(3u);
			if (three.prefixTarget != 1u || three.suffixTarget != 2u) {
				std::cerr << "shuffle_bag: target=3 composition should be (1P/2S)\n";
				return false;
			}
			const auto lockedPrefixTwo =
				CalamityAffixes::detail::DetermineLockedReforgeRerollTargets(2u, true);
			const auto lockedSuffixTwo =
				CalamityAffixes::detail::DetermineLockedReforgeRerollTargets(2u, false);
			const auto lockedPrefixThree =
				CalamityAffixes::detail::DetermineLockedReforgeRerollTargets(3u, true);
			const auto lockedSuffixThree =
				CalamityAffixes::detail::DetermineLockedReforgeRerollTargets(3u, false);
			if (lockedPrefixTwo.prefixTarget != 0u || lockedPrefixTwo.suffixTarget != 1u ||
				lockedSuffixTwo.prefixTarget != 1u || lockedSuffixTwo.suffixTarget != 0u ||
				lockedPrefixThree.prefixTarget != 0u || lockedPrefixThree.suffixTarget != 2u ||
				lockedSuffixThree.prefixTarget != 1u || lockedSuffixThree.suffixTarget != 1u) {
				std::cerr << "reforge: locked P/S target subtraction is inconsistent\n";
				return false;
			}

			if (!CalamityAffixes::detail::ShouldConsumeSuffixRollForSingleAffixTarget(1u, 0u)) {
				std::cerr << "shuffle_bag: single-affix suffix fallback should be allowed before prefix assignment\n";
				return false;
			}
			if (CalamityAffixes::detail::ShouldConsumeSuffixRollForSingleAffixTarget(1u, 1u)) {
				std::cerr << "shuffle_bag: single-affix suffix fallback should be blocked after prefix assignment\n";
				return false;
			}
			if (!CalamityAffixes::detail::ShouldConsumeSuffixRollForSingleAffixTarget(2u, 1u)) {
				std::cerr << "shuffle_bag: multi-affix suffix roll should remain enabled\n";
				return false;
			}

			{
				CalamityAffixes::InstanceAffixSlots previous{};
				CalamityAffixes::InstanceAffixSlots rolled{};
				(void)previous.AddToken(0xA0u);
				(void)previous.AddToken(0xB0u);
				(void)previous.AddToken(0xC0u);
				(void)rolled.AddToken(0xB0u);
				(void)rolled.AddToken(0xC0u);
				const auto regular = CalamityAffixes::detail::BuildRegularOnlyAffixSlots(previous, 0xA0u);
				if (regular.count != 2u || regular.tokens[0] != 0xB0u || regular.tokens[1] != 0xC0u) {
					std::cerr << "reforge: regular-slot snapshot should exclude preserved runeword token\n";
					return false;
				}
				if (!CalamityAffixes::detail::ShouldRetryRegularAffixReforgeRoll(regular, rolled, 0u, 4u)) {
					std::cerr << "reforge: identical regular reroll should retry before final attempt\n";
					return false;
				}
				if (CalamityAffixes::detail::ShouldRetryRegularAffixReforgeRoll(regular, rolled, 3u, 4u)) {
					std::cerr << "reforge: identical regular reroll should stop retrying on final attempt\n";
					return false;
				}
				CalamityAffixes::InstanceAffixSlots reordered{};
				(void)reordered.AddToken(0xC0u);
				(void)reordered.AddToken(0xB0u);
				if (!CalamityAffixes::detail::AreInstanceAffixTokenSetsEqual(regular, reordered) ||
					!CalamityAffixes::detail::HasCompleteLockedRegularAffixReforgeRoll(2u, reordered, 0xB0u)) {
					std::cerr << "reforge: locked exact-count/no-effect set policy mismatch\n";
					return false;
				}
			}
			if (!CalamityAffixes::detail::DidConsumeExactInventoryCount(
					5u, 3u, CalamityAffixes::detail::kLockedReforgeOrbCost) ||
				CalamityAffixes::detail::DidConsumeExactInventoryCount(
					5u, 4u, CalamityAffixes::detail::kLockedReforgeOrbCost) ||
				CalamityAffixes::detail::ResolveObservedInventoryConsumption(5u, 4u) != 1u ||
				CalamityAffixes::detail::ResolveObservedInventoryConsumption(5u, 6u) != 0u) {
				std::cerr << "reforge: locked two-orb exact-delta/refund policy mismatch\n";
				return false;
			}
		}

		{
			// Non-shared weapon pool must never pick armor indices and must skip disabled index 2.
			for (std::size_t draw = 0; draw < 40; ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					weaponPrefixes,
					prefixWeaponBag,
					prefixWeaponCursor,
					[&](std::size_t idx) {
						return idx < affixes.size() && affixes[idx].effectiveLootWeight > 0.0f;
					});
				if (!picked.has_value()) {
					std::cerr << "shuffle_bag: weapon-prefix draw unexpectedly failed\n";
					return false;
				}
				if (*picked >= 3u) {
					std::cerr << "shuffle_bag: weapon-only draw leaked into armor pool (idx=" << *picked << ")\n";
					return false;
				}
				if (*picked == 2u) {
					std::cerr << "shuffle_bag: disabled prefix was selected\n";
					return false;
				}
			}
		}

		{
			// Shared prefix pool with exclusions should converge to the only allowed prefix (idx=4).
			const std::vector<std::size_t> exclude{ 0u, 1u, 2u, 3u };
			for (std::size_t draw = 0; draw < 10; ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					sharedPrefixes,
					prefixSharedBag,
					prefixSharedCursor,
					[&](std::size_t idx) {
						if (idx >= affixes.size() || affixes[idx].effectiveLootWeight <= 0.0f) {
							return false;
						}
						return std::find(exclude.begin(), exclude.end(), idx) == exclude.end();
					});
				if (!picked.has_value() || *picked != 4u) {
					std::cerr << "shuffle_bag: shared-prefix exclusion constraint broken\n";
					return false;
				}
			}
		}

		{
			// Suffix family exclusion should never pick family='life' (idx=5,7).
			const std::vector<std::string> excludeFamilies{ "life" };
			for (std::size_t draw = 0; draw < 10; ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					sharedSuffixes,
					suffixSharedBag,
					suffixSharedCursor,
					[&](std::size_t idx) {
						if (idx >= affixes.size()) {
							return false;
						}
						const auto& affix = affixes[idx];
						if (!affix.isSuffix || affix.effectiveLootWeight <= 0.0f) {
							return false;
						}
						if (!affix.family.empty()) {
							return std::find(excludeFamilies.begin(), excludeFamilies.end(), affix.family) == excludeFamilies.end();
						}
						return true;
					});
				if (!picked.has_value() || *picked != 6u) {
					std::cerr << "shuffle_bag: suffix family exclusion constraint broken\n";
					return false;
				}
			}
		}

		(void)armorPrefixes;  // documents that armor pool exists in this integration-like scenario.
		return true;
	}

	bool CheckLootSlotSanitizer()
	{
		CalamityAffixes::InstanceAffixSlots slots{};
		slots.count = 3u;
		slots.tokens[0] = 101u;
		slots.tokens[1] = 202u;
		slots.tokens[2] = 101u;  // duplicate should be removed.

		std::array<std::uint64_t, CalamityAffixes::kMaxAffixesPerItem> removed{};
		std::uint8_t removedCount = 0u;
		const auto sanitized = CalamityAffixes::detail::BuildSanitizedInstanceAffixSlots(
			slots,
			[](std::uint64_t token) { return token != 202u; },
			&removed,
			&removedCount);

		if (sanitized.count != 1u || sanitized.tokens[0] != 101u) {
			std::cerr << "slot_sanitize: expected only token 101 to remain\n";
			return false;
		}
		if (removedCount != 2u) {
			std::cerr << "slot_sanitize: expected two removed tokens (disallowed + duplicate)\n";
			return false;
		}

		CalamityAffixes::InstanceAffixSlots corrupted{};
		corrupted.count = 7u;  // out-of-range count should be clamped internally.
		corrupted.tokens[0] = 0u;
		corrupted.tokens[1] = 301u;
		corrupted.tokens[2] = 302u;
		const auto sanitizedCorrupted = CalamityAffixes::detail::BuildSanitizedInstanceAffixSlots(
			corrupted,
			[](std::uint64_t) { return true; });
		if (sanitizedCorrupted.count != 2u ||
			sanitizedCorrupted.tokens[0] != 301u ||
			sanitizedCorrupted.tokens[1] != 302u) {
			std::cerr << "slot_sanitize: expected clamp+zero-filter behavior for corrupted slot input\n";
			return false;
		}

		return true;
	}
}
