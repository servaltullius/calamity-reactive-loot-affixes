#include "CalamityAffixes/LootRollSelection.h"

using CalamityAffixes::detail::DetermineLootPrefixSuffixTargets;
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
