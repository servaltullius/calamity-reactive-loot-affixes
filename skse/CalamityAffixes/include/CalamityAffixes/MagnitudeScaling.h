#pragma once

#include <algorithm>
#include <cstdint>

namespace CalamityAffixes
{
	struct MagnitudeScaling
	{
		enum class Source : std::uint8_t
		{
			kNone,
			kHitPhysicalDealt,
			kHitTotalDealt,
		};

		Source source{ Source::kNone };
		float mult{ 0.0f };
		float add{ 0.0f };
		float min{ 0.0f };
		float max{ 0.0f };
		bool spellBaseAsMin{ false };
	};

	[[nodiscard]] constexpr float ResolveMagnitudeOverride(
		float a_fallbackMagnitudeOverride,
		float a_spellBaseMagnitude,
		float a_hitPhysicalDealt,
		float a_hitTotalDealt,
		const MagnitudeScaling& a_scaling) noexcept
	{
		if (a_scaling.source == MagnitudeScaling::Source::kNone) {
			return a_fallbackMagnitudeOverride;
		}

		const float physicalDealt = std::max(0.0f, a_hitPhysicalDealt);
		const float totalDealt = std::max(0.0f, a_hitTotalDealt);

		float sourceValue = 0.0f;
		switch (a_scaling.source) {
		case MagnitudeScaling::Source::kHitPhysicalDealt:
			sourceValue = physicalDealt;
			break;
		case MagnitudeScaling::Source::kHitTotalDealt:
			sourceValue = totalDealt;
			break;
		default:
			return a_fallbackMagnitudeOverride;
		}

		float out = a_scaling.add + (a_scaling.mult * sourceValue);
		if (a_scaling.spellBaseAsMin) {
			out = std::max(out, a_spellBaseMagnitude);
		}
		if (a_scaling.min > 0.0f) {
			out = std::max(out, a_scaling.min);
		}
		if (a_scaling.max > 0.0f) {
			out = std::min(out, a_scaling.max);
		}
		return out;
	}
}
