#pragma once

namespace CalamityAffixes::detail
{
	enum class TrapWeaponFallbackSource
	{
		kNone,
		kReportedAggressor,
		kRoutedOwner,
	};

	// Hit-trigger ownership can be routed from a player-commanded summon back to
	// the player.  Weapon fallback must still inspect the actor that actually
	// produced the hit when HitData reports one; the routed owner is only a
	// compatibility fallback for incomplete genuine player projectile HitData.
	[[nodiscard]] constexpr TrapWeaponFallbackSource ResolveTrapWeaponFallbackSource(
		bool a_hasHitData,
		bool a_hasReportedAggressor,
		bool a_hasRoutedOwner) noexcept
	{
		if (!a_hasHitData) {
			return TrapWeaponFallbackSource::kNone;
		}
		if (a_hasReportedAggressor) {
			return TrapWeaponFallbackSource::kReportedAggressor;
		}
		return a_hasRoutedOwner ?
			TrapWeaponFallbackSource::kRoutedOwner :
			TrapWeaponFallbackSource::kNone;
	}

	// A direct weapon record is authoritative.  Missing weapon records may use
	// melee/explosion flags or an explicitly resolved bow/crossbow fallback, but
	// never promote a spell hit merely because its actor is holding a weapon.
	[[nodiscard]] constexpr bool IsTrapWeaponHitEvidence(
		bool a_hasHitData,
		bool a_hasDirectWeapon,
		bool a_hasAttackSpell,
		bool a_hasMeleeFlag,
		bool a_hasExplosionFlag,
		bool a_fallbackWeaponIsRanged) noexcept
	{
		if (!a_hasHitData) {
			return false;
		}
		if (a_hasDirectWeapon) {
			return true;
		}
		if (a_hasAttackSpell) {
			return false;
		}
		return a_hasMeleeFlag || a_hasExplosionFlag || a_fallbackWeaponIsRanged;
	}
}
