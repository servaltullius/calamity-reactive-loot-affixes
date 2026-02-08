#include "CalamityAffixes/RunewordUtil.h"

#include <array>

static_assert([] {
	constexpr std::array<std::uint64_t, 4> runes{ 1u, 2u, 1u, 3u };
	constexpr auto counts = CalamityAffixes::BuildRuneTokenCounts<8>(std::span(runes), 0);

	return counts.size == 3u &&
	       counts.entries[0].token == 1u && counts.entries[0].count == 2u &&
	       counts.entries[1].token == 2u && counts.entries[1].count == 1u &&
	       counts.entries[2].token == 3u && counts.entries[2].count == 1u;
}(),
	"BuildRuneTokenCounts: counts duplicates");

static_assert([] {
	constexpr std::array<std::uint64_t, 4> runes{ 1u, 2u, 1u, 3u };
	constexpr auto counts = CalamityAffixes::BuildRuneTokenCounts<8>(std::span(runes), 2);

	return counts.size == 2u &&
	       counts.entries[0].token == 1u && counts.entries[0].count == 1u &&
	       counts.entries[1].token == 3u && counts.entries[1].count == 1u;
}(),
	"BuildRuneTokenCounts: respects startIndex");

static_assert([] {
	constexpr std::array<std::uint64_t, 4> runes{ 0u, 0u, 5u, 0u };
	constexpr auto counts = CalamityAffixes::BuildRuneTokenCounts<8>(std::span(runes), 0);

	return counts.size == 1u &&
	       counts.entries[0].token == 5u && counts.entries[0].count == 1u;
}(),
	"BuildRuneTokenCounts: skips 0 tokens");

