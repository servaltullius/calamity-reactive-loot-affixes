#pragma once

namespace CalamityAffixes::detail
{
	// Whether a HitData may be routed as a Hit trigger.
	//
	// A direct weapon record or an attack-data spell is authoritative. Without
	// one, only melee/explosion flags or a bow/crossbow resolved from the hit's
	// own projectile (or active ranged attack) count, so an arrow with incomplete
	// HitData still routes. Holding a weapon never promotes a spell or hazard
	// tick: an Ice Storm crossing the target every 0.1s would otherwise fire
	// every on-hit affix as if each tick were a swing.
	[[nodiscard]] constexpr bool IsHitLikeSourceEvidence(
		bool a_hasDirectWeapon,
		bool a_hasAttackSpell,
		bool a_hasMeleeFlag,
		bool a_hasExplosionFlag,
		bool a_projectileWeaponIsRanged) noexcept
	{
		return a_hasDirectWeapon ||
		       a_hasAttackSpell ||
		       a_hasMeleeFlag ||
		       a_hasExplosionFlag ||
		       a_projectileWeaponIsRanged;
	}
}
