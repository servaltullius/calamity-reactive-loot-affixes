#include "CalamityAffixes/ProcChancePolicy.h"

static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(0u) == 1.0f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(1u) == 1.0f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(2u) == 0.8f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(3u) == 0.65f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(4u) == 0.5f);
static_assert(CalamityAffixes::ResolveMultiAffixProcPenalty(7u) == 0.5f);

static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(30.0f, 1.0f, 0.5f) == 15.0f);
static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(30.0f, 3.0f, 0.5f) == 45.0f);
static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(30.0f, 1.2f, 1.0f) == 36.0f);
static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(80.0f, 3.0f, 1.0f) == 100.0f);
static_assert(CalamityAffixes::ResolveEffectiveProcChancePct(-10.0f, 1.0f, 1.0f) == 0.0f);

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
