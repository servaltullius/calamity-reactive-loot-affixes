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

		_affixRegistry.lootSharedAffixes.clear();
		_affixRegistry.lootSharedAffixes.reserve(_affixRegistry.lootWeaponAffixes.size() + _affixRegistry.lootArmorAffixes.size());
		appendUnique(_affixRegistry.lootSharedAffixes, _affixRegistry.lootWeaponAffixes);
		appendUnique(_affixRegistry.lootSharedAffixes, _affixRegistry.lootArmorAffixes);

		_affixRegistry.lootSharedSuffixes.clear();
		_affixRegistry.lootSharedSuffixes.reserve(_affixRegistry.lootWeaponSuffixes.size() + _affixRegistry.lootArmorSuffixes.size());
		appendUnique(_affixRegistry.lootSharedSuffixes, _affixRegistry.lootWeaponSuffixes);
		appendUnique(_affixRegistry.lootSharedSuffixes, _affixRegistry.lootArmorSuffixes);
	}
}
