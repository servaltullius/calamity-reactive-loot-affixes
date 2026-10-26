#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace CalamityAffixes::Utf8Text
{
	// Game strings are raw bytes. A non-localized plugin saved by the Creation
	// Kit stores its names in Windows-1252 ("Mj\xF6lnir"), and nlohmann::json's
	// default dump() throws type_error 316 on such bytes. The Prisma panel
	// payload must always be valid UTF-8, so names are repaired at the JSON
	// boundary: valid UTF-8 sequences are kept as they are (Korean text, our
	// own config strings) and every other byte is read as Windows-1252.

	// Length of the well-formed UTF-8 sequence starting at a_index, or 0 when
	// the bytes there are not one (overlong forms, surrogates and code points
	// above U+10FFFF are rejected).
	[[nodiscard]] constexpr std::size_t ValidSequenceLength(std::string_view a_text, std::size_t a_index) noexcept
	{
		if (a_index >= a_text.size()) {
			return 0u;
		}
		const auto byteAt = [&](std::size_t a_offset) noexcept -> std::uint32_t {
			return static_cast<std::uint32_t>(static_cast<unsigned char>(a_text[a_index + a_offset]));
		};
		const auto isContinuation = [](std::uint32_t a_byte) noexcept { return (a_byte & 0xC0u) == 0x80u; };

		const auto lead = byteAt(0u);
		if (lead < 0x80u) {
			return 1u;
		}

		std::size_t length = 0u;
		std::uint32_t codePoint = 0u;
		std::uint32_t minimum = 0u;
		if ((lead & 0xE0u) == 0xC0u) {
			length = 2u;
			codePoint = lead & 0x1Fu;
			minimum = 0x80u;
		} else if ((lead & 0xF0u) == 0xE0u) {
			length = 3u;
			codePoint = lead & 0x0Fu;
			minimum = 0x800u;
		} else if ((lead & 0xF8u) == 0xF0u) {
			length = 4u;
			codePoint = lead & 0x07u;
			minimum = 0x10000u;
		} else {
			return 0u;
		}
		if (a_index + length > a_text.size()) {
			return 0u;
		}
		for (std::size_t i = 1u; i < length; ++i) {
			const auto next = byteAt(i);
			if (!isContinuation(next)) {
				return 0u;
			}
			codePoint = (codePoint << 6) | (next & 0x3Fu);
		}
		if (codePoint < minimum || codePoint > 0x10FFFFu || (codePoint >= 0xD800u && codePoint <= 0xDFFFu)) {
			return 0u;
		}
		return length;
	}

	[[nodiscard]] constexpr bool IsValidUtf8(std::string_view a_text) noexcept
	{
		for (std::size_t i = 0u; i < a_text.size();) {
			const auto length = ValidSequenceLength(a_text, i);
			if (length == 0u) {
				return false;
			}
			i += length;
		}
		return true;
	}

	// Windows-1252 byte to Unicode. The five unassigned bytes map to U+FFFD.
	[[nodiscard]] constexpr std::uint32_t Cp1252CodePoint(unsigned char a_byte) noexcept
	{
		constexpr std::uint32_t kHigh[32] = {
			0x20ACu, 0xFFFDu, 0x201Au, 0x0192u, 0x201Eu, 0x2026u, 0x2020u, 0x2021u,
			0x02C6u, 0x2030u, 0x0160u, 0x2039u, 0x0152u, 0xFFFDu, 0x017Du, 0xFFFDu,
			0xFFFDu, 0x2018u, 0x2019u, 0x201Cu, 0x201Du, 0x2022u, 0x2013u, 0x2014u,
			0x02DCu, 0x2122u, 0x0161u, 0x203Au, 0x0153u, 0xFFFDu, 0x017Eu, 0x0178u,
		};
		if (a_byte >= 0x80u && a_byte <= 0x9Fu) {
			return kHigh[a_byte - 0x80u];
		}
		return a_byte;
	}

	constexpr void AppendCodePoint(std::string& a_out, std::uint32_t a_codePoint)
	{
		if (a_codePoint < 0x80u) {
			a_out.push_back(static_cast<char>(a_codePoint));
		} else if (a_codePoint < 0x800u) {
			a_out.push_back(static_cast<char>(0xC0u | (a_codePoint >> 6)));
			a_out.push_back(static_cast<char>(0x80u | (a_codePoint & 0x3Fu)));
		} else if (a_codePoint < 0x10000u) {
			a_out.push_back(static_cast<char>(0xE0u | (a_codePoint >> 12)));
			a_out.push_back(static_cast<char>(0x80u | ((a_codePoint >> 6) & 0x3Fu)));
			a_out.push_back(static_cast<char>(0x80u | (a_codePoint & 0x3Fu)));
		} else {
			a_out.push_back(static_cast<char>(0xF0u | (a_codePoint >> 18)));
			a_out.push_back(static_cast<char>(0x80u | ((a_codePoint >> 12) & 0x3Fu)));
			a_out.push_back(static_cast<char>(0x80u | ((a_codePoint >> 6) & 0x3Fu)));
			a_out.push_back(static_cast<char>(0x80u | (a_codePoint & 0x3Fu)));
		}
	}

	// Keeps valid UTF-8 sequences and reads every other byte as Windows-1252,
	// so a generated name ("<cp1252 base> <UTF-8 affix>") repairs per byte.
	[[nodiscard]] constexpr std::string RepairToUtf8(std::string_view a_text)
	{
		std::string out;
		out.reserve(a_text.size() + 8u);
		for (std::size_t i = 0u; i < a_text.size();) {
			const auto length = ValidSequenceLength(a_text, i);
			if (length > 0u) {
				out.append(a_text.substr(i, length));
				i += length;
				continue;
			}
			AppendCodePoint(out, Cp1252CodePoint(static_cast<unsigned char>(a_text[i])));
			++i;
		}
		return out;
	}
}
