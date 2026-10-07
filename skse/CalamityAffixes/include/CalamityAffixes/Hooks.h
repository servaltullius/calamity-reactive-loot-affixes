#pragma once

#include <cstddef>

namespace RE
{
	class Actor;
}

namespace CalamityAffixes::Hooks
{
	[[nodiscard]] constexpr std::size_t HandleHealthDamageVfuncIndexForRuntime(bool a_isVR) noexcept
	{
		return a_isVR ? 0x106u : 0x104u;
	}

	void Install();
	[[nodiscard]] bool IsHandleHealthDamageHooked(const RE::Actor* a_actor) noexcept;
	void InvalidateDeferredTasks() noexcept;
	void ClearRuntimeState() noexcept;
	// Call just before casting an echo strike. The echo's damage reaches
	// HandleHealthDamage carrying the target's previous swing as lastHitData;
	// this marks that one callback as damage only, never a new hit.
	void ExpectEchoStrikeDamage(RE::Actor* a_target, RE::Actor* a_attacker) noexcept;
}
