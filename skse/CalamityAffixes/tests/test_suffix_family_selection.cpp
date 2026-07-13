#include "CalamityAffixes/SuffixFamilySelection.h"

using CalamityAffixes::detail::IsPreferredSuffixFamilyCandidate;
using CalamityAffixes::detail::ParseSuffixTierRank;
using CalamityAffixes::detail::PassiveSpellReconcileAction;
using CalamityAffixes::detail::ResolvePassiveSpellReconcileAction;
using CalamityAffixes::detail::SuffixFamilyBestCandidate;

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
	SuffixFamilyBestCandidate best;
	for (std::uint32_t i = 0; i < a_t1Count; ++i) {
		best.Consider("suffix_assassin_t1", 0u);
	}
	for (std::uint32_t i = 0; i < a_t2Count; ++i) {
		best.Consider("suffix_assassin_t2", 1u);
	}
	for (std::uint32_t i = 0; i < a_t3Count; ++i) {
		best.Consider("suffix_assassin_t3", 2u);
	}

	constexpr float kCritDamageBonusByIndex[]{ 10.0f, 20.0f, 30.0f };
	return best.selected ? kCritDamageBonusByIndex[best.index] : 0.0f;
}

static_assert(SelectAssassinCritDamageBonus(2u, 0u, 0u) == 10.0f);
static_assert(SelectAssassinCritDamageBonus(1u, 1u, 0u) == 20.0f);
static_assert(SelectAssassinCritDamageBonus(0u, 1u, 1u) == 30.0f);

static_assert(ResolvePassiveSpellReconcileAction(false, true, false) ==
	PassiveSpellReconcileAction::kRemove);
static_assert(ResolvePassiveSpellReconcileAction(true, false, false) ==
	PassiveSpellReconcileAction::kAdd);
static_assert(ResolvePassiveSpellReconcileAction(true, true, false) ==
	PassiveSpellReconcileAction::kKeep);
static_assert(ResolvePassiveSpellReconcileAction(true, true, true) ==
	PassiveSpellReconcileAction::kRemove);
