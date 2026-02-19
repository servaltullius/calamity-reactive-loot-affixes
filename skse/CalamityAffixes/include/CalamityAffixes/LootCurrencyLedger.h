#pragma once

#include <cstdint>

namespace CalamityAffixes::detail
{
	enum class LootCurrencySourceTier : std::uint8_t
	{
		kUnknown = 0,
		kCorpse,
		kContainer,
		kBossContainer,
		kWorld
	};

	[[nodiscard]] constexpr std::uint64_t MixLootCurrencyLedgerBits(std::uint64_t a_value) noexcept
	{
		a_value += 0x9E3779B97F4A7C15ull;
		a_value = (a_value ^ (a_value >> 30)) * 0xBF58476D1CE4E5B9ull;
		a_value = (a_value ^ (a_value >> 27)) * 0x94D049BB133111EBull;
		return a_value ^ (a_value >> 31);
	}

	[[nodiscard]] constexpr std::uint64_t BuildLootCurrencyLedgerKey(
		LootCurrencySourceTier a_sourceTier,
		std::uint32_t a_sourceFormId,
		std::uint32_t a_baseObj,
		std::uint16_t a_uniqueId,
		std::uint32_t a_dayStamp) noexcept
	{
		const std::uint64_t tierBits = static_cast<std::uint64_t>(static_cast<std::uint8_t>(a_sourceTier));
		const std::uint64_t sourceBits = static_cast<std::uint64_t>(a_sourceFormId);
		const std::uint64_t instanceBits = (static_cast<std::uint64_t>(a_baseObj) << 16) |
			static_cast<std::uint64_t>(a_uniqueId);
		const std::uint64_t dayBits = static_cast<std::uint64_t>(a_dayStamp);

		std::uint64_t mixed = MixLootCurrencyLedgerBits((tierBits << 56) | sourceBits);
		mixed ^= MixLootCurrencyLedgerBits(instanceBits ^ (dayBits << 16));
		mixed ^= MixLootCurrencyLedgerBits((tierBits << 40) ^ (dayBits << 24) ^ 0xA5A5A5A55A5A5A5Aull);
		return mixed != 0u ? mixed : 1u;
	}

	[[nodiscard]] constexpr bool IsLootCurrencyLedgerExpired(
		std::uint32_t a_storedDayStamp,
		std::uint32_t a_currentDayStamp,
		std::uint32_t a_ttlDays) noexcept
	{
		if (a_ttlDays == 0u || a_currentDayStamp < a_storedDayStamp) {
			return false;
		}
		return (a_currentDayStamp - a_storedDayStamp) >= a_ttlDays;
	}
}

