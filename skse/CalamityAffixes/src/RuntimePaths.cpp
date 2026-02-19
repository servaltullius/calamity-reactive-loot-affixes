#include "CalamityAffixes/RuntimePaths.h"

#include <vector>

#include <Windows.h>

namespace CalamityAffixes::RuntimePaths
{
	std::optional<std::filesystem::path> GetRuntimeDirectory()
	{
		std::vector<wchar_t> buffer(0x4000);
		const DWORD len = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (len == 0 || len >= buffer.size()) {
			return std::nullopt;
		}

		std::filesystem::path path(buffer.data());
		path = path.remove_filename();
		return path;
	}

	std::filesystem::path ResolveRuntimeRelativePath(std::string_view a_relativePath)
	{
		if (const auto runtimeDir = GetRuntimeDirectory(); runtimeDir) {
			return *runtimeDir / std::filesystem::path(a_relativePath);
		}
		return std::filesystem::current_path() / std::filesystem::path(a_relativePath);
	}
}
