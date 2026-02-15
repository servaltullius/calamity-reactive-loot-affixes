#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace CalamityAffixes::detail
{
	inline constexpr std::array<std::string_view, 5> kDefaultLootArmorEditorIdDenyContains{
		"adventurerguild",
		"adventurersguild",
		"rewardbox",
		"randombox",
		"lootbox"
	};

	[[nodiscard]] inline std::vector<std::string> MakeDefaultLootArmorEditorIdDenyContains()
	{
		std::vector<std::string> out;
		out.reserve(kDefaultLootArmorEditorIdDenyContains.size());
		for (const auto marker : kDefaultLootArmorEditorIdDenyContains) {
			out.emplace_back(marker);
		}
		return out;
	}

	[[nodiscard]] constexpr char ToLowerAscii(char a_char) noexcept
	{
		return (a_char >= 'A' && a_char <= 'Z') ? static_cast<char>(a_char + ('a' - 'A')) : a_char;
	}

	[[nodiscard]] constexpr bool ContainsCaseInsensitiveAscii(std::string_view a_text, std::string_view a_pattern) noexcept
	{
		if (a_pattern.empty() || a_text.size() < a_pattern.size()) {
			return false;
		}

		for (std::size_t i = 0; i + a_pattern.size() <= a_text.size(); ++i) {
			bool matched = true;
			for (std::size_t j = 0; j < a_pattern.size(); ++j) {
				if (ToLowerAscii(a_text[i + j]) != ToLowerAscii(a_pattern[j])) {
					matched = false;
					break;
				}
			}
			if (matched) {
				return true;
			}
		}

		return false;
	}

	template <class MarkerRange>
	[[nodiscard]] constexpr bool MatchesAnyCaseInsensitiveMarker(std::string_view a_text, const MarkerRange& a_markers) noexcept
	{
		if (a_text.empty()) {
			return false;
		}

		for (const auto& rawMarker : a_markers) {
			const std::string_view marker{ rawMarker };
			if (marker.empty()) {
				continue;
			}
			if (ContainsCaseInsensitiveAscii(a_text, marker)) {
				return true;
			}
		}
		return false;
	}

	struct LootArmorEligibilityInput
	{
		bool playable{ false };
		bool hasSlotMask{ false };
		bool hasArmorAddons{ false };
		bool hasTemplateArmor{ false };
		bool editorIdDenied{ false };
	};

	[[nodiscard]] constexpr bool IsLootArmorEligible(const LootArmorEligibilityInput& a_input) noexcept
	{
		if (!a_input.playable) {
			return false;
		}
		if (!a_input.hasSlotMask) {
			return false;
		}
		if (!a_input.hasArmorAddons && !a_input.hasTemplateArmor) {
			return false;
		}
		if (a_input.editorIdDenied) {
			return false;
		}
		return true;
	}
}
