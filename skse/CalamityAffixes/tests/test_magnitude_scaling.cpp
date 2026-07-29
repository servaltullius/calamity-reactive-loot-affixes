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
using CalamityAffixes::DirectElementalDamageMagnitudeSelector;
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

// 9) Crit Cast uses the direct shock damage, not the larger Disintegrate magnitude.
static_assert([] {
	DirectElementalDamageMagnitudeSelector selector;
	selector.Consider(200.0f, true, true, true, 1u);
	selector.Consider(25.0f, true, true, true, 0u);
	return FloatEq(selector.Resolve(), 25.0f);
}());

// 10) Slow/Fear-style utility effects do not become the spell damage baseline.
static_assert([] {
	DirectElementalDamageMagnitudeSelector selector;
	selector.Consider(50.0f, true, false, true, 3u);
	selector.Consider(99.0f, false, false, false, 15u);
	selector.Consider(40.0f, true, true, true, 0u);
	return FloatEq(selector.Resolve(), 40.0f);
}());

// 11) No matching direct elemental damage effect produces no spell-base floor.
static_assert([] {
	DirectElementalDamageMagnitudeSelector selector;
	selector.Consider(200.0f, false, true, true, 1u);
	selector.Consider(50.0f, true, false, true, 3u);
	return FloatEq(selector.Resolve(), 0.0f);
}());

// 12) Invalid magnitudes are ignored.
static_assert([] {
	DirectElementalDamageMagnitudeSelector selector;
	selector.Consider(-25.0f, true, true, true, 0u);
	selector.Consider(std::numeric_limits<float>::infinity(), true, true, true, 0u);
	selector.Consider(std::numeric_limits<float>::quiet_NaN(), true, true, true, 0u);
	return FloatEq(selector.Resolve(), 0.0f);
}());
