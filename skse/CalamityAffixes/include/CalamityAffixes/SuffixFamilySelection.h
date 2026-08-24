#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace CalamityAffixes::detail
{
	// Suffix data uses a stable trailing `_tN` contract (for example,
	// `suffix_vitality_t3`). A missing or malformed suffix is rank 0 so legacy
	// family entries remain deterministic without outranking an explicit tier.
	[[nodiscard]] constexpr std::uint32_t ParseSuffixTierRank(std::string_view a_id) noexcept
	{
		const auto marker = a_id.rfind("_t");
		if (marker == std::string_view::npos || marker + 2u >= a_id.size()) {
			return 0u;
		}

		std::uint32_t rank = 0u;
		for (std::size_t i = marker + 2u; i < a_id.size(); ++i) {
			const char ch = a_id[i];
			if (ch < '0' || ch > '9') {
				return 0u;
			}

			const auto digit = static_cast<std::uint32_t>(ch - '0');
			if (rank > (std::numeric_limits<std::uint32_t>::max() - digit) / 10u) {
				return 0u;
			}
			rank = (rank * 10u) + digit;
		}

		return rank;
	}

	[[nodiscard]] constexpr bool IsPreferredSuffixFamilyCandidate(
		std::string_view a_candidateId,
		std::size_t a_candidateIndex,
		std::string_view a_currentId,
		std::size_t a_currentIndex) noexcept
	{
		const auto candidateRank = ParseSuffixTierRank(a_candidateId);
		const auto currentRank = ParseSuffixTierRank(a_currentId);
		if (candidateRank != currentRank) {
			return candidateRank > currentRank;
		}

		// Same-rank or legacy rank-0 entries use config order as a stable tie-break.
		return a_candidateIndex < a_currentIndex;
	}

	struct SuffixFamilyBestCandidate
	{
		bool selected{ false };
		std::string_view id{};
		std::size_t index{ 0u };

		constexpr void Consider(std::string_view a_id, std::size_t a_index) noexcept
		{
			if (!selected || IsPreferredSuffixFamilyCandidate(a_id, a_index, id, index)) {
				selected = true;
				id = a_id;
				index = a_index;
			}
		}
	};

	inline constexpr std::uint32_t kMaxStackedSuffixTierRank = 3u;

	[[nodiscard]] constexpr std::uint32_t AccumulateSuffixFamilyRankPoints(
		std::uint32_t a_currentPoints,
		std::uint32_t a_rank,
		std::uint32_t a_equippedCount) noexcept
	{
		if (a_rank == 0u || a_equippedCount == 0u) {
			return a_currentPoints;
		}

		constexpr auto kMax = std::numeric_limits<std::uint32_t>::max();
		if (a_equippedCount > (kMax - a_currentPoints) / a_rank) {
			return kMax;
		}
		return a_currentPoints + (a_rank * a_equippedCount);
	}

	// Tiered suffix families add equipped rank points and then resolve one existing
	// configured tier, capped at T3. The best equipped member remains available as
	// the UI representative even when the effective runtime tier is promoted to an
	// otherwise unequipped definition.
	struct SuffixFamilyRankSelection
	{
		std::uint32_t rankPoints{ 0u };
		std::uint32_t maxConfiguredRank{ 0u };
		SuffixFamilyBestCandidate bestEquipped{};
		SuffixFamilyBestCandidate effective{};

		constexpr void ConsiderDefinition(std::string_view a_id) noexcept
		{
			maxConfiguredRank = std::max(maxConfiguredRank, ParseSuffixTierRank(a_id));
		}

		constexpr void ConsiderEquipped(
			std::string_view a_id,
			std::size_t a_index,
			std::uint32_t a_equippedCount) noexcept
		{
			if (a_equippedCount == 0u) {
				return;
			}
			bestEquipped.Consider(a_id, a_index);
			rankPoints = AccumulateSuffixFamilyRankPoints(
				rankPoints,
				ParseSuffixTierRank(a_id),
				a_equippedCount);
		}

		[[nodiscard]] constexpr std::uint32_t EffectiveRank() const noexcept
		{
			return std::min({ rankPoints, maxConfiguredRank, kMaxStackedSuffixTierRank });
		}

		constexpr void ConsiderEffectiveDefinition(
			std::string_view a_id,
			std::size_t a_index) noexcept
		{
			const auto rank = ParseSuffixTierRank(a_id);
			if (rank > 0u && rank <= EffectiveRank()) {
				effective.Consider(a_id, a_index);
			}
		}

		[[nodiscard]] constexpr SuffixFamilyBestCandidate ResolveEffectiveCandidate() const noexcept
		{
			// Preserve deterministic legacy family behavior for malformed/rank-0 IDs.
			return effective.selected ? effective : bestEquipped;
		}

		[[nodiscard]] constexpr std::uint32_t ResolvedEffectiveRank() const noexcept
		{
			const auto candidate = ResolveEffectiveCandidate();
			return candidate.selected ? ParseSuffixTierRank(candidate.id) : 0u;
		}
	};

	[[nodiscard]] constexpr std::size_t ResolveSuffixFamilyContributionSourceIndex(
		bool a_isFamilyRepresentative,
		SuffixFamilyBestCandidate a_effectiveCandidate,
		std::size_t a_equippedAffixIndex) noexcept
	{
		return a_isFamilyRepresentative && a_effectiveCandidate.selected ?
			a_effectiveCandidate.index :
			a_equippedAffixIndex;
	}

	struct SuffixFamilyClassification
	{
		bool isSuffix{ false };
		bool hasFamily{ false };
	};

	[[nodiscard]] constexpr bool ShouldDeferTieredSuffixFamily(
		SuffixFamilyClassification a_classification) noexcept
	{
		return a_classification.isSuffix && a_classification.hasFamily;
	}

	[[nodiscard]] constexpr bool ShouldAccumulateFamilylessSuffixValue(
		SuffixFamilyClassification a_classification) noexcept
	{
		return a_classification.isSuffix && !a_classification.hasFamily;
	}

	[[nodiscard]] inline bool IsAffixFamilyAvailable(
		std::span<const std::string> a_selectedFamilies,
		std::string_view a_candidateFamily) noexcept
	{
		return a_candidateFamily.empty() ||
		       std::find(a_selectedFamilies.begin(), a_selectedFamilies.end(), a_candidateFamily) ==
			       a_selectedFamilies.end();
	}

	inline void RecordSelectedAffixFamily(
		std::vector<std::string>& a_selectedFamilies,
		std::string_view a_selectedFamily)
	{
		if (!a_selectedFamily.empty()) {
			a_selectedFamilies.emplace_back(a_selectedFamily);
		}
	}

	enum class PassiveSpellReconcileAction : std::uint8_t
	{
		kKeep,
		kAdd,
		kRemove,
		kRefresh
	};

	struct PassiveSpellReconcileInput
	{
		bool desired{ false };
		bool present{ false };
		bool passivesDisabled{ false };
		bool refreshRequested{ false };
	};

	[[nodiscard]] constexpr PassiveSpellReconcileAction ResolvePassiveSpellReconcileAction(
		PassiveSpellReconcileInput a_input) noexcept
	{
		if (a_input.passivesDisabled || !a_input.desired) {
			return a_input.present ? PassiveSpellReconcileAction::kRemove : PassiveSpellReconcileAction::kKeep;
		}
		if (!a_input.present) {
			return PassiveSpellReconcileAction::kAdd;
		}
		return a_input.refreshRequested ? PassiveSpellReconcileAction::kRefresh : PassiveSpellReconcileAction::kKeep;
	}
}
