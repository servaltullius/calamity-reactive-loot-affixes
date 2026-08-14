#pragma once

#include "CalamityAffixes/RunewordUiContracts.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace CalamityAffixes
{
	struct LockedReforgeCommandKeys
	{
		std::uint64_t expectedInstanceKey{ 0u };
		std::uint64_t affixToken{ 0u };

		[[nodiscard]] constexpr bool operator==(const LockedReforgeCommandKeys&) const noexcept = default;
	};

	struct AffixExpandCommandKeys
	{
		std::uint64_t expectedInstanceKey{ 0u };
		std::uint8_t expectedRegularAffixCount{ 0u };

		[[nodiscard]] constexpr bool operator==(const AffixExpandCommandKeys&) const noexcept = default;
	};

	[[nodiscard]] constexpr std::optional<std::uint64_t> ParsePositiveDecimalUint64(
		std::string_view a_text) noexcept
	{
		if (a_text.empty()) {
			return std::nullopt;
		}

		std::uint64_t value = 0u;
		for (const char ch : a_text) {
			if (ch < '0' || ch > '9') {
				return std::nullopt;
			}
			const auto digit = static_cast<std::uint64_t>(ch - '0');
			if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10u) {
				return std::nullopt;
			}
			value = value * 10u + digit;
		}
		return value != 0u ? std::optional<std::uint64_t>{ value } : std::nullopt;
	}

	[[nodiscard]] constexpr std::optional<LockedReforgeCommandKeys> ParseLockedReforgeCommandKeys(
		std::string_view a_payload) noexcept
	{
		const auto separator = a_payload.find(':');
		if (separator == std::string_view::npos ||
			a_payload.find(':', separator + 1u) != std::string_view::npos) {
			return std::nullopt;
		}

		const auto expectedInstanceKey = ParsePositiveDecimalUint64(a_payload.substr(0u, separator));
		const auto affixToken = ParsePositiveDecimalUint64(a_payload.substr(separator + 1u));
		if (!expectedInstanceKey || !affixToken) {
			return std::nullopt;
		}
		return LockedReforgeCommandKeys{
			.expectedInstanceKey = *expectedInstanceKey,
			.affixToken = *affixToken,
		};
	}

	[[nodiscard]] constexpr std::optional<AffixExpandCommandKeys> ParseAffixExpandCommandKeys(
		std::string_view a_payload) noexcept
	{
		const auto separator = a_payload.find(':');
		if (separator == std::string_view::npos ||
			a_payload.find(':', separator + 1u) != std::string_view::npos) {
			return std::nullopt;
		}

		const auto expectedInstanceKey = ParsePositiveDecimalUint64(a_payload.substr(0u, separator));
		const auto expectedRegularAffixCount = ParsePositiveDecimalUint64(a_payload.substr(separator + 1u));
		if (!expectedInstanceKey || !expectedRegularAffixCount ||
			(*expectedRegularAffixCount != 1u && *expectedRegularAffixCount != 2u)) {
			return std::nullopt;
		}

		return AffixExpandCommandKeys{
			.expectedInstanceKey = *expectedInstanceKey,
			.expectedRegularAffixCount = static_cast<std::uint8_t>(*expectedRegularAffixCount),
		};
	}

	[[nodiscard]] constexpr bool IsSameCompletedRuneword(
		std::uint64_t a_completedResultAffixToken,
		std::uint64_t a_selectedResultAffixToken) noexcept
	{
		return a_completedResultAffixToken != 0u &&
		       a_completedResultAffixToken == a_selectedResultAffixToken;
	}

	[[nodiscard]] constexpr bool ShouldClearRunewordInProgressState(bool a_isCompletedBase) noexcept
	{
		return a_isCompletedBase;
	}

	[[nodiscard]] constexpr bool CanFinalizeRunewordFromPanel(
		std::uint32_t a_insertedRunes,
		std::uint32_t a_totalRunes,
		bool a_canApplyResult) noexcept
	{
		return a_insertedRunes >= a_totalRunes && a_canApplyResult;
	}

	[[nodiscard]] constexpr bool CanInsertRunewordFromPanel(bool a_hasAllRequiredRunes, bool a_canApplyResult) noexcept
	{
		return a_hasAllRequiredRunes && a_canApplyResult;
	}

	[[nodiscard]] constexpr std::vector<std::uint64_t> NormalizeRunewordRuneInventoryTokens(
		std::span<const std::uint64_t> a_tokens)
	{
		std::vector<std::uint64_t> normalized(a_tokens.begin(), a_tokens.end());

		std::ranges::sort(normalized);
		normalized.erase(std::unique(normalized.begin(), normalized.end()), normalized.end());
		return normalized;
	}

	[[nodiscard]] inline std::vector<std::uint64_t> BuildRunewordRuneDashboardTokens(
		std::span<const std::uint64_t> a_catalogTokens,
		std::span<const std::uint64_t> a_materialFilterTokens)
	{
		std::vector<std::uint64_t> combined;
		combined.reserve(a_catalogTokens.size() + a_materialFilterTokens.size());
		combined.insert(combined.end(), a_catalogTokens.begin(), a_catalogTokens.end());
		combined.insert(combined.end(), a_materialFilterTokens.begin(), a_materialFilterTokens.end());
		return NormalizeRunewordRuneInventoryTokens(combined);
	}

	template <class ResolveOwned>
	[[nodiscard]] std::optional<std::vector<RunewordRuneInventoryEntry>> BuildRunewordRuneInventorySnapshot(
		std::span<const std::uint64_t> a_referencedTokens,
		ResolveOwned&& a_resolveOwned)
	{
		const auto inventoryTokens = NormalizeRunewordRuneInventoryTokens(a_referencedTokens);
		if (inventoryTokens.empty()) {
			return std::nullopt;
		}
		std::vector<RunewordRuneInventoryEntry> snapshot;
		snapshot.reserve(inventoryTokens.size());
		for (const auto runeToken : inventoryTokens) {
			const std::optional<std::uint32_t> owned = a_resolveOwned(runeToken);
			if (!owned) {
				return std::nullopt;
			}
			snapshot.push_back({ .runeToken = runeToken, .owned = *owned });
		}
		return snapshot;
	}

	template <class ResolveName>
	[[nodiscard]] bool PopulateRunewordRuneInventoryNames(
		std::span<RunewordRuneInventoryEntry> a_entries,
		ResolveName&& a_resolveName)
	{
		for (auto& entry : a_entries) {
			const std::optional<std::string_view> name = a_resolveName(entry.runeToken);
			if (!name || name->empty()) {
				return false;
			}
			entry.runeName.assign(*name);
		}
		return true;
	}
}
