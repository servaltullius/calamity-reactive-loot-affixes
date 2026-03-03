#include "CalamityAffixes/EventBridge.h"

#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace CalamityAffixes
{
	namespace
	{
		[[nodiscard]] std::uint32_t RecipeBucket(std::string_view a_id, std::uint32_t a_modulo)
		{
			if (a_modulo == 0u) {
				return 0u;
			}
			std::uint32_t hash = 2166136261u;
			for (const auto c : a_id) {
				hash ^= static_cast<std::uint8_t>(c);
				hash *= 16777619u;
			}
			return hash % a_modulo;
		}

		[[nodiscard]] bool IdIsOneOf(std::string_view a_id, std::initializer_list<std::string_view> a_candidates)
		{
			for (const auto candidate : a_candidates) {
				if (a_id == candidate) {
					return true;
				}
			}
			return false;
		}
	}

	EventBridge::SyntheticRunewordStyle EventBridge::ResolveSyntheticRunewordStyle(const RunewordRecipe& a_recipe)
	{
		const std::string_view id = a_recipe.id;
		if (id == "rw_grief") {
			return SyntheticRunewordStyle::kSignatureGrief;
		}
		if (id == "rw_infinity") {
			return SyntheticRunewordStyle::kSignatureInfinity;
		}
		if (id == "rw_enigma") {
			return SyntheticRunewordStyle::kSignatureEnigma;
		}
		if (id == "rw_call_to_arms") {
			return SyntheticRunewordStyle::kSignatureCallToArms;
		}
		if (IdIsOneOf(id, { "rw_breath_of_the_dying" })) {
			return SyntheticRunewordStyle::kAdaptiveStrike;
		}
		if (IdIsOneOf(id, { "rw_heart_of_the_oak", "rw_last_wish" })) {
			return SyntheticRunewordStyle::kAdaptiveExposure;
		}
		if (IdIsOneOf(id, { "rw_fortitude" })) {
			return SyntheticRunewordStyle::kSelfWard;
		}
		if (IdIsOneOf(id, { "rw_exile" })) {
			return SyntheticRunewordStyle::kSelfBarrier;
		}
		if (IdIsOneOf(id, { "rw_spirit", "rw_insight" })) {
			return SyntheticRunewordStyle::kSelfMeditation;
		}
		if (IdIsOneOf(id, { "rw_chains_of_honor" })) {
			return SyntheticRunewordStyle::kSelfPhase;
		}
		if (IdIsOneOf(id, { "rw_phoenix" })) {
			return SyntheticRunewordStyle::kSelfPhoenix;
		}
		if (IdIsOneOf(id, { "rw_faith" })) {
			return SyntheticRunewordStyle::kSelfHaste;
		}
		if (IdIsOneOf(id, { "rw_doom" })) {
			return SyntheticRunewordStyle::kFrostStrike;
		}
		if (IdIsOneOf(id, { "rw_dream" })) {
			return SyntheticRunewordStyle::kShockStrike;
		}
		if (IdIsOneOf(id, { "rw_hand_of_justice", "rw_fury", "rw_memory" })) {
			return SyntheticRunewordStyle::kSelfHaste;
		}
		if (IdIsOneOf(id, { "rw_dragon", "rw_flickering_flame" })) {
			return SyntheticRunewordStyle::kSelfFlameCloak;
		}
		if (IdIsOneOf(id, { "rw_ice", "rw_voice_of_reason", "rw_hearth" })) {
			return SyntheticRunewordStyle::kSelfFrostCloak;
		}
		if (IdIsOneOf(id, { "rw_holy_thunder", "rw_zephyr", "rw_wind" })) {
			return SyntheticRunewordStyle::kSelfShockCloak;
		}
		if (IdIsOneOf(id, { "rw_bulwark", "rw_cure", "rw_ancients_pledge", "rw_lionheart", "rw_radiance" })) {
			return SyntheticRunewordStyle::kSelfOakflesh;
		}
		if (IdIsOneOf(id, { "rw_sanctuary", "rw_duress", "rw_peace", "rw_myth" })) {
			return SyntheticRunewordStyle::kSelfStoneflesh;
		}
		if (IdIsOneOf(id, { "rw_pride", "rw_stone" })) {
			return SyntheticRunewordStyle::kSelfIronflesh;
		}
		if (IdIsOneOf(id, { "rw_metamorphosis" })) {
			return SyntheticRunewordStyle::kSelfEbonyflesh;
		}
		if (IdIsOneOf(id, { "rw_nadir" })) {
			return SyntheticRunewordStyle::kCurseFear;
		}
		if (IdIsOneOf(id, { "rw_delirium" })) {
			return SyntheticRunewordStyle::kCurseFrenzy;
		}
		if (IdIsOneOf(id, { "rw_malice", "rw_venom", "rw_plague", "rw_bramble" })) {
			return SyntheticRunewordStyle::kPoisonBloom;
		}
		if (IdIsOneOf(id, { "rw_black", "rw_rift" })) {
			return SyntheticRunewordStyle::kTarBloom;
		}
		if (IdIsOneOf(id, { "rw_mosaic", "rw_obsession" })) {
			return SyntheticRunewordStyle::kSiphonBloom;
		}
		if (IdIsOneOf(id, { "rw_lawbringer", "rw_wrath", "rw_kingslayer", "rw_principle" })) {
			return SyntheticRunewordStyle::kCurseFragile;
		}
		if (IdIsOneOf(id, { "rw_death", "rw_famine" })) {
			return SyntheticRunewordStyle::kCurseSlowAttack;
		}
		if (IdIsOneOf(id, { "rw_leaf", "rw_destruction" })) {
			return SyntheticRunewordStyle::kFireStrike;
		}
		if (IdIsOneOf(id, { "rw_crescent_moon" })) {
			return SyntheticRunewordStyle::kShockStrike;
		}
		if (IdIsOneOf(id, { "rw_beast", "rw_hustle_w", "rw_harmony", "rw_unbending_will", "rw_passion" })) {
			return SyntheticRunewordStyle::kSelfHaste;
		}
		if (IdIsOneOf(id, { "rw_stealth", "rw_smoke", "rw_treachery" })) {
			return SyntheticRunewordStyle::kSelfMuffle;
		}
		if (IdIsOneOf(id, { "rw_gloom" })) {
			return SyntheticRunewordStyle::kSelfInvisibility;
		}
		if (IdIsOneOf(id, { "rw_wealth", "rw_obedience", "rw_honor", "rw_eternity" })) {
			return SyntheticRunewordStyle::kSoulTrap;
		}
		if (IdIsOneOf(id, { "rw_lore", "rw_melody", "rw_enlightenment" })) {
			return SyntheticRunewordStyle::kSelfMeditation;
		}
		if (IdIsOneOf(id, { "rw_steel", "rw_pattern", "rw_strength", "rw_kings_grace", "rw_edge", "rw_oath" })) {
			return SyntheticRunewordStyle::kAdaptiveStrike;
		}
		if (IdIsOneOf(id, { "rw_silence", "rw_brand" })) {
			return SyntheticRunewordStyle::kAdaptiveExposure;
		}
		if (IdIsOneOf(id, { "rw_hustle_a", "rw_rhyme" })) {
			return SyntheticRunewordStyle::kSelfPhase;
		}
		if (IdIsOneOf(id, { "rw_rain", "rw_ground" })) {
			return SyntheticRunewordStyle::kSelfPhoenix;
		}
		if (IdIsOneOf(id, { "rw_temper", "rw_prudence", "rw_bone", "rw_splendor" })) {
			return SyntheticRunewordStyle::kSelfWard;
		}
		if (IdIsOneOf(id, { "rw_mist" })) {
			return SyntheticRunewordStyle::kFrostStrike;
		}
		if (IdIsOneOf(id, { "rw_white" })) {
			return SyntheticRunewordStyle::kSelfMeditation;
		}
		if (IdIsOneOf(id, { "rw_wisdom" })) {
			return SyntheticRunewordStyle::kSelfPhoenix;
		}
		if (IdIsOneOf(id, { "rw_chaos" })) {
			return SyntheticRunewordStyle::kAdaptiveStrike;
		}

		if (a_recipe.recommendedBaseType) {
			if (*a_recipe.recommendedBaseType == LootItemType::kWeapon) {
				switch (RecipeBucket(id, 6u)) {
				case 0u:
					return SyntheticRunewordStyle::kAdaptiveStrike;
				case 1u:
					return SyntheticRunewordStyle::kAdaptiveExposure;
				case 2u:
					return SyntheticRunewordStyle::kFireStrike;
				case 3u:
					return SyntheticRunewordStyle::kFrostStrike;
				case 4u:
					return SyntheticRunewordStyle::kShockStrike;
				default:
					return SyntheticRunewordStyle::kSelfHaste;
				}
			}
			if (*a_recipe.recommendedBaseType == LootItemType::kArmor) {
				switch (RecipeBucket(id, 6u)) {
				case 0u:
					return SyntheticRunewordStyle::kSelfWard;
				case 1u:
					return SyntheticRunewordStyle::kSelfBarrier;
				case 2u:
					return SyntheticRunewordStyle::kSelfMeditation;
				case 3u:
					return SyntheticRunewordStyle::kSelfPhase;
				case 4u:
					return SyntheticRunewordStyle::kSelfMuffle;
				default:
					return SyntheticRunewordStyle::kSelfPhoenix;
				}
			}
		}

		return RecipeBucket(id, 2u) == 0u ? SyntheticRunewordStyle::kAdaptiveExposure : SyntheticRunewordStyle::kAdaptiveStrike;
	}

	std::string_view EventBridge::DescribeSyntheticRunewordStyle(SyntheticRunewordStyle a_style)
	{
		switch (a_style) {
		case SyntheticRunewordStyle::kAdaptiveStrike:
			return "AdaptiveStrike";
		case SyntheticRunewordStyle::kAdaptiveExposure:
			return "AdaptiveExposure";
		case SyntheticRunewordStyle::kSignatureGrief:
			return "SignatureGrief";
		case SyntheticRunewordStyle::kSignatureInfinity:
			return "SignatureInfinity";
		case SyntheticRunewordStyle::kSignatureEnigma:
			return "SignatureEnigma";
		case SyntheticRunewordStyle::kSignatureCallToArms:
			return "SignatureCallToArms";
		case SyntheticRunewordStyle::kFireStrike:
			return "FireStrike";
		case SyntheticRunewordStyle::kFrostStrike:
			return "FrostStrike";
		case SyntheticRunewordStyle::kShockStrike:
			return "ShockStrike";
		case SyntheticRunewordStyle::kPoisonBloom:
			return "PoisonBloom";
		case SyntheticRunewordStyle::kTarBloom:
			return "TarBloom";
		case SyntheticRunewordStyle::kSiphonBloom:
			return "SiphonBloom";
		case SyntheticRunewordStyle::kCurseFragile:
			return "CurseFragile";
		case SyntheticRunewordStyle::kCurseSlowAttack:
			return "CurseSlowAttack";
		case SyntheticRunewordStyle::kCurseFear:
			return "CurseFear";
		case SyntheticRunewordStyle::kCurseFrenzy:
			return "CurseFrenzy";
		case SyntheticRunewordStyle::kSelfHaste:
			return "SelfHaste";
		case SyntheticRunewordStyle::kSelfWard:
			return "SelfWard";
		case SyntheticRunewordStyle::kSelfBarrier:
			return "SelfBarrier";
		case SyntheticRunewordStyle::kSelfMeditation:
			return "SelfMeditation";
		case SyntheticRunewordStyle::kSelfPhase:
			return "SelfPhase";
		case SyntheticRunewordStyle::kSelfPhoenix:
			return "SelfPhoenix";
		case SyntheticRunewordStyle::kSelfFlameCloak:
			return "SelfFlameCloak";
		case SyntheticRunewordStyle::kSelfFrostCloak:
			return "SelfFrostCloak";
		case SyntheticRunewordStyle::kSelfShockCloak:
			return "SelfShockCloak";
		case SyntheticRunewordStyle::kSelfOakflesh:
			return "SelfOakflesh";
		case SyntheticRunewordStyle::kSelfStoneflesh:
			return "SelfStoneflesh";
		case SyntheticRunewordStyle::kSelfIronflesh:
			return "SelfIronflesh";
		case SyntheticRunewordStyle::kSelfEbonyflesh:
			return "SelfEbonyflesh";
		case SyntheticRunewordStyle::kSelfMuffle:
			return "SelfMuffle";
		case SyntheticRunewordStyle::kSelfInvisibility:
			return "SelfInvisibility";
		case SyntheticRunewordStyle::kSoulTrap:
			return "SoulTrap";
		case SyntheticRunewordStyle::kSummonFamiliar:
			return "SummonFamiliar";
		case SyntheticRunewordStyle::kSummonFlameAtronach:
			return "SummonFlameAtronach";
		case SyntheticRunewordStyle::kSummonFrostAtronach:
			return "SummonFrostAtronach";
		case SyntheticRunewordStyle::kSummonStormAtronach:
			return "SummonStormAtronach";
		case SyntheticRunewordStyle::kSummonDremoraLord:
			return "SummonDremoraLord";
		case SyntheticRunewordStyle::kLegacyFallback:
			return "LegacyFallback";
		default:
			return "Unknown";
		}
	}
}
