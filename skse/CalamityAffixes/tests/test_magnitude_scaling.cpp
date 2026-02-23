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

// 4) kNone source returns fallback override regardless of inputs.
static_assert(FloatEq(
				  ResolveMagnitudeOverride(
					  /*fallbackMagnitudeOverride=*/42.0f,
					  /*spellBaseMagnitude=*/100.0f,
					  /*hitPhysicalDealt=*/9999.0f,
					  /*hitTotalDealt=*/9999.0f,
					  MagnitudeScaling{
						  .source = MagnitudeScaling::Source::kNone,
						  .mult = 5.0f,
						  .add = 10.0f,
						  .min = 0.0f,
						  .max = 0.0f,
						  .spellBaseAsMin = true,
					  }),
				  42.0f),
	"kNone_returns_fallback");

// 5) Negative hit damage is clamped to zero before scaling.
static_assert(FloatEq(
				  ResolveMagnitudeOverride(
					  /*fallbackMagnitudeOverride=*/0.0f,
					  /*spellBaseMagnitude=*/0.0f,
					  /*hitPhysicalDealt=*/-50.0f,
					  /*hitTotalDealt=*/-100.0f,
					  MagnitudeScaling{
						  .source = MagnitudeScaling::Source::kHitPhysicalDealt,
						  .mult = 1.0f,
						  .add = 0.0f,
						  .min = 0.0f,
						  .max = 0.0f,
						  .spellBaseAsMin = false,
					  }),
				  0.0f),
	"negative_hit_clamped_to_zero");

// 6) HitTotalDealt source path.
static_assert(FloatEq(
				  ResolveMagnitudeOverride(
					  /*fallbackMagnitudeOverride=*/0.0f,
					  /*spellBaseMagnitude=*/0.0f,
					  /*hitPhysicalDealt=*/50.0f,
					  /*hitTotalDealt=*/200.0f,
					  MagnitudeScaling{
						  .source = MagnitudeScaling::Source::kHitTotalDealt,
						  .mult = 0.5f,
						  .add = 10.0f,
						  .min = 0.0f,
						  .max = 0.0f,
						  .spellBaseAsMin = false,
					  }),
				  110.0f),
	"hitTotalDealt_path");

// 7) min clamp alone (max=0 disables upper clamp).
static_assert(FloatEq(
				  ResolveMagnitudeOverride(
					  /*fallbackMagnitudeOverride=*/0.0f,
					  /*spellBaseMagnitude=*/0.0f,
					  /*hitPhysicalDealt=*/5.0f,
					  /*hitTotalDealt=*/0.0f,
					  MagnitudeScaling{
						  .source = MagnitudeScaling::Source::kHitPhysicalDealt,
						  .mult = 1.0f,
						  .add = 0.0f,
						  .min = 20.0f,
						  .max = 0.0f,
						  .spellBaseAsMin = false,
					  }),
				  20.0f),
	"clampMin_only");

// 8) add component contributes to output.
static_assert(FloatEq(
				  ResolveMagnitudeOverride(
					  /*fallbackMagnitudeOverride=*/0.0f,
					  /*spellBaseMagnitude=*/0.0f,
					  /*hitPhysicalDealt=*/0.0f,
					  /*hitTotalDealt=*/0.0f,
					  MagnitudeScaling{
						  .source = MagnitudeScaling::Source::kHitPhysicalDealt,
						  .mult = 0.0f,
						  .add = 15.0f,
						  .min = 0.0f,
						  .max = 0.0f,
						  .spellBaseAsMin = false,
					  }),
				  15.0f),
	"add_only");
