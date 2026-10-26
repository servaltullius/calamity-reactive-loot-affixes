#include "CalamityAffixes/ArchmageSpellHitPolicy.h"

using namespace CalamityAffixes::detail;

static_assert(IsArchmageSourceSpellType(true, false), "a cast spell such as Firebolt fires Archmage");
static_assert(IsArchmageSourceSpellType(false, true), "a scroll is a spell hit too");
static_assert(!IsArchmageSourceSpellType(false, false), "shouts, powers, poisons and enchantments never fire Archmage");

static_assert(ResolveArchmageSpellHitDedupWindowMs(false) == kArchmageSpellHitDedupWindowMs);
static_assert(ResolveArchmageSpellHitDedupWindowMs(true) == kArchmageConcentrationHitDedupWindowMs);
static_assert(kArchmageConcentrationHitDedupWindowMs > kArchmageSpellHitDedupWindowMs,
	"a Flames beam is evaluated less often than separate casts");

static_assert(!ShouldSuppressArchmageSpellHit(false, 0u, 250u), "the first hit of a pair is evaluated");
static_assert(ShouldSuppressArchmageSpellHit(true, 100u, 250u), "a repeat event inside the window is skipped");
static_assert(!ShouldSuppressArchmageSpellHit(true, 250u, 250u), "a hit after the window is a new spell hit");
