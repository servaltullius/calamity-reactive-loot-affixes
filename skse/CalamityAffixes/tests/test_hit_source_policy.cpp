#include "CalamityAffixes/HitSourcePolicy.h"

using CalamityAffixes::detail::IsHitLikeSourceEvidence;

// Arguments: direct weapon, attack-data spell, melee flag, explosion flag,
// bow/crossbow resolved from the hit's own projectile.
static_assert(IsHitLikeSourceEvidence(true, false, false, false, false),
	"a swing's own weapon record routes as a hit");
static_assert(IsHitLikeSourceEvidence(false, true, false, false, false),
	"an attack-data spell keeps routing as a hit (Archmage lane)");
static_assert(IsHitLikeSourceEvidence(false, false, true, false, false),
	"an unarmed melee swing carries only the melee flag and still routes");
static_assert(IsHitLikeSourceEvidence(false, false, false, true, false),
	"explosion hits keep their existing routing");
static_assert(IsHitLikeSourceEvidence(false, false, false, false, true),
	"an arrow with incomplete HitData routes through its projectile's bow");
static_assert(!IsHitLikeSourceEvidence(false, false, false, false, false),
	"a spell or hazard tick never routes as a hit, whatever the attacker holds");
