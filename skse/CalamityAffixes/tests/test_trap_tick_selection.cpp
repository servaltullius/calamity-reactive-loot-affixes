#include "CalamityAffixes/TrapTickSelection.h"

namespace
{
	using namespace CalamityAffixes::detail;

	// Distance is squared, so the radius test never needs a sqrt.
	static_assert(TrapTargetDistanceSq(3.0f, 4.0f, 0.0f) == 25.0f);
	static_assert(TrapTargetDistanceSq(0.0f, 0.0f, 0.0f) == 0.0f);
	// Sign must not survive the square.
	static_assert(TrapTargetDistanceSq(-3.0f, -4.0f, 0.0f) == 25.0f);
	// All three axes contribute; a trap must not ignore vertical separation.
	static_assert(TrapTargetDistanceSq(0.0f, 0.0f, 5.0f) == 25.0f);

	// Boundary is inclusive: standing exactly on the radius still triggers.
	static_assert(IsWithinTrapRadiusSq(25.0f, 25.0f));
	static_assert(IsWithinTrapRadiusSq(24.9f, 25.0f));
	static_assert(!IsWithinTrapRadiusSq(25.1f, 25.0f));

	// Owner-dependent filter: owner, corpses, and non-hostiles are all skipped.
	static_assert(IsTrapTickTargetEligible(false, false, true));
	static_assert(!IsTrapTickTargetEligible(true, false, true));   // the trap's own owner
	static_assert(!IsTrapTickTargetEligible(false, true, true));   // already dead
	static_assert(!IsTrapTickTargetEligible(false, false, false)); // not hostile
	static_assert(!IsTrapTickTargetEligible(true, true, false));

	// Budget 0 means unmetered, not "no casts allowed".
	static_assert(HasTrapCastBudget(0u, 0u));
	static_assert(HasTrapCastBudget(9999u, 0u));
	// A metered budget admits casts up to and including the cap.
	static_assert(HasTrapCastBudget(0u, 4u));
	static_assert(HasTrapCastBudget(3u, 4u));
	static_assert(!HasTrapCastBudget(4u, 4u));
	// Multi-cast steps must not be admitted when only part of them fits.
	static_assert(HasTrapCastBudget(2u, 4u, 2u));
	static_assert(!HasTrapCastBudget(3u, 4u, 2u));
}
