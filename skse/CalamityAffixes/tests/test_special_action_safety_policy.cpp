#include "CalamityAffixes/SpecialActionSafetyPolicy.h"

using CalamityAffixes::detail::IsCalamityProcSource;
using CalamityAffixes::detail::CastOnCritWeaponSource;
using CalamityAffixes::detail::ResolveCastOnCritWeaponSource;
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

static_assert(IsCalamityProcSource("CAFF_SPEL_PROC_FIRE"));
static_assert(IsCalamityProcSource("CAFF_"));
static_assert(!IsCalamityProcSource("caFF_SPEL_PROC_FIRE"));
static_assert(!IsCalamityProcSource("OTHER_SPEL_PROC_FIRE"));
static_assert(!IsCalamityProcSource(""));
