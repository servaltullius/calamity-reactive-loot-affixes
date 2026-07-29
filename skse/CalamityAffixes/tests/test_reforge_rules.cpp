#include "CalamityAffixes/LootRollSelection.h"

using CalamityAffixes::detail::DetermineLootPrefixSuffixTargets;
using CalamityAffixes::detail::BuildRegularOnlyAffixSlots;
using CalamityAffixes::detail::DidConsumeExactInventoryCount;
using CalamityAffixes::detail::DidRestoreExactInventoryCount;
using CalamityAffixes::detail::ShouldRetryRegularAffixReforgeRoll;
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
	       regularSlots.tokens[0] == 0xBBu &&
	       regularSlots.tokens[1] == 0xCCu;
}(),
	"BuildRegularOnlyAffixSlots: removes preserved runeword token while keeping regular slot order");

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

static_assert(DidConsumeExactInventoryCount(3u, 2u, 1u),
	"DidConsumeExactInventoryCount: accepts exactly one consumed reforge orb");
static_assert(!DidConsumeExactInventoryCount(3u, 3u, 1u),
	"DidConsumeExactInventoryCount: rejects a failed removal");
static_assert(!DidConsumeExactInventoryCount(3u, 1u, 1u),
	"DidConsumeExactInventoryCount: rejects an unexpected multi-item delta");
static_assert(!DidConsumeExactInventoryCount(2u, 3u, 1u),
	"DidConsumeExactInventoryCount: rejects an inventory count increase");

static_assert(DidRestoreExactInventoryCount(1u, 2u, 1u),
	"DidRestoreExactInventoryCount: accepts exactly one restored reforge orb");
static_assert(!DidRestoreExactInventoryCount(1u, 1u, 1u),
	"DidRestoreExactInventoryCount: rejects a failed compensation");
static_assert(!DidRestoreExactInventoryCount(1u, 3u, 1u),
	"DidRestoreExactInventoryCount: rejects an unexpected multi-item compensation");
