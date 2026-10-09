#include "EventBridge.Config.RunewordSynthesis.SpellSet.h"
#include "EventBridge.Config.Shared.h"

#include "CalamityAffixes/PluginEditorIds.h"

#include <string_view>

namespace CalamityAffixes::RunewordSynthesis
{
	SpellSet BuildSpellSet(RE::TESDataHandler* a_handler)
	{
		const auto parseSpellFromString = [a_handler](std::string_view a_spec) -> RE::SpellItem* {
			return ConfigShared::LookupSpellFromSpec(a_spec, a_handler);
		};

		SpellSet out{};
		out.dynamicFire = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_DMG_FIRE_DYNAMIC");
		out.dynamicFrost = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_DMG_FROST_DYNAMIC");
		out.dynamicShock = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_DMG_SHOCK_DYNAMIC");
		out.spellArcLightning = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_ARC_LIGHTNING");
		out.shredFire = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_FIRE_SHRED");
		out.shredFrost = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_FROST_SHRED");
		out.shredShock = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_SHOCK_SHRED");
		out.spellWard = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_RW_FORTITUDE_WARD");
		out.spellPhase = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_RW_ENIGMA_PHASE");
		out.spellMeditation = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_RW_INSIGHT_MEDITATION");
		out.spellBarrier = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_RW_EXILE_BARRIER");
		out.spellPhoenix = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_RW_PHOENIX_SURGE");
		out.spellHaste = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_SWAP_JACKPOT_HASTE");
		out.spellDotPoison = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_DOT_BLOOM_POISON");
		out.spellDotTar = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_DOT_BLOOM_TAR_SLOW");
		out.spellDotSiphon = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_DOT_BLOOM_SIPHON_MAG");
		out.spellChaosSunder = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_CHAOS_CURSE_SUNDER");
		out.spellChaosFragile = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_CHAOS_CURSE_FRAGILE");
		out.spellChaosSlowAttack = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_CHAOS_CURSE_SLOW_ATTACK");
		out.spellVanillaSoulTrap = parseSpellFromString("Skyrim.esm|0004DBA4");
		out.spellCustomInvisibility = PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_INVISIBILITY");
		out.spellVanillaMuffle = parseSpellFromString("Skyrim.esm|0008F3EB");
		out.spellVanillaFlameCloak = parseSpellFromString("Skyrim.esm|0003AE9F");
		out.spellVanillaFrostCloak = parseSpellFromString("Skyrim.esm|0003AEA2");
		out.spellVanillaShockCloak = parseSpellFromString("Skyrim.esm|0003AEA3");
		out.spellVanillaOakflesh = parseSpellFromString("Skyrim.esm|0005AD5C");
		out.spellVanillaStoneflesh = parseSpellFromString("Skyrim.esm|0005AD5D");
		out.spellVanillaIronflesh = parseSpellFromString("Skyrim.esm|00051B16");
		out.spellVanillaEbonyflesh = parseSpellFromString("Skyrim.esm|0005AD5E");
		out.spellVanillaFear = parseSpellFromString("Skyrim.esm|0004DEEA");
		out.spellVanillaFrenzy = parseSpellFromString("Skyrim.esm|0004DEEE");
		out.spellSummonFamiliar = parseSpellFromString("Skyrim.esm|000640B6");
		out.spellSummonFlameAtronach = parseSpellFromString("Skyrim.esm|000204C3");
		out.spellSummonFrostAtronach = parseSpellFromString("Skyrim.esm|000204C4");
		out.spellSummonStormAtronach = parseSpellFromString("Skyrim.esm|000204C5");
		out.spellSummonDremoraLord = parseSpellFromString("Skyrim.esm|0010DDEC");
		return out;
	}

	RE::SpellItem* SpellSet::ResolveIdentitySpell(detail::RunewordIdentitySpellRole a_role) const
	{
		switch (a_role) {
		case detail::RunewordIdentitySpellRole::kDynamicFire:
			return dynamicFire;
		case detail::RunewordIdentitySpellRole::kDynamicFrost:
			return dynamicFrost;
		case detail::RunewordIdentitySpellRole::kDynamicShock:
			return dynamicShock;
		case detail::RunewordIdentitySpellRole::kArcLightning:
			return spellArcLightning ? spellArcLightning : dynamicShock;
		case detail::RunewordIdentitySpellRole::kHaste:
			return spellHaste;
		case detail::RunewordIdentitySpellRole::kMeditation:
			return spellMeditation;
		case detail::RunewordIdentitySpellRole::kWard:
			return spellWard;
		case detail::RunewordIdentitySpellRole::kPhase:
			return spellPhase;
		case detail::RunewordIdentitySpellRole::kDotPoison:
			return spellDotPoison;
		case detail::RunewordIdentitySpellRole::kDotTar:
			return spellDotTar;
		case detail::RunewordIdentitySpellRole::kDotSiphon:
			return spellDotSiphon;
		case detail::RunewordIdentitySpellRole::kChaosSunder:
			return spellChaosSunder;
		case detail::RunewordIdentitySpellRole::kChaosFragile:
			return spellChaosFragile;
		case detail::RunewordIdentitySpellRole::kChaosSlowAttack:
			return spellChaosSlowAttack;
		case detail::RunewordIdentitySpellRole::kFear:
			return spellVanillaFear ? spellVanillaFear : spellChaosSlowAttack;
		case detail::RunewordIdentitySpellRole::kSoulTrap:
			return spellVanillaSoulTrap ? spellVanillaSoulTrap : spellChaosFragile;
		default:
			return nullptr;
		}
	}
}
