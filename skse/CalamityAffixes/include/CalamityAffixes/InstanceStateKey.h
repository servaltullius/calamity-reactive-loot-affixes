#pragma once

#include <cstddef>
#include <cstdint>

namespace CalamityAffixes
{
	struct InstanceStateKey
	{
		std::uint64_t instanceKey{ 0 };
		std::uint64_t affixToken{ 0 };

		[[nodiscard]] constexpr bool operator==(const InstanceStateKey&) const noexcept = default;
	};

	[[nodiscard]] constexpr std::uint64_t MixInstanceStateHash64(std::uint64_t a_value) noexcept
	{
		a_value ^= a_value >> 30;
		a_value *= 0xBF58476D1CE4E5B9ull;
		a_value ^= a_value >> 27;
		a_value *= 0x94D049BB133111EBull;
		a_value ^= a_value >> 31;
		return a_value;
	}

	[[nodiscard]] constexpr std::size_t HashInstanceStateKey(
		std::uint64_t a_instanceKey,
		std::uint64_t a_affixToken) noexcept
	{
		const std::uint64_t left = MixInstanceStateHash64(a_instanceKey);
		const std::uint64_t right = MixInstanceStateHash64(a_affixToken);
		const std::uint64_t mixed = left ^ (right + 0x9E3779B97F4A7C15ull + (left << 6) + (left >> 2));
		if constexpr (sizeof(std::size_t) >= sizeof(std::uint64_t)) {
			return static_cast<std::size_t>(mixed);
		} else {
			return static_cast<std::size_t>(mixed ^ (mixed >> 32));
		}
	}

	[[nodiscard]] constexpr std::size_t HashInstanceStateKey(const InstanceStateKey& a_key) noexcept
	{
		return HashInstanceStateKey(a_key.instanceKey, a_key.affixToken);
	}

	[[nodiscard]] constexpr InstanceStateKey MakeInstanceStateKey(
		std::uint64_t a_instanceKey,
		std::uint64_t a_affixToken) noexcept
	{
		return InstanceStateKey{
			.instanceKey = a_instanceKey,
			.affixToken = a_affixToken
		};
	}

	struct InstanceStateKeyHash
	{
		[[nodiscard]] constexpr std::size_t operator()(const InstanceStateKey& a_key) const noexcept
		{
			return HashInstanceStateKey(a_key);
		}
	};
}

