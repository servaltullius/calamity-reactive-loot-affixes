#include "CalamityAffixes/UserSettingsPersistence.h"
#include "CalamityAffixes/RuntimePaths.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>

#include <SKSE/SKSE.h>

namespace CalamityAffixes::UserSettingsPersistence
{
	namespace
	{
		std::mutex g_userSettingsIoLock;

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

		[[nodiscard]] bool QuarantineUnreadableJsonFileUnlocked(const std::filesystem::path& a_path)
		{
			std::error_code existsEc;
			const bool exists = std::filesystem::exists(a_path, existsEc);
			if (existsEc) {
				SKSE::log::warn(
					"CalamityAffixes: failed to inspect unreadable user settings file {} before quarantine ({}).",
					a_path.string(),
					existsEc.message());
				return false;
			}
			if (!exists) {
				return true;
			}

			const auto now = std::chrono::system_clock::now();
			const auto unixSeconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
			const auto pid = static_cast<unsigned long>(::GetCurrentProcessId());
			std::filesystem::path quarantinedPath = a_path;
			quarantinedPath += ".corrupt.";
			quarantinedPath += std::to_string(unixSeconds);
			quarantinedPath += ".";
			quarantinedPath += std::to_string(pid);

			for (std::uint32_t attempt = 0; attempt < 32u; ++attempt) {
				std::filesystem::path candidate = quarantinedPath;
				if (attempt > 0u) {
					candidate += ".";
					candidate += std::to_string(attempt);
				}

				std::error_code renameEc;
				std::filesystem::rename(a_path, candidate, renameEc);
				if (!renameEc) {
					SKSE::log::warn(
						"CalamityAffixes: quarantined unreadable user settings file {} -> {}.",
						a_path.string(),
						candidate.string());
					return true;
				}
				if (renameEc != std::errc::file_exists) {
					SKSE::log::warn(
						"CalamityAffixes: failed to quarantine unreadable user settings file {} ({}).",
						a_path.string(),
						renameEc.message());
					return false;
				}
			}

			SKSE::log::warn(
				"CalamityAffixes: failed to quarantine unreadable user settings file {} (name collision limit reached).",
				a_path.string());
			return false;
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
		const auto path = RuntimePaths::ResolveRuntimeRelativePath(a_relativePath);
		std::scoped_lock lk{ g_userSettingsIoLock };
		return LoadJsonObjectUnlocked(path, a_outRoot) == LoadJsonStatus::kLoaded;
	}

	bool SaveJsonObject(std::string_view a_relativePath, const nlohmann::json& a_root)
	{
		const auto path = RuntimePaths::ResolveRuntimeRelativePath(a_relativePath);
		std::scoped_lock lk{ g_userSettingsIoLock };
		return SaveJsonObjectUnlocked(path, a_root);
	}

	bool UpdateJsonObject(
		std::string_view a_relativePath,
		const std::function<void(nlohmann::json&)>& a_mutator)
	{
		const auto path = RuntimePaths::ResolveRuntimeRelativePath(a_relativePath);
		std::scoped_lock lk{ g_userSettingsIoLock };

		nlohmann::json root = nlohmann::json::object();
		const auto loadStatus = LoadJsonObjectUnlocked(path, root);
		if (loadStatus == LoadJsonStatus::kLoaded) {
			if (!root.is_object()) {
				root = nlohmann::json::object();
			}
		} else if (loadStatus == LoadJsonStatus::kParseError || loadStatus == LoadJsonStatus::kIoError) {
			if (!QuarantineUnreadableJsonFileUnlocked(path)) {
				return false;
			}
			root = nlohmann::json::object();
		}

		a_mutator(root);
		return SaveJsonObjectUnlocked(path, root);
	}
}
