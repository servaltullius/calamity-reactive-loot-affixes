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
}
