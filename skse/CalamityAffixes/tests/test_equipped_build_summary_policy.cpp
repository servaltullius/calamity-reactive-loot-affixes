#include "CalamityAffixes/EquippedBuildSummaryPolicy.h"

namespace
{
	using namespace CalamityAffixes::detail;

	static_assert(!ResolveEquippedBuildSummaryReady(false, true, true, true));
	static_assert(!ResolveEquippedBuildSummaryReady(true, true, false, true));
	static_assert(!ResolveEquippedBuildSummaryReady(true, true, true, false));
	static_assert(ResolveEquippedBuildSummaryReady(true, true, true, true));
	static_assert(ResolveEquippedBuildSummaryReady(true, false, false, false));

	constexpr auto kEnabledPassiveSpell = ResolveEquippedBuildPassiveContributionState({
		.hasPassiveSpell = true,
	});
	static_assert(kEnabledPassiveSpell.hasPassiveContribution);
	static_assert(kEnabledPassiveSpell.passiveContributionActive);
	static_assert(!kEnabledPassiveSpell.passiveSpellDisabled);

	constexpr auto kDisabledPassiveSpell = ResolveEquippedBuildPassiveContributionState({
		.hasPassiveSpell = true,
		.passiveSpellsDisabled = true,
	});
	static_assert(kDisabledPassiveSpell.hasPassiveContribution);
	static_assert(!kDisabledPassiveSpell.passiveContributionActive);
	static_assert(kDisabledPassiveSpell.passiveSpellDisabled);

	constexpr auto kSuppressedCrit = ResolveEquippedBuildPassiveContributionState({
		.hasCritContribution = true,
		.suffixFamilySuppressed = true,
	});
	static_assert(kSuppressedCrit.hasPassiveContribution);
	static_assert(!kSuppressedCrit.passiveContributionActive);

	constexpr auto kSuppressedScroll = ResolveEquippedBuildPassiveContributionState({
		.hasScrollContribution = true,
		.suffixFamilySuppressed = true,
	});
	static_assert(kSuppressedScroll.hasPassiveContribution);
	static_assert(kSuppressedScroll.passiveContributionActive);

	constexpr EquippedBuildPolicyInput kOffenseProc{
		.trigger = EquippedBuildTriggerKind::kHit,
		.slot = EquippedBuildSlotKind::kPrefix,
		.procLane = EquippedBuildProcLane::kStandard,
		.configuredProcChancePct = 30.0f,
	};
	static_assert(ShouldShowEquippedBuildEntry(kOffenseProc));
	static_assert(HasEquippedBuildProcRoll(kOffenseProc));
	static_assert(!IsEquippedBuildPassive(kOffenseProc));
	static_assert(ResolveEquippedBuildGroup(kOffenseProc) == EquippedBuildGroup::kOffense);
	static_assert(ResolveEquippedBuildTriggerKey(kOffenseProc) == EquippedBuildTriggerKey::kHit);

	constexpr EquippedBuildPolicyInput kIncomingProc{
		.trigger = EquippedBuildTriggerKind::kIncomingHit,
		.slot = EquippedBuildSlotKind::kPrefix,
		.procLane = EquippedBuildProcLane::kSpecial,
		.configuredProcChancePct = 25.0f,
		.luckyHitChancePct = 40.0f,
	};
	static_assert(ResolveEquippedBuildGroup(kIncomingProc) == EquippedBuildGroup::kDefense);
	static_assert(ResolveEquippedBuildTriggerKey(kIncomingProc) == EquippedBuildTriggerKey::kIncomingHit);
	static_assert(HasEquippedBuildLuckyHitGate(kIncomingProc));

	constexpr EquippedBuildPolicyInput kKillProc{
		.trigger = EquippedBuildTriggerKind::kKill,
		.slot = EquippedBuildSlotKind::kRuneword,
		.procLane = EquippedBuildProcLane::kSpecial,
		.configuredProcChancePct = 100.0f,
		.luckyHitChancePct = 50.0f,
	};
	static_assert(ResolveEquippedBuildGroup(kKillProc) == EquippedBuildGroup::kKill);
	static_assert(ResolveEquippedBuildTriggerKey(kKillProc) == EquippedBuildTriggerKey::kKill);
	static_assert(!HasEquippedBuildLuckyHitGate(kKillProc));

	constexpr EquippedBuildPolicyInput kFamilySuffix{
		.trigger = EquippedBuildTriggerKind::kHit,
		.slot = EquippedBuildSlotKind::kSuffix,
		.procLane = EquippedBuildProcLane::kNone,
		.hasPassiveContribution = true,
		.isDebugNotify = true,
	};
	static_assert(ShouldShowEquippedBuildEntry(kFamilySuffix));
	static_assert(IsEquippedBuildPassive(kFamilySuffix));
	static_assert(ResolveEquippedBuildGroup(kFamilySuffix) == EquippedBuildGroup::kPassive);
	static_assert(ResolveEquippedBuildTriggerKey(kFamilySuffix) == EquippedBuildTriggerKey::kPassive);

	constexpr EquippedBuildPolicyInput kHiddenDebugHelper{
		.trigger = EquippedBuildTriggerKind::kHit,
		.slot = EquippedBuildSlotKind::kRuneword,
		.procLane = EquippedBuildProcLane::kNone,
		.hasPassiveContribution = false,
		.isDebugNotify = true,
	};
	static_assert(!ShouldShowEquippedBuildEntry(kHiddenDebugHelper));

	constexpr EquippedBuildPolicyInput kPassiveRuneword{
		.trigger = EquippedBuildTriggerKind::kHit,
		.slot = EquippedBuildSlotKind::kRuneword,
		.procLane = EquippedBuildProcLane::kNone,
		.hasPassiveContribution = true,
	};
	static_assert(IsEquippedBuildPassive(kPassiveRuneword));
	static_assert(ResolveEquippedBuildGroup(kPassiveRuneword) == EquippedBuildGroup::kPassive);

	constexpr EquippedBuildPolicyInput kHybridRuneword{
		.trigger = EquippedBuildTriggerKind::kHit,
		.slot = EquippedBuildSlotKind::kRuneword,
		.procLane = EquippedBuildProcLane::kStandard,
		.hasPassiveContribution = true,
		.configuredProcChancePct = 35.0f,
	};
	static_assert(HasEquippedBuildProcRoll(kHybridRuneword));
	static_assert(!IsEquippedBuildPassive(kHybridRuneword));
	static_assert(ResolveEquippedBuildGroup(kHybridRuneword) == EquippedBuildGroup::kOffense);
	static_assert(ResolveEquippedBuildTriggerKey(kHybridRuneword) == EquippedBuildTriggerKey::kHit);

	static_assert(ResolveEquippedBuildSuffixState(false, false, false) == EquippedBuildSuffixState::kNone);
	static_assert(ResolveEquippedBuildSuffixState(true, false, false) == EquippedBuildSuffixState::kStacking);
	static_assert(ResolveEquippedBuildSuffixState(true, true, true) == EquippedBuildSuffixState::kHighest);
	static_assert(ResolveEquippedBuildSuffixState(true, true, false) == EquippedBuildSuffixState::kSuppressed);

	static_assert(DescribeEquippedBuildGroup(EquippedBuildGroup::kOffense) == "offense");
	static_assert(DescribeEquippedBuildGroup(EquippedBuildGroup::kDefense) == "defense");
	static_assert(DescribeEquippedBuildTriggerKey(EquippedBuildTriggerKey::kDotApply) == "dotApply");
	static_assert(DescribeEquippedBuildSlotKind(EquippedBuildSlotKind::kRuneword) == "runeword");
	static_assert(DescribeEquippedBuildSuffixState(EquippedBuildSuffixState::kSuppressed) == "suppressed");
}
