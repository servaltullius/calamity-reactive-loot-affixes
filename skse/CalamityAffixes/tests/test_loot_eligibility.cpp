#include "CalamityAffixes/LootEligibility.h"

using CalamityAffixes::detail::ContainsCaseInsensitiveAscii;
using CalamityAffixes::detail::IsLootArmorEligible;
using CalamityAffixes::detail::LootArmorEligibilityInput;
using CalamityAffixes::detail::MatchesAnyCaseInsensitiveMarker;

static_assert(ContainsCaseInsensitiveAscii("AdventurersGuild_RewardBox", "adventurersguild"));
static_assert(ContainsCaseInsensitiveAscii("RewardBoxTier03", "rewardbox"));
static_assert(!ContainsCaseInsensitiveAscii("SteelArmor", "rewardbox"));

static_assert([] {
	constexpr std::array<std::string_view, 2> markers{ "randombox", "lootbox" };
	return MatchesAnyCaseInsensitiveMarker("AG_RANDOMBOX_TIER1", markers);
}());

static_assert([] {
	constexpr std::array<std::string_view, 2> markers{ "rewardbox", "lootbox" };
	return !MatchesAnyCaseInsensitiveMarker("steel_cuirass", markers);
}());

static_assert(IsLootArmorEligible(LootArmorEligibilityInput{
	.playable = true,
	.hasSlotMask = true,
	.hasArmorAddons = true,
	.editorIdDenied = false
}));

static_assert(!IsLootArmorEligible(LootArmorEligibilityInput{
	.playable = true,
	.hasSlotMask = true,
	.hasArmorAddons = false,
	.editorIdDenied = false
}));

static_assert(!IsLootArmorEligible(LootArmorEligibilityInput{
	.playable = false,
	.hasSlotMask = true,
	.hasArmorAddons = true,
	.editorIdDenied = false
}));

static_assert(!IsLootArmorEligible(LootArmorEligibilityInput{
	.playable = true,
	.hasSlotMask = false,
	.hasArmorAddons = true,
	.editorIdDenied = false
}));

static_assert(!IsLootArmorEligible(LootArmorEligibilityInput{
	.playable = true,
	.hasSlotMask = true,
	.hasArmorAddons = true,
	.editorIdDenied = true
}));

// Bound weapons vanish with their spell, so they never take affixes or serve as
// a crafting base; a non-playable weapon only stops being a crafting base.
static_assert(!CalamityAffixes::detail::IsLootWeaponEligible({ .bound = true, .playable = true }));
static_assert(!CalamityAffixes::detail::IsLootWeaponEligible({ .bound = true, .playable = true, .asCraftingBase = true }));
static_assert(CalamityAffixes::detail::IsLootWeaponEligible({ .bound = false, .playable = true, .asCraftingBase = true }));
static_assert(!CalamityAffixes::detail::IsLootWeaponEligible({ .bound = false, .playable = false, .asCraftingBase = true }));
static_assert(CalamityAffixes::detail::IsLootWeaponEligible({ .bound = false, .playable = false }));
