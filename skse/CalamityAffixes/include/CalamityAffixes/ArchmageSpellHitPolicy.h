#pragma once

#include <cstdint>

namespace CalamityAffixes::detail
{
	// Archmage reads the spell from TESHitEvent::source. HitData never names a
	// cast spell (attackDataSpell is only set for melee attack-data spells), so
	// the hit event is the one place a player's Firebolt is visible as a spell.
	//
	// Only cast spells and scrolls count. Shouts, powers, abilities, poisons,
	// enchantments and staffs are not "spell hits".
	[[nodiscard]] constexpr bool IsArchmageSourceSpellType(bool a_isCastSpell, bool a_isScroll) noexcept
	{
		return a_isCastSpell || a_isScroll;
	}

	// A concentration spell raises a TESHitEvent on every effect refresh, and
	// one projectile can raise more than one event. Archmage has no ICD, so one
	// (target, spell) pair is evaluated at most once per window.
	inline constexpr std::uint64_t kArchmageSpellHitDedupWindowMs = 250u;
	inline constexpr std::uint64_t kArchmageConcentrationHitDedupWindowMs = 1000u;

	[[nodiscard]] constexpr std::uint64_t ResolveArchmageSpellHitDedupWindowMs(bool a_isConcentration) noexcept
	{
		return a_isConcentration ? kArchmageConcentrationHitDedupWindowMs : kArchmageSpellHitDedupWindowMs;
	}

	[[nodiscard]] constexpr bool ShouldSuppressArchmageSpellHit(
		bool a_hasRecord,
		std::uint64_t a_elapsedMs,
		std::uint64_t a_windowMs) noexcept
	{
		return a_hasRecord && a_elapsedMs < a_windowMs;
	}
}
