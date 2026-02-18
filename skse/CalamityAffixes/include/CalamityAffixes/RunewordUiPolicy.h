#pragma once

#include <cstdint>

namespace CalamityAffixes
{
	[[nodiscard]] constexpr bool ShouldClearRunewordInProgressState(bool a_isCompletedBase) noexcept
	{
		return a_isCompletedBase;
	}

	[[nodiscard]] constexpr bool CanFinalizeRunewordFromPanel(
		std::uint32_t a_insertedRunes,
		std::uint32_t a_totalRunes,
		bool a_canApplyResult) noexcept
	{
		return a_insertedRunes >= a_totalRunes && a_canApplyResult;
	}

	[[nodiscard]] constexpr bool CanInsertRunewordFromPanel(bool a_hasAllRequiredRunes, bool a_canApplyResult) noexcept
	{
		return a_hasAllRequiredRunes && a_canApplyResult;
	}
}
