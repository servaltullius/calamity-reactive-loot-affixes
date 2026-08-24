#include "CalamityAffixes/SpecialActionSafetyPolicy.h"

using CalamityAffixes::detail::IsCalamityProcSource;
using CalamityAffixes::detail::CastOnCritWeaponSource;
using CalamityAffixes::detail::IsEligibleNormalWeaponHitFlags;
using CalamityAffixes::detail::ResolveCastOnCritSelectionCount;
using CalamityAffixes::detail::ResolveCastOnCritWeaponSource;
using CalamityAffixes::detail::ResolveCyclicCandidateIndex;
using CalamityAffixes::detail::ResolveSpecialActionProcChancePct;

static_assert(ResolveCastOnCritWeaponSource(true, false, false, false, false, false, false) ==
	CastOnCritWeaponSource::kHitDataWeapon);
static_assert(ResolveCastOnCritWeaponSource(true, true, true, true, false, false, true) ==
	CastOnCritWeaponSource::kHitDataWeapon,
	"an authoritative direct weapon preserves the existing melee/ranged path");

static_assert(ResolveCastOnCritWeaponSource(false, false, true, true, true, false, false) ==
	CastOnCritWeaponSource::kProjectileWeapon,
	"a projectile carrying its own bow/crossbow source is a ranged weapon hit");
static_assert(ResolveCastOnCritWeaponSource(false, true, true, true, true, true, true) ==
	CastOnCritWeaponSource::kNone,
	"spell HitData must not recurse even if other ranged evidence is present");
static_assert(ResolveCastOnCritWeaponSource(false, false, true, true, false, true, true) ==
	CastOnCritWeaponSource::kNone,
	"a spell projectile without a weapon source must not borrow the active bow");
static_assert(ResolveCastOnCritWeaponSource(false, false, true, false, false, true, true) ==
	CastOnCritWeaponSource::kNone,
	"an unrecognized or stale source reference fails closed");

static_assert(ResolveCastOnCritWeaponSource(false, false, false, false, false, true, true) ==
	CastOnCritWeaponSource::kActiveRangedAttack,
	"missing source references may use a current ranged attack with matching attack data");
static_assert(ResolveCastOnCritWeaponSource(false, false, false, false, false, false, true) ==
	CastOnCritWeaponSource::kNone,
	"a merely held ranged weapon is insufficient without hit attack data");
static_assert(ResolveCastOnCritWeaponSource(false, false, false, false, false, true, false) ==
	CastOnCritWeaponSource::kNone,
	"the null-weapon fallback is ranged-only");

static_assert(ResolveSpecialActionProcChancePct(-1.0f) == 0.0f);
static_assert(ResolveSpecialActionProcChancePct(0.0f) == 0.0f);
static_assert(ResolveSpecialActionProcChancePct(37.5f) == 37.5f);
static_assert(ResolveSpecialActionProcChancePct(100.0f) == 100.0f);
static_assert(ResolveSpecialActionProcChancePct(125.0f) == 100.0f);

static_assert(IsEligibleNormalWeaponHitFlags(false, false, false, false, false, false));
static_assert(!IsEligibleNormalWeaponHitFlags(true, false, false, false, false, false),
	"critical hits retain their existing qualifying-hit path");
static_assert(!IsEligibleNormalWeaponHitFlags(false, true, false, false, false, false),
	"power attacks retain their existing qualifying-hit path");
static_assert(!IsEligibleNormalWeaponHitFlags(false, false, true, false, false, false),
	"spell HitData must not be promoted to a normal weapon hit");
static_assert(!IsEligibleNormalWeaponHitFlags(false, false, false, true, false, false),
	"bashes must not be promoted to normal weapon hits");
static_assert(!IsEligibleNormalWeaponHitFlags(false, false, false, false, true, false),
	"timed bashes must not be promoted to normal weapon hits");
static_assert(!IsEligibleNormalWeaponHitFlags(false, false, false, false, false, true),
	"explosions must not be promoted to normal weapon hits");

static_assert(ResolveCastOnCritSelectionCount(0u, false, false) == 0u);
static_assert(ResolveCastOnCritSelectionCount(1u, false, false) == 1u);
static_assert(ResolveCastOnCritSelectionCount(2u, false, false) == 2u);
static_assert(ResolveCastOnCritSelectionCount(7u, false, false) == 2u,
	"qualifying melee hits select at most two successful candidates");
static_assert(ResolveCastOnCritSelectionCount(7u, true, false) == 1u,
	"ranged hits retain their existing one-result limit");
static_assert(ResolveCastOnCritSelectionCount(7u, false, true) == 1u,
	"normal melee hits roll only one selected candidate");

static_assert(ResolveCyclicCandidateIndex(0u, 99u, 99u) == 0u);
static_assert(ResolveCyclicCandidateIndex(3u, 0u, 0u) == 0u);
static_assert(ResolveCyclicCandidateIndex(3u, 0u, 1u) == 1u);
static_assert(ResolveCyclicCandidateIndex(3u, 2u, 0u) == 2u);
static_assert(ResolveCyclicCandidateIndex(3u, 2u, 1u) == 0u);
static_assert(ResolveCyclicCandidateIndex(3u, 4u, 0u) == 1u);
static_assert(ResolveCyclicCandidateIndex(3u, 4u, 1u) == 2u,
	"advancing the cursor by two rotates a capped pair fairly across three candidates");

static_assert(IsCalamityProcSource("CAFF_SPEL_PROC_FIRE"));
static_assert(IsCalamityProcSource("CAFF_"));
static_assert(!IsCalamityProcSource("caFF_SPEL_PROC_FIRE"));
static_assert(!IsCalamityProcSource("OTHER_SPEL_PROC_FIRE"));
static_assert(!IsCalamityProcSource(""));
