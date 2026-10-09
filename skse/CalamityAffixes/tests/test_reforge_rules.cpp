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

#include "CalamityAffixes/AffixCraftingPolicy.h"
#include <initializer_list>
namespace {
constexpr bool CheckCurrencyCrafting() {
    using namespace CalamityAffixes;
    using namespace CalamityAffixes::detail;
    std::uint32_t frequencies[3]{};
    for (std::uint32_t roll = 0; roll < 100; ++roll) ++frequencies[IdentifyAffixCount(roll) - 1];
    if (frequencies[0] != 60 || frequencies[1] != 30 || frequencies[2] != 10) return false;
    for (const auto rune : { 0u, 99u }) {
        for (std::uint8_t count = 0u; count <= 3u; ++count) {
            InstanceAffixSlots before{};
            if (rune) before.AddToken(rune);
            InstanceAffixSlots regular{};
            for (std::uint64_t token = 1u; token <= count; ++token) {
                before.AddToken(token);
                regular.AddToken(token);
            }
            if (CanCraftAffixes(AffixCraftAction::kIdentify, regular, 0u) != (count == 0)) return false;
            if (CanCraftAffixes(AffixCraftAction::kScour, regular, 0u) != (count != 0)) return false;
            if (CanCraftAffixes(AffixCraftAction::kReforge, regular, 99u)) return false;
            for (std::uint64_t token = 1; token <= count; ++token) {
                auto after = before;
                if (!CanCraftAffixes(AffixCraftAction::kReforge, regular, token) ||
                    !ReplaceSelectedAffix(after, token, 50u) ||
                    !IsValidCraftResult(AffixCraftAction::kReforge, before, after, rune, token)) return false;
                if (IsValidCraftResult(AffixCraftAction::kReforge, before, before, rune, token)) return false;
                if (ReplaceSelectedAffix(after, 50u, 50u)) return false;
            }
            // Scour rerolls in place: same slot count, runeword kept primary,
            // and an identical result is rejected before any orb is spent.
            InstanceAffixSlots empty{};
            if (rune) empty.AddToken(rune);
            InstanceAffixSlots scoured = empty;
            for (std::uint64_t token = 1u; token <= count; ++token) scoured.AddToken(token + 20u);
            if (IsValidCraftResult(AffixCraftAction::kScour, before, scoured, rune, 0u) != (count > 0)) return false;
            if (IsValidCraftResult(AffixCraftAction::kScour, before, before, rune, 0u)) return false;
            if (count > 0 && IsValidCraftResult(AffixCraftAction::kScour, before, empty, rune, 0u)) return false;
            InstanceAffixSlots identified = empty;
            identified.AddToken(8u);
            identified.AddToken(9u);
            if (!IsValidCraftResult(AffixCraftAction::kIdentify, empty, identified, rune, 0u)) return false;
        }
    }
    return true;
}
static_assert(CheckCurrencyCrafting(), "Identify/reforge/scour must preserve unselected slots and runewords");
}

namespace {
using CalamityAffixes::AffixCraftAction;
using CalamityAffixes::detail::CanCraftAffixLayout;
using CalamityAffixes::detail::IsCanonicalRegularAffixLayout;
using CalamityAffixes::detail::ScourAffixCount;
constexpr AffixCraftAction kAllCraftActions[]{
    AffixCraftAction::kIdentify, AffixCraftAction::kReforge, AffixCraftAction::kScour };
constexpr bool AllActionsAllowLayout(std::uint8_t count, std::uint8_t prefix, std::uint8_t suffix) {
    const auto head = CalamityAffixes::detail::ResolveAffixHead(false, prefix);
    for (const auto action : kAllCraftActions)
        if (!CanCraftAffixLayout(action, head, count, prefix, suffix, false)) return false;
    return true;
}
constexpr auto kNoHead = CalamityAffixes::AffixHead::kNone;
constexpr auto kPrefixHead = CalamityAffixes::AffixHead::kPrefix;
}
static_assert(IsCanonicalRegularAffixLayout(1u, 1u, 0u) && IsCanonicalRegularAffixLayout(2u, 1u, 1u) &&
    IsCanonicalRegularAffixLayout(3u, 1u, 2u), "every finished layout up to 1P+2S is canonical");
static_assert(!IsCanonicalRegularAffixLayout(0u, 0u, 0u) && !IsCanonicalRegularAffixLayout(1u, 0u, 1u) &&
    !IsCanonicalRegularAffixLayout(2u, 2u, 0u) && !IsCanonicalRegularAffixLayout(4u, 1u, 3u),
    "non-canonical splits are rejected");
static_assert(AllActionsAllowLayout(3u, 1u, 2u), "3-affix gear must stay craftable (regression: crafting-test1)");
static_assert(AllActionsAllowLayout(1u, 1u, 0u) && AllActionsAllowLayout(2u, 1u, 1u) && AllActionsAllowLayout(0u, 0u, 0u));
static_assert(!CanCraftAffixLayout(AffixCraftAction::kReforge, kNoHead, 1u, 0u, 1u, false) &&
    !CanCraftAffixLayout(AffixCraftAction::kIdentify, kNoHead, 1u, 0u, 1u, false) &&
    !CanCraftAffixLayout(AffixCraftAction::kReforge, kPrefixHead, 3u, 1u, 2u, true),
    "legacy layouts cannot be reforged or identified");
static_assert(CanCraftAffixLayout(AffixCraftAction::kScour, kNoHead, 1u, 0u, 1u, false) &&
    CanCraftAffixLayout(AffixCraftAction::kScour, kPrefixHead, 3u, 1u, 2u, true) &&
    CanCraftAffixLayout(AffixCraftAction::kScour, kNoHead, 4u, 0u, 0u, true), "scour repairs any legacy layout");
static_assert(ScourAffixCount(1u) == 1u && ScourAffixCount(3u) == 3u && ScourAffixCount(4u) == 3u,
    "scour keeps the slot count, clamping legacy overflow");

// Scroll trades are one way: Identify Scrolls in, a scarcer currency out.
static_assert(CalamityAffixes::detail::ResolveCurrencyExchange(CalamityAffixes::CurrencyExchange::kReforgeOrb).sourceCost == 3u &&
    CalamityAffixes::detail::ResolveCurrencyExchange(CalamityAffixes::CurrencyExchange::kScouringOrb).sourceCost == 10u,
    "exchange rates are 3 scrolls per Reforge Orb and 10 per Scouring Orb");
static_assert(CalamityAffixes::detail::ResolveCurrencyExchange(CalamityAffixes::CurrencyExchange::kReforgeOrb).sourceEditorId ==
        "CAFF_Misc_IdentifyScroll" &&
    CalamityAffixes::detail::ResolveCurrencyExchange(CalamityAffixes::CurrencyExchange::kScouringOrb).sourceEditorId ==
        "CAFF_Misc_IdentifyScroll",
    "every trade spends Identify Scrolls");
static_assert(CalamityAffixes::detail::ResolveCurrencyExchange(CalamityAffixes::CurrencyExchange::kReforgeOrb).targetEditorId ==
        "CAFF_Misc_ReforgeOrb" &&
    CalamityAffixes::detail::ResolveCurrencyExchange(CalamityAffixes::CurrencyExchange::kScouringOrb).targetEditorId ==
        "CAFF_Misc_ScouringOrb",
    "trades grant the matching orb");

// Selected reforge: 6 per item until the item is scoured or identified again.
using CalamityAffixes::detail::NextSelectedReforgeCount;
using CalamityAffixes::detail::RemainingSelectedReforges;
static_assert(CalamityAffixes::detail::kSelectedReforgesPerItem == 6u);
static_assert(RemainingSelectedReforges(0u) == 6u && RemainingSelectedReforges(5u) == 1u,
    "each selected reforge uses one of the item's six");
static_assert(RemainingSelectedReforges(6u) == 0u && RemainingSelectedReforges(0xFFu) == 0u,
    "a spent item stays locked until it is scoured");
static_assert(NextSelectedReforgeCount(0u) == 1u && NextSelectedReforgeCount(0xFEu) == 0xFFu &&
    NextSelectedReforgeCount(0xFFu) == 0xFFu, "the per-item count saturates instead of wrapping back to unlocked");

// v2.3.0: a runeword holds the head slot instead of sitting on top of the prefix.
namespace {
using CalamityAffixes::AffixHead;
using CalamityAffixes::detail::HeadSlotAffixCount;
using CalamityAffixes::detail::IsCanonicalAffixExpansionLayout;
using CalamityAffixes::detail::IsCanonicalAffixLayout;
using CalamityAffixes::detail::IsValidCraftResult;
using CalamityAffixes::detail::MaxRegularAffixCount;
using CalamityAffixes::detail::ResolveAffixHead;
using CalamityAffixes::detail::RunewordIdentifySuffixCount;
using CalamityAffixes::InstanceAffixSlots;

constexpr InstanceAffixSlots Slots(std::initializer_list<std::uint64_t> a_tokens) {
    InstanceAffixSlots slots{};
    for (const auto token : a_tokens) slots.AddToken(token);
    return slots;
}

constexpr bool CheckRunewordIdentifySplit() {
    std::uint32_t frequencies[2]{};
    for (std::uint32_t roll = 0; roll < 100; ++roll) ++frequencies[RunewordIdentifySuffixCount(roll) - 1];
    return frequencies[0] == 75 && frequencies[1] == 25;
}

// Runeword 99 on the head; prefixes 1-9, suffixes 11-19, new rolls 21+.
constexpr bool CheckRemoveRuneword() {
    // A runeword head (two suffix slots) gives its slot to a new prefix; suffixes keep their place.
    const auto head = Slots({ 99u, 11u, 12u });
    const auto remove = AffixCraftAction::kRemoveRuneword;
    if (!IsValidCraftResult(remove, head, Slots({ 21u, 11u, 12u }), 99u, 0u, 2u)) return false;
    if (IsValidCraftResult(remove, head, Slots({ 21u, 12u, 11u }), 99u, 0u, 2u)) return false;
    if (IsValidCraftResult(remove, head, Slots({ 11u, 12u }), 99u, 0u, 2u)) return false;
    if (IsValidCraftResult(remove, head, head, 99u, 0u, 2u)) return false;
    if (!IsValidCraftResult(remove, Slots({ 99u }), Slots({ 21u }), 99u, 0u, 2u)) return false;
    // A legacy runeword sat on a prefix: it just leaves, nothing is rolled.
    const auto legacy = Slots({ 99u, 1u, 11u });
    if (!IsValidCraftResult(remove, legacy, Slots({ 1u, 11u }), 99u, 0u, 3u)) return false;
    if (IsValidCraftResult(remove, legacy, Slots({ 11u, 1u }), 99u, 0u, 3u)) return false;
    if (IsValidCraftResult(remove, legacy, Slots({ 21u, 1u, 11u }), 99u, 0u, 3u)) return false;
    // Nothing to remove without a runeword.
    return !IsValidCraftResult(remove, Slots({ 1u, 11u }), Slots({ 21u, 11u }), 0u, 0u, 2u);
}

constexpr bool CheckRunewordHeadCrafting() {
    // Identify on a runeword-only item adds 1-2 suffixes, never a third regular affix.
    const auto bare = Slots({ 99u });
    if (!IsValidCraftResult(AffixCraftAction::kIdentify, bare, Slots({ 99u, 11u, 12u }), 99u, 0u, 2u)) return false;
    if (IsValidCraftResult(AffixCraftAction::kIdentify, bare, Slots({ 99u, 11u, 12u, 13u }), 99u, 0u, 2u)) return false;
    // Scour keeps the count and clamps legacy overflow to the two suffix slots.
    if (!IsValidCraftResult(AffixCraftAction::kScour, Slots({ 99u, 11u, 12u }), Slots({ 99u, 13u, 14u }), 99u, 0u, 2u)) return false;
    if (!IsValidCraftResult(AffixCraftAction::kScour, Slots({ 99u, 11u, 12u, 13u }), Slots({ 99u, 14u, 15u }), 99u, 0u, 2u)) return false;
    return !IsValidCraftResult(AffixCraftAction::kScour, Slots({ 99u, 11u, 12u, 13u }), Slots({ 99u, 14u, 15u, 16u }), 99u, 0u, 2u);
}
}
static_assert(ResolveAffixHead(false, 0u) == AffixHead::kNone && ResolveAffixHead(false, 1u) == AffixHead::kPrefix &&
    ResolveAffixHead(true, 0u) == AffixHead::kRuneword && ResolveAffixHead(true, 1u) == AffixHead::kLegacyRunewordPrefix);
static_assert(MaxRegularAffixCount(AffixHead::kRuneword) == 2u && MaxRegularAffixCount(AffixHead::kPrefix) == 3u &&
    MaxRegularAffixCount(AffixHead::kLegacyRunewordPrefix) == 3u, "a runeword head leaves two suffix slots");
static_assert(HeadSlotAffixCount(AffixHead::kRuneword, 0u) == 1u && HeadSlotAffixCount(AffixHead::kRuneword, 2u) == 3u &&
    HeadSlotAffixCount(AffixHead::kPrefix, 2u) == 2u && HeadSlotAffixCount(AffixHead::kLegacyRunewordPrefix, 3u) == 3u,
    "a runeword head fills one of the three slots; a legacy runeword sits outside them");
static_assert(CheckRunewordIdentifySplit(), "runeword identify rolls 1 suffix 75%, 2 suffixes 25%");
static_assert(IsCanonicalAffixLayout(AffixHead::kRuneword, 1u, 0u, 1u) && IsCanonicalAffixLayout(AffixHead::kRuneword, 2u, 0u, 2u) &&
    !IsCanonicalAffixLayout(AffixHead::kRuneword, 3u, 0u, 3u) && !IsCanonicalAffixLayout(AffixHead::kRuneword, 0u, 0u, 0u) &&
    IsCanonicalAffixLayout(AffixHead::kLegacyRunewordPrefix, 3u, 1u, 2u),
    "runeword heads take 1-2 suffixes; legacy items keep the 1P+2S rule");
static_assert(CanCraftAffixLayout(AffixCraftAction::kIdentify, AffixHead::kRuneword, 0u, 0u, 0u, false) &&
    CanCraftAffixLayout(AffixCraftAction::kReforge, AffixHead::kRuneword, 2u, 0u, 2u, false) &&
    CanCraftAffixLayout(AffixCraftAction::kScour, AffixHead::kRuneword, 2u, 0u, 2u, false) &&
    CanCraftAffixLayout(AffixCraftAction::kReforge, AffixHead::kLegacyRunewordPrefix, 3u, 1u, 2u, false),
    "runeword-head and legacy items stay craftable");
static_assert(CanCraftAffixLayout(AffixCraftAction::kRemoveRuneword, AffixHead::kRuneword, 0u, 0u, 0u, false) &&
    CanCraftAffixLayout(AffixCraftAction::kRemoveRuneword, AffixHead::kRuneword, 2u, 0u, 2u, false) &&
    CanCraftAffixLayout(AffixCraftAction::kRemoveRuneword, AffixHead::kLegacyRunewordPrefix, 2u, 1u, 1u, false) &&
    !CanCraftAffixLayout(AffixCraftAction::kRemoveRuneword, AffixHead::kPrefix, 2u, 1u, 1u, false) &&
    !CanCraftAffixLayout(AffixCraftAction::kRemoveRuneword, AffixHead::kNone, 0u, 0u, 0u, false) &&
    !CanCraftAffixLayout(AffixCraftAction::kRemoveRuneword, AffixHead::kRuneword, 3u, 0u, 3u, true),
    "removal needs a runeword and a layout it can keep");
static_assert(CheckRemoveRuneword(), "runeword removal changes only the head slot");
static_assert(CheckRunewordHeadCrafting(), "runeword-head identify and scour roll suffixes only");
static_assert(IsCanonicalAffixExpansionLayout(AffixHead::kRuneword, 0u, 0u, 0u) &&
    IsCanonicalAffixExpansionLayout(AffixHead::kRuneword, 1u, 0u, 1u) &&
    !IsCanonicalAffixExpansionLayout(AffixHead::kRuneword, 2u, 0u, 2u) &&
    IsCanonicalAffixExpansionLayout(AffixHead::kPrefix, 1u, 1u, 0u),
    "a runeword head expands like a prefix head");
static_assert(CalamityAffixes::detail::CraftCurrency(AffixCraftAction::kRemoveRuneword) == "CAFF_Misc_ScouringOrb",
    "removing a runeword costs a Scouring Orb");
