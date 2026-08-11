#pragma once

#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/SpecialActionSafetyPolicy.h"

#include <RE/Skyrim.h>

namespace CalamityAffixes::HitDataUtil
{
	[[nodiscard]] inline bool IsBowOrCrossbow(const RE::TESObjectWEAP* a_weapon) noexcept
	{
		return a_weapon &&
		       (a_weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow ||
		        a_weapon->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow);
	}

	[[nodiscard]] inline RE::TESObjectWEAP* ResolveCastOnCritHitWeapon(
		const RE::HitData* a_hitData,
		RE::Actor* a_attacker) noexcept
	{
		if (!a_hitData) {
			return nullptr;
		}

		auto* directWeapon = a_hitData->weapon ? SanitizeObjectPointer(a_hitData->weapon) : nullptr;
		const bool hasSourceReference = static_cast<bool>(a_hitData->sourceRef);

		RE::TESObjectWEAP* projectileWeapon = nullptr;
		bool sourceIsProjectile = false;
		if (!directWeapon && !a_hitData->attackDataSpell && hasSourceReference) {
			const auto sourceHolder = a_hitData->sourceRef.get();
			auto* sourceRef = SanitizeObjectPointer(sourceHolder.get());
			auto* projectile = sourceRef ? SanitizeObjectPointer(sourceRef->AsProjectile()) : nullptr;
			if (projectile) {
				sourceIsProjectile = true;
				projectileWeapon = SanitizeObjectPointer(projectile->GetProjectileRuntimeData().weaponSource);
			}
		}

		RE::TESObjectWEAP* activeAttackWeapon = nullptr;
		if (!directWeapon && !a_hitData->attackDataSpell && !hasSourceReference && a_hitData->attackData) {
			a_attacker = SanitizeObjectPointer(a_attacker);
			if (a_attacker) {
				if (auto* entry = a_attacker->GetAttackingWeapon()) {
					auto* object = SanitizeObjectPointer(entry->GetObject());
					activeAttackWeapon = object ? object->As<RE::TESObjectWEAP>() : nullptr;
				}
			}
		}

		using detail::CastOnCritWeaponSource;
		const auto source = detail::ResolveCastOnCritWeaponSource(
			directWeapon != nullptr,
			a_hitData->attackDataSpell != nullptr,
			hasSourceReference,
			sourceIsProjectile,
			IsBowOrCrossbow(projectileWeapon),
			a_hitData->attackData != nullptr,
			IsBowOrCrossbow(activeAttackWeapon));

		switch (source) {
		case CastOnCritWeaponSource::kHitDataWeapon:
			return directWeapon;
		case CastOnCritWeaponSource::kProjectileWeapon:
			return projectileWeapon;
		case CastOnCritWeaponSource::kActiveRangedAttack:
			return activeAttackWeapon;
		case CastOnCritWeaponSource::kNone:
		default:
			return nullptr;
		}
	}

	[[nodiscard]] inline const RE::HitData* GetLastHitData(RE::Actor* a_target)
	{
		if (!a_target) {
			return nullptr;
		}

		const auto& runtime = a_target->GetActorRuntimeData();
		auto* process = SanitizeObjectPointer(runtime.currentProcess);
		if (!process) {
			return nullptr;
		}

		auto* middleHigh = SanitizeObjectPointer(process->middleHigh);
		if (!middleHigh) {
			return nullptr;
		}

		return SanitizeObjectPointer(middleHigh->lastHitData);
	}

	[[nodiscard]] inline RE::TESObjectWEAP* ResolveHitWeapon(const RE::HitData* a_hitData, RE::Actor* a_attacker) noexcept
	{
		if (a_hitData) {
			if (auto* weapon = SanitizeObjectPointer(a_hitData->weapon)) {
				return weapon;
			}
		}

		a_attacker = SanitizeObjectPointer(a_attacker);
		if (!a_attacker) {
			return nullptr;
		}

		if (auto* entry = a_attacker->GetAttackingWeapon(); entry) {
			auto* object = SanitizeObjectPointer(entry->GetObject());
			if (auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr) {
				return weapon;
			}
		}

		if (auto* equippedRight = SanitizeObjectPointer(a_attacker->GetEquippedObject(false)); equippedRight) {
			if (auto* weapon = equippedRight->As<RE::TESObjectWEAP>()) {
				return weapon;
			}
		}

		if (auto* equippedLeft = SanitizeObjectPointer(a_attacker->GetEquippedObject(true)); equippedLeft) {
			if (auto* weapon = equippedLeft->As<RE::TESObjectWEAP>()) {
				return weapon;
			}
		}

		return nullptr;
	}

	[[nodiscard]] inline RE::FormID GetHitSourceFormID(const RE::HitData* a_hitData, RE::Actor* a_attacker = nullptr) noexcept
	{
		if (!a_hitData) {
			return 0;
		}
		if (a_hitData->weapon) {
			return a_hitData->weapon->GetFormID();
		}
		if (a_hitData->attackDataSpell) {
			return a_hitData->attackDataSpell->GetFormID();
		}
		// Bow/crossbow arrows: hitData->weapon may be null — resolve from attacker.
		if (auto* weapon = ResolveHitWeapon(a_hitData, a_attacker)) {
			return weapon->GetFormID();
		}
		return 0;
	}

	[[nodiscard]] inline bool IsWeaponLikeHit(const RE::HitData* a_hitData, RE::Actor* a_attacker) noexcept
	{
		if (!a_hitData) {
			return false;
		}

		if (ResolveHitWeapon(a_hitData, a_attacker)) {
			return true;
		}

		return a_hitData->flags.any(RE::HitData::Flag::kMeleeAttack) ||
		       a_hitData->flags.any(RE::HitData::Flag::kExplosion);
	}

	[[nodiscard]] inline bool HitDataMatchesActors(
		const RE::HitData* a_hitData,
		const RE::Actor* a_target,
		const RE::Actor* a_attacker) noexcept
	{
		if (!a_hitData || !a_target) {
			return false;
		}

		const auto hitTarget = a_hitData->target.get().get();
		if (hitTarget && hitTarget != a_target) {
			return false;
		}

		const auto hitAggressor = a_hitData->aggressor.get().get();
		if (a_attacker) {
			if (hitAggressor && hitAggressor != a_attacker) {
				return false;
			}
		} else if (hitAggressor) {
			return false;
		}

		return true;
	}

	[[nodiscard]] inline bool HasHitLikeSource(const RE::HitData* a_hitData, RE::Actor* a_attacker) noexcept
	{
		if (!a_hitData) {
			return false;
		}

		if (a_hitData->weapon != nullptr || a_hitData->attackDataSpell != nullptr) {
			return true;
		}

		// Bow/crossbow arrows: hitData->weapon may be null — resolve from attacker.
		if (ResolveHitWeapon(a_hitData, a_attacker)) {
			return true;
		}

		return a_hitData->flags.any(RE::HitData::Flag::kMeleeAttack) ||
		       a_hitData->flags.any(RE::HitData::Flag::kExplosion);
	}
}
