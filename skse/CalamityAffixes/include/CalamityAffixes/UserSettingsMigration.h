#pragma once

#include <cstddef>
#include <string_view>

namespace CalamityAffixes::UserSettingsMigration
{
	// user_settings.json "version". 2 (v2.3.0): the one-time Scouring Orb default
	// bump below has run and the file was saved again.
	inline constexpr int kCurrentVersion = 2;

	// 2% was the Scouring Orb default until selected reforge got a per-item limit
	// (now 4%). Only a file written before version 2 can hold that old default;
	// a 2 the player picks afterwards stays 2.
	[[nodiscard]] constexpr double MigrateScouringOrbChancePercent(int a_storedVersion, double a_value) noexcept
	{
		return (a_storedVersion < 2 && a_value == 2.0) ? 4.0 : a_value;
	}

	[[nodiscard]] constexpr std::string_view TrimIniText(std::string_view a_text) noexcept
	{
		while (!a_text.empty() && (a_text.front() == ' ' || a_text.front() == '\t' || a_text.front() == '\r')) {
			a_text.remove_prefix(1);
		}
		while (!a_text.empty() && (a_text.back() == ' ' || a_text.back() == '\t' || a_text.back() == '\r')) {
			a_text.remove_suffix(1);
		}
		return a_text;
	}

	[[nodiscard]] constexpr bool EqualsIgnoreCaseAscii(std::string_view a_lhs, std::string_view a_rhs) noexcept
	{
		if (a_lhs.size() != a_rhs.size()) {
			return false;
		}
		for (std::size_t i = 0; i < a_lhs.size(); ++i) {
			auto lower = [](char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; };
			if (lower(a_lhs[i]) != lower(a_rhs[i])) {
				return false;
			}
		}
		return true;
	}

	// The value of a_key in [a_section] of an MCM Helper settings ini, or empty
	// when it is not there. Section and key names match case-insensitively;
	// ';' and '#' start a comment line.
	[[nodiscard]] constexpr std::string_view FindIniValue(
		std::string_view a_text,
		std::string_view a_section,
		std::string_view a_key) noexcept
	{
		bool inSection = false;
		while (!a_text.empty()) {
			const auto lineEnd = a_text.find('\n');
			auto line = TrimIniText(a_text.substr(0, lineEnd));
			a_text = lineEnd == std::string_view::npos ? std::string_view{} : a_text.substr(lineEnd + 1);
			if (line.empty() || line.front() == ';' || line.front() == '#') {
				continue;
			}
			if (line.front() == '[') {
				const auto close = line.find(']');
				inSection = close != std::string_view::npos &&
				            EqualsIgnoreCaseAscii(TrimIniText(line.substr(1, close - 1)), a_section);
				continue;
			}
			if (!inSection) {
				continue;
			}
			const auto equals = line.find('=');
			if (equals == std::string_view::npos) {
				continue;
			}
			if (EqualsIgnoreCaseAscii(TrimIniText(line.substr(0, equals)), a_key)) {
				return TrimIniText(line.substr(equals + 1));
			}
		}
		return {};
	}
}
