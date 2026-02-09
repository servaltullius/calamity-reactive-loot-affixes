#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>

namespace CalamityAffixes
{
	struct TooltipResolutionCandidate
	{
		std::string_view rowName{};
		std::uint64_t affixToken{ 0 };
		std::uint64_t instanceKey{ 0 };
	};

	struct TooltipSelectionResolution
	{
		std::optional<std::size_t> matchedIndex{};
		bool ambiguous{ false };
	};

	[[nodiscard]] constexpr TooltipSelectionResolution ResolveTooltipCandidateSelection(
		std::span<const TooltipResolutionCandidate> a_candidates,
		std::string_view a_selectedDisplayName) noexcept
	{
		TooltipSelectionResolution resolution{};
		if (a_selectedDisplayName.empty()) {
			if (!a_candidates.empty()) {
				resolution.matchedIndex = 0u;
			}
			return resolution;
		}

		for (std::size_t index = 0; index < a_candidates.size(); ++index) {
			const auto& candidate = a_candidates[index];
			if (candidate.rowName.empty() || candidate.rowName != a_selectedDisplayName) {
				continue;
			}

			if (!resolution.matchedIndex) {
				resolution.matchedIndex = index;
				continue;
			}

			const auto& previous = a_candidates[*resolution.matchedIndex];
			if (previous.affixToken != candidate.affixToken || previous.instanceKey != candidate.instanceKey) {
				resolution.matchedIndex.reset();
				resolution.ambiguous = true;
				return resolution;
			}
		}

		return resolution;
	}

	[[nodiscard]] constexpr std::optional<std::uint32_t> ResolveRunewordBaseCycleCursor(
		std::span<const std::uint64_t> a_candidates,
		std::uint64_t a_instanceKey) noexcept
	{
		if (a_instanceKey == 0u || a_candidates.empty()) {
			return std::nullopt;
		}

		for (std::size_t index = 0; index < a_candidates.size(); ++index) {
			if (a_candidates[index] != a_instanceKey) {
				continue;
			}

			if (index > std::numeric_limits<std::uint32_t>::max()) {
				return std::nullopt;
			}

			return static_cast<std::uint32_t>(index);
		}

		return std::nullopt;
	}
}
