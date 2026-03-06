#pragma once

#include <cstddef>
#include <vector>

namespace CalamityAffixes
{
	struct AffixSpecialActionState
	{
		std::vector<std::size_t> castOnCritAffixIndices{};
		std::vector<std::size_t> convertAffixIndices{};
		std::vector<std::size_t> mindOverMatterAffixIndices{};
		std::vector<std::size_t> archmageAffixIndices{};
		std::vector<std::size_t> corpseExplosionAffixIndices{};
		std::vector<std::size_t> summonCorpseExplosionAffixIndices{};
	};
}
