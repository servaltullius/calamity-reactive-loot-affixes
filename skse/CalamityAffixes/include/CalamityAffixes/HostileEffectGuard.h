#pragma once

#include <RE/Skyrim.h>

namespace CalamityAffixes
{
	[[nodiscard]] bool IsHostileEffectTarget(RE::Actor* a_owner, RE::Actor* a_target) noexcept;

	void CastHostileOnlySpellImmediate(
		RE::MagicCaster* a_magicCaster,
		RE::SpellItem* a_spell,
		bool a_noHitArt,
		RE::TESObjectREFR* a_target,
		float a_effectiveness,
		float a_magnitudeOverride,
		RE::Actor* a_caster) noexcept;

	[[nodiscard]] bool ShouldSuppressNonHostileCalamityHealthDamage(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData) noexcept;

	void ClearHostileEffectGuardRuntimeState() noexcept;
}
