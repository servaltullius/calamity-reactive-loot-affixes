#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace CalamityAffixes::RuntimePaths
{
	[[nodiscard]] std::optional<std::filesystem::path> GetRuntimeDirectory();
	[[nodiscard]] std::filesystem::path ResolveRuntimeRelativePath(std::string_view a_relativePath);
}
