#include "runtime_gate_store_checks_common.h"

int main()
{
	using namespace RuntimeGateStoreChecks;

	const bool gateOk = CheckNonHostileFirstHitGate();
	const bool storeOk = CheckPerTargetCooldownStore();
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
	const bool lootChanceMcmCleanupOk = CheckLootChanceMcmCleanupPolicy();
	const bool mcmDropChanceBridgeOk = CheckMcmDropChanceRuntimeBridgePolicy();
	const bool runewordCompletedSelectionOk = CheckRunewordCompletedSelectionPolicy();
	const bool runewordRecipeEntriesMappingOk = CheckRunewordRecipeEntriesMappingPolicy();
	const bool runewordRecipeRuntimeEligibilityOk = CheckRunewordRecipeRuntimeEligibilityPolicy();
	const bool runewordContractSnapshotOk = CheckRunewordContractSnapshotPolicy();
	const bool synthesizedAffixDisplayNameFallbackOk = CheckSynthesizedAffixDisplayNameFallbackPolicy();
	const bool runewordUiPolicyHelpersOk = CheckRunewordUiPolicyHelpers();
	const bool runewordTransmuteSafetyOk = CheckRunewordTransmuteSafetyPolicy();
	const bool runewordReforgeSafetyOk = CheckRunewordReforgeSafetyPolicy();
	const bool prismaTooltipImmediateRefreshOk = CheckPrismaTooltipImmediateRefreshPolicy();
	const bool runtimeUserSettingsDebounceOk = CheckRuntimeUserSettingsDebounceBehavior();
	const bool externalUserSettingsPersistenceOk = CheckExternalUserSettingsPersistencePolicy();
	return (gateOk && storeOk && lootSelectionOk && shuffleBagSelectionOk && weightedShuffleBagSelectionOk &&
	        shuffleBagConstraintsOk && slotSanitizerOk && fixedWindowBudgetOk && recentlyLuckyOk && tooltipPolicyOk &&
	        lootPreviewPolicyOk &&
	        lootRerollExploitGuardOk &&
	        lootChanceMcmCleanupOk &&
	        mcmDropChanceBridgeOk &&
	        runewordCompletedSelectionOk && runewordRecipeEntriesMappingOk &&
	        runewordRecipeRuntimeEligibilityOk &&
	        runewordContractSnapshotOk &&
	        synthesizedAffixDisplayNameFallbackOk &&
	        runewordUiPolicyHelpersOk &&
	        runewordTransmuteSafetyOk &&
	        runewordReforgeSafetyOk &&
	        prismaTooltipImmediateRefreshOk &&
	        runtimeUserSettingsDebounceOk &&
	        externalUserSettingsPersistenceOk) ? 0 :
	                                                     1;
}
