#include "CalamityAffixes/LootRollSelection.h"

using CalamityAffixes::detail::DetermineLootPrefixSuffixTargets;
using CalamityAffixes::detail::DetermineLockedReforgeRerollTargets;
using CalamityAffixes::detail::AreInstanceAffixTokenSetsEqual;
using CalamityAffixes::detail::BuildRegularOnlyAffixSlots;
using CalamityAffixes::detail::CanLockRegularAffixForReforge;
using CalamityAffixes::detail::DidConsumeExactInventoryCount;
using CalamityAffixes::detail::DidRestoreExactInventoryCount;
using CalamityAffixes::detail::HasCompleteLockedRegularAffixReforgeRoll;
using CalamityAffixes::detail::HasCompleteRegularAffixReforgeRoll;
using CalamityAffixes::detail::HasUniqueAffixTokens;
using CalamityAffixes::detail::IsExpectedLockedReforgeInstance;
using CalamityAffixes::detail::IsCanonicalRegularAffixExpansionLayout;
using CalamityAffixes::detail::IsExpectedAffixExpansionState;
using CalamityAffixes::detail::ResolveRegularAffixExpansionPolicy;
using CalamityAffixes::detail::ResolveObservedInventoryConsumption;
using CalamityAffixes::detail::ShouldRetryRegularAffixReforgeRoll;
using CalamityAffixes::detail::TryPromotePreservedRunewordPrimary;
using CalamityAffixes::detail::ResolveReforgeTargetAffixCount;
static constexpr auto kMaxSlots = static_cast<std::uint8_t>(CalamityAffixes::kMaxRegularAffixesPerItem);

static_assert(ResolveReforgeTargetAffixCount(0u) == 1u,
	"ResolveReforgeTargetAffixCount: allow no-affix item reforge with 1 target roll");

static_assert(ResolveReforgeTargetAffixCount(1u) == 1u,
	"ResolveReforgeTargetAffixCount: preserve existing 1-affix reroll size");

static_assert(ResolveReforgeTargetAffixCount(kMaxSlots) == kMaxSlots,
	"ResolveReforgeTargetAffixCount: preserve existing max-size reroll");

static_assert(ResolveReforgeTargetAffixCount(7u) == kMaxSlots,
	"ResolveReforgeTargetAffixCount: clamp corrupted legacy counts to max slots");

static_assert(CalamityAffixes::detail::kStandardReforgeOrbCost == 1u);
static_assert(CalamityAffixes::detail::kLockedReforgeOrbCost == 2u);
static_assert(CalamityAffixes::detail::kExpandAffixOneToTwoOrbCost == 2u);
static_assert(CalamityAffixes::detail::kExpandAffixTwoToThreeOrbCost == 4u);
static_assert(!CanLockRegularAffixForReforge(1u));
static_assert(CanLockRegularAffixForReforge(2u));
static_assert(IsExpectedLockedReforgeInstance(0x100u, 0x100u));
static_assert(!IsExpectedLockedReforgeInstance(0u, 0u));
static_assert(!IsExpectedLockedReforgeInstance(0x100u, 0x200u));

static_assert(!ResolveRegularAffixExpansionPolicy(0u));
static_assert(ResolveRegularAffixExpansionPolicy(1u)->targetRegularAffixCount == 2u);
static_assert(ResolveRegularAffixExpansionPolicy(1u)->orbCost == 2u);
static_assert(ResolveRegularAffixExpansionPolicy(2u)->targetRegularAffixCount == 3u);
static_assert(ResolveRegularAffixExpansionPolicy(2u)->orbCost == 4u);
static_assert(!ResolveRegularAffixExpansionPolicy(3u));
static_assert(IsCanonicalRegularAffixExpansionLayout(1u, 1u, 0u));
static_assert(IsCanonicalRegularAffixExpansionLayout(2u, 1u, 1u));
static_assert(!IsCanonicalRegularAffixExpansionLayout(1u, 0u, 1u));
static_assert(!IsCanonicalRegularAffixExpansionLayout(2u, 2u, 0u));
static_assert(IsExpectedAffixExpansionState(0x100u, 0x100u, 1u, 1u));
static_assert(!IsExpectedAffixExpansionState(0x100u, 0x100u, 1u, 2u));
static_assert(!IsExpectedAffixExpansionState(0x100u, 0x200u, 1u, 1u));
static_assert(!IsExpectedAffixExpansionState(0x100u, 0x100u, 3u, 3u));

static_assert([] {
	const auto targets = DetermineLockedReforgeRerollTargets(2u, true);
	return targets.prefixTarget == 0u && targets.suffixTarget == 1u;
}(), "Locked prefix: reroll only the remaining suffix at regular count 2");

static_assert([] {
	const auto targets = DetermineLockedReforgeRerollTargets(2u, false);
	return targets.prefixTarget == 1u && targets.suffixTarget == 0u;
}(), "Locked suffix: reroll only the remaining prefix at regular count 2");

static_assert([] {
	const auto targets = DetermineLockedReforgeRerollTargets(3u, true);
	return targets.prefixTarget == 0u && targets.suffixTarget == 2u;
}(), "Locked prefix: preserve the 1P+2S layout at regular count 3");

static_assert([] {
	const auto targets = DetermineLockedReforgeRerollTargets(3u, false);
	return targets.prefixTarget == 1u && targets.suffixTarget == 1u;
}(), "Locked suffix: preserve the 1P+2S layout at regular count 3");

static_assert(HasCompleteRegularAffixReforgeRoll(3u, 3u),
	"HasCompleteRegularAffixReforgeRoll: accepts an exact count-preserving roll");
static_assert(!HasCompleteRegularAffixReforgeRoll(3u, 2u),
	"HasCompleteRegularAffixReforgeRoll: rejects a partial roll");
static_assert(!HasCompleteRegularAffixReforgeRoll(1u, 0u),
	"HasCompleteRegularAffixReforgeRoll: rejects an empty roll");

static_assert([] {
	const auto targets = DetermineLootPrefixSuffixTargets(ResolveReforgeTargetAffixCount(0u));
	return targets.prefixTarget == 1u && targets.suffixTarget == 0u;
}(),
	"Reforge(0-affix): follows current prefix-only rollout policy");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	(void)slots.AddToken(0xAAu);
	(void)slots.AddToken(0xBBu);
	(void)slots.AddToken(0xCCu);
	const auto regularSlots = BuildRegularOnlyAffixSlots(slots, 0xAAu);
	return regularSlots.count == 2u &&
	       ResolveReforgeTargetAffixCount(regularSlots.count) == 2u &&
	       regularSlots.tokens[0] == 0xBBu &&
	       regularSlots.tokens[1] == 0xCCu;
}(),
	"Reforge target: excludes the preserved runeword token and keeps the regular affix count");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	(void)slots.AddToken(0xAAu);
	const auto regularSlots = BuildRegularOnlyAffixSlots(slots, 0xAAu);
	return regularSlots.count == 0u && ResolveReforgeTargetAffixCount(regularSlots.count) == 1u;
}(),
	"Reforge target: a runeword-only base receives one regular affix roll");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots previous{};
	CalamityAffixes::InstanceAffixSlots rolled{};
	(void)previous.AddToken(0x10u);
	(void)previous.AddToken(0x20u);
	(void)rolled.AddToken(0x10u);
	(void)rolled.AddToken(0x20u);
	return ShouldRetryRegularAffixReforgeRoll(previous, rolled, 0u, 4u);
}(),
	"ShouldRetryRegularAffixReforgeRoll: retries same-slot regular rerolls before final attempt");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots previous{};
	CalamityAffixes::InstanceAffixSlots rolled{};
	(void)previous.AddToken(0x10u);
	(void)rolled.AddToken(0x10u);
	return !ShouldRetryRegularAffixReforgeRoll(previous, rolled, 3u, 4u);
}(),
	"ShouldRetryRegularAffixReforgeRoll: accepts same-slot reroll on final attempt");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots previous{};
	CalamityAffixes::InstanceAffixSlots reordered{};
	(void)previous.AddToken(0x10u);
	(void)previous.AddToken(0x20u);
	(void)reordered.AddToken(0x20u);
	(void)reordered.AddToken(0x10u);
	return AreInstanceAffixTokenSetsEqual(previous, reordered);
}(), "Locked reforge no-effect check is order-insensitive");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots rolled{};
	(void)rolled.AddToken(0x10u);
	(void)rolled.AddToken(0x20u);
	return HasCompleteLockedRegularAffixReforgeRoll(2u, rolled, 0x10u) &&
	       !HasCompleteLockedRegularAffixReforgeRoll(3u, rolled, 0x10u) &&
	       !HasCompleteLockedRegularAffixReforgeRoll(2u, rolled, 0x30u) &&
	       HasUniqueAffixTokens(rolled);
}(), "Locked reforge requires exact count, unique tokens, and the locked token");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots rolled{};
	(void)rolled.AddToken(0x10u);
	(void)rolled.AddToken(0x20u);
	return TryPromotePreservedRunewordPrimary(rolled, 0x99u) &&
	       rolled.count == 3u &&
	       rolled.GetPrimary() == 0x99u &&
	       rolled.HasToken(0x10u) &&
	       rolled.HasToken(0x20u);
}(), "A preserved runeword is inserted as primary without dropping regular affixes");

static_assert(DidConsumeExactInventoryCount(3u, 2u, 1u),
	"DidConsumeExactInventoryCount: accepts exactly one consumed reforge orb");
static_assert(!DidConsumeExactInventoryCount(3u, 3u, 1u),
	"DidConsumeExactInventoryCount: rejects a failed removal");
static_assert(!DidConsumeExactInventoryCount(3u, 1u, 1u),
	"DidConsumeExactInventoryCount: rejects an unexpected multi-item delta");
static_assert(!DidConsumeExactInventoryCount(2u, 3u, 1u),
	"DidConsumeExactInventoryCount: rejects an inventory count increase");
static_assert(DidConsumeExactInventoryCount(5u, 3u, CalamityAffixes::detail::kLockedReforgeOrbCost),
	"Locked reforge consumes exactly two orbs");
static_assert(!DidConsumeExactInventoryCount(5u, 4u, CalamityAffixes::detail::kLockedReforgeOrbCost),
	"Locked reforge rejects a partial one-orb removal");
static_assert(!DidConsumeExactInventoryCount(5u, 2u, CalamityAffixes::detail::kLockedReforgeOrbCost),
	"Locked reforge rejects an excessive three-orb removal");
static_assert(ResolveObservedInventoryConsumption(5u, 4u) == 1u,
	"Refund only the observed positive inventory delta");
static_assert(ResolveObservedInventoryConsumption(5u, 6u) == 0u,
	"Never refund when inventory unexpectedly increased");

static_assert(DidRestoreExactInventoryCount(1u, 2u, 1u),
	"DidRestoreExactInventoryCount: accepts exactly one restored reforge orb");
static_assert(!DidRestoreExactInventoryCount(1u, 1u, 1u),
	"DidRestoreExactInventoryCount: rejects a failed compensation");
static_assert(!DidRestoreExactInventoryCount(1u, 3u, 1u),
	"DidRestoreExactInventoryCount: rejects an unexpected multi-item compensation");
