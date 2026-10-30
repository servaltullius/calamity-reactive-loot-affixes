#pragma once

namespace CalamityAffixes
{
	// The maximum a health or magicka bar shows: base plus permanent modifier
	// (the permanent actor value) plus the temporary modifier, where Fortify
	// Health/Magicka effects (enchantments, potions, abilities) add theirs. A
	// non-positive sum falls back to the permanent value.
	[[nodiscard]] constexpr float CombineActorValueMax(float a_permanentValue, float a_temporaryModifier) noexcept
	{
		const float permanent = a_permanentValue > 0.0f ? a_permanentValue : 0.0f;
		const float combined = permanent + a_temporaryModifier;
		return combined > 0.0f ? combined : permanent;
	}
}
