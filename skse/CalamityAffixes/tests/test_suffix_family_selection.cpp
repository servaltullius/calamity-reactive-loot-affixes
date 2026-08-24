#include "CalamityAffixes/SuffixFamilySelection.h"

#include <limits>

using CalamityAffixes::detail::IsPreferredSuffixFamilyCandidate;
using CalamityAffixes::detail::ParseSuffixTierRank;
using CalamityAffixes::detail::PassiveSpellReconcileAction;
using CalamityAffixes::detail::PassiveSpellReconcileInput;
using CalamityAffixes::detail::ResolvePassiveSpellReconcileAction;
using CalamityAffixes::detail::ResolveSuffixFamilyContributionSourceIndex;
using CalamityAffixes::detail::SuffixFamilyBestCandidate;
using CalamityAffixes::detail::SuffixFamilyRankSelection;

static_assert(ParseSuffixTierRank("suffix_vitality_t1") == 1u);
static_assert(ParseSuffixTierRank("suffix_vitality_t3") == 3u);
static_assert(ParseSuffixTierRank("suffix_future_t12") == 12u);
static_assert(ParseSuffixTierRank("suffix_vitality") == 0u);
static_assert(ParseSuffixTierRank("suffix_vitality_t") == 0u);
static_assert(ParseSuffixTierRank("suffix_vitality_t3_extra") == 0u);
static_assert(ParseSuffixTierRank("runeword_spirit_final") == 0u);

static_assert(IsPreferredSuffixFamilyCandidate(
	"suffix_vitality_t3", 9u, "suffix_vitality_t2", 4u));
static_assert(!IsPreferredSuffixFamilyCandidate(
	"suffix_vitality_t1", 1u, "suffix_vitality_t2", 4u));
static_assert(IsPreferredSuffixFamilyCandidate(
	"legacy_family_bonus", 2u, "legacy_family_other", 5u));
static_assert(!IsPreferredSuffixFamilyCandidate(
	"legacy_family_bonus", 7u, "legacy_family_other", 5u));

[[nodiscard]] constexpr float SelectAssassinCritDamageBonus(
	std::uint32_t a_t1Count,
	std::uint32_t a_t2Count,
	std::uint32_t a_t3Count) noexcept
{
	SuffixFamilyRankSelection selection;
	selection.ConsiderDefinition("suffix_assassin_t1");
	selection.ConsiderDefinition("suffix_assassin_t2");
	selection.ConsiderDefinition("suffix_assassin_t3");
	selection.ConsiderEquipped("suffix_assassin_t1", 0u, a_t1Count);
	selection.ConsiderEquipped("suffix_assassin_t2", 1u, a_t2Count);
	selection.ConsiderEquipped("suffix_assassin_t3", 2u, a_t3Count);
	selection.ConsiderEffectiveDefinition("suffix_assassin_t1", 0u);
	selection.ConsiderEffectiveDefinition("suffix_assassin_t2", 1u);
	selection.ConsiderEffectiveDefinition("suffix_assassin_t3", 2u);
	const auto best = selection.ResolveEffectiveCandidate();

	constexpr float kCritDamageBonusByIndex[]{ 10.0f, 20.0f, 30.0f };
	return best.selected ? kCritDamageBonusByIndex[best.index] : 0.0f;
}

static_assert(SelectAssassinCritDamageBonus(2u, 0u, 0u) == 20.0f);
static_assert(SelectAssassinCritDamageBonus(1u, 1u, 0u) == 30.0f);
static_assert(SelectAssassinCritDamageBonus(0u, 1u, 1u) == 30.0f);
static_assert(SelectAssassinCritDamageBonus(0u, 2u, 0u) == 30.0f);

constexpr auto kTwoTierOneSelection = [] {
	SuffixFamilyRankSelection selection;
	selection.ConsiderDefinition("suffix_vitality_t1");
	selection.ConsiderDefinition("suffix_vitality_t2");
	selection.ConsiderDefinition("suffix_vitality_t3");
	selection.ConsiderEquipped("suffix_vitality_t1", 4u, 2u);
	selection.ConsiderEffectiveDefinition("suffix_vitality_t1", 4u);
	selection.ConsiderEffectiveDefinition("suffix_vitality_t2", 5u);
	selection.ConsiderEffectiveDefinition("suffix_vitality_t3", 6u);
	return selection;
}();
static_assert(kTwoTierOneSelection.rankPoints == 2u);
static_assert(kTwoTierOneSelection.EffectiveRank() == 2u);
static_assert(kTwoTierOneSelection.ResolvedEffectiveRank() == 2u);
static_assert(kTwoTierOneSelection.bestEquipped.index == 4u);
static_assert(kTwoTierOneSelection.ResolveEffectiveCandidate().index == 5u);

constexpr auto kSparseTierSelection = [] {
	SuffixFamilyRankSelection selection;
	selection.ConsiderDefinition("suffix_sparse_t1");
	selection.ConsiderDefinition("suffix_sparse_t3");
	selection.ConsiderEquipped("suffix_sparse_t1", 7u, 2u);
	selection.ConsiderEffectiveDefinition("suffix_sparse_t1", 7u);
	selection.ConsiderEffectiveDefinition("suffix_sparse_t3", 8u);
	return selection;
}();
static_assert(kSparseTierSelection.rankPoints == 2u);
static_assert(kSparseTierSelection.EffectiveRank() == 2u);
static_assert(kSparseTierSelection.ResolvedEffectiveRank() == 1u);
static_assert(kSparseTierSelection.ResolveEffectiveCandidate().index == 7u);

constexpr SuffixFamilyBestCandidate kPromotedContributionSource{
	.selected = true,
	.id = "suffix_shape_t2",
	.index = 12u,
};
static_assert(ResolveSuffixFamilyContributionSourceIndex(true, kPromotedContributionSource, 4u) == 12u);
static_assert(ResolveSuffixFamilyContributionSourceIndex(false, kPromotedContributionSource, 4u) == 4u);
static_assert(ResolveSuffixFamilyContributionSourceIndex(true, {}, 4u) == 4u);
static_assert(CalamityAffixes::detail::AccumulateSuffixFamilyRankPoints(
	1u,
	3u,
	std::numeric_limits<std::uint32_t>::max()) == std::numeric_limits<std::uint32_t>::max());

static_assert(ResolvePassiveSpellReconcileAction({ .desired = false, .present = true }) ==
	PassiveSpellReconcileAction::kRemove);
static_assert(ResolvePassiveSpellReconcileAction({ .desired = true, .present = false }) ==
	PassiveSpellReconcileAction::kAdd);
static_assert(ResolvePassiveSpellReconcileAction({ .desired = true, .present = true }) ==
	PassiveSpellReconcileAction::kKeep);
static_assert(ResolvePassiveSpellReconcileAction(PassiveSpellReconcileInput{
	.desired = true,
	.present = true,
	.passivesDisabled = true,
	.refreshRequested = true,
}) ==
	PassiveSpellReconcileAction::kRemove);
static_assert(ResolvePassiveSpellReconcileAction({
	.desired = true,
	.present = true,
	.refreshRequested = true,
}) ==
	PassiveSpellReconcileAction::kRefresh);
static_assert(ResolvePassiveSpellReconcileAction({
	.desired = false,
	.present = true,
	.refreshRequested = true,
}) ==
	PassiveSpellReconcileAction::kRemove);
