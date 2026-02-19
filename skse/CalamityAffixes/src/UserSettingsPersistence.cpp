#include "CalamityAffixes/UserSettingsPersistence.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
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

		enum class LoadJsonStatus : std::uint8_t
		{
			kLoaded = 0,
			kMissing,
			kIoError,
			kParseError,
		};

		[[nodiscard]] LoadJsonStatus LoadJsonObjectUnlocked(const std::filesystem::path& a_path, nlohmann::json& a_outRoot)
		{
			std::error_code ec;
			const bool exists = std::filesystem::exists(a_path, ec);
			if (ec) {
				SKSE::log::warn(
					"CalamityAffixes: failed to inspect user settings file {} ({})",
					a_path.string(),
					ec.message());
				return LoadJsonStatus::kIoError;
			}
			if (!exists) {
				return LoadJsonStatus::kMissing;
			}

			std::ifstream in(a_path, std::ios::binary);
			if (!in.is_open()) {
				SKSE::log::warn("CalamityAffixes: failed to open user settings file {}", a_path.string());
				return LoadJsonStatus::kIoError;
			}

			try {
				in >> a_outRoot;
			} catch (const std::exception& e) {
				SKSE::log::warn("CalamityAffixes: failed to parse user settings file {} ({})", a_path.string(), e.what());
				return LoadJsonStatus::kParseError;
			}

			if (!in.good() && !in.eof()) {
				SKSE::log::warn("CalamityAffixes: failed while reading user settings file {}", a_path.string());
				return LoadJsonStatus::kIoError;
			}

			return LoadJsonStatus::kLoaded;
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

			const std::string payload = a_root.dump(2);
			std::filesystem::path tempPath = a_path;
			tempPath += ".tmp";

			HANDLE tempFileHandle = ::CreateFileW(
				tempPath.c_str(),
				GENERIC_WRITE,
				0,
				nullptr,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				nullptr);
			if (tempFileHandle == INVALID_HANDLE_VALUE) {
				SKSE::log::warn(
					"CalamityAffixes: failed to create temp user settings file {} (err={}).",
					tempPath.string(),
					::GetLastError());
				return false;
			}

			const char* cursor = payload.data();
			std::size_t remaining = payload.size();
			bool writeOk = true;
			DWORD writeError = ERROR_SUCCESS;
			while (remaining > 0) {
				const DWORD chunkSize = static_cast<DWORD>(std::min<std::size_t>(
					remaining,
					static_cast<std::size_t>(std::numeric_limits<DWORD>::max())));
				DWORD written = 0;
				if (!::WriteFile(tempFileHandle, cursor, chunkSize, &written, nullptr) || written != chunkSize) {
					writeOk = false;
					writeError = ::GetLastError();
					break;
				}
				cursor += written;
				remaining -= written;
			}

			if (writeOk && !::FlushFileBuffers(tempFileHandle)) {
				writeOk = false;
				writeError = ::GetLastError();
			}

			::CloseHandle(tempFileHandle);

			if (!writeOk) {
				SKSE::log::warn(
					"CalamityAffixes: failed to write temp user settings file {} (err={}).",
					tempPath.string(),
					writeError);
				std::error_code removeEc;
				std::filesystem::remove(tempPath, removeEc);
				return false;
			}

			const DWORD moveError = ::MoveFileExW(
				tempPath.c_str(),
				a_path.c_str(),
				MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) ?
					ERROR_SUCCESS :
					::GetLastError();
			if (moveError != ERROR_SUCCESS) {
				SKSE::log::warn(
					"CalamityAffixes: failed to atomically replace user settings file {} (err={}).",
					a_path.string(),
					moveError);
				std::error_code removeEc;
				std::filesystem::remove(tempPath, removeEc);
				return false;
			}

			return true;
		}
	}

	bool LoadJsonObject(std::string_view a_relativePath, nlohmann::json& a_outRoot)
	{
		const auto path = ResolveRuntimeRelativePath(a_relativePath);
		std::scoped_lock lk{ g_userSettingsIoLock };
		return LoadJsonObjectUnlocked(path, a_outRoot) == LoadJsonStatus::kLoaded;
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
		const auto loadStatus = LoadJsonObjectUnlocked(path, root);
		if (loadStatus == LoadJsonStatus::kLoaded) {
			if (!root.is_object()) {
				root = nlohmann::json::object();
			}
		} else if (loadStatus == LoadJsonStatus::kParseError || loadStatus == LoadJsonStatus::kIoError) {
			SKSE::log::warn(
				"CalamityAffixes: refusing to overwrite unreadable user settings file {}.",
				path.string());
			return false;
		}

		a_mutator(root);
		return SaveJsonObjectUnlocked(path, root);
	}
}
