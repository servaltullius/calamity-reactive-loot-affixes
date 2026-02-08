#pragma once

namespace RE
{
	class Actor;
}

namespace CalamityAffixes::Hooks
{
	void Install();
	[[nodiscard]] bool IsHandleHealthDamageHooked(const RE::Actor* a_actor) noexcept;
}
