#include "CalamityAffixes/RunewordUiPolicy.h"

#include <array>

static_assert(
	CalamityAffixes::IsSameCompletedRuneword(0xA11CEu, 0xA11CEu),
	"same completed runeword must be recognized");

static_assert(
	!CalamityAffixes::IsSameCompletedRuneword(0u, 0u),
	"an empty base and empty selection are not a completed runeword");

static_assert(
	!CalamityAffixes::IsSameCompletedRuneword(0xA11CEu, 0xBEEFu),
	"a different selected runeword remains eligible for replacement");

static_assert(
	CalamityAffixes::ParseLockedReforgeCommandKeys("4294967297:18446744073709551614") ==
		CalamityAffixes::LockedReforgeCommandKeys{ 4294967297u, 18446744073709551614u },
	"locked reforge command must bind the expected base and affix token");

static_assert(!CalamityAffixes::ParseLockedReforgeCommandKeys("4294967297"));
static_assert(!CalamityAffixes::ParseLockedReforgeCommandKeys("0:10"));
static_assert(!CalamityAffixes::ParseLockedReforgeCommandKeys("10:0"));
static_assert(!CalamityAffixes::ParseLockedReforgeCommandKeys("10:20:30"));
static_assert(!CalamityAffixes::ParseLockedReforgeCommandKeys("10:18446744073709551616"));

static_assert([] {
	constexpr std::array<std::uint64_t, 7> input{ 42u, 7u, 42u, 0u, 9u, 7u, 1u };
	const auto normalized = CalamityAffixes::NormalizeRunewordRuneInventoryTokens(input);
	return normalized == std::vector<std::uint64_t>{ 0u, 1u, 7u, 9u, 42u };
}(), "rune inventory token policy must sort and deduplicate without hiding invalid zero tokens");
