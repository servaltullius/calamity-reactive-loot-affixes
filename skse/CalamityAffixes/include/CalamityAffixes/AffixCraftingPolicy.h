#pragma once
#include "CalamityAffixes/InstanceAffixSlots.h"
#include "CalamityAffixes/LootRollSelection.h"
#include <cstdint>
#include <string_view>

namespace CalamityAffixes
{
	// kRemoveRuneword takes the runeword off and rolls a prefix into the head slot it frees.
	enum class AffixCraftAction : std::uint8_t { kIdentify, kReforge, kScour, kRemoveRuneword };

	// An item has one head slot, held by a runeword or by a prefix, then up to
	// two suffixes (v2.3.0). A runeword used to sit on top of the prefix, so it
	// was always a pure gain; sharing the slot makes it a choice. Items that got
	// a runeword over a prefix before 2.3.0 keep that shape and its old rules
	// (kLegacyRunewordPrefix) until they are transmuted again or the runeword is
	// removed; nothing creates that shape any more.
	enum class AffixHead : std::uint8_t { kNone, kPrefix, kRuneword, kLegacyRunewordPrefix };
	enum class CurrencyExchange : std::uint8_t { kReforgeOrb, kScouringOrb };
	namespace detail
	{
		inline constexpr std::uint32_t kSelectedReforgeCost = 2u;
		// Selected reforge may be used 6 times per item; identifying or scouring the
		// item starts the count over. Without a limit, keeping every other affix
		// always beat rerolling all of them, so the rarer Scouring Orb had no job.
		inline constexpr std::uint8_t kSelectedReforgesPerItem = 6u;

		[[nodiscard]] constexpr std::uint8_t RemainingSelectedReforges(std::uint8_t a_reforgesDone) noexcept
		{
			return a_reforgesDone >= kSelectedReforgesPerItem ?
				0u :
				static_cast<std::uint8_t>(kSelectedReforgesPerItem - a_reforgesDone);
		}

		[[nodiscard]] constexpr std::uint8_t NextSelectedReforgeCount(std::uint8_t a_reforgesDone) noexcept
		{
			return a_reforgesDone == 0xFFu ? a_reforgesDone : static_cast<std::uint8_t>(a_reforgesDone + 1u);
		}

		[[nodiscard]] constexpr std::uint8_t IdentifyAffixCount(std::uint32_t a_roll) noexcept
		{
			return a_roll < 60u ? 1u : (a_roll < 90u ? 2u : 3u);
		}

		// The runeword already fills the head slot, so identify rolls suffixes only.
		// 75/25 is the 30:10 split of the prefix roll's 2- and 3-affix outcomes,
		// and the scroll always adds at least one affix.
		[[nodiscard]] constexpr std::uint8_t RunewordIdentifySuffixCount(std::uint32_t a_roll) noexcept
		{
			return a_roll < 75u ? 1u : 2u;
		}

		[[nodiscard]] constexpr AffixHead ResolveAffixHead(bool a_hasRuneword, std::uint8_t a_prefixCount) noexcept
		{
			if (a_hasRuneword) {
				return a_prefixCount > 0u ? AffixHead::kLegacyRunewordPrefix : AffixHead::kRuneword;
			}
			return a_prefixCount > 0u ? AffixHead::kPrefix : AffixHead::kNone;
		}

		// Regular (non-runeword) affixes the layout can hold.
		[[nodiscard]] constexpr std::uint8_t MaxRegularAffixCount(AffixHead a_head) noexcept
		{
			return a_head == AffixHead::kRuneword ?
				static_cast<std::uint8_t>(kMaxRegularAffixesPerItem - 1u) :
				static_cast<std::uint8_t>(kMaxRegularAffixesPerItem);
		}

		// Filled slots of the three, counting a runeword that holds the head slot.
		// A legacy runeword sits outside the three, as it always did.
		[[nodiscard]] constexpr std::uint8_t HeadSlotAffixCount(AffixHead a_head, std::uint8_t a_regularCount) noexcept
		{
			return a_head == AffixHead::kRuneword ? static_cast<std::uint8_t>(a_regularCount + 1u) : a_regularCount;
		}

		[[nodiscard]] constexpr std::string_view CraftCurrency(AffixCraftAction a_action) noexcept
		{
			switch (a_action) {
			case AffixCraftAction::kIdentify: return "CAFF_Misc_IdentifyScroll";
			case AffixCraftAction::kReforge: return "CAFF_Misc_ReforgeOrb";
			case AffixCraftAction::kScour:
			case AffixCraftAction::kRemoveRuneword: return "CAFF_Misc_ScouringOrb";
			}
			return {};
		}

		inline constexpr std::uint8_t kCraftRollMaxAttempts = 4u;

		// A complete regular layout: 1 Prefix + (count - 1) Suffixes, up to 3.
		// Not IsCanonicalRegularAffixExpansionLayout, which only accepts the
		// expandable 1-2 layouts and would reject every finished 3-affix base.
		[[nodiscard]] constexpr bool IsCanonicalRegularAffixLayout(
			std::uint8_t a_count, std::uint8_t a_prefixCount, std::uint8_t a_suffixCount) noexcept
		{
			return a_count >= 1u && a_count <= kMaxRegularAffixesPerItem &&
				a_prefixCount == 1u && a_suffixCount + 1u == a_count;
		}

		// A runeword holding the head slot leaves room for 1-2 suffixes and no
		// prefix. Every other head keeps the 1 Prefix + Suffixes rule above; a
		// legacy runeword-over-prefix item is not counted in its regular layout.
		[[nodiscard]] constexpr bool IsCanonicalAffixLayout(
			AffixHead a_head, std::uint8_t a_count, std::uint8_t a_prefixCount, std::uint8_t a_suffixCount) noexcept
		{
			if (a_head == AffixHead::kRuneword) {
				return a_count >= 1u && a_count <= MaxRegularAffixCount(a_head) &&
					a_prefixCount == 0u && a_suffixCount == a_count;
			}
			return IsCanonicalRegularAffixLayout(a_count, a_prefixCount, a_suffixCount);
		}

		// Expansion adds one suffix to a head-only or head + 1 suffix item. The
		// expansion count and its orb cost follow HeadSlotAffixCount, so a
		// runeword head grows exactly like a prefix head.
		[[nodiscard]] constexpr bool IsCanonicalAffixExpansionLayout(
			AffixHead a_head, std::uint8_t a_regularCount, std::uint8_t a_prefixCount, std::uint8_t a_suffixCount) noexcept
		{
			if (a_head == AffixHead::kRuneword) {
				return a_regularCount <= 1u && a_prefixCount == 0u && a_suffixCount == a_regularCount;
			}
			return IsCanonicalRegularAffixExpansionLayout(a_regularCount, a_prefixCount, a_suffixCount);
		}

		// Scour is the recovery path: it rerolls legacy layouts (unknown or
		// unslotted tokens, duplicate suffix families, other splits) into a
		// canonical one. Identify, selected reforge and runeword removal need a
		// canonical base; removal also needs the runeword to be there.
		[[nodiscard]] constexpr bool CanCraftAffixLayout(
			AffixCraftAction a_action, AffixHead a_head, std::uint8_t a_regularCount, std::uint8_t a_prefixCount,
			std::uint8_t a_suffixCount, bool a_hasLegacyAffix) noexcept
		{
			const bool canonical = !a_hasLegacyAffix &&
				IsCanonicalAffixLayout(a_head, a_regularCount, a_prefixCount, a_suffixCount);
			if (a_action == AffixCraftAction::kRemoveRuneword) {
				return (a_head == AffixHead::kRuneword && a_regularCount == 0u) ||
					((a_head == AffixHead::kRuneword || a_head == AffixHead::kLegacyRunewordPrefix) && canonical);
			}
			return a_regularCount == 0u || a_action == AffixCraftAction::kScour || canonical;
		}

		// Scour keeps the slot count; legacy overflow is clamped to what the head allows.
		[[nodiscard]] constexpr std::uint8_t ScourAffixCount(
			std::uint8_t a_regularCount, std::uint8_t a_maxRegular = kMaxRegularAffixesPerItem) noexcept
		{
			return a_regularCount < a_maxRegular ? a_regularCount : a_maxRegular;
		}

		// Identify Scrolls have no use once gear is identified (scour keeps the slot
		// count), so they pile up. One-way trades turn them into the currencies that
		// run out: 3:1 follows the 20% : 12% drop ratio with a small loss. 10:1
		// (15:1 before the reforge limit) keeps the Scouring Orb scarcer than a
		// Reforge Orb but reachable, since it is now how a spent item unlocks.
		inline constexpr std::uint32_t kExchangeScrollsPerReforgeOrb = 3u;
		inline constexpr std::uint32_t kExchangeScrollsPerScouringOrb = 10u;

		struct CurrencyExchangeRecipe
		{
			std::string_view sourceEditorId;
			std::string_view targetEditorId;
			std::uint32_t sourceCost{ 0u };
		};

		[[nodiscard]] constexpr CurrencyExchangeRecipe ResolveCurrencyExchange(CurrencyExchange a_exchange) noexcept
		{
			switch (a_exchange) {
			case CurrencyExchange::kReforgeOrb:
				return { CraftCurrency(AffixCraftAction::kIdentify), CraftCurrency(AffixCraftAction::kReforge),
					kExchangeScrollsPerReforgeOrb };
			case CurrencyExchange::kScouringOrb:
				return { CraftCurrency(AffixCraftAction::kIdentify), CraftCurrency(AffixCraftAction::kScour),
					kExchangeScrollsPerScouringOrb };
			}
			return {};
		}

		[[nodiscard]] constexpr bool CanCraftAffixes(
			AffixCraftAction a_action, const InstanceAffixSlots& a_regular,
			std::uint64_t a_selectedToken) noexcept
		{
			switch (a_action) {
			case AffixCraftAction::kIdentify: return a_regular.count == 0u && a_selectedToken == 0u;
			case AffixCraftAction::kReforge:
				return a_regular.count <= kMaxRegularAffixesPerItem && a_selectedToken != 0u &&
					a_regular.HasToken(a_selectedToken);
			case AffixCraftAction::kScour: return a_regular.count > 0u && a_selectedToken == 0u;
			case AffixCraftAction::kRemoveRuneword: return a_selectedToken == 0u;
			}
			return false;
		}

		// Replacing a slot must preserve its position and every unselected token.
		[[nodiscard]] constexpr bool ReplaceSelectedAffix(
			InstanceAffixSlots& a_slots, std::uint64_t a_old, std::uint64_t a_new) noexcept
		{
			if (a_old == 0u || a_new == 0u || a_slots.HasToken(a_new)) return false;
			for (std::uint8_t i = 0; i < a_slots.count; ++i) {
				if (a_slots.tokens[i] == a_old) {
					a_slots.tokens[i] = a_new;
					return true;
				}
			}
			return false;
		}

		// a_maxRegular is MaxRegularAffixCount of the item's head before the craft.
		[[nodiscard]] constexpr bool IsValidCraftResult(
			AffixCraftAction action, const InstanceAffixSlots& before, const InstanceAffixSlots& after,
			std::uint64_t runeword, std::uint64_t selected,
			std::uint8_t a_maxRegular = kMaxRegularAffixesPerItem) noexcept
		{
			if (before.count > kMaxAffixesPerItem || after.count > kMaxAffixesPerItem) return false;
			for (std::uint8_t i = 0; i < after.count; ++i) {
				if (after.tokens[i] == 0u) return false;
				for (std::uint8_t j = 0; j < i; ++j)
					if (after.tokens[i] == after.tokens[j]) return false;
			}
			if (action == AffixCraftAction::kRemoveRuneword) {
				// The runeword leaves; every other token stays where it was. When the
				// runeword held the head slot (a_maxRegular below the full three), a new
				// prefix takes that slot; a legacy runeword sat on a prefix and just leaves.
				if (runeword == 0u || before.GetPrimary() != runeword || after.HasToken(runeword)) return false;
				if (a_maxRegular < kMaxRegularAffixesPerItem) {
					if (after.count != before.count) return false;
					if (before.HasToken(after.tokens[0])) return false;
					for (std::uint8_t i = 1; i < after.count; ++i)
						if (after.tokens[i] != before.tokens[i]) return false;
					return true;
				}
				if (after.count + 1u != before.count) return false;
				for (std::uint8_t i = 0; i < after.count; ++i)
					if (after.tokens[i] != before.tokens[i + 1u]) return false;
				return true;
			}
			if (runeword != 0u && (before.GetPrimary() != runeword || after.GetPrimary() != runeword)) return false;
			const auto runeCount = runeword != 0u ? 1u : 0u;
			switch (action) {
			case AffixCraftAction::kIdentify:
				return before.count == runeCount && after.count > runeCount && after.count <= runeCount + a_maxRegular;
			case AffixCraftAction::kScour: {
				if (before.count <= runeCount) return false;
				const auto target = ScourAffixCount(static_cast<std::uint8_t>(before.count - runeCount), a_maxRegular);
				if (after.count != runeCount + target) return false;
				// An identical reroll would spend a rare orb for nothing.
				if (after.count != before.count) return true;
				for (std::uint8_t i = 0; i < after.count; ++i)
					if (!before.HasToken(after.tokens[i])) return true;
				return false;
			}
			case AffixCraftAction::kReforge:
				if (selected == 0u || selected == runeword || !before.HasToken(selected) ||
					after.HasToken(selected) || before.count != after.count) return false;
				for (std::uint8_t i = 0; i < before.count; ++i)
					if (before.tokens[i] != selected && before.tokens[i] != after.tokens[i]) return false;
				return true;
			case AffixCraftAction::kRemoveRuneword:
				return false;
			}
			return false;
		}
	}
}
