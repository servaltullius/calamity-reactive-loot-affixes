#include "CalamityAffixes/RunewordUiPolicy.h"

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
