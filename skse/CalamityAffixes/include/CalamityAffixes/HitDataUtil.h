#pragma once

#include "CalamityAffixes/PointerSafety.h"

#include <RE/Skyrim.h>

namespace CalamityAffixes::HitDataUtil
{
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
}
