#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace CalamityAffixes::PrismaLayoutPersistence
{
	struct PanelLayout
	{
		int left{ 0 };
		int top{ 0 };
		int width{ 0 };
		int height{ 0 };
		bool valid{ false };
	};

	struct TooltipLayout
	{
		int right{ 0 };
		int top{ 0 };
		int fontPermille{ 0 };
		bool valid{ false };
	};

	[[nodiscard]] bool ParsePanelLayoutPayload(std::string_view a_payload, PanelLayout& a_out);
	[[nodiscard]] bool ParseTooltipLayoutPayload(
		std::string_view a_payload,
		int a_minFontPermille,
		int a_maxFontPermille,
		TooltipLayout& a_out);

	[[nodiscard]] std::optional<PanelLayout> LoadPanelLayoutFile(std::string_view a_path);
	[[nodiscard]] std::optional<TooltipLayout> LoadTooltipLayoutFile(
		std::string_view a_path,
		int a_defaultRight,
		int a_defaultTop,
		int a_defaultFontPermille,
		int a_minFontPermille,
		int a_maxFontPermille);

	[[nodiscard]] bool SavePanelLayoutFile(std::string_view a_path, const PanelLayout& a_layout);
	[[nodiscard]] bool SaveTooltipLayoutFile(std::string_view a_path, const TooltipLayout& a_layout);

	[[nodiscard]] std::string EncodePanelLayoutJson(const PanelLayout& a_layout);
	[[nodiscard]] std::string EncodeTooltipLayoutJsonForDisk(const TooltipLayout& a_layout);
	[[nodiscard]] std::string EncodeTooltipLayoutJsonForJs(const TooltipLayout& a_layout);
}
