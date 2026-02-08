#include "CalamityAffixes/AffixToken.h"

static_assert([] {
	// FNV-1a 64-bit (UTF-8)
	return CalamityAffixes::MakeAffixToken("arc_lightning") == 0xA941C712A77418A6ull;
}());

static_assert([] {
	return CalamityAffixes::MakeAffixToken("hit_fire_damage") == 0xD318CF447DBAE5EDull;
}());

static_assert([] {
	return CalamityAffixes::MakeAffixToken("LoreBox_CAFF_AFFIX_HIT_ARC_LIGHTNING") == 0xB2D78398631CEFF6ull;
}());

static_assert([] {
	// Empty string should hash to the FNV offset basis.
	return CalamityAffixes::MakeAffixToken("") == 0xCBF29CE484222325ull;
}());

