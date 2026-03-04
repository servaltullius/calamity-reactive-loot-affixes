#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::ResetRuntimeStateForConfigReload()
	{
		_affixes.clear();
		_activeCounts.clear();
		_activeCritDamageBonusPct = 0.0f;
		_affixIndexById.clear();
		_affixIndexByToken.clear();
		_affixLabelSet.clear();
		_hitTriggerAffixIndices.clear();
		_incomingHitTriggerAffixIndices.clear();
		_dotApplyTriggerAffixIndices.clear();
		_killTriggerAffixIndices.clear();
		_lowHealthTriggerAffixIndices.clear();
		_castOnCritAffixIndices.clear();
		_convertAffixIndices.clear();
		_mindOverMatterAffixIndices.clear();
		_archmageAffixIndices.clear();
		_corpseExplosionAffixIndices.clear();
		_summonCorpseExplosionAffixIndices.clear();
		_lootWeaponAffixes.clear();
		_lootArmorAffixes.clear();
		_lootWeaponSuffixes.clear();
		_lootArmorSuffixes.clear();
		_lootSharedAffixes.clear();
		_lootSharedSuffixes.clear();
		_lootPrefixSharedBag = {};
		_lootPrefixWeaponBag = {};
		_lootPrefixArmorBag = {};
		_lootSuffixSharedBag = {};
		_lootSuffixWeaponBag = {};
		_lootSuffixArmorBag = {};
		_lootPreviewAffixes.clear();
		_lootPreviewRecent.clear();
		_appliedPassiveSpells.clear();
		_equippedInstanceKeysByToken.clear();
		_equippedTokenCacheReady = false;
		_traps.clear();
		_hasActiveTraps.store(false, std::memory_order_relaxed);
		_dotCooldowns.clear();
		_dotCooldownsLastPruneAt = {};
		_dotObservedMagicEffects.clear();
		_dotTagSafetyWarned = false;
		_dotObservedMagicEffectsCapWarned = false;
		_dotTagSafetySuppressed = false;
		_perTargetCooldownStore.Clear();
		_nonHostileFirstHitGate.Clear();
		_corpseExplosionSeenCorpses.clear();
		_corpseExplosionState = {};
		_summonCorpseExplosionSeenCorpses.clear();
		_summonCorpseExplosionState = {};
		_castOnCritNextAllowed = {};
		_castOnCritCycleCursor = 0;
		_equipResync.nextAtMs = 0;
		_equipResync.intervalMs = static_cast<std::uint64_t>(kEquipResyncInterval.count());
		_loot = {};
		_lootChanceEligibleFailStreak = 0;
		_runewordFragmentFailStreak = 0;
		_reforgeOrbFailStreak = 0;
		_runtimeEnabled = true;
		_runtimeProcChanceMult = 1.0f;
		_allowNonHostilePlayerOwnedOutgoingProcs.store(false, std::memory_order_relaxed);
		_runtimeUserSettingsPersist = {};
		_lootRerollGuard.Reset();
		_playerContainerStash.clear();
		_configLoaded = false;
		_lastHealthDamageSignature = 0;
		_lastHealthDamageSignatureAt = {};
		_triggerProcBudgetWindowStartMs = 0u;
		_triggerProcBudgetConsumed = 0u;
		_lastHitAt = {};
		_lastHit = {};
		_recentOwnerHitAt.clear();
		_recentOwnerKillAt.clear();
		_recentOwnerIncomingHitAt.clear();
		_lowHealthTriggerConsumed.clear();
		_lowHealthLastObservedPct.clear();
	}
}
