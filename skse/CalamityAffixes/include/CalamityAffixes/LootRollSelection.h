#pragma once

#include <cstddef>
#include <optional>
#include <random>
#include <vector>

namespace CalamityAffixes::detail
{
	// Shared loot-roll picker used by both production and runtime checks.
	// Uniform across eligible entries; caller decides eligibility beforehand.
	template <class URBG>
	[[nodiscard]] inline std::optional<std::size_t> SelectUniformEligibleLootIndex(
		URBG& a_rng,
		const std::vector<std::size_t>& a_eligible)
	{
		if (a_eligible.empty()) {
			return std::nullopt;
		}
		if (a_eligible.size() == 1) {
			return a_eligible.front();
		}

		std::uniform_int_distribution<std::size_t> dist(0, a_eligible.size() - 1);
		return a_eligible[dist(a_rng)];
	}
}
