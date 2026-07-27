#pragma once

#include <algorithm>
#include <cstdint>

namespace CalamityAffixes::detail
{
	inline constexpr std::uint32_t kSerializationDrainChunkBytes = 4096u;
	inline constexpr std::uint32_t kUnusuallyLargeSerializationDrainBytes = 10'000'000u;

	[[nodiscard]] constexpr std::uint32_t ResolveSerializationDrainChunkSize(
		std::uint32_t a_remainingBytes) noexcept
	{
		return std::min(a_remainingBytes, kSerializationDrainChunkBytes);
	}

	[[nodiscard]] constexpr bool ShouldWarnUnusuallyLargeSerializationDrain(
		std::uint32_t a_length) noexcept
	{
		return a_length > kUnusuallyLargeSerializationDrainBytes;
	}
}
