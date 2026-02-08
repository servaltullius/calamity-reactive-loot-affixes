#pragma once

#include <cstdint>
#include <string_view>

namespace CalamityAffixes
{
	// FNV-1a 64-bit hash over UTF-8 bytes. Used to compactly persist affix IDs in cosaves.
	[[nodiscard]] constexpr std::uint64_t MakeAffixToken(std::string_view a_affixId) noexcept
	{
		std::uint64_t hash = 0xCBF29CE484222325ull;  // offset basis
		for (const unsigned char c : a_affixId) {
			hash ^= static_cast<std::uint64_t>(c);
			hash *= 0x100000001B3ull;  // FNV prime
		}
		return hash;
	}
}

