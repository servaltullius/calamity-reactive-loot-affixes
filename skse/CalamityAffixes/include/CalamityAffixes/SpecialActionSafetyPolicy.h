#pragma once

#include <string_view>

namespace CalamityAffixes::detail
{
	enum class CastOnCritWeaponSource
	{
		kNone,
		kHitDataWeapon,
		kProjectileWeapon,
		kActiveRangedAttack
	};

	// HitData::weapon is normally authoritative, but Skyrim can leave it null
	// for bow/crossbow projectiles.  A null weapon is accepted only when the hit
	// still carries positive weapon-origin evidence.  In particular, merely
	// having a bow equipped is not enough: a spell projectile fired while a bow
	// is equipped must not recursively trigger CastOnCrit.
	[[nodiscard]] constexpr CastOnCritWeaponSource ResolveCastOnCritWeaponSource(
		bool a_hasDirectWeapon,
		bool a_hasAttackDataSpell,
		bool a_hasSourceReference,
		bool a_sourceIsProjectile,
		bool a_projectileHasRangedWeapon,
		bool a_hasAttackData,
		bool a_attackerHasActiveRangedWeapon) noexcept
	{
		if (a_hasDirectWeapon) {
			return CastOnCritWeaponSource::kHitDataWeapon;
		}

		// Spell-origin HitData is authoritative even if the attacker happens to
		// be holding or attacking with a ranged weapon at the same time.
		if (a_hasAttackDataSpell) {
			return CastOnCritWeaponSource::kNone;
		}

		// When the engine supplies a source reference, fail closed unless it is
		// a projectile that itself records a bow/crossbow weapon source.
		if (a_hasSourceReference) {
			return a_sourceIsProjectile && a_projectileHasRangedWeapon ?
			         CastOnCritWeaponSource::kProjectileWeapon :
			         CastOnCritWeaponSource::kNone;
		}

		// Some HitData arrives without a resolvable projectile reference.  The
		// narrow fallback requires both weapon attack data on this hit and the
		// attacker's current attack entry to be a bow/crossbow.  It deliberately
		// does not inspect merely equipped weapons.
		return a_hasAttackData && a_attackerHasActiveRangedWeapon ?
		         CastOnCritWeaponSource::kActiveRangedAttack :
		         CastOnCritWeaponSource::kNone;
	}

	[[nodiscard]] constexpr float ResolveSpecialActionProcChancePct(
		float a_configuredChancePct) noexcept
	{
		if (a_configuredChancePct <= 0.0f) {
			return 0.0f;
		}
		return a_configuredChancePct >= 100.0f ? 100.0f : a_configuredChancePct;
	}

	[[nodiscard]] constexpr bool IsCalamityProcSource(std::string_view a_sourceEditorId) noexcept
	{
		return a_sourceEditorId.starts_with("CAFF_");
	}
}
