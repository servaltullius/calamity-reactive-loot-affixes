#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace CalamityAffixes
{
	static constexpr std::size_t kMaxAffixesPerItem = 3;

	struct InstanceAffixSlots
	{
		std::array<std::uint64_t, kMaxAffixesPerItem> tokens{};
		std::uint8_t count = 0;

		[[nodiscard]] constexpr auto begin() const noexcept { return tokens.begin(); }
		[[nodiscard]] constexpr auto end() const noexcept { return tokens.begin() + count; }

		[[nodiscard]] constexpr bool HasToken(std::uint64_t a_token) const noexcept
		{
			for (std::uint8_t i = 0; i < count; ++i) {
				if (tokens[i] == a_token) {
					return true;
				}
			}
			return false;
		}

		constexpr bool AddToken(std::uint64_t a_token) noexcept
		{
			if (a_token == 0u) {
				return false;
			}
			if (count >= kMaxAffixesPerItem) {
				return false;
			}
			if (HasToken(a_token)) {
				return false;
			}
			tokens[count] = a_token;
			++count;
			return true;
		}

		[[nodiscard]] constexpr std::uint64_t GetPrimary() const noexcept
		{
			return (count > 0) ? tokens[0] : 0u;
		}

		constexpr void Clear() noexcept
		{
			tokens.fill(0u);
			count = 0;
		}

		constexpr void ReplaceAll(std::uint64_t a_singleToken) noexcept
		{
			Clear();
			if (a_singleToken != 0u) {
				tokens[0] = a_singleToken;
				count = 1;
			}
		}
	};
}
