#include "CalamityAffixes/TrapCellPolicy.h"
#include "CalamityAffixes/TrapMarkerAnimationPolicy.h"

using CalamityAffixes::detail::IsTrapCellUsable;
using CalamityAffixes::detail::CanSpawnPlacedTrapMarker;
using CalamityAffixes::detail::ResolvePlacedTrapMarkerCleanupPolicy;
using CalamityAffixes::detail::ShouldDeferUnresolvedPlacedTrapMarker;
using CalamityAffixes::detail::ShouldReusePlacedTrapMarker;
using CalamityAffixes::detail::kMaxPlacedTrapMarkers;
using CalamityAffixes::detail::IsTrapMarkerAnimationGraphReady;
using CalamityAffixes::detail::ShouldRetryTrapMarkerAnimation;
using CalamityAffixes::detail::ExtendTrapArmedAtForAcceptedOpenAnimation;
using CalamityAffixes::detail::ShouldStartTrapMarkerRearmAnimation;
using CalamityAffixes::detail::ShouldBlockTrapCastForMarkerAnimation;
using CalamityAffixes::detail::kMaxTrapMarkerAnimationAttempts;

static_assert(!IsTrapCellUsable(false, false),
	"Trap cells must exist before runtime effects can use them");
static_assert(!IsTrapCellUsable(false, true),
	"An attached state without a cell cannot make a trap usable");
static_assert(!IsTrapCellUsable(true, false),
	"Detached cells must not retain active traps or receive runtime effects");
static_assert(IsTrapCellUsable(true, true),
	"Attached cells should keep their traps active");

static_assert(CanSpawnPlacedTrapMarker(kMaxPlacedTrapMarkers - 1u),
	"The final slot in the placed-reference marker budget must remain usable");
static_assert(!CanSpawnPlacedTrapMarker(kMaxPlacedTrapMarkers),
	"Placed-reference markers must stop at the hard safety cap");
static_assert(CanSpawnPlacedTrapMarker(kMaxPlacedTrapMarkers - 2u, 1u),
	"Pending cleanup handles and live markers must share the fixed budget");
static_assert(!CanSpawnPlacedTrapMarker(kMaxPlacedTrapMarkers - 1u, 1u),
	"A deferred cleanup handle must consume the final placed-reference slot");
static_assert(ShouldDeferUnresolvedPlacedTrapMarker(true, false),
	"An allocated but unresolved handle must enter fail-closed cleanup ownership");
static_assert(!ShouldDeferUnresolvedPlacedTrapMarker(false, false),
	"An empty handle has no deferred reference ownership to retain");
static_assert(!ShouldDeferUnresolvedPlacedTrapMarker(true, true),
	"A resolved reference should stay under strong TrapInstance ownership");
static_assert(ShouldReusePlacedTrapMarker(true, false, false),
	"A live placed marker should survive trap phase transitions");
static_assert(!ShouldReusePlacedTrapMarker(false, false, false),
	"An unresolved handle cannot prove that a placed marker is reusable");
static_assert(!ShouldReusePlacedTrapMarker(true, true, false),
	"Disabled placed markers must not be reused");
static_assert(!ShouldReusePlacedTrapMarker(true, false, true),
	"References pending deletion must not be reused");

static_assert(!IsTrapMarkerAnimationGraphReady(false, true, true),
	"An animation manager alone cannot animate a marker without loaded 3D");
static_assert(!IsTrapMarkerAnimationGraphReady(true, false, true),
	"A loaded marker still needs an animation graph manager");
static_assert(!IsTrapMarkerAnimationGraphReady(true, true, false),
	"A graph manager without a loaded graph is not ready for events");
static_assert(IsTrapMarkerAnimationGraphReady(true, true, true),
	"A loaded marker with a populated graph manager is ready for events");
static_assert(ShouldRetryTrapMarkerAnimation(true, false, kMaxTrapMarkerAnimationAttempts - 1u),
	"The final bounded animation attempt must remain reachable");
static_assert(!ShouldRetryTrapMarkerAnimation(true, false, kMaxTrapMarkerAnimationAttempts),
	"Animation retries must stop at the hard attempt cap");
static_assert(!ShouldRetryTrapMarkerAnimation(false, false, 1u),
	"An unusable placed reference must not be retried");
static_assert(!ShouldRetryTrapMarkerAnimation(true, true, 1u),
	"An accepted graph event must not be delivered twice");

constexpr auto kAnimationEpoch = std::chrono::steady_clock::time_point{};
static_assert(
	ExtendTrapArmedAtForAcceptedOpenAnimation(
		kAnimationEpoch + std::chrono::milliseconds(1000),
		kAnimationEpoch + std::chrono::milliseconds(800),
		std::chrono::milliseconds(900)) == kAnimationEpoch + std::chrono::milliseconds(1700),
	"The rearm gate must start when Reset01 is accepted, including after a late retry");
static_assert(
	ExtendTrapArmedAtForAcceptedOpenAnimation(
		kAnimationEpoch + std::chrono::milliseconds(2000),
		kAnimationEpoch + std::chrono::milliseconds(800),
		std::chrono::milliseconds(900)) == kAnimationEpoch + std::chrono::milliseconds(2000),
	"An accepted rearm event must never shorten an existing gameplay delay");
static_assert(!ShouldStartTrapMarkerRearmAnimation(
		kAnimationEpoch + std::chrono::milliseconds(1000),
		kAnimationEpoch + std::chrono::milliseconds(2000),
		std::chrono::milliseconds(900)),
	"Reset01 must not start before its open-animation pre-roll window");
static_assert(ShouldStartTrapMarkerRearmAnimation(
		kAnimationEpoch + std::chrono::milliseconds(1100),
		kAnimationEpoch + std::chrono::milliseconds(2000),
		std::chrono::milliseconds(900)),
	"Reset01 should pre-roll so its gate can finish at the original rearm deadline");
static_assert(ShouldBlockTrapCastForMarkerAnimation(
		kAnimationEpoch + std::chrono::milliseconds(1000),
		kAnimationEpoch + std::chrono::milliseconds(1000),
		true),
	"A pending Reset01 retry must block casts even after the gameplay timer expires");
static_assert(!ShouldBlockTrapCastForMarkerAnimation(
		kAnimationEpoch + std::chrono::milliseconds(1000),
		kAnimationEpoch + std::chrono::milliseconds(1000),
		false),
	"Exhausted Reset01 retries must fail open once the gameplay timer expires");

constexpr auto kAttachedMarkerCleanup = ResolvePlacedTrapMarkerCleanupPolicy(true, true, true);
static_assert(kAttachedMarkerCleanup.disable && kAttachedMarkerCleanup.markForDeletion,
	"Attached markers must disappear immediately and retire their reference");
constexpr auto kDetachedMarkerCleanup = ResolvePlacedTrapMarkerCleanupPolicy(true, true, false);
static_assert(!kDetachedMarkerCleanup.disable && kDetachedMarkerCleanup.markForDeletion,
	"Detached cells must skip 3D mutation while still retiring the reference");
constexpr auto kCelllessMarkerCleanup = ResolvePlacedTrapMarkerCleanupPolicy(true, false, false);
static_assert(kCelllessMarkerCleanup.disable && kCelllessMarkerCleanup.markForDeletion,
	"A resolved cellless marker remains safe to disable before deletion");
constexpr auto kUnresolvedMarkerCleanup = ResolvePlacedTrapMarkerCleanupPolicy(false, false, false);
static_assert(!kUnresolvedMarkerCleanup.disable && !kUnresolvedMarkerCleanup.markForDeletion,
	"An unresolved handle must not issue reference calls");
