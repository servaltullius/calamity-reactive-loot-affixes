#pragma once

#include <RE/Skyrim.h>

#include "CalamityAffixes/PlayerOwnership.h"

namespace CalamityAffixes
{
	struct CombatTriggerContext
	{
		bool hasTarget{ false };
		bool hasAttacker{ false };
		bool targetIsPlayer{ false };
		bool attackerIsPlayerOwned{ false };
		RE::Actor* playerOwner{ nullptr };
		bool hasPlayerOwner{ false };
		bool hostileEitherDirection{ false };
	};

	[[nodiscard]] inline bool IsHostileEitherDirection(RE::Actor* a_lhs, RE::Actor* a_rhs) noexcept
	{
		return a_lhs && a_rhs &&
		       (a_lhs->IsHostileToActor(a_rhs) || a_rhs->IsHostileToActor(a_lhs));
	}

	[[nodiscard]] inline CombatTriggerContext BuildCombatTriggerContext(
		RE::Actor* a_target,
		RE::Actor* a_attacker) noexcept
	{
		CombatTriggerContext context{};
		context.hasTarget = (a_target != nullptr);
		context.hasAttacker = (a_attacker != nullptr);
		context.targetIsPlayer = context.hasTarget && a_target->IsPlayerRef();
		context.attackerIsPlayerOwned = context.hasAttacker && IsPlayerOwnedActor(a_attacker);
		context.playerOwner = context.attackerIsPlayerOwned ? ResolvePlayerOwnerActor(a_attacker) : nullptr;
		context.hasPlayerOwner = (context.playerOwner != nullptr);

		if (context.hasTarget && context.hasAttacker) {
			if (context.targetIsPlayer) {
				context.hostileEitherDirection = IsHostileEitherDirection(a_target, a_attacker);
			} else if (context.playerOwner) {
				context.hostileEitherDirection = IsHostileEitherDirection(context.playerOwner, a_target);
			}
		}

		return context;
	}
}
