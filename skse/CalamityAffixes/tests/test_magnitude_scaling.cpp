#include "CalamityAffixes/MagnitudeScaling.h"

namespace
{
	constexpr bool FloatEq(float a, float b, float eps = 0.0001f)
	{
		const float d = (a > b) ? (a - b) : (b - a);
		return d <= eps;
	}
}

using CalamityAffixes::MagnitudeScaling;
using CalamityAffixes::ResolveMagnitudeOverride;

// 1) spellBaseAsMin=true keeps baseline when hit damage is low.
static_assert(FloatEq(
				  ResolveMagnitudeOverride(
					  /*fallbackMagnitudeOverride=*/0.0f,
					  /*spellBaseMagnitude=*/25.0f,
					  /*hitPhysicalDealt=*/10.0f,
					  /*hitTotalDealt=*/10.0f,
					  MagnitudeScaling{
						  .source = MagnitudeScaling::Source::kHitPhysicalDealt,
						  .mult = 0.3f,
						  .add = 0.0f,
						  .min = 0.0f,
						  .max = 0.0f,
						  .spellBaseAsMin = true,
					  }),
				  25.0f),
	"spellBaseAsMin");

// 2) scaling uses HitPhysicalDealt.
static_assert(FloatEq(
				  ResolveMagnitudeOverride(
					  /*fallbackMagnitudeOverride=*/0.0f,
					  /*spellBaseMagnitude=*/0.0f,
					  /*hitPhysicalDealt=*/100.0f,
					  /*hitTotalDealt=*/999.0f,
					  MagnitudeScaling{
						  .source = MagnitudeScaling::Source::kHitPhysicalDealt,
						  .mult = 0.3f,
						  .add = 0.0f,
						  .min = 0.0f,
						  .max = 0.0f,
						  .spellBaseAsMin = false,
					  }),
				  30.0f),
	"hitPhysicalDealt");

// 3) clamps apply.
static_assert(FloatEq(
				  ResolveMagnitudeOverride(
					  /*fallbackMagnitudeOverride=*/0.0f,
					  /*spellBaseMagnitude=*/0.0f,
					  /*hitPhysicalDealt=*/1000.0f,
					  /*hitTotalDealt=*/0.0f,
					  MagnitudeScaling{
						  .source = MagnitudeScaling::Source::kHitPhysicalDealt,
						  .mult = 1.0f,
						  .add = 0.0f,
						  .min = 50.0f,
						  .max = 60.0f,
						  .spellBaseAsMin = false,
					  }),
				  60.0f),
	"clampMax");
