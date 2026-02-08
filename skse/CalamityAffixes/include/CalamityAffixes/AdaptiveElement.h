#pragma once

#include <cstdint>

namespace CalamityAffixes
{
	enum class AdaptiveElement : std::uint8_t
	{
		kFire,
		kFrost,
		kShock,
	};

	enum class AdaptiveElementMode : std::uint8_t
	{
		kWeakestResist,
		kStrongestResist,
	};

	// Picks an element based on the target's current resistances.
	// Tie-breaking is stable and deterministic: Fire > Frost > Shock.
	[[nodiscard]] constexpr AdaptiveElement PickAdaptiveElement(
		float a_resistFire,
		float a_resistFrost,
		float a_resistShock,
		AdaptiveElementMode a_mode) noexcept
	{
		if (a_mode == AdaptiveElementMode::kStrongestResist) {
			if (a_resistFire >= a_resistFrost && a_resistFire >= a_resistShock) {
				return AdaptiveElement::kFire;
			}
			if (a_resistFrost >= a_resistShock) {
				return AdaptiveElement::kFrost;
			}
			return AdaptiveElement::kShock;
		}

		// WeakestResist (default)
		if (a_resistFire <= a_resistFrost && a_resistFire <= a_resistShock) {
			return AdaptiveElement::kFire;
		}
		if (a_resistFrost <= a_resistShock) {
			return AdaptiveElement::kFrost;
		}
		return AdaptiveElement::kShock;
	}
}

