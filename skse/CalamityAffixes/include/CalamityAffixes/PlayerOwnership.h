#pragma once

#include <RE/Skyrim.h>

namespace CalamityAffixes
{
	[[nodiscard]] inline bool IsPlayerOwnedActor(RE::Actor* a_actor) noexcept
	{
		if (!a_actor) {
			return false;
		}
		if (a_actor->IsPlayerRef()) {
			return true;
		}

		auto commandingAny = a_actor->GetCommandingActor();
		auto* commanding = commandingAny.get();
		return commanding && commanding->IsPlayerRef();
	}

	[[nodiscard]] inline RE::Actor* ResolvePlayerOwnerActor(RE::Actor* a_actor) noexcept
	{
		if (!a_actor) {
			return nullptr;
		}
		if (a_actor->IsPlayerRef()) {
			return a_actor;
		}

		auto commandingAny = a_actor->GetCommandingActor();
		auto* commanding = commandingAny.get();
		return (commanding && commanding->IsPlayerRef()) ? commanding : nullptr;
	}
}
