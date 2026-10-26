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

	// Whether a spell summons a creature (kSummonCreature archetype).
	[[nodiscard]] bool IsSummonCreatureSpellRecord(const RE::SpellItem* a_spell) noexcept;

	// Whether a_owner commands a living summon or thrall that is not a Calamity
	// summon (one the player cast or raised themselves). A summon proc shares
	// the owner's summon slot and would push that one out.
	[[nodiscard]] bool CommandsNonCalamitySummon(RE::Actor* a_owner) noexcept;

	// Load/revert: everything, including which live summons are ours.
	void ClearHostileEffectGuardRuntimeState() noexcept;
	// Runtime disabled in the MCM: pending casts and windows only. Live
	// Calamity summons (up to 60 s) keep their protection and the registry a
	// save is about to restore survives.
	void ClearHostileEffectGuardTransientState() noexcept;

	// Exact reference identities only; never infer Calamity provenance from a
	// vanilla summon base/spell shared with manual casts or enemy summons.
	inline constexpr std::uint32_t kCalamitySummonRecord = 0x4353554Du;  // CSUM
	void SaveCalamitySummons(SKSE::SerializationInterface* a_intfc);
	void LoadCalamitySummons(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version, std::uint32_t a_length);
	void RestoreCalamitySummonsAfterLoad();
}
