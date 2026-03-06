#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::ResetRuntimeStateForConfigReload()
	{
			_affixes.clear();
			_activeCounts.clear();
			_activeCritDamageBonusPct = 0.0f;
			_affixRegistry = {};
			_affixSpecialActions = {};
		_lootState.ResetForConfigReload();
		_appliedPassiveSpells.clear();
		_equippedInstanceKeysByToken.clear();
		_equippedTokenCacheReady = false;
		_trapState.Reset();
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
