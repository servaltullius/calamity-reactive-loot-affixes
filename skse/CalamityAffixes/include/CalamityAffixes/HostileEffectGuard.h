#pragma once

#include <RE/Skyrim.h>

namespace SKSE { class SerializationInterface; }

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

	// Exact reference identities only; never infer Calamity provenance from a
	// vanilla summon base/spell shared with manual casts or enemy summons.
	inline constexpr std::uint32_t kCalamitySummonRecord = 0x4353554Du;  // CSUM
	void SaveCalamitySummons(SKSE::SerializationInterface* a_intfc);
	void LoadCalamitySummons(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version, std::uint32_t a_length);
	void RestoreCalamitySummonsAfterLoad();
}
