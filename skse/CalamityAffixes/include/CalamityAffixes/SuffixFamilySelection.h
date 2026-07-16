#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

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

	enum class PassiveSpellReconcileAction : std::uint8_t
	{
		kKeep,
		kAdd,
		kRemove,
		kRefresh
	};

	[[nodiscard]] constexpr PassiveSpellReconcileAction ResolvePassiveSpellReconcileAction(
		bool a_desired,
		bool a_present,
		bool a_passivesDisabled,
		bool a_refreshRequested) noexcept
	{
		if (a_passivesDisabled || !a_desired) {
			return a_present ? PassiveSpellReconcileAction::kRemove : PassiveSpellReconcileAction::kKeep;
		}
		if (!a_present) {
			return PassiveSpellReconcileAction::kAdd;
		}
		return a_refreshRequested ? PassiveSpellReconcileAction::kRefresh : PassiveSpellReconcileAction::kKeep;
	}
}
