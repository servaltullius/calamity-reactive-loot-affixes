#pragma once

namespace CalamityAffixes::detail
{
	// Whether the HitData attached to a target may be trusted to drive procs on
	// the TESHitEvent fallback path.
	//
	// Some weapons -- especially modded ones with custom attack animations --
	// raise TESHitEvent before the engine has committed HitData to the target.
	// The first event then carries stale or actor-mismatched data and a second
	// event follows with the real thing.  Proccing off the first one attributes
	// damage to the wrong actor or to a source that never hit anything, so the
	// fallback waits until all three facts hold.
	//
	// Callers that get `false` must also reset their duplicate-hit tracking, or
	// the follow-up event carrying the committed data gets suppressed as a
	// duplicate and the proc is lost entirely.
	[[nodiscard]] constexpr bool IsCommittedFallbackHitData(
		bool a_hasHitData,
		bool a_matchesActors,
		bool a_hasHitLikeSource) noexcept
	{
		return a_hasHitData && a_matchesActors && a_hasHitLikeSource;
	}
}
