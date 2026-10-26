#include "CalamityAffixes/LowHealthRearmPolicy.h"

using namespace CalamityAffixes::detail;

namespace
{
	constexpr LowHealthBands kLionheart = ResolveLowHealthBands(30.0f, 40.0f);

	// Lionheart fired at 25%, the player healed to 90% and took no damage
	// during the 45 s ICD. The first hit after the ICD lands 90% -> 25%.
	constexpr bool CheckHealDuringCooldownRearms()
	{
		const float preHit = ResolveLowHealthPreHitPct(25.0f, 65.0f, 35.0f);
		return ShouldRearmLowHealthTrigger(preHit, 25.0f, kLionheart) &&
		       PassesLowHealthFireGate(25.0f, false, kLionheart);
	}

	// A hit 45% -> 35% lands between the lines: the pre-hit 45% proves the
	// player was back above the rearm line, so it rearms, but 35% does not fire.
	constexpr bool CheckHitFromAboveRearmLineIntoGapRearmsWithoutFiring()
	{
		const float preHit = ResolveLowHealthPreHitPct(35.0f, 10.0f, 20.0f);
		return ShouldRearmLowHealthTrigger(preHit, 35.0f, kLionheart) &&
		       !PassesLowHealthFireGate(35.0f, false, kLionheart);
	}

	// A hit 38% -> 33% never crossed the rearm line: stays consumed, no fire.
	constexpr bool CheckHitInsideGapNeitherRearmsNorFires()
	{
		const float preHit = ResolveLowHealthPreHitPct(33.0f, 5.0f, 90.0f);
		return !ShouldRearmLowHealthTrigger(preHit, 33.0f, kLionheart) &&
		       !PassesLowHealthFireGate(33.0f, false, kLionheart);
	}
}

static_assert(CheckHealDuringCooldownRearms(), "a heal above the rearm line rearms even through the ICD");
static_assert(CheckHitFromAboveRearmLineIntoGapRearmsWithoutFiring(), "pre-hit health above the rearm line rearms");
static_assert(CheckHitInsideGapNeitherRearmsNorFires(), "a hit inside the gap does not rearm");
static_assert(ShouldRearmLowHealthTrigger(ResolveLowHealthPreHitPct(35.0f, 0.0f, 45.0f), 35.0f, kLionheart), "unknown damage rearms from a snapshot above the line");
static_assert(!ShouldRearmLowHealthTrigger(ResolveLowHealthPreHitPct(35.0f, 0.0f, 36.0f), 35.0f, kLionheart), "unknown damage with a snapshot below the line stays consumed");
static_assert(ShouldRearmLowHealthTrigger(0.0f, 40.0f, kLionheart), "health at the rearm line rearms");
static_assert(!ShouldRearmLowHealthTrigger(39.0f, 20.0f, kLionheart), "a hit from below the rearm line keeps it consumed");
static_assert(!PassesLowHealthFireGate(20.0f, true, kLionheart), "a consumed trigger does not fire again");
static_assert(PassesLowHealthFireGate(30.0f, false, kLionheart), "the threshold itself fires");

static_assert(ResolveLowHealthPreHitPct(20.0f, 0.0f, 62.0f) == 62.0f, "unknown damage falls back to the snapshot");
static_assert(ResolveLowHealthPreHitPct(80.0f, 50.0f, 0.0f) == 100.0f, "pre-hit health is capped at 100%");

static_assert(ResolveLowHealthBands(30.0f, 20.0f).rearmPct == 31.0f, "the rearm line always sits above the threshold");
static_assert(ResolveLowHealthBands(0.0f, 0.0f).thresholdPct == 1.0f);
