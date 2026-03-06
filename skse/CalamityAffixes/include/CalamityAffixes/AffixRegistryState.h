#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CalamityAffixes
{
	struct AffixRegistryState
	{
		std::unordered_map<std::string, std::size_t> affixIndexById{};
		std::unordered_map<std::uint64_t, std::size_t> affixIndexByToken{};
		std::unordered_set<std::string> affixLabelSet{};

		std::vector<std::size_t> hitTriggerAffixIndices{};
		std::vector<std::size_t> incomingHitTriggerAffixIndices{};
		std::vector<std::size_t> dotApplyTriggerAffixIndices{};
		std::vector<std::size_t> killTriggerAffixIndices{};
		std::vector<std::size_t> lowHealthTriggerAffixIndices{};

		std::vector<std::size_t> lootWeaponAffixes{};
		std::vector<std::size_t> lootArmorAffixes{};
		std::vector<std::size_t> lootWeaponSuffixes{};
		std::vector<std::size_t> lootArmorSuffixes{};
		std::vector<std::size_t> lootSharedAffixes{};
		std::vector<std::size_t> lootSharedSuffixes{};
	};
}
