#pragma once

#include "RunewordIdentityOverrides.h"

#include <RE/Skyrim.h>

namespace CalamityAffixes::RunewordSynthesis
{
	struct SpellSet
	{
		RE::SpellItem* dynamicFire{ nullptr };
		RE::SpellItem* dynamicFrost{ nullptr };
		RE::SpellItem* dynamicShock{ nullptr };
		RE::SpellItem* spellArcLightning{ nullptr };
		RE::SpellItem* shredFire{ nullptr };
		RE::SpellItem* shredFrost{ nullptr };
		RE::SpellItem* shredShock{ nullptr };
		RE::SpellItem* spellWard{ nullptr };
		RE::SpellItem* spellPhase{ nullptr };
		RE::SpellItem* spellMeditation{ nullptr };
		RE::SpellItem* spellBarrier{ nullptr };
		RE::SpellItem* spellPhoenix{ nullptr };
		RE::SpellItem* spellHaste{ nullptr };
		RE::SpellItem* spellDotPoison{ nullptr };
		RE::SpellItem* spellDotTar{ nullptr };
		RE::SpellItem* spellDotSiphon{ nullptr };
		RE::SpellItem* spellChaosSunder{ nullptr };
		RE::SpellItem* spellChaosFragile{ nullptr };
		RE::SpellItem* spellChaosSlowAttack{ nullptr };
		RE::SpellItem* spellVanillaSoulTrap{ nullptr };
		RE::SpellItem* spellCustomInvisibility{ nullptr };
		RE::SpellItem* spellVanillaMuffle{ nullptr };
		RE::SpellItem* spellVanillaFlameCloak{ nullptr };
		RE::SpellItem* spellVanillaFrostCloak{ nullptr };
		RE::SpellItem* spellVanillaShockCloak{ nullptr };
		RE::SpellItem* spellVanillaOakflesh{ nullptr };
		RE::SpellItem* spellVanillaStoneflesh{ nullptr };
		RE::SpellItem* spellVanillaIronflesh{ nullptr };
		RE::SpellItem* spellVanillaEbonyflesh{ nullptr };
		RE::SpellItem* spellVanillaFear{ nullptr };
		RE::SpellItem* spellVanillaFrenzy{ nullptr };
		RE::SpellItem* spellSummonFamiliar{ nullptr };
		RE::SpellItem* spellSummonFlameAtronach{ nullptr };
		RE::SpellItem* spellSummonFrostAtronach{ nullptr };
		RE::SpellItem* spellSummonStormAtronach{ nullptr };
		RE::SpellItem* spellSummonDremoraLord{ nullptr };

		[[nodiscard]] RE::SpellItem* ResolveIdentitySpell(detail::RunewordIdentitySpellRole a_role) const;
	};

	[[nodiscard]] SpellSet BuildSpellSet(RE::TESDataHandler* a_handler);
}
