#include "runtime_gate_store_checks_common.h"

int main()
{
	using namespace RuntimeGateStoreChecks;

	const bool gateOk = CheckNonHostileFirstHitGate();
	const bool storeOk = CheckPerTargetCooldownStore();
	const bool hookIndexPolicyOk = CheckHandleHealthDamageVfuncIndexPolicy();
	const bool hooksDispatchExtractionOk = CheckHooksDispatchExtractionPolicy();
	const bool pluginLoggingExceptionSafetyOk = CheckPluginLoggingExceptionSafetyPolicy();
	const bool rebuildActiveCountsLoggingOk = CheckRebuildActiveCountsLoggingPolicy();
	const bool rebuildActiveCountsExtractionOk = CheckRebuildActiveCountsExtractionPolicy();
	const bool healthDamageSignatureWindowOk = CheckHealthDamageSignatureWindowPolicy();
	const bool healthDamageGuardHelperFlowOk = CheckHealthDamageGuardHelperFlow();
	const bool tesHitFallbackSourceValidationOk = CheckTesHitFallbackSourceValidationPolicy();
	const bool configLoadPipelineExtractionOk = CheckConfigLoadPipelineExtractionPolicy();
	const bool hybridCurrencyDropPolicyOk = CheckHybridCurrencyDropPolicy();
	const bool affixSpecialActionStateExtractionOk = CheckAffixSpecialActionStateExtractionPolicy();
	const bool triggerProcPolicyExtractionOk = CheckTriggerProcPolicyExtraction();
	const bool processTriggerExtractionOk = CheckProcessTriggerExtractionPolicy();
	const bool lootSelectionOk = CheckUniformLootRollSelection();
	const bool shuffleBagSelectionOk = CheckShuffleBagLootRollSelection();
	const bool weightedShuffleBagSelectionOk = CheckWeightedShuffleBagLootRollSelection();
	const bool shuffleBagConstraintsOk = CheckShuffleBagSanitizeAndRollConstraints();
	const bool slotSanitizerOk = CheckLootSlotSanitizer();
	const bool fixedWindowBudgetOk = CheckFixedWindowBudget();
	const bool recentlyLuckyOk = CheckRecentlyAndLuckyHitGuards();
	const bool tooltipPolicyOk = CheckRunewordTooltipOverlayPolicy();
	const bool lootPreviewPolicyOk = CheckLootPreviewRuntimePolicy();
	const bool lootServiceExtractionOk = CheckLootServiceExtractionPolicy();
	const bool lootDisplayNameExtractionOk = CheckLootDisplayNameExtractionPolicy();
	const bool lootTrackedSanitizeExtractionOk = CheckLootTrackedSanitizeExtractionPolicy();
	const bool lootSlotSanitizeHelperExtractionOk = CheckLootSlotSanitizeHelperExtractionPolicy();
	const bool lootRerollExploitGuardOk = CheckLootRerollExploitGuardPolicy();
	const bool lootCurrencyLedgerPolicyOk = CheckLootCurrencyLedgerSerializationPolicy();
	const bool suffixProcChanceParsingPolicyOk = CheckSuffixProcChanceParsingPolicy();
	const bool serializationDrainSafetyPolicyOk = CheckSerializationDrainSafetyPolicy();
	const bool specialActionProcSafetyPolicyOk = CheckSpecialActionProcSafetyPolicy();
	const bool corpseExplosionBudgetSafetyPolicyOk = CheckCorpseExplosionBudgetSafetyPolicy();
	const bool serializationTransientResetOk = CheckSerializationTransientRuntimeResetPolicy();
	const bool lootChanceMcmCleanupOk = CheckLootChanceMcmCleanupPolicy();
	const bool mcmDropChanceBridgeOk = CheckMcmDropChanceRuntimeBridgePolicy();
	const bool runewordCompletedSelectionOk = CheckRunewordCompletedSelectionPolicy();
	const bool runewordRecipeEntriesMappingOk = CheckRunewordRecipeEntriesMappingPolicy();
	const bool runewordRecipeTooltipTextOk = CheckRunewordRecipeTooltipTextPolicy();
	const bool runewordUiContractExtractionOk = CheckRunewordUiContractExtractionPolicy();
	const bool runewordRuntimeStateExtractionOk = CheckRunewordRuntimeStateExtractionPolicy();
	const bool runewordCoverageConsistencyOk = CheckRunewordCoverageConsistencyPolicy();
	const bool affixRegistryStateExtractionOk = CheckAffixRegistryStateExtractionPolicy();
	const bool lowHealthTriggerSnapshotOk = CheckLowHealthTriggerSnapshotPolicy();
	const bool runewordRecipeRuntimeEligibilityOk = CheckRunewordRecipeRuntimeEligibilityPolicy();
	const bool runewordContractSnapshotOk = CheckRunewordContractSnapshotPolicy();
	const bool synthesizedAffixDisplayNameFallbackOk = CheckSynthesizedAffixDisplayNameFallbackPolicy();
	const bool synthesizedRunewordTooltipSummaryOk = CheckSynthesizedRunewordTooltipSummaryPolicy();
	const bool runewordUiPolicyHelpersOk = CheckRunewordUiPolicyHelpers();
	const bool runewordTransmuteSafetyOk = CheckRunewordTransmuteSafetyPolicy();
	const bool runewordReforgeSafetyOk = CheckRunewordReforgeSafetyPolicy();
	const bool prismaTooltipImmediateRefreshOk = CheckPrismaTooltipImmediateRefreshPolicy();
	const bool prismaPanelUiBootstrapExtractionOk = CheckPrismaPanelUiBootstrapExtractionPolicy();
	const bool prismaPanelRenderViewModelExtractionOk = CheckPrismaPanelRenderViewModelExtractionPolicy();
	const bool prismaPanelUxFlowPolicyOk = CheckPrismaPanelUxFlowPolicy();
	const bool prismaPanelDataExtractionOk = CheckPrismaPanelDataExtractionPolicy();
	const bool prismaPanelCommandRoutingExtractionOk = CheckPrismaPanelCommandRoutingExtractionPolicy();
	const bool prismaTooltipLifecycleExtractionOk = CheckPrismaTooltipLifecycleExtractionPolicy();
	const bool prismaSettingsLayoutExtractionOk = CheckPrismaSettingsLayoutExtractionPolicy();
	const bool prismaTooltipWorkerSchedulingPolicyOk = CheckPrismaTooltipWorkerSchedulingPolicy();
	const bool prismaTooltipTelemetryPolicyOk = CheckPrismaTooltipTelemetryPolicy();
	const bool runtimeUserSettingsDebounceOk = CheckRuntimeUserSettingsDebounceBehavior();
	const bool externalUserSettingsPersistenceOk = CheckExternalUserSettingsPersistencePolicy();
	const bool runtimeUserSettingsRoundTripFieldsOk = CheckRuntimeUserSettingsRoundTripFieldPolicy();
	const bool eventBridgeStateMutexReentrancyOk = CheckEventBridgeStateMutexReentrancyPolicy();
	const bool combatRuntimeStateResetOk = CheckCombatRuntimeStateResetBehavior();
	const bool combatRuntimeStateExtractionOk = CheckCombatRuntimeStateExtractionPolicy();
	const bool lootRuntimeStateResetOk = CheckLootRuntimeStateResetBehavior();
	const bool lootRuntimeStateExtractionOk = CheckLootRuntimeStateExtractionPolicy();
	const bool configReloadTransientResetOk = CheckConfigReloadTransientRuntimeResetPolicy();
	const bool runewordUiContractDefaultsOk = CheckRunewordUiContractDefaults();
	const bool perTargetPruningOk = CheckPerTargetCooldownStorePruning();
	const bool perTargetIndependenceOk = CheckPerTargetCooldownStoreIndependence();
	const bool nonHostileTtlExpiryOk = CheckNonHostileFirstHitGateTtlExpiry();
	const bool nonHostileCapacityEvictionOk = CheckNonHostileFirstHitGateCapacityEviction();
	const bool lootRerollLifecycleOk = CheckLootRerollGuardLifecycle();
	const bool lootRerollCircularOverflowOk = CheckLootRerollGuardCircularOverflow();
	const bool lootRerollEdgeCasesOk = CheckLootRerollGuardEdgeCases();
	const bool serializationLoadStateHelpersOk = CheckSerializationLoadStateHelpers();
	const bool lowHealthTriggerSnapshotHelpersOk = CheckLowHealthTriggerSnapshotHelpers();
	return (gateOk && storeOk && hookIndexPolicyOk && hooksDispatchExtractionOk && pluginLoggingExceptionSafetyOk && rebuildActiveCountsLoggingOk && rebuildActiveCountsExtractionOk && healthDamageSignatureWindowOk && healthDamageGuardHelperFlowOk && tesHitFallbackSourceValidationOk && configLoadPipelineExtractionOk && hybridCurrencyDropPolicyOk && affixSpecialActionStateExtractionOk && triggerProcPolicyExtractionOk && processTriggerExtractionOk && lootSelectionOk && shuffleBagSelectionOk && weightedShuffleBagSelectionOk &&
	        shuffleBagConstraintsOk && slotSanitizerOk && fixedWindowBudgetOk && recentlyLuckyOk && tooltipPolicyOk &&
	        lootPreviewPolicyOk &&
	        lootServiceExtractionOk &&
	        lootDisplayNameExtractionOk &&
	        lootTrackedSanitizeExtractionOk &&
	        lootSlotSanitizeHelperExtractionOk &&
	        lootRerollExploitGuardOk &&
	        lootCurrencyLedgerPolicyOk &&
	        suffixProcChanceParsingPolicyOk &&
	        serializationDrainSafetyPolicyOk &&
	        specialActionProcSafetyPolicyOk &&
	        corpseExplosionBudgetSafetyPolicyOk &&
	        serializationTransientResetOk &&
	        lootChanceMcmCleanupOk &&
	        mcmDropChanceBridgeOk &&
	        runewordCompletedSelectionOk && runewordRecipeEntriesMappingOk &&
	        runewordRecipeTooltipTextOk &&
	        runewordUiContractExtractionOk &&
	        runewordRuntimeStateExtractionOk &&
	        runewordCoverageConsistencyOk &&
	        affixRegistryStateExtractionOk &&
	        lowHealthTriggerSnapshotOk &&
	        runewordRecipeRuntimeEligibilityOk &&
	        runewordContractSnapshotOk &&
	        synthesizedAffixDisplayNameFallbackOk &&
	        synthesizedRunewordTooltipSummaryOk &&
	        runewordUiPolicyHelpersOk &&
	        runewordTransmuteSafetyOk &&
	        runewordReforgeSafetyOk &&
	        prismaTooltipImmediateRefreshOk &&
	        prismaPanelUiBootstrapExtractionOk &&
	        prismaPanelRenderViewModelExtractionOk &&
	        prismaPanelUxFlowPolicyOk &&
	        prismaPanelDataExtractionOk &&
	        prismaPanelCommandRoutingExtractionOk &&
	        prismaTooltipLifecycleExtractionOk &&
	        prismaSettingsLayoutExtractionOk &&
	        prismaTooltipWorkerSchedulingPolicyOk &&
	        prismaTooltipTelemetryPolicyOk &&
	        runtimeUserSettingsDebounceOk &&
	        externalUserSettingsPersistenceOk &&
	        runtimeUserSettingsRoundTripFieldsOk &&
	        eventBridgeStateMutexReentrancyOk &&
	        combatRuntimeStateResetOk &&
	        combatRuntimeStateExtractionOk &&
	        lootRuntimeStateResetOk &&
	        lootRuntimeStateExtractionOk &&
	        configReloadTransientResetOk &&
	        runewordUiContractDefaultsOk &&
	        perTargetPruningOk &&
	        perTargetIndependenceOk &&
	        nonHostileTtlExpiryOk &&
	        nonHostileCapacityEvictionOk &&
	        lootRerollLifecycleOk &&
	        lootRerollCircularOverflowOk &&
	        lootRerollEdgeCasesOk &&
	        serializationLoadStateHelpersOk &&
	        lowHealthTriggerSnapshotHelpersOk) ? 0 :
		                                1;
}
