#pragma once

namespace CalamityAffixes::detail
{
	[[nodiscard]] constexpr float ResolveAffixKidLootWeight(
		float a_currentLootWeight,
		float a_kidChancePct) noexcept
	{
		return a_kidChancePct > 0.0f ? a_kidChancePct : a_currentLootWeight;
	}

	[[nodiscard]] constexpr float ResolveSuffixProcChancePct() noexcept
	{
		return 0.0f;
	}
}
