#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace CalamityAffixes
{
	struct RuneTokenCount
	{
		std::uint64_t token{ 0 };
		std::uint32_t count{ 0 };
	};

	template <std::size_t MaxUnique>
	struct RuneTokenCounts
	{
		std::array<RuneTokenCount, MaxUnique> entries{};
		std::size_t size{ 0 };
	};

	// Build an allocation-free list of unique rune tokens and their required counts,
	// scanning from startIndex to the end of the rune sequence.
	template <std::size_t MaxUnique>
	[[nodiscard]] constexpr RuneTokenCounts<MaxUnique> BuildRuneTokenCounts(
		std::span<const std::uint64_t> a_runeTokens,
		std::size_t a_startIndex) noexcept
	{
		RuneTokenCounts<MaxUnique> out{};

		const std::size_t start = (a_startIndex > a_runeTokens.size()) ? a_runeTokens.size() : a_startIndex;
		for (std::size_t i = start; i < a_runeTokens.size(); ++i) {
			const auto token = a_runeTokens[i];
			if (token == 0u) {
				continue;
			}

			bool found = false;
			for (std::size_t j = 0; j < out.size; ++j) {
				if (out.entries[j].token == token) {
					out.entries[j].count += 1u;
					found = true;
					break;
				}
			}

			if (found) {
				continue;
			}
			if (out.size >= out.entries.size()) {
				continue;
			}

			out.entries[out.size] = RuneTokenCount{ .token = token, .count = 1u };
			out.size += 1u;
		}

		return out;
	}
}

