#include "CalamityAffixes/CurrencyDropRuntimePolicy.h"

using CalamityAffixes::detail::ResolveCorpseDeathOnlyCurrencyDropPolicy;

static_assert([] {
	constexpr auto policy = ResolveCorpseDeathOnlyCurrencyDropPolicy();
	return !policy.broadRuntimeDropsEnabled && policy.corpseDeathRuntimeDropsEnabled;
}());
