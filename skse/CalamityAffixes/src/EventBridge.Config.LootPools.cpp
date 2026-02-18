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

		_lootSharedAffixes.clear();
		_lootSharedAffixes.reserve(_lootWeaponAffixes.size() + _lootArmorAffixes.size());
		appendUnique(_lootSharedAffixes, _lootWeaponAffixes);
		appendUnique(_lootSharedAffixes, _lootArmorAffixes);

		_lootSharedSuffixes.clear();
		_lootSharedSuffixes.reserve(_lootWeaponSuffixes.size() + _lootArmorSuffixes.size());
		appendUnique(_lootSharedSuffixes, _lootWeaponSuffixes);
		appendUnique(_lootSharedSuffixes, _lootArmorSuffixes);
	}
}
