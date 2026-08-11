#pragma once

#include <cstdint>
#include <utility>

#include "CalamityAffixes/SerializationWireContract.h"

namespace CalamityAffixes::SerializationWire
{
	inline constexpr std::uint32_t kMaxCurrentShuffleBagSize = 100'000u;

	enum class CurrentShuffleBagReadStatus : std::uint8_t
	{
		kComplete = 0,
		kTruncatedHeader,
		kTruncatedPayload,
		kSizeLimitExceeded,
	};

	struct CurrentShuffleBagReadResult
	{
		CurrentShuffleBagReadStatus status{ CurrentShuffleBagReadStatus::kComplete };
		std::uint32_t invalidBagSize{ 0u };
	};

	template <class Read, class Apply>
	[[nodiscard]] CurrentShuffleBagReadResult ReadCurrentLootShuffleBagsPayload(
		Read& a_read,
		Apply&& a_apply)
	{
		std::uint8_t bagCount = 0u;
		if (!static_cast<bool>(a_read(bagCount))) {
			return { .status = CurrentShuffleBagReadStatus::kTruncatedHeader };
		}

		for (std::uint8_t i = 0u; i < bagCount; ++i) {
			LootShuffleBagEntry entry{};
			std::uint32_t size = 0u;
			if (!static_cast<bool>(a_read(entry.id)) ||
				!static_cast<bool>(a_read(entry.cursor)) ||
				!static_cast<bool>(a_read(size))) {
				return { .status = CurrentShuffleBagReadStatus::kTruncatedPayload };
			}
			if (size > kMaxCurrentShuffleBagSize) {
				return {
					.status = CurrentShuffleBagReadStatus::kSizeLimitExceeded,
					.invalidBagSize = size,
				};
			}

			entry.order.reserve(size);
			for (std::uint32_t n = 0u; n < size; ++n) {
				std::uint32_t index = 0u;
				if (!static_cast<bool>(a_read(index))) {
					return { .status = CurrentShuffleBagReadStatus::kTruncatedPayload };
				}
				entry.order.push_back(index);
			}

			a_apply(std::move(entry));
		}

		return {};
	}

	template <class Read>
	[[nodiscard]] bool ReadCurrentMigrationFlagsPayload(
		Read& a_read,
		std::uint8_t& a_flags)
	{
		return static_cast<bool>(a_read(a_flags));
	}
}
