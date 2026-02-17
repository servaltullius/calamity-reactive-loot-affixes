#pragma once

#include "CalamityAffixes/InstanceAffixSlots.h"

#include <array>
#include <cstdint>

namespace CalamityAffixes::detail
{
	template <class IsTokenAllowedFn>
	[[nodiscard]] inline InstanceAffixSlots BuildSanitizedInstanceAffixSlots(
		const InstanceAffixSlots& a_slots,
		IsTokenAllowedFn&& a_isTokenAllowed,
		std::array<std::uint64_t, kMaxAffixesPerItem>* a_removedTokens = nullptr,
		std::uint8_t* a_removedCount = nullptr)
	{
		InstanceAffixSlots sanitized{};
		std::array<std::uint64_t, kMaxAffixesPerItem> removed{};
		std::uint8_t removedCount = 0;

		const auto inputCount = static_cast<std::uint8_t>(
			(a_slots.count > static_cast<std::uint8_t>(kMaxAffixesPerItem)) ?
				static_cast<std::uint8_t>(kMaxAffixesPerItem) :
				a_slots.count);

		for (std::uint8_t i = 0; i < inputCount; ++i) {
			const auto token = a_slots.tokens[i];
			bool keep = token != 0u && a_isTokenAllowed(token);
			if (keep) {
				keep = sanitized.AddToken(token);
			}

			if (!keep && removedCount < removed.size()) {
				removed[removedCount++] = token;
			}
		}

		if (a_removedTokens) {
			*a_removedTokens = removed;
		}
		if (a_removedCount) {
			*a_removedCount = removedCount;
		}

		return sanitized;
	}
}
