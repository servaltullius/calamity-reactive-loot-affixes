#include "CalamityAffixes/SpecialActionSafetyPolicy.h"

using CalamityAffixes::detail::IsCalamityProcSource;
using CalamityAffixes::detail::ResolveSpecialActionProcChancePct;

static_assert(ResolveSpecialActionProcChancePct(-1.0f) == 0.0f);
static_assert(ResolveSpecialActionProcChancePct(0.0f) == 0.0f);
static_assert(ResolveSpecialActionProcChancePct(37.5f) == 37.5f);
static_assert(ResolveSpecialActionProcChancePct(100.0f) == 100.0f);
static_assert(ResolveSpecialActionProcChancePct(125.0f) == 100.0f);

static_assert(IsCalamityProcSource("CAFF_SPEL_PROC_FIRE"));
static_assert(IsCalamityProcSource("CAFF_"));
static_assert(!IsCalamityProcSource("caFF_SPEL_PROC_FIRE"));
static_assert(!IsCalamityProcSource("OTHER_SPEL_PROC_FIRE"));
static_assert(!IsCalamityProcSource(""));
