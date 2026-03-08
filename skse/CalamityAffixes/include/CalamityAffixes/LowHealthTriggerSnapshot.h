#pragma once

#include <cstdint>

namespace CalamityAffixes
{
	struct LowHealthTriggerSnapshot
	{
		std::uint32_t ownerFormID{ 0u };
		bool hasSnapshot{ false };
		float currentPct{ 100.0f };
		float previousPct{ 100.0f };
	};

	[[nodiscard]] constexpr LowHealthTriggerSnapshot BuildLowHealthTriggerSnapshot(
		bool a_isLowHealthTrigger,
		std::uint32_t a_ownerFormID,
		float a_currentPct,
		float a_previousPct) noexcept
	{
		if (!a_isLowHealthTrigger || a_ownerFormID == 0u) {
			return {};
		}

		return LowHealthTriggerSnapshot{
			.ownerFormID = a_ownerFormID,
			.hasSnapshot = true,
			.currentPct = a_currentPct,
			.previousPct = a_previousPct
		};
	}
}
