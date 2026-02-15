#include "CalamityAffixes/NonHostileFirstHitGate.h"

using CalamityAffixes::NonHostileFirstHitGate;

static_assert(NonHostileFirstHitGate::IsValidKey(0x14u, 0x1234u));
static_assert(!NonHostileFirstHitGate::IsValidKey(0u, 0x1234u));
static_assert(!NonHostileFirstHitGate::IsValidKey(0x14u, 0u));

static_assert(NonHostileFirstHitGate::ShouldTrackNonHostileHit(true, false, false));
static_assert(!NonHostileFirstHitGate::ShouldTrackNonHostileHit(false, false, false));
static_assert(!NonHostileFirstHitGate::ShouldTrackNonHostileHit(true, true, false));
static_assert(!NonHostileFirstHitGate::ShouldTrackNonHostileHit(true, false, true));

static_assert(
	NonHostileFirstHitGate::IsPruneDue(std::chrono::seconds(11), std::chrono::seconds(10)));
static_assert(
	!NonHostileFirstHitGate::IsPruneDue(std::chrono::seconds(10), std::chrono::seconds(10)));

static_assert(
	NonHostileFirstHitGate::IsReentryAllowed(std::chrono::milliseconds(100), std::chrono::milliseconds(100)));
static_assert(
	!NonHostileFirstHitGate::IsReentryAllowed(std::chrono::milliseconds(101), std::chrono::milliseconds(100)));
