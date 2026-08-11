#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

namespace CalamityAffixes
{
	struct LockedReforgeCommandKeys
	{
		std::uint64_t expectedInstanceKey{ 0u };
		std::uint64_t affixToken{ 0u };

		[[nodiscard]] constexpr bool operator==(const LockedReforgeCommandKeys&) const noexcept = default;
	};

	[[nodiscard]] constexpr std::optional<std::uint64_t> ParsePositiveDecimalUint64(
		std::string_view a_text) noexcept
	{
		if (a_text.empty()) {
			return std::nullopt;
		}

		std::uint64_t value = 0u;
		for (const char ch : a_text) {
			if (ch < '0' || ch > '9') {
				return std::nullopt;
			}
			const auto digit = static_cast<std::uint64_t>(ch - '0');
			if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10u) {
				return std::nullopt;
			}
			value = value * 10u + digit;
		}
		return value != 0u ? std::optional<std::uint64_t>{ value } : std::nullopt;
	}

	[[nodiscard]] constexpr std::optional<LockedReforgeCommandKeys> ParseLockedReforgeCommandKeys(
		std::string_view a_payload) noexcept
	{
		const auto separator = a_payload.find(':');
		if (separator == std::string_view::npos ||
			a_payload.find(':', separator + 1u) != std::string_view::npos) {
			return std::nullopt;
		}

		const auto expectedInstanceKey = ParsePositiveDecimalUint64(a_payload.substr(0u, separator));
		const auto affixToken = ParsePositiveDecimalUint64(a_payload.substr(separator + 1u));
		if (!expectedInstanceKey || !affixToken) {
			return std::nullopt;
		}
		return LockedReforgeCommandKeys{
			.expectedInstanceKey = *expectedInstanceKey,
			.affixToken = *affixToken,
		};
	}

	[[nodiscard]] constexpr bool IsSameCompletedRuneword(
		std::uint64_t a_completedResultAffixToken,
		std::uint64_t a_selectedResultAffixToken) noexcept
	{
		return a_completedResultAffixToken != 0u &&
		       a_completedResultAffixToken == a_selectedResultAffixToken;
	}

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
