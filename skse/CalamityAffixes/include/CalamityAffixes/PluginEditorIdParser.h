#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

// Reads the editor IDs a Skyrim SE plugin defines, straight from the plugin file.
//
// The game keeps editor IDs in memory only for a few form types (keywords, quests,
// globals, sounds, ...). Spells, magic effects, misc items, leveled lists and art
// objects drop theirs at load, so TESForm::LookupByEditorID finds them only when
// another plugin (powerofthree's Tweaks "Load EditorIDs") restores them. Reading our
// own plugin lets the DLL resolve its forms without that dependency.
//
// Pure byte parsing: no game types, so the host checks can run it on the shipped ESP.
namespace CalamityAffixes::PluginEditorIdParser
{
	struct Entry
	{
		std::string editorId;
		std::uint32_t localFormId{ 0 };  // FormID without the load-order byte
	};

	struct Result
	{
		bool ok{ false };
		std::size_t masterCount{ 0 };
		std::size_t compressedSkipped{ 0 };
		std::vector<Entry> entries;  // records the plugin defines itself (overrides excluded)
	};

	namespace detail
	{
		inline constexpr std::size_t kRecordHeaderSize = 24;
		inline constexpr std::size_t kSubrecordHeaderSize = 6;
		inline constexpr std::uint32_t kCompressedFlag = 0x00040000;

		[[nodiscard]] inline std::uint32_t ReadU32(std::string_view a_data, std::size_t a_at) noexcept
		{
			std::uint32_t value = 0;
			std::memcpy(&value, a_data.data() + a_at, sizeof(value));
			return value;
		}

		[[nodiscard]] inline std::uint16_t ReadU16(std::string_view a_data, std::size_t a_at) noexcept
		{
			std::uint16_t value = 0;
			std::memcpy(&value, a_data.data() + a_at, sizeof(value));
			return value;
		}

		[[nodiscard]] inline bool HasType(std::string_view a_data, std::size_t a_at, std::string_view a_type) noexcept
		{
			return a_data.substr(a_at, 4) == a_type;
		}

		// Counts MAST subrecords in the TES4 header body.
		[[nodiscard]] inline bool CountMasters(std::string_view a_body, std::size_t& a_out) noexcept
		{
			std::size_t at = 0;
			std::uint32_t nextSize = 0;  // set by an XXXX subrecord for oversized data
			while (at < a_body.size()) {
				if (a_body.size() - at < kSubrecordHeaderSize) {
					return false;
				}
				const auto size = nextSize ? nextSize : ReadU16(a_body, at + 4);
				const bool extended = HasType(a_body, at, "XXXX");
				if (extended) {
					if (size < 4 || a_body.size() - at - kSubrecordHeaderSize < 4) {
						return false;
					}
					nextSize = ReadU32(a_body, at + kSubrecordHeaderSize);
				} else {
					nextSize = 0;
				}
				if (HasType(a_body, at, "MAST")) {
					++a_out;
				}
				if (a_body.size() - at - kSubrecordHeaderSize < size) {
					return false;
				}
				at += kSubrecordHeaderSize + size;
			}
			return true;
		}
	}

	[[nodiscard]] inline Result Parse(std::string_view a_data)
	{
		using namespace detail;

		Result result;
		if (a_data.size() < kRecordHeaderSize || !HasType(a_data, 0, "TES4")) {
			return result;
		}

		const auto headerSize = ReadU32(a_data, 4);
		if (a_data.size() - kRecordHeaderSize < headerSize ||
			!CountMasters(a_data.substr(kRecordHeaderSize, headerSize), result.masterCount)) {
			return result;
		}

		// Groups nest contiguously, so a flat walk visits every record: step into a GRUP
		// by its header, step over a record by its header plus data.
		std::size_t at = kRecordHeaderSize + headerSize;
		while (at < a_data.size()) {
			if (a_data.size() - at < kRecordHeaderSize) {
				return result;
			}
			const auto dataSize = ReadU32(a_data, at + 4);
			if (HasType(a_data, at, "GRUP")) {
				if (dataSize < kRecordHeaderSize || a_data.size() - at < dataSize) {
					return result;
				}
				at += kRecordHeaderSize;
				continue;
			}
			if (a_data.size() - at - kRecordHeaderSize < dataSize) {
				return result;
			}

			const auto flags = ReadU32(a_data, at + 8);
			const auto formId = ReadU32(a_data, at + 12);
			const auto body = a_data.substr(at + kRecordHeaderSize, dataSize);
			at += kRecordHeaderSize + dataSize;

			if ((formId >> 24) != result.masterCount) {
				continue;  // an override of a master's record
			}
			if (flags & kCompressedFlag) {
				++result.compressedSkipped;
				continue;
			}
			if (body.size() < kSubrecordHeaderSize || !HasType(body, 0, "EDID")) {
				continue;
			}
			const auto size = ReadU16(body, 4);
			if (body.size() - kSubrecordHeaderSize < size) {
				return result;
			}
			auto editorId = body.substr(kSubrecordHeaderSize, size);
			if (const auto nul = editorId.find('\0'); nul != std::string_view::npos) {
				editorId = editorId.substr(0, nul);
			}
			if (!editorId.empty()) {
				result.entries.push_back(Entry{ std::string(editorId), formId & 0x00FFFFFFu });
			}
		}

		result.ok = true;
		return result;
	}
}
