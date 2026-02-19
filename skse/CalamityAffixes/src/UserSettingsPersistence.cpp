#include "CalamityAffixes/UserSettingsPersistence.h"

#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <Windows.h>

#include <nlohmann/json.hpp>

#include <SKSE/SKSE.h>

namespace CalamityAffixes::UserSettingsPersistence
{
	namespace
	{
		std::mutex g_userSettingsIoLock;

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

		[[nodiscard]] std::filesystem::path ResolveRuntimeRelativePath(std::string_view a_relativePath)
		{
			if (const auto runtimeDir = GetRuntimeDirectory(); runtimeDir) {
				return *runtimeDir / std::filesystem::path(a_relativePath);
			}
			return std::filesystem::current_path() / std::filesystem::path(a_relativePath);
		}

		[[nodiscard]] bool LoadJsonObjectUnlocked(const std::filesystem::path& a_path, nlohmann::json& a_outRoot)
		{
			std::error_code ec;
			const bool exists = std::filesystem::exists(a_path, ec);
			if (ec || !exists) {
				return false;
			}

			std::ifstream in(a_path);
			if (!in.good()) {
				return false;
			}

			try {
				in >> a_outRoot;
			} catch (const std::exception& e) {
				SKSE::log::warn("CalamityAffixes: failed to parse user settings file {} ({})", a_path.string(), e.what());
				return false;
			}

			return true;
		}

		[[nodiscard]] bool SaveJsonObjectUnlocked(const std::filesystem::path& a_path, const nlohmann::json& a_root)
		{
			std::error_code ec;
			std::filesystem::create_directories(a_path.parent_path(), ec);
			if (ec) {
				SKSE::log::warn(
					"CalamityAffixes: failed to create user settings directory {} ({})",
					a_path.parent_path().string(),
					ec.message());
				return false;
			}

			std::ofstream out(a_path, std::ios::trunc);
			if (!out.good()) {
				return false;
			}

			out << a_root.dump(2);
			return out.good();
		}
	}

	bool LoadJsonObject(std::string_view a_relativePath, nlohmann::json& a_outRoot)
	{
		const auto path = ResolveRuntimeRelativePath(a_relativePath);
		std::scoped_lock lk{ g_userSettingsIoLock };
		return LoadJsonObjectUnlocked(path, a_outRoot);
	}

	bool SaveJsonObject(std::string_view a_relativePath, const nlohmann::json& a_root)
	{
		const auto path = ResolveRuntimeRelativePath(a_relativePath);
		std::scoped_lock lk{ g_userSettingsIoLock };
		return SaveJsonObjectUnlocked(path, a_root);
	}

	bool UpdateJsonObject(
		std::string_view a_relativePath,
		const std::function<void(nlohmann::json&)>& a_mutator)
	{
		const auto path = ResolveRuntimeRelativePath(a_relativePath);
		std::scoped_lock lk{ g_userSettingsIoLock };

		nlohmann::json root = nlohmann::json::object();
		if (LoadJsonObjectUnlocked(path, root)) {
			if (!root.is_object()) {
				root = nlohmann::json::object();
			}
		}

		a_mutator(root);
		return SaveJsonObjectUnlocked(path, root);
	}
}
