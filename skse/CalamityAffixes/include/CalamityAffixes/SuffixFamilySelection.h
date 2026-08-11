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
