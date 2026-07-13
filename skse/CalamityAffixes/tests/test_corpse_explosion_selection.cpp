#include "CalamityAffixes/CorpseExplosionSelectionPolicy.h"

using CalamityAffixes::detail::ClampCorpseExplosionSelectionPriority;
using CalamityAffixes::detail::IsPreferredCorpseExplosionCandidate;

static_assert(ClampCorpseExplosionSelectionPriority(-1) == 0);
static_assert(ClampCorpseExplosionSelectionPriority(20) == 20);
static_assert(ClampCorpseExplosionSelectionPriority(101) == 100);

static_assert(IsPreferredCorpseExplosionCandidate(20, 10.0f, 10, 1000.0f));
static_assert(IsPreferredCorpseExplosionCandidate(10, 25.0f, 10, 20.0f));
static_assert(!IsPreferredCorpseExplosionCandidate(10, 20.0f, 10, 20.0f));
