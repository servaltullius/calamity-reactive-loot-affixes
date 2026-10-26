#include "CalamityAffixes/BowDrawSpeed.h"

namespace
{
	using CalamityAffixes::ResolveBowDrawSpeedDelta;

	constexpr bool Near(float a_value, float a_expected)
	{
		const float diff = a_value - a_expected;
		return diff < 0.0001f && diff > -0.0001f;
	}

	constexpr bool CheckUnsetValueReadsAsOne()
	{
		// No Quick Shot: the value is 0, which the engine reads as 1.
		return Near(ResolveBowDrawSpeedDelta(0.0f, 0.15f, true), 1.15f);
	}

	constexpr bool CheckScalesAnExistingValue()
	{
		// Quick Shot's 1.3 becomes 1.3 * 1.15.
		return Near(0.0f + 1.3f + ResolveBowDrawSpeedDelta(1.3f, 0.15f, true), 1.495f);
	}

	constexpr bool CheckNothingWithoutABowOrBonus()
	{
		return ResolveBowDrawSpeedDelta(1.3f, 0.15f, false) == 0.0f && ResolveBowDrawSpeedDelta(0.0f, 0.0f, true) == 0.0f &&
		       ResolveBowDrawSpeedDelta(0.0f, -0.1f, true) == 0.0f;
	}

	constexpr bool CheckNegativeValueReadsAsOne()
	{
		// A slowed actor below 0 still ends at the bonus over 1, never slower.
		return Near(-0.2f + ResolveBowDrawSpeedDelta(-0.2f, 0.3f, true), 1.3f);
	}
}

static_assert(CheckUnsetValueReadsAsOne(), "an unset WeaponSpeedMult is read as 1, so the bonus lands on 1");
static_assert(CheckScalesAnExistingValue(), "an existing WeaponSpeedMult is scaled by the bonus");
static_assert(CheckNothingWithoutABowOrBonus(), "no bow or no bonus adds nothing");
static_assert(CheckNegativeValueReadsAsOne(), "a value below zero is treated as unset");
