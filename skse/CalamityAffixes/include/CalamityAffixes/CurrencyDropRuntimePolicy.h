#pragma once

namespace CalamityAffixes::detail
{
	struct CurrencyDropRuntimePolicy
	{
		bool broadRuntimeDropsEnabled{ false };
		bool corpseDeathRuntimeDropsEnabled{ true };
	};

	[[nodiscard]] constexpr CurrencyDropRuntimePolicy ResolveCorpseDeathOnlyCurrencyDropPolicy() noexcept
	{
		return {};
	}
}
