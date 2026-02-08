#pragma once

#include <cstdint>

namespace CalamityAffixes
{
	struct ResyncScheduler
	{
		std::uint64_t nextAtMs{ 0 };
		std::uint64_t intervalMs{ 0 };

		[[nodiscard]] constexpr bool ShouldRun(std::uint64_t a_nowMs) noexcept
		{
			if (a_nowMs < nextAtMs) {
				return false;
			}

			nextAtMs = a_nowMs + intervalMs;
			return true;
		}
	};
}

