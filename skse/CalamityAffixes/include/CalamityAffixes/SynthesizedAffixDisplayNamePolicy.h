#pragma once

#include <string_view>

namespace CalamityAffixes::detail
{
	[[nodiscard]] constexpr std::string_view ResolveSynthesizedAffixDisplayName(
		std::string_view a_displayName,
		std::string_view a_label,
		std::string_view a_id) noexcept
	{
		if (!a_displayName.empty()) {
			return a_displayName;
		}
		return !a_label.empty() ? a_label : a_id;
	}

	[[nodiscard]] constexpr std::string_view ResolveSynthesizedLocalizedDisplayName(
		std::string_view a_localizedDisplayName,
		std::string_view a_fallbackDisplayName) noexcept
	{
		return !a_localizedDisplayName.empty() ? a_localizedDisplayName : a_fallbackDisplayName;
	}
}
