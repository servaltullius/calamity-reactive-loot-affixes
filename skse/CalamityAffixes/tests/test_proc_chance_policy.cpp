#include "CalamityAffixes/ProcChancePolicy.h"

#include <array>

namespace
{
	[[nodiscard]] constexpr bool NearlyEqual(float a_lhs, float a_rhs, float a_epsilon = 0.001f) noexcept
	{
		return a_lhs >= a_rhs - a_epsilon && a_lhs <= a_rhs + a_epsilon;
	}
}

static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(0u) == 1.0f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(1u) == 1.0f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(2u) == 0.8f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(3u) == 0.65f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(4u) == 0.5f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(7u) == 0.5f);

static_assert(CalamityAffixes::IsProcPenaltyEligible(false, 30.0f, true));
static_assert(!CalamityAffixes::IsProcPenaltyEligible(true, 30.0f, true));   // suffixes never count
static_assert(!CalamityAffixes::IsProcPenaltyEligible(false, 0.0f, true));   // 0%-chance passive prefix
static_assert(!CalamityAffixes::IsProcPenaltyEligible(false, 30.0f, false)); // DebugNotify-style action

static_assert(CalamityAffixes::CountProcCapableSlots(0u, [](std::uint8_t) { return true; }) == 0u);
static_assert(CalamityAffixes::CountProcCapableSlots(4u, [](std::uint8_t) { return false; }) == 0u);
static_assert(CalamityAffixes::CountProcCapableSlots(4u, [](std::uint8_t) { return true; }) == 4u);
static_assert(CalamityAffixes::CountProcCapableSlots(4u, [](std::uint8_t a_slot) { return a_slot == 0u; }) == 1u);
// One proc prefix sharing an item with three passive suffixes keeps its full chance:
// the penalty tier counts proc-capable slots only.
static_assert(
	CalamityAffixes::ResolveMultiAffixProcPenalty(
		CalamityAffixes::CountProcCapableSlots(4u, [](std::uint8_t a_slot) { return a_slot == 0u; })) == 1.0f);
// Two proc prefixes on one item still get the 2-slot tier regardless of suffix padding.
static_assert(
	CalamityAffixes::ResolveMultiAffixProcPenalty(
		CalamityAffixes::CountProcCapableSlots(4u, [](std::uint8_t a_slot) { return a_slot < 2u; })) == 0.8f);

static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(30.0f, 1.0f, 0.5f) == 15.0f);
static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(30.0f, 3.0f, 0.5f) == 45.0f);
static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(30.0f, 1.2f, 1.0f) == 36.0f);
static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(80.0f, 3.0f, 1.0f) == 100.0f);
static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(-10.0f, 1.0f, 1.0f) == 0.0f);

constexpr std::array kOneFullPenalty{ 1.0f };
constexpr std::array kTwoFullPenalties{ 1.0f, 1.0f };
constexpr std::array kThreeMixedPenalties{ 1.0f, 0.8f, 0.5f };
constexpr std::array kFourPenalties{ 1.0f, 1.0f, 1.0f, 1.0f };
static_assert(NearlyEqual(CalamityAffixes::ResolveStackedProcChancePct(30.0f, 1.0f, {}), 30.0f));
static_assert(NearlyEqual(CalamityAffixes::ResolveStackedProcChancePct(30.0f, 1.0f, kOneFullPenalty), 30.0f));
static_assert(NearlyEqual(CalamityAffixes::ResolveStackedProcChancePct(30.0f, 1.0f, kTwoFullPenalties), 51.0f));
static_assert(NearlyEqual(CalamityAffixes::ResolveStackedProcChancePct(30.0f, 1.0f, kThreeMixedPenalties), 54.78f));
static_assert(NearlyEqual(CalamityAffixes::ResolveStackedProcChancePct(30.0f, 1.0f, kFourPenalties), 65.7f));
static_assert(NearlyEqual(CalamityAffixes::ResolveStackedProcChancePct(100.0f, 1.0f, kTwoFullPenalties), 100.0f));
static_assert(NearlyEqual(CalamityAffixes::ResolveStackedProcChancePct(0.0f, 1.0f, kTwoFullPenalties), 0.0f));

static_assert(CalamityAffixes::ShouldShowAdjustedProcChance(30.0f, 15.0f));
static_assert(!CalamityAffixes::ShouldShowAdjustedProcChance(30.0f, 30.0f));

static_assert(
	CalamityAffixes::ResolveProcChanceDisplayMode(true, true, true, true, true) ==
	CalamityAffixes::ProcChanceDisplayMode::kCurrent);
static_assert(
	CalamityAffixes::ResolveProcChanceDisplayMode(true, false, true, true, true) ==
	CalamityAffixes::ProcChanceDisplayMode::kEquippedAloneEstimate);
static_assert(
	CalamityAffixes::ResolveProcChanceDisplayMode(false, false, true, true, true) ==
	CalamityAffixes::ProcChanceDisplayMode::kInactive);
static_assert(
	CalamityAffixes::ResolveProcChanceDisplayMode(true, true, true, false, true) ==
	CalamityAffixes::ProcChanceDisplayMode::kInactive);
static_assert(
	CalamityAffixes::ResolveProcChanceDisplayMode(true, true, false, true, true) ==
	CalamityAffixes::ProcChanceDisplayMode::kInactive);
static_assert(
	CalamityAffixes::ResolveProcChanceDisplayMode(true, true, true, true, false) ==
	CalamityAffixes::ProcChanceDisplayMode::kInactive);

static_assert(
	CalamityAffixes::ResolveDisplayedProcChancePct(
		CalamityAffixes::ProcChanceDisplayMode::kCurrent,
		true,
		30.0f,
		1.0f,
		0.5f,
		24.0f) == 24.0f);
static_assert(
	CalamityAffixes::ResolveDisplayedProcChancePct(
		CalamityAffixes::ProcChanceDisplayMode::kEquippedAloneEstimate,
		true,
		30.0f,
		1.0f,
		0.5f,
		24.0f) == 15.0f);
static_assert(
	CalamityAffixes::ResolveDisplayedProcChancePct(
		CalamityAffixes::ProcChanceDisplayMode::kCurrent,
		false,
		30.0f,
		1.2f,
		0.5f,
		7.0f) == 36.0f);
