#pragma once

#include <functional>
#include <string_view>

#include <nlohmann/json_fwd.hpp>

namespace CalamityAffixes::UserSettingsPersistence
{
	[[nodiscard]] bool LoadJsonObject(std::string_view a_relativePath, nlohmann::json& a_outRoot);
	[[nodiscard]] bool SaveJsonObject(std::string_view a_relativePath, const nlohmann::json& a_root);
	[[nodiscard]] bool UpdateJsonObject(
		std::string_view a_relativePath,
		const std::function<void(nlohmann::json&)>& a_mutator);
}
