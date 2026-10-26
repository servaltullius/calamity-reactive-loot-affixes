#pragma once

#include <algorithm>

namespace CalamityAffixes::detail
{
	// Low-health triggers (Lionheart, Last Wish, Banish) fire once when the
	// owner drops to the threshold and rearm once health is back at the rearm
	// line. Rearm is pure state: it runs on every low-health observation before
	// the cooldown, chance or budget gates, so a heal during the ICD (or with no
	// damage taken at all) still rearms.
	struct LowHealthBands
	{
		float thresholdPct{ 30.0f };
		float rearmPct{ 40.0f };
	};

	[[nodiscard]] constexpr LowHealthBands ResolveLowHealthBands(float a_thresholdPct, float a_rearmPct) noexcept
	{
		const float threshold = std::clamp(a_thresholdPct, 1.0f, 95.0f);
		return LowHealthBands{
			.thresholdPct = threshold,
			.rearmPct = std::clamp(a_rearmPct, threshold + 1.0f, 100.0f),
		};
	}

	// Health just before this damage: current + this damage. The last stored
	// snapshot can be a minute old (it is only taken when something reported
	// damage), so it is used only when the damage amount is unknown.
	[[nodiscard]] constexpr float ResolveLowHealthPreHitPct(
		float a_currentPct,
		float a_damagePct,
		float a_snapshotPct) noexcept
	{
		if (a_damagePct > 0.0f) {
			return std::clamp(a_currentPct + a_damagePct, 0.0f, 100.0f);
		}
		return std::clamp(a_snapshotPct, 0.0f, 100.0f);
	}

	[[nodiscard]] constexpr bool ShouldRearmLowHealthTrigger(
		float a_preHitPct,
		float a_currentPct,
		const LowHealthBands& a_bands) noexcept
	{
		// Health now at the rearm line rearms, and so does health just before
		// this damage: a pre-hit value at or above the line proves the owner
		// was back above it since the last fire (for example 90% -> 35%, or
		// one big hit from above the line straight to the threshold). When
		// the damage amount is unknown the pre-hit value is the last stored
		// snapshot (ResolveLowHealthPreHitPct).
		const float current = std::clamp(a_currentPct, 0.0f, 100.0f);
		const float preHit = std::clamp(a_preHitPct, 0.0f, 100.0f);
		return current >= a_bands.rearmPct || preHit >= a_bands.rearmPct;
	}

	[[nodiscard]] constexpr bool PassesLowHealthFireGate(
		float a_currentPct,
		bool a_consumed,
		const LowHealthBands& a_bands) noexcept
	{
		return std::clamp(a_currentPct, 0.0f, 100.0f) <= a_bands.thresholdPct && !a_consumed;
	}
}
