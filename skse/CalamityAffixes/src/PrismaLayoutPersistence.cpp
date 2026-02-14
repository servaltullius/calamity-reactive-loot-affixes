#include "CalamityAffixes/PrismaLayoutPersistence.h"

#include <array>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <string_view>

#include <nlohmann/json.hpp>

namespace CalamityAffixes::PrismaLayoutPersistence
{
	namespace
	{
		void TrimAscii(std::string_view& a_text)
		{
			while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.front()))) {
				a_text.remove_prefix(1);
			}
			while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.back()))) {
				a_text.remove_suffix(1);
			}
		}

		template <std::size_t N>
		[[nodiscard]] bool ParseCsvInts(std::string_view a_payload, std::array<int, N>& a_values)
		{
			std::size_t cursor = 0;
			for (std::size_t i = 0; i < N; ++i) {
				const std::size_t next = a_payload.find(',', cursor);
				if (next == std::string_view::npos && i < N - 1) {
					return false;
				}

				std::string_view token =
					next == std::string_view::npos
						? a_payload.substr(cursor)
						: a_payload.substr(cursor, next - cursor);
				TrimAscii(token);
				if (token.empty()) {
					return false;
				}

				int parsed = 0;
				const auto* first = token.data();
				const auto* last = first + token.size();
				const auto [ptr, ec] = std::from_chars(first, last, parsed);
				if (ec != std::errc{} || ptr != last) {
					return false;
				}
				a_values[i] = parsed;

				cursor = next == std::string_view::npos ? a_payload.size() : next + 1;
			}

			if (cursor < a_payload.size()) {
				std::string_view trailing = a_payload.substr(cursor);
				TrimAscii(trailing);
				if (!trailing.empty()) {
					return false;
				}
			}

			return true;
		}
	}

	bool ParsePanelLayoutPayload(std::string_view a_payload, PanelLayout& a_out)
	{
		std::array<int, 4> values{};
		if (!ParseCsvInts(a_payload, values)) {
			return false;
		}
		if (values[2] <= 0 || values[3] <= 0) {
			return false;
		}

		a_out.left = values[0];
		a_out.top = values[1];
		a_out.width = values[2];
		a_out.height = values[3];
		a_out.valid = true;
		return true;
	}

	bool ParseTooltipLayoutPayload(
		std::string_view a_payload,
		int a_minFontPermille,
		int a_maxFontPermille,
		TooltipLayout& a_out)
	{
		std::array<int, 3> values{};
		if (!ParseCsvInts(a_payload, values)) {
			return false;
		}
		if (values[0] < 0 || values[1] < 0) {
			return false;
		}
		if (values[2] < a_minFontPermille || values[2] > a_maxFontPermille) {
			return false;
		}

		a_out.right = values[0];
		a_out.top = values[1];
		a_out.fontPermille = values[2];
		a_out.valid = true;
		return true;
	}

	std::optional<PanelLayout> LoadPanelLayoutFile(std::string_view a_path)
	{
		const std::filesystem::path path{ std::string(a_path) };
		std::error_code ec;
		if (!std::filesystem::exists(path, ec) || ec) {
			return std::nullopt;
		}

		std::ifstream in(path);
		if (!in.good()) {
			return std::nullopt;
		}

		nlohmann::json j;
		in >> j;

		PanelLayout loaded{};
		loaded.left = j.value("left", 0);
		loaded.top = j.value("top", 0);
		loaded.width = j.value("width", 0);
		loaded.height = j.value("height", 0);
		loaded.valid = loaded.width > 0 && loaded.height > 0;
		if (!loaded.valid) {
			return std::nullopt;
		}

		return loaded;
	}

	std::optional<TooltipLayout> LoadTooltipLayoutFile(
		std::string_view a_path,
		int a_defaultRight,
		int a_defaultTop,
		int a_defaultFontPermille,
		int a_minFontPermille,
		int a_maxFontPermille)
	{
		const std::filesystem::path path{ std::string(a_path) };
		std::error_code ec;
		if (!std::filesystem::exists(path, ec) || ec) {
			return std::nullopt;
		}

		std::ifstream in(path);
		if (!in.good()) {
			return std::nullopt;
		}

		nlohmann::json j;
		in >> j;

		TooltipLayout loaded{};
		loaded.right = j.value("right", a_defaultRight);
		loaded.top = j.value("top", a_defaultTop);
		loaded.fontPermille = j.value("fontPermille", a_defaultFontPermille);
		loaded.valid = loaded.right >= 0 &&
			loaded.top >= 0 &&
			loaded.fontPermille >= a_minFontPermille &&
			loaded.fontPermille <= a_maxFontPermille;
		if (!loaded.valid) {
			return std::nullopt;
		}

		return loaded;
	}

	bool SavePanelLayoutFile(std::string_view a_path, const PanelLayout& a_layout)
	{
		if (!a_layout.valid) {
			return false;
		}

		const std::filesystem::path path{ std::string(a_path) };
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		if (ec) {
			return false;
		}

		std::ofstream out(path, std::ios::trunc);
		if (!out.good()) {
			return false;
		}

		const nlohmann::json j{
			{ "left", a_layout.left },
			{ "top", a_layout.top },
			{ "width", a_layout.width },
			{ "height", a_layout.height }
		};
		out << j.dump(2);
		return out.good();
	}

	bool SaveTooltipLayoutFile(std::string_view a_path, const TooltipLayout& a_layout)
	{
		if (!a_layout.valid) {
			return false;
		}

		const std::filesystem::path path{ std::string(a_path) };
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		if (ec) {
			return false;
		}

		std::ofstream out(path, std::ios::trunc);
		if (!out.good()) {
			return false;
		}

		const nlohmann::json j{
			{ "right", a_layout.right },
			{ "top", a_layout.top },
			{ "fontPermille", a_layout.fontPermille }
		};
		out << j.dump(2);
		return out.good();
	}

	std::string EncodePanelLayoutJson(const PanelLayout& a_layout)
	{
		const nlohmann::json j{
			{ "left", a_layout.left },
			{ "top", a_layout.top },
			{ "width", a_layout.width },
			{ "height", a_layout.height }
		};
		return j.dump();
	}

	std::string EncodeTooltipLayoutJsonForDisk(const TooltipLayout& a_layout)
	{
		const nlohmann::json j{
			{ "right", a_layout.right },
			{ "top", a_layout.top },
			{ "fontPermille", a_layout.fontPermille }
		};
		return j.dump();
	}

	std::string EncodeTooltipLayoutJsonForJs(const TooltipLayout& a_layout)
	{
		const nlohmann::json j{
			{ "right", a_layout.right },
			{ "top", a_layout.top },
			{ "fontScale", static_cast<double>(a_layout.fontPermille) / 1000.0 }
		};
		return j.dump();
	}
}
