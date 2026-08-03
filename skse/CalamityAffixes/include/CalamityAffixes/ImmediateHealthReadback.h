#pragma once

#include <cmath>
#include <optional>

namespace CalamityAffixes
{
	struct ImmediateHealthReadbackResult
	{
		std::optional<float> healthBefore{};
		std::optional<float> healthAfter{};
		std::optional<float> healthChange{};
	};

	template <class ReadHealth, class CastSpell>
	[[nodiscard]] ImmediateHealthReadbackResult ObserveImmediateHealthChange(
		bool a_observe,
		ReadHealth&& a_readHealth,
		CastSpell&& a_castSpell)
	{
		ImmediateHealthReadbackResult result{};
		if (!a_observe) {
			a_castSpell();
			return result;
		}

		result.healthBefore = a_readHealth();
		a_castSpell();
		result.healthAfter = a_readHealth();

		if (result.healthBefore && !std::isfinite(*result.healthBefore)) {
			result.healthBefore.reset();
		}
		if (result.healthAfter && !std::isfinite(*result.healthAfter)) {
			result.healthAfter.reset();
		}
		if (result.healthBefore && result.healthAfter) {
			result.healthChange = *result.healthAfter - *result.healthBefore;
		}

		return result;
	}
}
