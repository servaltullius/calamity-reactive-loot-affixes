#pragma once

#include "CalamityAffixes/EventBridge.h"

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace CalamityAffixes::ConfigShared
{
	inline std::string_view Trim(std::string_view a_text) noexcept
	{
		auto isWs = [](unsigned char c) {
			return c == ' ' || c == '\t' || c == '\r' || c == '\n';
		};

		while (!a_text.empty() && isWs(static_cast<unsigned char>(a_text.front()))) {
			a_text.remove_prefix(1);
		}
		while (!a_text.empty() && isWs(static_cast<unsigned char>(a_text.back()))) {
			a_text.remove_suffix(1);
		}

		return a_text;
	}

	inline RE::SpellItem* LookupSpellFromSpec(std::string_view a_spec, RE::TESDataHandler* a_handler)
	{
		const auto text = Trim(a_spec);
		if (text.empty()) {
			return nullptr;
		}

		const auto pipe = text.find('|');
		if (pipe == std::string_view::npos) {
			return RE::TESForm::LookupByEditorID<RE::SpellItem>(std::string(text));
		}

		if (!a_handler) {
			return nullptr;
		}

		const auto modName = text.substr(0, pipe);
		const auto formIDText = text.substr(pipe + 1);

		std::uint32_t localID = 0;
		auto begin = formIDText.data();
		auto end = formIDText.data() + formIDText.size();
		if (formIDText.starts_with("0x") || formIDText.starts_with("0X")) {
			begin += 2;
		}

		const auto result = std::from_chars(begin, end, localID, 16);
		if (result.ec != std::errc{}) {
			return nullptr;
		}

		return a_handler->LookupForm<RE::SpellItem>(static_cast<RE::FormID>(localID), modName);
	}
}
