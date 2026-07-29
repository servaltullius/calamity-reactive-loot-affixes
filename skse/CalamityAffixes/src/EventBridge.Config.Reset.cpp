#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::ResetRuntimeStateForConfigReload()
	{
		_affixRuntimeState.affixes.clear();
		_affixRuntimeState.activeCounts.clear();
		_affixRuntimeState.activeCritDamageBonusPct = 0.0f;
		_affixRuntimeState.affixRegistry = {};
		_affixSpecialActions = {};
		_lootState.ResetForConfigReload();
		_instanceTrackingState.appliedPassiveSpells.clear();
		_instanceTrackingState.equippedInstanceKeysByToken.clear();
		_instanceTrackingState.equippedTokenCacheReady = false;
		ClearTrapRuntimeState();
		_corpseExplosionSeenCorpses.clear();
		_corpseExplosionState = {};
		_summonCorpseExplosionSeenCorpses.clear();
		_summonCorpseExplosionState = {};
			_combatState.ResetTransientState();
			_equipResync.nextAtMs = 0;
			_equipResync.intervalMs = static_cast<std::uint64_t>(kEquipResyncInterval.count());
		_loot = {};
		_runtimeSettings.Reset();
		_configLoaded = false;
	}
}
