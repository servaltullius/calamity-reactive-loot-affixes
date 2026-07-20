#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace CalamityAffixes::detail
{
	enum class CorpseCurrencyRewardTier : std::uint8_t
	{
		kNormal,
		kUnique,
		kBoss
	};

	struct CorpseCurrencyActorTierInput
	{
		bool hasBossLocationRefType{ false };
		bool actorBaseIsUnique{ false };
	};

	[[nodiscard]] constexpr CorpseCurrencyRewardTier ResolveCorpseCurrencyRewardTier(
		const CorpseCurrencyActorTierInput& a_input) noexcept
	{
		if (a_input.hasBossLocationRefType) {
			return CorpseCurrencyRewardTier::kBoss;
		}
		if (a_input.actorBaseIsUnique) {
			return CorpseCurrencyRewardTier::kUnique;
		}
		return CorpseCurrencyRewardTier::kNormal;
	}

	[[nodiscard]] constexpr std::string_view CorpseCurrencyRewardTierLabel(
		CorpseCurrencyRewardTier a_tier) noexcept
	{
		switch (a_tier) {
		case CorpseCurrencyRewardTier::kUnique:
			return "unique";
		case CorpseCurrencyRewardTier::kBoss:
			return "boss";
		case CorpseCurrencyRewardTier::kNormal:
		default:
			return "normal";
		}
	}

	struct CorpseCurrencyRewardPlan
	{
		bool useNormalRandomRolls{ false };
		bool grantRunewordFragment{ false };
		bool grantReforgeOrb{ false };
	};

	[[nodiscard]] constexpr CorpseCurrencyRewardPlan ResolveCorpseCurrencyRewardPlan(
		CorpseCurrencyRewardTier a_tier,
		bool a_allowRuneword,
		bool a_allowReforge,
		float a_uniqueRunewordRoll,
		float a_uniqueRunewordChancePercent) noexcept
	{
		switch (a_tier) {
		case CorpseCurrencyRewardTier::kBoss:
			return {
				.useNormalRandomRolls = false,
				.grantRunewordFragment = a_allowRuneword,
				.grantReforgeOrb = a_allowReforge
			};
		case CorpseCurrencyRewardTier::kUnique:
			if (a_allowRuneword && a_allowReforge) {
				const float runewordChance = std::clamp(a_uniqueRunewordChancePercent, 0.0f, 100.0f);
				const bool chooseRuneword = a_uniqueRunewordRoll < runewordChance;
				return {
					.useNormalRandomRolls = false,
					.grantRunewordFragment = chooseRuneword,
					.grantReforgeOrb = !chooseRuneword
				};
			}
			return {
				.useNormalRandomRolls = false,
				.grantRunewordFragment = a_allowRuneword,
				.grantReforgeOrb = a_allowReforge
			};
		case CorpseCurrencyRewardTier::kNormal:
		default:
			return {
				.useNormalRandomRolls = a_allowRuneword || a_allowReforge,
				.grantRunewordFragment = false,
				.grantReforgeOrb = false
			};
		}
	}
}
