#pragma once

namespace CalamityAffixes
{
	// Bow draw speed (v2.3.0). Eagle Eye suffixes and Zephyr carry their bonus on
	// a no-op effect; the runtime turns it into WeaponSpeedMult while a bow or
	// crossbow is drawn. The engine reads a WeaponSpeedMult of 0 as 1 (vanilla
	// Quick Shot sets 1.3, not +0.3), so a plain +0.05 modifier would make the
	// bow 20 times slower: the runtime scales whatever the value is without it.

	// The change to add to WeaponSpeedMult. a_valueWithoutOurs is the current
	// value minus what the runtime added before; a_bonus is a fraction (+15% is
	// 0.15).
	[[nodiscard]] constexpr float ResolveBowDrawSpeedDelta(float a_valueWithoutOurs, float a_bonus, bool a_bowDrawn) noexcept
	{
		if (!a_bowDrawn || a_bonus <= 0.0f) {
			return 0.0f;
		}
		const float effective = a_valueWithoutOurs > 0.0f ? a_valueWithoutOurs : 1.0f;
		return effective * (1.0f + a_bonus) - a_valueWithoutOurs;
	}

	// Below this the applied change is left alone (float drift between ticks).
	inline constexpr float kBowDrawSpeedEpsilon = 0.001f;
}
