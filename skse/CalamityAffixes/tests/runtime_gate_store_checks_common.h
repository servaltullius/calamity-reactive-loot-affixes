#pragma once

#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/LootCurrencyLedger.h"
#include "CalamityAffixes/CorpseCurrencyPolicy.h"
#include "CalamityAffixes/CorpseCurrencyRewardPolicy.h"
#include "CalamityAffixes/EquippedBuildSummaryPolicy.h"
#include "CalamityAffixes/LootSlotSanitizer.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "CalamityAffixes/LootRerollGuard.h"
#include "CalamityAffixes/NonHostileFirstHitGate.h"
#include "CalamityAffixes/LowHealthTriggerSnapshot.h"
#include "CalamityAffixes/PerTargetCooldownStore.h"
#include "CalamityAffixes/RuntimeUserSettingsDebounce.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/ImmediateHealthReadback.h"
#include "CalamityAffixes/RuntimePolicy.h"
#include "CalamityAffixes/RunewordUiPolicy.h"
#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace RuntimeGateStoreChecks
{
	// Read the Prisma panel view as one buffer, across however many files it is
	// split into.
	//
	// index.html once held the markup, a single <style> block and a single
	// <script> block, and every check that inspects the panel grew up reading
	// that one file. Splitting it breaks those checks twice over: positive pins
	// stop finding text that moved out, and -- worse -- NEGATIVE pins start
	// passing because the text they forbid is no longer in the buffer being
	// searched. The second failure mode is silent; coverage disappears without
	// a single check going red.
	//
	// Inlining each referenced file at its reference site removes the
	// difference, so a check sees the same text either way. The <link> and
	// <script src> tags stay the single source of load order -- a list
	// maintained here could drift from what the browser actually loads.
	[[nodiscard]] inline std::optional<std::string> LoadPrismaViewSource()
	{
		namespace fs = std::filesystem;
		// __FILE__ is .../skse/CalamityAffixes/tests/<this header>; four hops
		// reach the repository root.
		const fs::path thisFile{ __FILE__ };
		const fs::path viewDir = thisFile.parent_path().parent_path().parent_path().parent_path() /
			"Data" / "PrismaUI" / "views" / "CalamityAffixes";

		const auto readFile = [](const fs::path& a_path) -> std::optional<std::string> {
			std::ifstream in(a_path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		auto source = readFile(viewDir / "index.html");
		if (!source.has_value()) {
			return std::nullopt;
		}

		// Substitute at the reference site rather than appending, so document
		// order survives. Checks that slice "from marker A to marker B" depend
		// on it.
		//
		// The inlined body is wrapped in the block tag it replaces, matching
		// tools/prisma_view_source.py byte for byte. Two loaders that disagree
		// would let a check pass on one side and fail on the other.
		const auto inlineReferences = [&](std::string_view a_tagOpen,
									 std::string_view a_attribute,
									 std::string_view a_closeTag,
									 std::string_view a_blockTag) -> bool {
			std::size_t pos = 0;
			while ((pos = source->find(a_tagOpen, pos)) != std::string::npos) {
				const auto tagEnd = source->find('>', pos);
				if (tagEnd == std::string::npos) {
					return false;
				}
				const auto attributePos = source->find(a_attribute, pos);
				if (attributePos == std::string::npos || attributePos > tagEnd) {
					// No such attribute inside this tag -- an inline <style> or
					// <script> block. Leave it where it is.
					pos = tagEnd + 1;
					continue;
				}
				const auto valueStart = attributePos + a_attribute.size();
				const auto valueEnd = source->find('"', valueStart);
				if (valueEnd == std::string::npos || valueEnd > tagEnd) {
					return false;
				}

				const std::string relative = source->substr(valueStart, valueEnd - valueStart);
				auto replaceEnd = tagEnd + 1;
				if (!a_closeTag.empty()) {
					const auto closePos = source->find(a_closeTag, tagEnd);
					if (closePos == std::string::npos) {
						return false;
					}
					replaceEnd = closePos + a_closeTag.size();
				}

				const auto content = readFile(viewDir / relative);
				if (!content.has_value()) {
					std::cerr << "prisma view: referenced file missing: " << relative << "\n";
					return false;
				}

				const std::string replacement = "<" + std::string(a_blockTag) + ">\n" + *content +
												"\n</" + std::string(a_blockTag) + ">";
				source->replace(pos, replaceEnd - pos, replacement);
				// Skip the whole replacement: the inlined body may itself
				// contain the tag text we are scanning for.
				pos += replacement.size();
			}
			return true;
		};

		if (!inlineReferences("<link", "href=\"", "", "style") ||
			!inlineReferences("<script", "src=\"", "</script>", "script")) {
			return std::nullopt;
		}
		return source;
	}

	bool CheckNonHostileFirstHitGate();
	bool CheckPerTargetCooldownStore();
	bool CheckHandleHealthDamageVfuncIndexPolicy();
	bool CheckHooksDispatchExtractionPolicy();
	bool CheckPluginLoggingExceptionSafetyPolicy();
	bool CheckRebuildActiveCountsLoggingPolicy();
	bool CheckRebuildActiveCountsExtractionPolicy();
	bool CheckEquippedBuildSummaryPolicy();
	bool CheckHealthDamageSignatureWindowPolicy();
	bool CheckHealthDamageGuardHelperFlow();
	bool CheckTesHitFallbackSourceValidationPolicy();
	bool CheckBloomTrapProcFeedbackPolicy();
	bool CheckConfigLoadPipelineExtractionPolicy();
	bool CheckHybridCurrencyDropPolicy();
	bool CheckAffixSpecialActionStateExtractionPolicy();
	bool CheckTriggerProcPolicyExtraction();
	bool CheckProcessTriggerExtractionPolicy();
	bool CheckUniformLootRollSelection();
	bool CheckShuffleBagLootRollSelection();
	bool CheckWeightedShuffleBagLootRollSelection();
	bool CheckFixedWindowBudget();
	bool CheckImmediateHealthReadback();
	bool CheckRecentlyAndLuckyHitGuards();
	bool CheckShuffleBagSanitizeAndRollConstraints();
	bool CheckLootSlotSanitizer();

	bool CheckRunewordTooltipOverlayPolicy();
	bool CheckLootPreviewRuntimePolicy();
	bool CheckLootServiceExtractionPolicy();
	bool CheckLootDisplayNameExtractionPolicy();
	bool CheckLootTrackedSanitizeExtractionPolicy();
	bool CheckLootSlotSanitizeHelperExtractionPolicy();
	bool CheckLootRerollExploitGuardPolicy();
	bool CheckLootCurrencyLedgerSerializationPolicy();
	bool CheckCorpseCurrencySpecialRewardPolicy();
	bool CheckLootEligibilityCleanupSafetyPolicy();
	bool CheckSuffixProcChanceParsingPolicy();
	bool CheckSerializationDrainSafetyPolicy();
	bool CheckSpecialActionProcSafetyPolicy();
	bool CheckCorpseExplosionBudgetSafetyPolicy();
	bool CheckSerializationTransientRuntimeResetPolicy();
	bool CheckLootChanceMcmCleanupPolicy();
	bool CheckMcmDropChanceRuntimeBridgePolicy();

	bool CheckRunewordCompletedSelectionPolicy();
	bool CheckRunewordRecipeEntriesMappingPolicy();
	bool CheckRunewordRecipeTooltipTextPolicy();
	bool CheckRunewordUiContractExtractionPolicy();
	bool CheckRunewordRuntimeStateExtractionPolicy();
	bool CheckRunewordCoverageConsistencyPolicy();
	bool CheckAffixRegistryStateExtractionPolicy();
	bool CheckLowHealthTriggerSnapshotPolicy();
	bool CheckRunewordRecipeRuntimeEligibilityPolicy();
	bool CheckRunewordContractSnapshotPolicy();
	bool CheckSynthesizedAffixDisplayNameFallbackPolicy();
	bool CheckSynthesizedRunewordTooltipSummaryPolicy();
	bool CheckRunewordTransmuteSafetyPolicy();
	bool CheckRunewordUiPolicyHelpers();
	bool CheckRunewordReforgeSafetyPolicy();
	bool CheckPrismaTooltipImmediateRefreshPolicy();
	bool CheckPrismaPanelUiBootstrapExtractionPolicy();
	bool CheckPrismaPanelRenderViewModelExtractionPolicy();
	bool CheckPrismaPanelUxFlowPolicy();
	bool CheckPrismaPanelRecipeScrollPerformancePolicy();
	bool CheckPrismaPanelDataExtractionPolicy();
	bool CheckPrismaPanelCommandRoutingExtractionPolicy();
	bool CheckPrismaTooltipLifecycleExtractionPolicy();
	bool CheckPrismaSettingsLayoutExtractionPolicy();
	bool CheckPrismaTooltipWorkerSchedulingPolicy();
	bool CheckPrismaTooltipTelemetryPolicy();

	bool CheckRuntimeUserSettingsDebounceBehavior();
	bool CheckExternalUserSettingsPersistencePolicy();
	bool CheckRuntimeUserSettingsRoundTripFieldPolicy();
	bool CheckRuntimeDebugSettingsSplitPolicy();
	bool CheckPlayerHealthDamageHookDefaultPolicy();
	bool CheckEventBridgeStateMutexReentrancyPolicy();
	bool CheckCombatRuntimeStateResetBehavior();
	bool CheckCombatRuntimeStateExtractionPolicy();
	bool CheckLootRuntimeStateResetBehavior();
	bool CheckLootRuntimeStateExtractionPolicy();
	bool CheckConfigReloadTransientRuntimeResetPolicy();
	bool CheckRunewordUiContractDefaults();

	bool CheckPerTargetCooldownStorePruning();
	bool CheckPerTargetCooldownStoreIndependence();
	bool CheckNonHostileFirstHitGateTtlExpiry();
	bool CheckNonHostileFirstHitGateCapacityEviction();
	bool CheckLootRerollGuardLifecycle();
	bool CheckLootRerollGuardCircularOverflow();
	bool CheckLootRerollGuardEdgeCases();
	bool CheckSerializationLoadStateHelpers();
	bool CheckSerializationWireContract();
	bool CheckLowHealthTriggerSnapshotHelpers();

	bool CheckScopedProcDepthGuard();

	bool CheckTriggerDispatchSnapshotIsolation();
	bool CheckTriggerDispatchSnapshotNullSource();
	bool CheckTriggerDispatchSnapshotBufferReuse();

	bool CheckAffixCountRollDistribution();
	bool CheckAffixCountRollUnitBounds();
	bool CheckReforgeTargetAffixCountBounds();
	bool CheckReforgeTargetWithRunewordSlots();
	bool CheckAffixCountWeightsAllowMultiAffix();
}
