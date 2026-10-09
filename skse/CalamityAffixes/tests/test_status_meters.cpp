#include "CalamityAffixes/StatusMeters.h"

namespace
{
	using CalamityAffixes::BuildUpEmptiesAtMs;
	using CalamityAffixes::BuildUpThreshold;
	using CalamityAffixes::DecayedBuildUp;
	using CalamityAffixes::StatusKind;
	using CalamityAffixes::StatusMeters;

	constexpr std::uint32_t kTarget = 0x1234u;
	constexpr std::uint32_t kOther = 0x5678u;

	constexpr bool CheckFillsAndPaysOffOnce()
	{
		StatusMeters meters{};
		auto gain = meters.Gain(kTarget, StatusKind::kFreeze, 40.0f, 0, false, 3000);
		if (!gain.applied || gain.paidOff || gain.value != 40.0f) return false;
		gain = meters.Gain(kTarget, StatusKind::kFreeze, 40.0f, 500, false, 3000);
		if (gain.paidOff || gain.value != 80.0f) return false;
		gain = meters.Gain(kTarget, StatusKind::kFreeze, 40.0f, 1000, false, 3000);
		if (!gain.paidOff || gain.value != 0.0f || meters.Value(kTarget, StatusKind::kFreeze, 1000) != 0.0f) return false;
		// Locked while the freeze lasts, open again after it.
		gain = meters.Gain(kTarget, StatusKind::kFreeze, 40.0f, 2000, false, 3000);
		if (gain.applied) return false;
		gain = meters.Gain(kTarget, StatusKind::kFreeze, 40.0f, 4000, false, 3000);
		return gain.applied && gain.value == 40.0f;
	}

	constexpr bool CheckKindsAndTargetsAreSeparate()
	{
		StatusMeters meters{};
		meters.Gain(kTarget, StatusKind::kFreeze, 50.0f, 0, false, 0);
		meters.Gain(kTarget, StatusKind::kShock, 30.0f, 0, false, 0);
		meters.Gain(kOther, StatusKind::kBleed, 20.0f, 0, false, 0);
		return meters.Value(kTarget, StatusKind::kFreeze, 0) == 50.0f &&
		       meters.Value(kTarget, StatusKind::kShock, 0) == 30.0f &&
		       meters.Value(kTarget, StatusKind::kBleed, 0) == 0.0f &&
		       meters.Value(kOther, StatusKind::kBleed, 0) == 20.0f &&
		       !meters.Gain(kTarget, StatusKind::kBurning, 10.0f, 0, false, 0).applied &&
		       !meters.Gain(kTarget, StatusKind::kExposed, 10.0f, 0, false, 0).applied;
	}

	constexpr bool CheckDecay()
	{
		// Holds for 3 s, then drains 15 per second.
		return DecayedBuildUp(60.0f, 0, 3000) == 60.0f && DecayedBuildUp(60.0f, 0, 4000) == 45.0f &&
		       DecayedBuildUp(60.0f, 0, 7000) == 0.0f && DecayedBuildUp(60.0f, 0, 60000) == 0.0f &&
		       BuildUpEmptiesAtMs(60.0f, 1000) == 8000 && BuildUpEmptiesAtMs(0.0f, 1000) == 1000;
	}

	constexpr bool CheckDecayBetweenGains()
	{
		StatusMeters meters{};
		meters.Gain(kTarget, StatusKind::kBleed, 60.0f, 0, false, 0);
		const auto gain = meters.Gain(kTarget, StatusKind::kBleed, 10.0f, 5000, false, 0);  // 60 - 30 + 10
		return gain.value == 40.0f;
	}

	constexpr bool CheckToughThresholdClimbs()
	{
		if (BuildUpThreshold(false, 9u) != 100.0f) return false;
		if (BuildUpThreshold(true, 0u) != 150.0f || BuildUpThreshold(true, 2u) != 250.0f) return false;
		if (BuildUpThreshold(true, 10u) != 300.0f) return false;
		StatusMeters meters{};
		if (meters.Gain(kTarget, StatusKind::kShock, 120.0f, 0, true, 0).paidOff) return false;
		if (!meters.Gain(kTarget, StatusKind::kShock, 40.0f, 100, true, 0).paidOff) return false;
		const auto next = meters.Gain(kTarget, StatusKind::kShock, 160.0f, 200, true, 0);
		return !next.paidOff && next.threshold == 200.0f;
	}

	constexpr bool CheckBurningStacks()
	{
		StatusMeters meters{};
		if (meters.AddBurning(kTarget, 2u, 0) != 2u) return false;
		if (meters.AddBurning(kTarget, 2u, 1000) != 4u) return false;
		if (meters.AddBurning(kTarget, 2u, 2000) != 5u) return false;  // capped
		if (meters.BurningStacks(kTarget, 5999) != 5u) return false;   // refreshed at 2000
		if (meters.BurningStacks(kTarget, 6000) != 0u) return false;
		return meters.AddBurning(kTarget, 1u, 7000) == 1u;  // expired stacks start over
	}

	constexpr bool CheckTakeRemovesTarget()
	{
		StatusMeters meters{};
		meters.Gain(kTarget, StatusKind::kBleed, 70.0f, 0, false, 0);
		meters.AddBurning(kTarget, 3u, 0);
		meters.Gain(kOther, StatusKind::kFreeze, 10.0f, 0, false, 0);
		const auto snapshot = meters.Take(kTarget, 1000);
		return snapshot.values[1] == 70.0f && snapshot.burningStacks == 3u && meters.Size() == 1u &&
		       meters.Value(kTarget, StatusKind::kBleed, 1000) == 0.0f;
	}

	constexpr bool CheckFullTableReusesOldest()
	{
		StatusMeters meters{};
		for (std::uint32_t i = 0; i < StatusMeters::kCapacity; ++i) {
			meters.Gain(100u + i, StatusKind::kFreeze, 10.0f, static_cast<std::int64_t>(i), false, 0);
		}
		meters.Gain(kTarget, StatusKind::kFreeze, 10.0f, 1000, false, 0);
		return meters.Size() == StatusMeters::kCapacity && meters.Value(100u, StatusKind::kFreeze, 1000) == 0.0f &&
		       meters.Value(101u, StatusKind::kFreeze, 1000) == 10.0f &&
		       meters.Value(kTarget, StatusKind::kFreeze, 1000) == 10.0f;
	}
}

static_assert(CheckFillsAndPaysOffOnce(), "a full meter pays off once, empties and locks for the payoff");
static_assert(CheckKindsAndTargetsAreSeparate(), "each target keeps one meter per build-up kind");
static_assert(CheckDecay(), "meters hold for 3 s, then drain 15 per second");
static_assert(CheckDecayBetweenGains(), "a gain adds to the decayed meter");
static_assert(CheckToughThresholdClimbs(), "tough targets start at 150 and climb 50 per payoff up to 300");
static_assert(CheckBurningStacks(), "Burning stacks to 5 and expires 4 s after the last stack");
static_assert(CheckTakeRemovesTarget(), "Contagion takes a dying target's meters");
static_assert(CheckFullTableReusesOldest(), "a full table reuses the slot touched longest ago");
