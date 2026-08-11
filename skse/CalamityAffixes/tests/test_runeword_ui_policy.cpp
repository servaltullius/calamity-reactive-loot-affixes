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
