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
		[[nodiscard]] constexpr std::string_view TrimLeadingAsciiWhitespace(std::string_view a_text) noexcept
		{
			std::size_t pos = 0;
			while (pos < a_text.size()) {
				const char ch = a_text[pos];
				if (ch != ' ' && ch != '\t' && ch != '\r') {
					break;
				}
				++pos;
			}
			return a_text.substr(pos);
		}

		[[nodiscard]] constexpr char AsciiLower(char a_ch) noexcept
		{
			return (a_ch >= 'A' && a_ch <= 'Z') ? static_cast<char>(a_ch - 'A' + 'a') : a_ch;
		}

		[[nodiscard]] constexpr bool StartsWithAsciiInsensitive(std::string_view a_text, std::string_view a_prefix) noexcept
		{
			if (a_prefix.size() > a_text.size()) {
				return false;
			}

			for (std::size_t i = 0; i < a_prefix.size(); ++i) {
				if (AsciiLower(a_text[i]) != AsciiLower(a_prefix[i])) {
					return false;
				}
			}

			return true;
		}

		[[nodiscard]] constexpr std::size_t SkipLeadingSpaces(std::string_view a_text) noexcept
		{
			std::size_t pos = 0;
			while (pos < a_text.size() && a_text[pos] == ' ') {
				++pos;
			}
			return pos;
		}

		[[nodiscard]] constexpr std::size_t SkipTrailingSpaces(std::string_view a_text) noexcept
		{
			std::size_t end = a_text.size();
			while (end > 0 && a_text[end - 1] == ' ') {
				--end;
			}
			return end;
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

	[[nodiscard]] constexpr bool HasTrailingLootStarMarker(std::string_view a_name) noexcept
	{
		const auto trimmedEnd = detail::SkipTrailingSpaces(a_name);
		if (trimmedEnd == 0) {
			return false;
		}

		std::size_t utf8Count = 0;
		std::size_t utf8Pos = trimmedEnd;
		while (utf8Pos >= 3 && detail::HasUtf8StarAt(a_name, utf8Pos - 3)) {
			++utf8Count;
			utf8Pos -= 3;
		}
		if (utf8Count >= 1 && utf8Count <= 3) {
			return true;
		}

		std::size_t asciiCount = 0;
		std::size_t asciiPos = trimmedEnd;
		while (asciiPos > 0 && a_name[asciiPos - 1] == '*') {
			++asciiCount;
			--asciiPos;
		}
		return asciiCount >= 1 && asciiCount <= 3;
	}

	[[nodiscard]] constexpr std::string_view StripTrailingLootStarMarker(std::string_view a_name) noexcept
	{
		const auto trimmedEnd = detail::SkipTrailingSpaces(a_name);
		if (trimmedEnd == 0) {
			return a_name.substr(0, trimmedEnd);
		}

		std::size_t markerPos = trimmedEnd;
		std::size_t utf8Count = 0;
		while (markerPos >= 3 && detail::HasUtf8StarAt(a_name, markerPos - 3)) {
			++utf8Count;
			markerPos -= 3;
		}
		if (utf8Count >= 1 && utf8Count <= 3) {
			while (markerPos > 0 && a_name[markerPos - 1] == ' ') {
				--markerPos;
			}
			return a_name.substr(0, markerPos);
		}

		markerPos = trimmedEnd;
		std::size_t asciiCount = 0;
		while (markerPos > 0 && a_name[markerPos - 1] == '*') {
			++asciiCount;
			--markerPos;
		}
		if (asciiCount >= 1 && asciiCount <= 3) {
			while (markerPos > 0 && a_name[markerPos - 1] == ' ') {
				--markerPos;
			}
			return a_name.substr(0, markerPos);
		}

		return a_name;
	}

	[[nodiscard]] constexpr std::string_view StripLootStarMarkers(std::string_view a_name) noexcept
	{
		if (!HasLeadingLootStarPrefix(a_name) && !HasTrailingLootStarMarker(a_name)) {
			return a_name;
		}

		auto stripped = a_name;
		for (std::size_t i = 0; i < 4; ++i) {
			const auto next = StripTrailingLootStarMarker(StripLeadingLootStarPrefix(stripped));
			if (next == stripped) {
				return next;
			}
			stripped = next;
		}
		return stripped;
	}

	// Policy: runeword progress/details are shown in the dedicated runeword panel only.
	// Item tooltip overlay should show affix lines only.
	[[nodiscard]] constexpr bool ShouldShowRunewordTooltipInItemOverlay() noexcept
	{
		return false;
	}

	[[nodiscard]] constexpr bool IsPrismaTooltipRelevantMenu(std::string_view a_name) noexcept
	{
		return a_name == "InventoryMenu" ||
		       a_name == "BarterMenu" ||
		       a_name == "ContainerMenu" ||
		       a_name == "GiftMenu";
	}

	[[nodiscard]] constexpr bool IsPreviewItemSourceMenu(std::string_view a_name) noexcept
	{
		return a_name == "BarterMenu" ||
		       a_name == "ContainerMenu" ||
		       a_name == "GiftMenu";
	}

	[[nodiscard]] constexpr bool ShouldAllowPreviewUniqueIdRemap(
		bool a_playerOwnsEither,
		bool a_trackedPreview,
		bool a_previewMenuContextOpen) noexcept
	{
		return a_playerOwnsEither || (a_trackedPreview && a_previewMenuContextOpen);
	}

	[[nodiscard]] constexpr bool ShouldUseSelectedLootPreviewHint(
		bool a_previewMenuContextOpen,
		bool a_selectedPreviewTracked) noexcept
	{
		return a_previewMenuContextOpen && a_selectedPreviewTracked;
	}

	[[nodiscard]] constexpr bool IsRunewordOverlayTooltipLine(std::string_view a_line) noexcept
	{
		const auto line = detail::TrimLeadingAsciiWhitespace(a_line);
		return detail::StartsWithAsciiInsensitive(line, "Runeword:") ||
		       detail::StartsWithAsciiInsensitive(line, "Runeword Base:") ||
		       line.starts_with("룬워드:") ||
		       line.starts_with("룬워드 베이스:");
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
