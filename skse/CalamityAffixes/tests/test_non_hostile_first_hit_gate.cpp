#include "CalamityAffixes/NonHostileFirstHitGate.h"

using CalamityAffixes::detail::IsNonHostileFirstHitPruneDue;
using CalamityAffixes::detail::IsNonHostileFirstHitReentryAllowed;
using CalamityAffixes::detail::IsValidNonHostileFirstHitKey;
using CalamityAffixes::detail::ShouldTrackNonHostileFirstHit;

static_assert(IsValidNonHostileFirstHitKey(0x14u, 0x1234u));
static_assert(!IsValidNonHostileFirstHitKey(0u, 0x1234u));
static_assert(!IsValidNonHostileFirstHitKey(0x14u, 0u));

static_assert(ShouldTrackNonHostileFirstHit(true, false, false));
static_assert(!ShouldTrackNonHostileFirstHit(false, false, false));
static_assert(!ShouldTrackNonHostileFirstHit(true, true, false));
static_assert(!ShouldTrackNonHostileFirstHit(true, false, true));

static_assert(
	IsNonHostileFirstHitPruneDue(std::chrono::seconds(11), std::chrono::seconds(10)));
static_assert(
	!IsNonHostileFirstHitPruneDue(std::chrono::seconds(10), std::chrono::seconds(10)));

static_assert(
	IsNonHostileFirstHitReentryAllowed(std::chrono::milliseconds(100), std::chrono::milliseconds(100)));
static_assert(
	!IsNonHostileFirstHitReentryAllowed(std::chrono::milliseconds(101), std::chrono::milliseconds(100)));
