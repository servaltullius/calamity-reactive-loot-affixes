#pragma once

namespace CalamityAffixes::detail
{
	[[nodiscard]] constexpr bool IsHostileOnlyEffectTargetAllowed(
		bool a_hasOwner,
		bool a_hasTarget,
		bool a_isSameActor,
		bool a_targetIsProtectedPlayerAlly,
		bool a_ownerHostileToTarget,
		bool a_targetHostileToOwner) noexcept
	{
		return a_hasOwner && a_hasTarget && !a_isSameActor && !a_targetIsProtectedPlayerAlly &&
		       (a_ownerHostileToTarget || a_targetHostileToOwner);
	}

	struct HostileEffectDamagePolicyInput
	{
		bool hasTarget{ false };
		bool hasAttacker{ false };
		bool hasPlayerOwner{ false };
		bool targetIsHostileToPlayerOwner{ false };
		bool hostileOnlyCastScopeActive{ false };
		bool attackerIsRegisteredCalamitySummon{ false };
		bool sourceExplosionOwnedByRegisteredCalamitySummon{ false };
	};

	struct RegisteredSummonExplosionDamageInput
	{
		bool hitDataIsExplosion{ false };
		bool hitTargetMatches{ false };
		bool hitAggressorCompatible{ false };
		bool sourceExplosionOwnedByRegisteredSummon{ false };
		bool sourceTargetWindowSeen{ false };
		bool sourceTargetWindowActive{ false };
	};

	// lastHitData can outlive the HandleHealthDamage callback that produced it.
	// Accept an exact registered-summon explosion once per source/target pair,
	// allow its synchronous companion callbacks through a short window, and do
	// not reopen that window from stale hit data after it expires.
	[[nodiscard]] constexpr bool ShouldAcceptRegisteredSummonExplosionDamage(
		RegisteredSummonExplosionDamageInput a_input) noexcept
	{
		return a_input.hitDataIsExplosion && a_input.hitTargetMatches &&
		       a_input.hitAggressorCompatible &&
		       a_input.sourceExplosionOwnedByRegisteredSummon &&
		       (!a_input.sourceTargetWindowSeen || a_input.sourceTargetWindowActive);
	}

	// Suppression is intentionally narrower than player ownership alone.  It
	// protects the player, allies, and neutral actors only when the incoming
	// health damage can be attributed to a Calamity hostile-only effect.
	// Ordinary player attacks, manually cast spells, and unrecognized summons
	// therefore continue through the engine's normal friendly-fire rules.
	[[nodiscard]] constexpr bool ShouldSuppressNonHostileCalamityHealthDamage(
		HostileEffectDamagePolicyInput a_input) noexcept
	{
		if (!a_input.hasTarget || !a_input.hasPlayerOwner || a_input.targetIsHostileToPlayerOwner) {
			return false;
		}

		return a_input.hostileOnlyCastScopeActive ||
		       (a_input.hasAttacker && a_input.attackerIsRegisteredCalamitySummon) ||
		       a_input.sourceExplosionOwnedByRegisteredCalamitySummon;
	}
}
