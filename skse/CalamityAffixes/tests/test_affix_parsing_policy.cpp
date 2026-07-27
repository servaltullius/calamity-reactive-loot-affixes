#include "CalamityAffixes/AffixParsingPolicy.h"

using CalamityAffixes::detail::ResolveAffixKidLootWeight;
using CalamityAffixes::detail::ResolveSuffixProcChancePct;

static_assert(ResolveAffixKidLootWeight(2.0f, 7.5f) == 7.5f);
static_assert(ResolveAffixKidLootWeight(2.0f, 0.0f) == 2.0f);
static_assert(ResolveAffixKidLootWeight(2.0f, -1.0f) == 2.0f);
static_assert(ResolveSuffixProcChancePct() == 0.0f);
