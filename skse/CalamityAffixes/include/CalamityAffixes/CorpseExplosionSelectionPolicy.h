#pragma once

#include <algorithm>
#include <cstdint>

namespace CalamityAffixes::detail
{
	inline constexpr std::int32_t kCorpseExplosionSelectionPriorityMin = 0;
	inline constexpr std::int32_t kCorpseExplosionSelectionPriorityMax = 100;

	[[nodiscard]] constexpr std::int32_t ClampCorpseExplosionSelectionPriority(
		std::int32_t a_priority) noexcept
	{
		return std::clamp(
			a_priority,
			kCorpseExplosionSelectionPriorityMin,
			kCorpseExplosionSelectionPriorityMax);
	}

	[[nodiscard]] constexpr bool IsPreferredCorpseExplosionCandidate(
		std::int32_t a_candidatePriority,
		float a_candidateBaseDamage,
		std::int32_t a_currentPriority,
		float a_currentBaseDamage) noexcept
	{
		if (a_candidatePriority != a_currentPriority) {
			return a_candidatePriority > a_currentPriority;
		}

		// Strict comparison intentionally preserves the first/config-order candidate
		// when both priority and damage are equal.
		return a_candidateBaseDamage > a_currentBaseDamage;
	}
}
