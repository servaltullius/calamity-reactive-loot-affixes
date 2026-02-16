#include "CalamityAffixes/EventBridge.h"

#include <algorithm>

namespace CalamityAffixes
{
	void EventBridge::IndexConfiguredAffixes()
	{
		for (std::size_t i = 0; i < _affixes.size(); i++) {
			const auto& affix = _affixes[i];
			if (!affix.id.empty()) {
				const auto [it, inserted] = _affixIndexById.emplace(affix.id, i);
				if (!inserted) {
					SKSE::log::error(
						"CalamityAffixes: duplicate affix id '{}' in runtime config (firstIdx={}, dupIdx={}).",
						affix.id,
						it->second,
						i);
				}
			}
			if (affix.token != 0u) {
				const auto [it, inserted] = _affixIndexByToken.emplace(affix.token, i);
				if (!inserted) {
					SKSE::log::error(
						"CalamityAffixes: duplicate affix token in runtime config (id='{}', firstIdx={}, dupIdx={}).",
						affix.id,
						it->second,
						i);
				}
			}

			if (affix.lootType) {
				if (affix.slot == AffixSlot::kSuffix) {
					if (*affix.lootType == LootItemType::kWeapon) {
						_lootWeaponSuffixes.push_back(i);
					} else if (*affix.lootType == LootItemType::kArmor) {
						_lootArmorSuffixes.push_back(i);
					}
				} else {
					if (*affix.lootType == LootItemType::kWeapon) {
						_lootWeaponAffixes.push_back(i);
					} else if (*affix.lootType == LootItemType::kArmor) {
						_lootArmorAffixes.push_back(i);
					}
				}
			}
		}
	}

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
