#pragma once

#include "CalamityAffixes/RunewordUiContracts.h"

#include <cstdint>
#include <span>
#include <string>

#include <nlohmann/json.hpp>

namespace CalamityAffixes
{
	[[nodiscard]] inline nlohmann::json BuildRunewordRuneTokenArrayJson(
		std::span<const std::uint64_t> a_tokens)
	{
		nlohmann::json payload = nlohmann::json::array();
		for (const auto token : a_tokens) {
			// JavaScript numbers cannot exactly represent every uint64_t. Keep the
			// wire contract decimal-string based, including recipe duplicates.
			payload.push_back(std::to_string(token));
		}
		return payload;
	}

	[[nodiscard]] inline nlohmann::json BuildRunewordRuneInventoryJson(
		std::span<const RunewordRuneInventoryEntry> a_entries)
	{
		nlohmann::json payload = nlohmann::json::array();
		for (const auto& entry : a_entries) {
			payload.push_back({
				{ "runeToken", std::to_string(entry.runeToken) },
				{ "runeName", entry.runeName },
				{ "owned", entry.owned },
			});
		}
		return payload;
	}
}
