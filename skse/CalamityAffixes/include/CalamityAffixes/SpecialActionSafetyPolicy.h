#pragma once

#include <string_view>

namespace CalamityAffixes::detail
{
	[[nodiscard]] constexpr float ResolveSpecialActionProcChancePct(
		float a_configuredChancePct) noexcept
	{
		if (a_configuredChancePct <= 0.0f) {
			return 0.0f;
		}
		return a_configuredChancePct >= 100.0f ? 100.0f : a_configuredChancePct;
	}

	[[nodiscard]] constexpr bool IsCalamityProcSource(std::string_view a_sourceEditorId) noexcept
	{
		return a_sourceEditorId.starts_with("CAFF_");
	}
}
