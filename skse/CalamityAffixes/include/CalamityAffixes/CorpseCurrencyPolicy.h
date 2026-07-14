#pragma once

#include <cstdint>

namespace CalamityAffixes::detail
{
	struct CorpseRuntimeCurrencyEligibilityInput
	{
		bool eventIsDeath{ false };
		bool dyingIsDead{ false };
		bool dyingIsPlayerOwned{ false };
		bool dyingIsPlayerTeammate{ false };
		bool dyingIsSummoned{ false };
		bool dyingIsCommanded{ false };
		bool dyingIsChild{ false };
		bool killerIsPlayerOwned{ false };
		bool hostileToPlayerOwner{ false };
	};

	[[nodiscard]] constexpr bool IsCorpseRuntimeCurrencyDropEligible(
		const CorpseRuntimeCurrencyEligibilityInput& a_input) noexcept
	{
		return a_input.eventIsDeath &&
			a_input.dyingIsDead &&
			!a_input.dyingIsPlayerOwned &&
			!a_input.dyingIsPlayerTeammate &&
			!a_input.dyingIsSummoned &&
			!a_input.dyingIsCommanded &&
			!a_input.dyingIsChild &&
			a_input.killerIsPlayerOwned &&
			a_input.hostileToPlayerOwner;
	}

	inline constexpr std::uint8_t kCorpseCurrencyRunewordProcessed = 1u << 0;
	inline constexpr std::uint8_t kCorpseCurrencyReforgeProcessed = 1u << 1;
	inline constexpr std::uint8_t kCorpseCurrencyAllProcessed =
		kCorpseCurrencyRunewordProcessed | kCorpseCurrencyReforgeProcessed;

	struct CorpseCurrencyRollAllowance
	{
		bool runeword{ false };
		bool reforge{ false };
	};

	[[nodiscard]] constexpr CorpseCurrencyRollAllowance ResolveCorpseCurrencyRollAllowance(
		std::uint8_t a_processedMask,
		bool a_hasRunewordCurrency,
		bool a_hasReforgeCurrency) noexcept
	{
		return CorpseCurrencyRollAllowance{
			.runeword = (a_processedMask & kCorpseCurrencyRunewordProcessed) == 0u && !a_hasRunewordCurrency,
			.reforge = (a_processedMask & kCorpseCurrencyReforgeProcessed) == 0u && !a_hasReforgeCurrency
		};
	}
}
