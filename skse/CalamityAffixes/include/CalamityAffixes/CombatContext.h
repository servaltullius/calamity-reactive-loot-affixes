#pragma once

#include <RE/Skyrim.h>

#include "CalamityAffixes/PointerSafety.h"
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

	[[nodiscard]] inline bool IsCombatTargetEitherDirection(RE::Actor* a_lhs, RE::Actor* a_rhs) noexcept
	{
		a_lhs = SanitizeObjectPointer(a_lhs);
		a_rhs = SanitizeObjectPointer(a_rhs);
		if (!a_lhs || !a_rhs) {
			return false;
		}

		const auto lhsTargetAny = a_lhs->GetActorRuntimeData().currentCombatTarget.get();
		const auto rhsTargetAny = a_rhs->GetActorRuntimeData().currentCombatTarget.get();
		auto* lhsTarget = SanitizeObjectPointer(lhsTargetAny.get());
		auto* rhsTarget = SanitizeObjectPointer(rhsTargetAny.get());
		return lhsTarget == a_rhs || rhsTarget == a_lhs;
	}

	[[nodiscard]] inline bool IsHostileEitherDirection(RE::Actor* a_lhs, RE::Actor* a_rhs) noexcept
	{
		a_lhs = SanitizeObjectPointer(a_lhs);
		a_rhs = SanitizeObjectPointer(a_rhs);
		if (!a_lhs || !a_rhs) {
			return false;
		}

		// Primary signal: faction/relationship hostility.
		if (a_lhs->IsHostileToActor(a_rhs) || a_rhs->IsHostileToActor(a_lhs)) {
			return true;
		}

		// Fallback: some actors (notably certain undead setups) can remain "non-hostile" while still being engaged
		// in combat. Treat mutual combat targeting as hostile for proc gating, but keep a conservative filter to
		// avoid enabling procs for brawls/training with non-combatant NPCs.
		if (!a_lhs->IsInCombat() || !a_rhs->IsInCombat()) {
			return false;
		}
		if (!IsCombatTargetEitherDirection(a_lhs, a_rhs)) {
			return false;
		}
		return a_lhs->CalculateCachedOwnerIsUndead() ||
		       a_rhs->CalculateCachedOwnerIsUndead() ||
		       a_lhs->CalculateCachedOwnerIsInCombatantFaction() ||
		       a_rhs->CalculateCachedOwnerIsInCombatantFaction();
	}

	[[nodiscard]] inline CombatTriggerContext BuildCombatTriggerContext(
		RE::Actor* a_target,
		RE::Actor* a_attacker) noexcept
	{
		a_target = SanitizeObjectPointer(a_target);
		a_attacker = SanitizeObjectPointer(a_attacker);

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
