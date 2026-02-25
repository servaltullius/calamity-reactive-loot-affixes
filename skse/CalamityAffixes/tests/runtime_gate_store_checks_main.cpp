#include "runtime_gate_store_checks_common.h"

int main()
{
	using namespace RuntimeGateStoreChecks;

	const bool gateOk = CheckNonHostileFirstHitGate();
	const bool storeOk = CheckPerTargetCooldownStore();
	const bool hookIndexPolicyOk = CheckHandleHealthDamageVfuncIndexPolicy();
	const bool lootSelectionOk = CheckUniformLootRollSelection();
	const bool shuffleBagSelectionOk = CheckShuffleBagLootRollSelection();
	const bool weightedShuffleBagSelectionOk = CheckWeightedShuffleBagLootRollSelection();
	const bool shuffleBagConstraintsOk = CheckShuffleBagSanitizeAndRollConstraints();
	const bool slotSanitizerOk = CheckLootSlotSanitizer();
	const bool fixedWindowBudgetOk = CheckFixedWindowBudget();
	const bool recentlyLuckyOk = CheckRecentlyAndLuckyHitGuards();
	const bool tooltipPolicyOk = CheckRunewordTooltipOverlayPolicy();
	const bool lootPreviewPolicyOk = CheckLootPreviewRuntimePolicy();
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
	const bool runewordCoverageConsistencyOk = CheckRunewordCoverageConsistencyPolicy();
	const bool lowHealthTriggerSnapshotOk = CheckLowHealthTriggerSnapshotPolicy();
	const bool runewordRecipeRuntimeEligibilityOk = CheckRunewordRecipeRuntimeEligibilityPolicy();
	const bool runewordContractSnapshotOk = CheckRunewordContractSnapshotPolicy();
	const bool synthesizedAffixDisplayNameFallbackOk = CheckSynthesizedAffixDisplayNameFallbackPolicy();
	const bool synthesizedRunewordTooltipSummaryOk = CheckSynthesizedRunewordTooltipSummaryPolicy();
	const bool runewordUiPolicyHelpersOk = CheckRunewordUiPolicyHelpers();
	const bool runewordTransmuteSafetyOk = CheckRunewordTransmuteSafetyPolicy();
	const bool runewordReforgeSafetyOk = CheckRunewordReforgeSafetyPolicy();
	const bool prismaTooltipImmediateRefreshOk = CheckPrismaTooltipImmediateRefreshPolicy();
	const bool prismaTooltipWorkerSchedulingPolicyOk = CheckPrismaTooltipWorkerSchedulingPolicy();
	const bool runtimeUserSettingsDebounceOk = CheckRuntimeUserSettingsDebounceBehavior();
	const bool externalUserSettingsPersistenceOk = CheckExternalUserSettingsPersistencePolicy();
	const bool runtimeUserSettingsRoundTripFieldsOk = CheckRuntimeUserSettingsRoundTripFieldPolicy();
	const bool eventBridgeStateMutexReentrancyOk = CheckEventBridgeStateMutexReentrancyPolicy();
	return (gateOk && storeOk && hookIndexPolicyOk && lootSelectionOk && shuffleBagSelectionOk && weightedShuffleBagSelectionOk &&
	        shuffleBagConstraintsOk && slotSanitizerOk && fixedWindowBudgetOk && recentlyLuckyOk && tooltipPolicyOk &&
	        lootPreviewPolicyOk &&
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
		        runewordCoverageConsistencyOk &&
		        lowHealthTriggerSnapshotOk &&
		        runewordRecipeRuntimeEligibilityOk &&
	        runewordContractSnapshotOk &&
	        synthesizedAffixDisplayNameFallbackOk &&
	        synthesizedRunewordTooltipSummaryOk &&
	        runewordUiPolicyHelpersOk &&
	        runewordTransmuteSafetyOk &&
	        runewordReforgeSafetyOk &&
	        prismaTooltipImmediateRefreshOk &&
	        prismaTooltipWorkerSchedulingPolicyOk &&
	        runtimeUserSettingsDebounceOk &&
	        externalUserSettingsPersistenceOk &&
	        runtimeUserSettingsRoundTripFieldsOk &&
	        eventBridgeStateMutexReentrancyOk) ? 0 :
		                                                     1;
}
