#include "CalamityAffixes/EventBridge.h"

#include <algorithm>

namespace CalamityAffixes
{
	void EventBridge::RebuildSharedLootPools()
	{
		auto appendUnique = [](std::vector<std::size_t>& a_out, const std::vector<std::size_t>& a_src) {
			for (const auto idx : a_src) {
				if (std::find(a_out.begin(), a_out.end(), idx) == a_out.end()) {
					a_out.push_back(idx);
				}
			}
		};

		_affixRuntimeState.affixRegistry.lootSharedAffixes.clear();
		_affixRuntimeState.affixRegistry.lootSharedAffixes.reserve(_affixRuntimeState.affixRegistry.lootWeaponAffixes.size() + _affixRuntimeState.affixRegistry.lootArmorAffixes.size());
		appendUnique(_affixRuntimeState.affixRegistry.lootSharedAffixes, _affixRuntimeState.affixRegistry.lootWeaponAffixes);
		appendUnique(_affixRuntimeState.affixRegistry.lootSharedAffixes, _affixRuntimeState.affixRegistry.lootArmorAffixes);

		_affixRuntimeState.affixRegistry.lootSharedSuffixes.clear();
		_affixRuntimeState.affixRegistry.lootSharedSuffixes.reserve(_affixRuntimeState.affixRegistry.lootWeaponSuffixes.size() + _affixRuntimeState.affixRegistry.lootArmorSuffixes.size());
		appendUnique(_affixRuntimeState.affixRegistry.lootSharedSuffixes, _affixRuntimeState.affixRegistry.lootWeaponSuffixes);
		appendUnique(_affixRuntimeState.affixRegistry.lootSharedSuffixes, _affixRuntimeState.affixRegistry.lootArmorSuffixes);
	}
}
