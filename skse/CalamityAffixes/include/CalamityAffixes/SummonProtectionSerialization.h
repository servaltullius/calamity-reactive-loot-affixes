#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace CalamityAffixes::SummonProtectionSerialization
{
	inline constexpr std::uint32_t kVersion = 1u;
	inline constexpr std::uint32_t kMaxSummons = 64u;

	struct Record
	{
		std::array<std::uint32_t, kMaxSummons> formIDs{};
		std::uint32_t count{ 0u };
	};

	[[nodiscard]] constexpr bool IsValidRecord(std::uint32_t a_version, std::uint32_t a_length) noexcept
	{
		return a_version == kVersion && a_length <= kMaxSummons * sizeof(std::uint32_t) &&
		       a_length % sizeof(std::uint32_t) == 0u;
	}

	// Read the entire bounded record before exposing any identities. A truncated
	// record must not partially mark unrelated actors as protected summons.
	template <class Reader>
	[[nodiscard]] std::optional<Record> Read(std::uint32_t a_version, std::uint32_t a_length, Reader&& a_read)
	{
		if (!IsValidRecord(a_version, a_length)) {
			return std::nullopt;
		}
		Record record;
		record.count = a_length / sizeof(std::uint32_t);
		if (a_length != 0u && a_read(record.formIDs.data(), a_length) != a_length) {
			return std::nullopt;
		}
		return record;
	}
}
