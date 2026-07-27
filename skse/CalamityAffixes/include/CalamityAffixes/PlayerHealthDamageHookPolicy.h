#pragma once

namespace CalamityAffixes::detail
{
	// Player incoming hits always route through the HandleHealthDamage hook.
	//
	// The `allowPlayerHealthDamageHook` runtime key is a legacy escape hatch that
	// is no longer honoured; ApplyRuntimeUserSettingsOverrides logs a deprecation
	// warning when it is present and then ignores it.
	//
	// Deliberately takes no parameter. An earlier shape accepted the "a legacy
	// override was present" flag and discarded it, which made the call site read
	// as though the override still influenced the result -- a reader, or a later
	// refactor, could reasonably wire it back up. Since the answer does not
	// depend on configuration at all, the signature says so.
	[[nodiscard]] constexpr bool ResolvePlayerHealthDamageHookEnabled() noexcept
	{
		return true;
	}
}
