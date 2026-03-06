#pragma once

#include <cstddef>
#include <vector>

namespace CalamityAffixes
{
	struct LootShuffleBagState
	{
		std::vector<std::size_t> order{};
		std::size_t cursor{ 0 };
	};
}
