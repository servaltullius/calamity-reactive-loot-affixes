#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>

namespace CalamityAffixes
{
	namespace detail
	{
		[[nodiscard]] constexpr std::size_t SkipLeadingSpaces(std::string_view a_text) noexcept
		{
			std::size_t pos = 0;
			while (pos < a_text.size() && a_text[pos] == ' ') {
				++pos;
			}
			return pos;
		}

		[[nodiscard]] constexpr bool HasUtf8StarAt(std::string_view a_text, std::size_t a_pos) noexcept
		{
			return a_pos + 2 < a_text.size() &&
			       static_cast<unsigned char>(a_text[a_pos]) == 0xE2 &&
			       static_cast<unsigned char>(a_text[a_pos + 1]) == 0x98 &&
			       static_cast<unsigned char>(a_text[a_pos + 2]) == 0x85;
		}

		[[nodiscard]] constexpr std::size_t ConsumeAsciiStarMarkerLen(std::string_view a_text, std::size_t a_pos) noexcept
		{
			std::size_t count = 0;
			std::size_t pos = a_pos;
			while (pos < a_text.size() && a_text[pos] == '*') {
				++count;
				++pos;
			}

			if (count == 0 || count > 3) {
				return 0;
			}

			if (pos >= a_text.size() || a_text[pos] != ' ') {
				return 0;
			}

			// Include the trailing marker delimiter space.
			return count + 1;
		}
	}

	[[nodiscard]] constexpr bool HasLeadingLootStarPrefix(std::string_view a_name) noexcept
	{
		const auto pos = detail::SkipLeadingSpaces(a_name);
		return detail::HasUtf8StarAt(a_name, pos) || detail::ConsumeAsciiStarMarkerLen(a_name, pos) > 0;
	}

	[[nodiscard]] constexpr std::string_view StripLeadingLootStarPrefix(std::string_view a_name) noexcept
	{
		std::size_t pos = detail::SkipLeadingSpaces(a_name);

		if (detail::HasUtf8StarAt(a_name, pos)) {
			while (detail::HasUtf8StarAt(a_name, pos)) {
				pos += 3;
			}
			while (pos < a_name.size() && a_name[pos] == ' ') {
				++pos;
			}
			return a_name.substr(pos);
		}

		if (const auto markerLen = detail::ConsumeAsciiStarMarkerLen(a_name, pos); markerLen > 0) {
			pos += markerLen;
			while (pos < a_name.size() && a_name[pos] == ' ') {
				++pos;
			}
			return a_name.substr(pos);
		}

		// Keep legacy behavior: leading spaces are trimmed even when no marker is present.
		return a_name.substr(pos);
	}

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
