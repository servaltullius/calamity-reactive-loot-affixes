#pragma once

#include <cstddef>
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

	// Shared safety contract for an optional normal-weapon-hit proc lane.  The
	// caller decides whether melee/ranged is allowed; critical/power, spell,
	// bash, timed-bash, and explosion HitData are never classified as normal.
	[[nodiscard]] constexpr bool IsEligibleNormalWeaponHitFlags(
		bool a_isCritical,
		bool a_isPowerAttack,
		bool a_hasAttackDataSpell,
		bool a_isBash,
		bool a_isTimedBash,
		bool a_isExplosion) noexcept
	{
		return !a_isCritical &&
		       !a_isPowerAttack &&
		       !a_hasAttackDataSpell &&
		       !a_isBash &&
		       !a_isTimedBash &&
		       !a_isExplosion;
	}

	[[nodiscard]] constexpr std::size_t ResolveCyclicCandidateIndex(
		std::size_t a_candidateCount,
		std::size_t a_cursor,
		std::size_t a_offset = 0u) noexcept
	{
		if (a_candidateCount == 0u) {
			return 0u;
		}
		const auto start = a_cursor % a_candidateCount;
		const auto offset = a_offset % a_candidateCount;
		const auto remaining = a_candidateCount - start;
		return offset < remaining ? start + offset : offset - remaining;
	}

	[[nodiscard]] constexpr std::size_t ResolveCastOnCritSelectionCount(
		std::size_t a_candidateCount,
		bool a_isRangedWeapon,
		bool a_isNormalMeleeHit) noexcept
	{
		const std::size_t limit = (a_isRangedWeapon || a_isNormalMeleeHit) ? 1u : 2u;
		return a_candidateCount < limit ? a_candidateCount : limit;
	}

	[[nodiscard]] constexpr bool IsCalamityProcSource(std::string_view a_sourceEditorId) noexcept
	{
		return a_sourceEditorId.starts_with("CAFF_");
	}
}
