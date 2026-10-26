#include "CalamityAffixes/TesHitFallbackPolicy.h"

namespace
{
	using CalamityAffixes::detail::IsCommittedFallbackHitData;

	// Only the fully committed case may drive a proc.
	static_assert(IsCommittedFallbackHitData(true, true, true));

	// No HitData at all: the engine has not attached anything yet.
	static_assert(!IsCommittedFallbackHitData(false, false, false));
	static_assert(!IsCommittedFallbackHitData(false, true, true));

	// Actor mismatch is the stale-data case -- HitData left over from a
	// different exchange would attribute this hit to the wrong actor.
	static_assert(!IsCommittedFallbackHitData(true, false, true));

	// No hit-like source (no weapon, no attack spell) means nothing actually
	// landed, so a proc here would fire on a phantom hit.
	static_assert(!IsCommittedFallbackHitData(true, true, false));

	// Neither partial signal is sufficient on its own.
	static_assert(!IsCommittedFallbackHitData(true, false, false));

	using CalamityAffixes::detail::IsReusedFallbackHitContent;

	// The swing that was already processed, read back by a later spell event.
	static_assert(IsReusedFallbackHitContent(0x1234u, true, 0x1234u));
	// A new swing rewrites lastHitData, so its content differs.
	static_assert(!IsReusedFallbackHitContent(0x5678u, true, 0x1234u));
	// The first hit of a pair has nothing stored yet.
	static_assert(!IsReusedFallbackHitContent(0x1234u, false, 0u));
	// No HitData never counts as reused.
	static_assert(!IsReusedFallbackHitContent(0u, true, 0u));
}
