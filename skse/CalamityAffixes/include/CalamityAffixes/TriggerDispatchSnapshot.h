#pragma once

#include <cstddef>
#include <vector>

namespace CalamityAffixes::detail
{
	// Copies the active trigger-index list into a caller-owned buffer.
	//
	// ProcessTrigger used to iterate `*ResolveActiveTriggerIndices(trigger)`
	// directly, i.e. a range-for over a vector owned by EventBridge, while the
	// loop body dispatched affix actions.  Those actions re-enter the engine
	// (CastSpellImmediate, trap spawns), and the engine can synchronously raise
	// TESEquipEvent -- a bound-weapon proc equipping the player is the obvious
	// case.  Our TESEquipEvent sink calls RebuildActiveCounts(), which reaches
	// RebuildActiveTriggerIndexCaches() and rebuilds that very vector with
	// clear() + reserve() + push_back().  reserve() on a grown source
	// reallocates, so the in-flight range-for was left iterating freed storage.
	//
	// procDepth does not protect this: it gates CanProcessTriggerDispatch, not
	// RebuildActiveCounts.  Iterating a private copy is what makes the loop
	// independent of any re-entrant rebuild, so the dispatch simply finishes
	// against the set it started with.
	//
	// Returns the number of indices copied.  A null source yields an empty
	// buffer, which callers treat as "nothing to dispatch".
	inline std::size_t SnapshotTriggerIndices(
		const std::vector<std::size_t>* a_source,
		std::vector<std::size_t>& a_out)
	{
		a_out.clear();
		if (!a_source) {
			return 0u;
		}

		a_out.assign(a_source->begin(), a_source->end());
		return a_out.size();
	}
}
