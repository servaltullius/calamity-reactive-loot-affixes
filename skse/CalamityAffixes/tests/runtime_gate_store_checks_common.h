#pragma once

#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/LootCurrencyLedger.h"
#include "CalamityAffixes/LootSlotSanitizer.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "CalamityAffixes/NonHostileFirstHitGate.h"
#include "CalamityAffixes/PerTargetCooldownStore.h"
#include "CalamityAffixes/RuntimeUserSettingsDebounce.h"
#include "CalamityAffixes/RunewordUiPolicy.h"
#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace RuntimeGateStoreChecks
{
	bool CheckNonHostileFirstHitGate();
	bool CheckPerTargetCooldownStore();
	bool CheckUniformLootRollSelection();
	bool CheckShuffleBagLootRollSelection();
	bool CheckWeightedShuffleBagLootRollSelection();
	bool CheckFixedWindowBudget();
	bool CheckRecentlyAndLuckyHitGuards();
	bool CheckShuffleBagSanitizeAndRollConstraints();
	bool CheckLootSlotSanitizer();

	bool CheckRunewordTooltipOverlayPolicy();
	bool CheckLootPreviewRuntimePolicy();
	bool CheckLootRerollExploitGuardPolicy();
	bool CheckLootCurrencyLedgerSerializationPolicy();
	bool CheckLootChanceMcmCleanupPolicy();
	bool CheckMcmDropChanceRuntimeBridgePolicy();

	bool CheckRunewordCompletedSelectionPolicy();
	bool CheckRunewordRecipeEntriesMappingPolicy();
	bool CheckRunewordCoverageConsistencyPolicy();
	bool CheckLowHealthTriggerSnapshotPolicy();
	bool CheckRunewordRecipeRuntimeEligibilityPolicy();
	bool CheckRunewordContractSnapshotPolicy();
	bool CheckSynthesizedAffixDisplayNameFallbackPolicy();
	bool CheckSynthesizedRunewordTooltipSummaryPolicy();
	bool CheckRunewordTransmuteSafetyPolicy();
	bool CheckRunewordUiPolicyHelpers();
	bool CheckRunewordReforgeSafetyPolicy();
	bool CheckPrismaTooltipImmediateRefreshPolicy();

	bool CheckRuntimeUserSettingsDebounceBehavior();
	bool CheckExternalUserSettingsPersistencePolicy();
}
