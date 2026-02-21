#include "CalamityAffixes/EventBridge.h"
#include "EventBridge.Config.Shared.h"
#include "RunewordIdentityOverrides.h"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace CalamityAffixes
{
	namespace
	{
		using ConfigShared::LookupSpellFromSpec;
	}

	void EventBridge::SynthesizeRunewordRuntimeAffixes()
	{
		auto* handler = RE::TESDataHandler::GetSingleton();
		auto parseSpellFromString = [handler](std::string_view a_spec) -> RE::SpellItem* {
			return LookupSpellFromSpec(a_spec, handler);
		};

			auto* dynamicFire = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DMG_FIRE_DYNAMIC");
			auto* dynamicFrost = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DMG_FROST_DYNAMIC");
			auto* dynamicShock = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DMG_SHOCK_DYNAMIC");
			auto* spellArcLightning = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_ARC_LIGHTNING");
			auto* shredFire = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_FIRE_SHRED");
			auto* shredFrost = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_FROST_SHRED");
			auto* shredShock = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_SHOCK_SHRED");
			auto* spellWard = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_FORTITUDE_WARD");
			auto* spellPhase = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_ENIGMA_PHASE");
			auto* spellMeditation = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_INSIGHT_MEDITATION");
			auto* spellBarrier = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_EXILE_BARRIER");
			auto* spellPhoenix = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_PHOENIX_SURGE");
			auto* spellHaste = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_SWAP_JACKPOT_HASTE");
			auto* spellDotPoison = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DOT_BLOOM_POISON");
			auto* spellDotTar = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DOT_BLOOM_TAR_SLOW");
			auto* spellDotSiphon = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DOT_BLOOM_SIPHON_MAG");
			auto* spellChaosSunder = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_CHAOS_CURSE_SUNDER");
			auto* spellChaosFragile = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_CHAOS_CURSE_FRAGILE");
			auto* spellChaosSlowAttack = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_CHAOS_CURSE_SLOW_ATTACK");
			auto* spellVanillaSoulTrap = parseSpellFromString("Skyrim.esm|0004DBA4");
			auto* spellCustomInvisibility = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_INVISIBILITY");
			auto* spellVanillaMuffle = parseSpellFromString("Skyrim.esm|0008F3EB");
			auto* spellVanillaFlameCloak = parseSpellFromString("Skyrim.esm|0003AE9F");
			auto* spellVanillaFrostCloak = parseSpellFromString("Skyrim.esm|0003AEA2");
			auto* spellVanillaShockCloak = parseSpellFromString("Skyrim.esm|0003AEA3");
			auto* spellVanillaOakflesh = parseSpellFromString("Skyrim.esm|0005AD5C");
			auto* spellVanillaStoneflesh = parseSpellFromString("Skyrim.esm|0005AD5D");
			auto* spellVanillaIronflesh = parseSpellFromString("Skyrim.esm|00051B16");
			auto* spellVanillaEbonyflesh = parseSpellFromString("Skyrim.esm|0005AD5E");
			auto* spellVanillaFear = parseSpellFromString("Skyrim.esm|0004DEEA");
			auto* spellVanillaFrenzy = parseSpellFromString("Skyrim.esm|0004DEEE");
			auto* spellSummonFamiliar = parseSpellFromString("Skyrim.esm|000640B6");
		auto* spellSummonFlameAtronach = parseSpellFromString("Skyrim.esm|000204C3");
		auto* spellSummonFrostAtronach = parseSpellFromString("Skyrim.esm|000204C4");
		auto* spellSummonStormAtronach = parseSpellFromString("Skyrim.esm|000204C5");
		auto* spellSummonDremoraLord = parseSpellFromString("Skyrim.esm|0010DDEC");

		auto resolveIdentitySpell = [&](detail::RunewordIdentitySpellRole a_role) -> RE::SpellItem* {
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
		};

			enum class SyntheticRunewordStyle : std::uint8_t
			{
				kAdaptiveStrike,
				kAdaptiveExposure,
				kSignatureGrief,
				kSignatureInfinity,
				kSignatureEnigma,
				kSignatureCallToArms,
				kFireStrike,
				kFrostStrike,
				kShockStrike,
				kPoisonBloom,
				kTarBloom,
				kSiphonBloom,
				kCurseFragile,
				kCurseSlowAttack,
				kCurseFear,
				kCurseFrenzy,
				kSelfHaste,
				kSelfWard,
				kSelfBarrier,
				kSelfMeditation,
				kSelfPhase,
				kSelfPhoenix,
				kSelfFlameCloak,
				kSelfFrostCloak,
				kSelfShockCloak,
				kSelfOakflesh,
				kSelfStoneflesh,
				kSelfIronflesh,
				kSelfEbonyflesh,
				kSelfMuffle,
				kSelfInvisibility,
				kSoulTrap,
				kSummonFamiliar,
				kSummonFlameAtronach,
				kSummonFrostAtronach,
				kSummonStormAtronach,
				kSummonDremoraLord,
				kLegacyFallback
			};

			auto styleName = [](SyntheticRunewordStyle a_style) -> std::string_view {
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
			};

			auto recipeBucket = [](std::string_view a_id, std::uint32_t a_modulo) -> std::uint32_t {
				if (a_modulo == 0u) {
					return 0u;
				}
				std::uint32_t hash = 2166136261u;
				for (const auto c : a_id) {
					hash ^= static_cast<std::uint8_t>(c);
					hash *= 16777619u;
				}
				return hash % a_modulo;
			};

			auto idIsOneOf = [](std::string_view a_id, std::initializer_list<std::string_view> a_candidates) -> bool {
				for (const auto candidate : a_candidates) {
					if (a_id == candidate) {
						return true;
					}
				}
				return false;
			};

				auto resolveRunewordStyle = [&](const RunewordRecipe& a_recipe) -> SyntheticRunewordStyle {
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
					if (idIsOneOf(id, { "rw_breath_of_the_dying" })) {
						return SyntheticRunewordStyle::kAdaptiveStrike;
					}
					if (idIsOneOf(id, { "rw_heart_of_the_oak", "rw_last_wish" })) {
						return SyntheticRunewordStyle::kAdaptiveExposure;
					}
					if (idIsOneOf(id, { "rw_fortitude" })) {
						return SyntheticRunewordStyle::kSelfWard;
					}
					if (idIsOneOf(id, { "rw_exile" })) {
						return SyntheticRunewordStyle::kSelfBarrier;
					}
					if (idIsOneOf(id, { "rw_spirit", "rw_insight" })) {
						return SyntheticRunewordStyle::kSelfMeditation;
					}
					if (idIsOneOf(id, { "rw_chains_of_honor" })) {
						return SyntheticRunewordStyle::kSelfPhase;
					}
					if (idIsOneOf(id, { "rw_phoenix" })) {
						return SyntheticRunewordStyle::kSelfPhoenix;
					}
					if (idIsOneOf(id, { "rw_faith" })) {
						return SyntheticRunewordStyle::kSelfHaste;
					}
					if (idIsOneOf(id, { "rw_doom" })) {
						return SyntheticRunewordStyle::kFrostStrike;
					}
					if (idIsOneOf(id, { "rw_dream" })) {
						return SyntheticRunewordStyle::kShockStrike;
					}
					if (idIsOneOf(id, { "rw_bone" })) {
						return SyntheticRunewordStyle::kSummonFamiliar;
					}
					if (idIsOneOf(id, { "rw_dragon", "rw_hand_of_justice", "rw_flickering_flame", "rw_temper" })) {
						return SyntheticRunewordStyle::kSelfFlameCloak;
					}
				if (idIsOneOf(id, { "rw_ice", "rw_voice_of_reason", "rw_hearth" })) {
					return SyntheticRunewordStyle::kSelfFrostCloak;
				}
				if (idIsOneOf(id, { "rw_holy_thunder", "rw_zephyr", "rw_wind" })) {
					return SyntheticRunewordStyle::kSelfShockCloak;
				}
				if (idIsOneOf(id, { "rw_bulwark", "rw_cure", "rw_ancients_pledge", "rw_lionheart", "rw_radiance" })) {
					return SyntheticRunewordStyle::kSelfOakflesh;
				}
				if (idIsOneOf(id, { "rw_sanctuary", "rw_duress", "rw_peace", "rw_myth" })) {
					return SyntheticRunewordStyle::kSelfStoneflesh;
				}
				if (idIsOneOf(id, { "rw_pride", "rw_stone", "rw_prudence", "rw_mist" })) {
					return SyntheticRunewordStyle::kSelfIronflesh;
				}
				if (idIsOneOf(id, { "rw_metamorphosis" })) {
					return SyntheticRunewordStyle::kSelfEbonyflesh;
				}
				if (idIsOneOf(id, { "rw_nadir" })) {
					return SyntheticRunewordStyle::kCurseFear;
				}
				if (idIsOneOf(id, { "rw_delirium", "rw_chaos" })) {
					return SyntheticRunewordStyle::kCurseFrenzy;
				}
				if (idIsOneOf(id, { "rw_malice", "rw_venom", "rw_plague", "rw_bramble" })) {
					return SyntheticRunewordStyle::kPoisonBloom;
				}
				if (idIsOneOf(id, { "rw_black", "rw_rift" })) {
					return SyntheticRunewordStyle::kTarBloom;
				}
				if (idIsOneOf(id, { "rw_mosaic", "rw_obsession", "rw_white" })) {
					return SyntheticRunewordStyle::kSiphonBloom;
				}
				if (idIsOneOf(id, { "rw_lawbringer", "rw_wrath", "rw_kingslayer", "rw_principle" })) {
					return SyntheticRunewordStyle::kCurseFragile;
				}
				if (idIsOneOf(id, { "rw_death", "rw_famine" })) {
					return SyntheticRunewordStyle::kCurseSlowAttack;
				}
				if (idIsOneOf(id, { "rw_leaf", "rw_destruction" })) {
					return SyntheticRunewordStyle::kFireStrike;
				}
				if (idIsOneOf(id, { "rw_crescent_moon" })) {
					return SyntheticRunewordStyle::kShockStrike;
				}
				if (idIsOneOf(id, { "rw_beast", "rw_hustle_w", "rw_harmony", "rw_fury", "rw_unbending_will", "rw_passion" })) {
					return SyntheticRunewordStyle::kSelfHaste;
				}
				if (idIsOneOf(id, { "rw_stealth", "rw_smoke", "rw_treachery" })) {
					return SyntheticRunewordStyle::kSelfMuffle;
				}
				if (idIsOneOf(id, { "rw_gloom" })) {
					return SyntheticRunewordStyle::kSelfInvisibility;
				}
				if (idIsOneOf(id, { "rw_wealth", "rw_obedience", "rw_honor", "rw_eternity" })) {
					return SyntheticRunewordStyle::kSoulTrap;
				}
				if (idIsOneOf(id, { "rw_memory", "rw_wisdom", "rw_lore", "rw_melody", "rw_enlightenment" })) {
					return SyntheticRunewordStyle::kSelfMeditation;
				}
				if (idIsOneOf(id, { "rw_steel", "rw_pattern", "rw_strength", "rw_kings_grace", "rw_edge", "rw_oath" })) {
					return SyntheticRunewordStyle::kAdaptiveStrike;
				}
				if (idIsOneOf(id, { "rw_silence", "rw_brand" })) {
					return SyntheticRunewordStyle::kAdaptiveExposure;
				}
				if (idIsOneOf(id, { "rw_hustle_a", "rw_splendor", "rw_rhyme" })) {
					return SyntheticRunewordStyle::kSelfPhase;
				}
				if (idIsOneOf(id, { "rw_rain", "rw_ground" })) {
					return SyntheticRunewordStyle::kSelfPhoenix;
				}

				if (a_recipe.recommendedBaseType) {
					if (*a_recipe.recommendedBaseType == LootItemType::kWeapon) {
						switch (recipeBucket(id, 6u)) {
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
						switch (recipeBucket(id, 6u)) {
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

				return recipeBucket(id, 2u) == 0u ? SyntheticRunewordStyle::kAdaptiveExposure : SyntheticRunewordStyle::kAdaptiveStrike;
			};

			auto toDurationMs = [](float a_seconds) -> std::chrono::milliseconds {
				const float clamped = std::max(0.0f, a_seconds);
				return std::chrono::milliseconds(static_cast<std::int64_t>(clamped * 1000.0f));
			};

			auto applyLegacyRunewordFallback = [&](AffixRuntime& a_out, const RunewordRecipe& a_recipe, std::uint32_t a_runeCount) -> bool {
				if (a_recipe.recommendedBaseType && *a_recipe.recommendedBaseType == LootItemType::kArmor) {
					RE::SpellItem* spell = nullptr;
					switch (a_runeCount % 4u) {
					case 0u:
						spell = spellWard;
						break;
					case 1u:
						spell = spellPhase;
						break;
					case 2u:
						spell = spellMeditation;
						break;
					default:
						spell = spellBarrier;
						break;
					}
					if (!spell) {
						spell = spellWard ? spellWard : (spellPhase ? spellPhase : (spellMeditation ? spellMeditation : spellBarrier));
					}
					if (!spell) {
						return false;
					}
					a_out.trigger = Trigger::kIncomingHit;
					a_out.procChancePct = std::clamp(14.0f + static_cast<float>(a_runeCount) * 1.9f, 14.0f, 28.0f);
					const float icdSeconds = std::clamp(20.0f - static_cast<float>(a_runeCount) * 1.2f, 8.0f, 20.0f);
					a_out.icd = toDurationMs(icdSeconds);
					a_out.action.type = ActionType::kCastSpell;
					a_out.action.spell = spell;
					a_out.action.applyToSelf = true;
					a_out.action.noHitEffectArt = true;
					return true;
				}
				if (a_recipe.recommendedBaseType && *a_recipe.recommendedBaseType == LootItemType::kWeapon) {
					if (!dynamicFire && !dynamicFrost && !dynamicShock) {
						return false;
					}
					a_out.trigger = Trigger::kHit;
					a_out.procChancePct = std::clamp(20.0f + static_cast<float>(a_runeCount) * 2.4f, 20.0f, 36.0f);
					const float icdSeconds = std::clamp(1.50f - static_cast<float>(a_runeCount) * 0.16f, 0.35f, 1.25f);
					a_out.icd = toDurationMs(icdSeconds);
					a_out.action.type = ActionType::kCastSpellAdaptiveElement;
					a_out.action.adaptiveMode = AdaptiveElementMode::kWeakestResist;
					a_out.action.adaptiveFire = dynamicFire;
					a_out.action.adaptiveFrost = dynamicFrost;
					a_out.action.adaptiveShock = dynamicShock;
					a_out.action.noHitEffectArt = false;
					a_out.action.magnitudeScaling.source = MagnitudeScaling::Source::kHitPhysicalDealt;
					a_out.action.magnitudeScaling.mult =
						std::clamp(0.08f + static_cast<float>(a_runeCount) * 0.012f, 0.08f, 0.18f);
					a_out.action.magnitudeScaling.spellBaseAsMin = true;
					return true;
				}
				if (!shredFire && !shredFrost && !shredShock) {
					return false;
				}
				a_out.trigger = Trigger::kHit;
				a_out.procChancePct = std::clamp(18.0f + static_cast<float>(a_runeCount) * 2.0f, 18.0f, 34.0f);
				const float icdSeconds = std::clamp(3.2f - static_cast<float>(a_runeCount) * 0.45f, 0.9f, 3.2f);
				a_out.icd = toDurationMs(icdSeconds);
				a_out.action.type = ActionType::kCastSpellAdaptiveElement;
				a_out.action.adaptiveMode = AdaptiveElementMode::kWeakestResist;
				a_out.action.adaptiveFire = shredFire;
				a_out.action.adaptiveFrost = shredFrost;
				a_out.action.adaptiveShock = shredShock;
				a_out.action.noHitEffectArt = false;
				return true;
			};

		std::string runewordTemplateKeywordEditorId;
		RE::BGSKeyword* runewordTemplateKeyword = nullptr;
		for (const auto& affix : _affixes) {
			if (affix.id.rfind("runeword_", 0) == 0 && affix.keyword) {
				runewordTemplateKeywordEditorId = affix.keywordEditorId;
				runewordTemplateKeyword = affix.keyword;
				break;
			}
		}

		if (!_runewordRecipes.empty()) {
			// Runeword synthesis appends many entries; reserve once to reduce realloc churn.
			_affixes.reserve(_affixes.size() + _runewordRecipes.size());
		}

		std::uint32_t synthesizedRunewordAffixes = 0u;
		for (const auto& recipe : _runewordRecipes) {
			if (recipe.resultAffixToken == 0u || _affixIndexByToken.contains(recipe.resultAffixToken)) {
				continue;
			}

			const std::uint32_t runeCount =
				static_cast<std::uint32_t>(std::max<std::size_t>(recipe.runeIds.size(), 1u));

			std::string runeSequence;
			for (std::size_t i = 0; i < recipe.runeIds.size(); ++i) {
				if (i > 0) {
					runeSequence.push_back('-');
				}
				runeSequence.append(recipe.runeIds[i]);
			}

			AffixRuntime out{};
			out.id = recipe.id + "_auto";
			out.token = recipe.resultAffixToken;
			out.action.sourceToken = out.token;
			const std::string suffix = runeSequence.empty() ? "(Auto)" : ("[" + runeSequence + "]");
			const std::string recipeNameEn = recipe.displayNameEn.empty() ? recipe.displayName : recipe.displayNameEn;
			const std::string recipeNameKo = recipe.displayNameKo.empty() ? recipe.displayName : recipe.displayNameKo;
			out.displayNameEn = "Runeword " + recipeNameEn + " " + suffix;
			out.displayNameKo = "룬워드 " + recipeNameKo + " " + suffix;
			out.displayName = out.displayNameKo;
			out.label = recipeNameKo;
			if (runewordTemplateKeyword) {
				out.keywordEditorId = runewordTemplateKeywordEditorId;
				out.keyword = runewordTemplateKeyword;
			}

				auto style = resolveRunewordStyle(recipe);
				const float runeScale = static_cast<float>(runeCount);
				const bool armorRecommended =
					(recipe.recommendedBaseType && *recipe.recommendedBaseType == LootItemType::kArmor);
				const Trigger summonTrigger = armorRecommended ? Trigger::kIncomingHit : Trigger::kHit;
				const Trigger offensiveTrigger = armorRecommended ? Trigger::kIncomingHit : Trigger::kHit;
				const Trigger auraTrigger = armorRecommended ? Trigger::kIncomingHit : Trigger::kHit;

				auto configureAdaptiveSpell = [&](AdaptiveElementMode a_mode,
											 RE::SpellItem* a_fire,
											 RE::SpellItem* a_frost,
											 RE::SpellItem* a_shock,
											 float a_procBase,
											 float a_procPerRune,
											 float a_procMin,
											 float a_procMax,
											 float a_icdBase,
											 float a_icdPerRune,
											 float a_icdMin,
											 float a_icdMax,
											 float a_scaleBase,
											 float a_scalePerRune,
											 float a_perTargetIcdSeconds) -> bool {
					if (!a_fire && !a_frost && !a_shock) {
						return false;
					}
					out.trigger = Trigger::kHit;
					out.procChancePct = std::clamp(a_procBase + runeScale * a_procPerRune, a_procMin, a_procMax);
					out.icd = toDurationMs(std::clamp(a_icdBase - runeScale * a_icdPerRune, a_icdMin, a_icdMax));
					if (a_perTargetIcdSeconds > 0.0f) {
						out.perTargetIcd = toDurationMs(a_perTargetIcdSeconds);
					}
					out.action.type = ActionType::kCastSpellAdaptiveElement;
					out.action.adaptiveMode = a_mode;
					out.action.adaptiveFire = a_fire;
					out.action.adaptiveFrost = a_frost;
					out.action.adaptiveShock = a_shock;
					out.action.noHitEffectArt = false;
					if (a_scaleBase > 0.0f || a_scalePerRune > 0.0f) {
						out.action.magnitudeScaling.source = MagnitudeScaling::Source::kHitPhysicalDealt;
						out.action.magnitudeScaling.mult = std::clamp(a_scaleBase + runeScale * a_scalePerRune, 0.0f, 0.25f);
						out.action.magnitudeScaling.spellBaseAsMin = true;
					}
					return true;
				};

				auto configureSingleTargetSpell = [&](RE::SpellItem* a_spell,
												 float a_procBase,
												 float a_procPerRune,
												 float a_procMin,
												 float a_procMax,
												 float a_icdBase,
												 float a_icdPerRune,
												 float a_icdMin,
												 float a_icdMax,
												 float a_scaleBase,
												 float a_scalePerRune,
												 float a_perTargetIcdSeconds,
												 Trigger a_trigger = Trigger::kHit) -> bool {
					if (!a_spell) {
						return false;
					}
					out.trigger = a_trigger;
					out.procChancePct = std::clamp(a_procBase + runeScale * a_procPerRune, a_procMin, a_procMax);
					out.icd = toDurationMs(std::clamp(a_icdBase - runeScale * a_icdPerRune, a_icdMin, a_icdMax));
					if (a_perTargetIcdSeconds > 0.0f) {
						out.perTargetIcd = toDurationMs(a_perTargetIcdSeconds);
					}
					out.action.type = ActionType::kCastSpell;
					out.action.spell = a_spell;
					out.action.applyToSelf = false;
					out.action.noHitEffectArt = true;
					if (a_scaleBase > 0.0f || a_scalePerRune > 0.0f) {
						out.action.magnitudeScaling.source = MagnitudeScaling::Source::kHitPhysicalDealt;
						out.action.magnitudeScaling.mult = std::clamp(a_scaleBase + runeScale * a_scalePerRune, 0.0f, 0.25f);
						out.action.magnitudeScaling.spellBaseAsMin = true;
					}
					return true;
				};

				auto configureSelfBuff = [&](RE::SpellItem* a_spell,
										 Trigger a_trigger,
										 float a_procBase,
										 float a_procPerRune,
										 float a_procMin,
										 float a_procMax,
										 float a_icdBase,
										 float a_icdPerRune,
										 float a_icdMin,
										 float a_icdMax) -> bool {
					if (!a_spell) {
						return false;
					}
					out.trigger = a_trigger;
					out.procChancePct = std::clamp(a_procBase + runeScale * a_procPerRune, a_procMin, a_procMax);
					out.icd = toDurationMs(std::clamp(a_icdBase - runeScale * a_icdPerRune, a_icdMin, a_icdMax));
					out.action.type = ActionType::kCastSpell;
					out.action.spell = a_spell;
					out.action.applyToSelf = true;
					out.action.noHitEffectArt = true;
					return true;
				};

				auto configureSummonSpell = [&](RE::SpellItem* a_spell,
										   float a_procBase,
										   float a_procPerRune,
										   float a_procMin,
										   float a_procMax,
										   float a_icdBase,
										   float a_icdPerRune,
										   float a_icdMin,
										   float a_icdMax) -> bool {
					if (!a_spell) {
						return false;
					}
					out.trigger = summonTrigger;
					out.procChancePct = std::clamp(a_procBase + runeScale * a_procPerRune, a_procMin, a_procMax);
					out.icd = toDurationMs(std::clamp(a_icdBase - runeScale * a_icdPerRune, a_icdMin, a_icdMax));
					out.action.type = ActionType::kCastSpell;
					out.action.spell = a_spell;
					out.action.applyToSelf = true;
					out.action.noHitEffectArt = true;
					return true;
				};

				bool ready = false;
				switch (style) {
				case SyntheticRunewordStyle::kAdaptiveStrike:
					ready = configureAdaptiveSpell(
						AdaptiveElementMode::kWeakestResist,
						dynamicFire,
						dynamicFrost,
						dynamicShock,
						20.0f,
						2.2f,
						20.0f,
						36.0f,
						1.45f,
						0.16f,
						0.35f,
						1.35f,
						0.08f,
						0.012f,
						0.0f);
					break;
				case SyntheticRunewordStyle::kAdaptiveExposure:
					ready = configureAdaptiveSpell(
						AdaptiveElementMode::kStrongestResist,
						shredFire,
						shredFrost,
						shredShock,
						16.0f,
						1.6f,
						16.0f,
						30.0f,
						3.2f,
						0.35f,
						0.95f,
						3.2f,
						0.0f,
						0.0f,
						8.0f);
					break;
				case SyntheticRunewordStyle::kSignatureGrief:
					ready = configureAdaptiveSpell(
						AdaptiveElementMode::kWeakestResist,
						dynamicFire,
						dynamicFrost,
						dynamicShock,
						26.0f,
						2.4f,
						26.0f,
						40.0f,
						0.95f,
						0.10f,
						0.30f,
						0.95f,
						0.11f,
						0.015f,
						0.0f);
					break;
				case SyntheticRunewordStyle::kSignatureInfinity:
					ready = configureAdaptiveSpell(
						AdaptiveElementMode::kStrongestResist,
						shredFire,
						shredFrost,
						shredShock,
						22.0f,
						1.8f,
						22.0f,
						34.0f,
						2.4f,
						0.24f,
						0.85f,
						2.4f,
						0.0f,
						0.0f,
						4.5f);
					break;
				case SyntheticRunewordStyle::kSignatureEnigma:
					ready = configureSelfBuff(
						spellPhase ? spellPhase : (spellCustomInvisibility ? spellCustomInvisibility : spellWard),
						Trigger::kIncomingHit,
						20.0f,
						1.8f,
						20.0f,
						34.0f,
						10.0f,
						0.65f,
						4.0f,
						10.0f);
					break;
				case SyntheticRunewordStyle::kSignatureCallToArms:
					ready = configureSelfBuff(
						spellHaste ? spellHaste : (spellWard ? spellWard : spellPhase),
						Trigger::kHit,
						24.0f,
						1.7f,
						24.0f,
						36.0f,
						9.0f,
						0.55f,
						3.0f,
						9.0f);
					break;
				case SyntheticRunewordStyle::kFireStrike:
					ready = configureSingleTargetSpell(
						dynamicFire,
						18.0f,
						1.8f,
						18.0f,
						34.0f,
						1.2f,
						0.12f,
						0.35f,
						1.2f,
						0.06f,
						0.010f,
						0.0f);
					break;
				case SyntheticRunewordStyle::kFrostStrike:
					ready = configureSingleTargetSpell(
						dynamicFrost,
						18.0f,
						1.8f,
						18.0f,
						34.0f,
						1.25f,
						0.13f,
						0.40f,
						1.25f,
						0.06f,
						0.010f,
						0.0f);
					break;
				case SyntheticRunewordStyle::kShockStrike:
					ready = configureSingleTargetSpell(
						spellArcLightning ? spellArcLightning : dynamicShock,
						19.0f,
						1.9f,
						19.0f,
						35.0f,
						1.05f,
						0.11f,
						0.30f,
						1.05f,
						0.07f,
						0.011f,
						0.0f);
					break;
				case SyntheticRunewordStyle::kPoisonBloom:
					ready = configureSingleTargetSpell(
						spellDotPoison,
						12.0f,
						1.6f,
						12.0f,
						26.0f,
						2.6f,
						0.26f,
						0.9f,
						2.6f,
						0.0f,
						0.0f,
						4.0f);
					break;
				case SyntheticRunewordStyle::kTarBloom:
					ready = configureSingleTargetSpell(
						spellDotTar,
						12.0f,
						1.5f,
						12.0f,
						24.0f,
						2.8f,
						0.30f,
						0.9f,
						2.8f,
						0.0f,
						0.0f,
						4.0f);
					break;
				case SyntheticRunewordStyle::kSiphonBloom:
					ready = configureSingleTargetSpell(
						spellDotSiphon,
						11.0f,
						1.5f,
						11.0f,
						24.0f,
						2.9f,
						0.30f,
						1.0f,
						2.9f,
						0.0f,
						0.0f,
						5.0f);
					break;
				case SyntheticRunewordStyle::kCurseFragile:
					ready = configureSingleTargetSpell(
						spellChaosFragile ? spellChaosFragile : spellChaosSunder,
						15.0f,
						1.6f,
						15.0f,
						29.0f,
						4.4f,
						0.45f,
						1.6f,
						4.4f,
						0.0f,
						0.0f,
						8.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kCurseSlowAttack:
					ready = configureSingleTargetSpell(
						spellChaosSlowAttack ? spellChaosSlowAttack : spellChaosSunder,
						16.0f,
						1.6f,
						16.0f,
						30.0f,
						4.0f,
						0.40f,
						1.4f,
						4.0f,
						0.0f,
						0.0f,
						7.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kCurseFear:
					ready = configureSingleTargetSpell(
						spellVanillaFear ? spellVanillaFear : spellChaosSlowAttack,
						13.0f,
						1.2f,
						13.0f,
						23.0f,
						6.0f,
						0.6f,
						2.4f,
						6.0f,
						0.0f,
						0.0f,
						8.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kCurseFrenzy:
					ready = configureSingleTargetSpell(
						spellVanillaFrenzy ? spellVanillaFrenzy : spellChaosFragile,
						11.0f,
						1.1f,
						11.0f,
						21.0f,
						6.8f,
						0.7f,
						2.6f,
						6.8f,
						0.0f,
						0.0f,
						9.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kSelfHaste:
					ready = configureSelfBuff(spellHaste, Trigger::kHit, 18.0f, 1.5f, 18.0f, 32.0f, 7.5f, 0.75f, 2.4f, 7.5f);
					break;
				case SyntheticRunewordStyle::kSelfWard:
					ready = configureSelfBuff(spellWard, Trigger::kIncomingHit, 15.0f, 1.7f, 15.0f, 28.0f, 19.0f, 1.1f, 8.0f, 19.0f);
					break;
				case SyntheticRunewordStyle::kSelfBarrier:
					ready = configureSelfBuff(spellBarrier, Trigger::kIncomingHit, 15.0f, 1.7f, 15.0f, 28.0f, 18.0f, 1.0f, 7.5f, 18.0f);
					break;
				case SyntheticRunewordStyle::kSelfMeditation:
					ready = configureSelfBuff(spellMeditation, Trigger::kIncomingHit, 16.0f, 1.5f, 16.0f, 27.0f, 17.0f, 0.9f, 7.0f, 17.0f);
					break;
				case SyntheticRunewordStyle::kSelfPhase:
					ready = configureSelfBuff(spellPhase, Trigger::kIncomingHit, 14.0f, 1.4f, 14.0f, 24.0f, 16.0f, 0.8f, 7.0f, 16.0f);
					break;
				case SyntheticRunewordStyle::kSelfPhoenix:
					ready = configureSelfBuff(spellPhoenix ? spellPhoenix : spellPhase, Trigger::kIncomingHit, 14.0f, 1.4f, 14.0f, 25.0f, 16.0f, 0.8f, 6.5f, 16.0f);
					break;
				case SyntheticRunewordStyle::kSelfFlameCloak:
					ready = configureSelfBuff(
						spellVanillaFlameCloak ? spellVanillaFlameCloak : (spellPhoenix ? spellPhoenix : spellPhase),
						auraTrigger,
						12.0f,
						1.3f,
						12.0f,
						23.0f,
						14.0f,
						0.7f,
						4.5f,
						14.0f);
					break;
				case SyntheticRunewordStyle::kSelfFrostCloak:
					ready = configureSelfBuff(
						spellVanillaFrostCloak ? spellVanillaFrostCloak : (spellWard ? spellWard : spellPhase),
						auraTrigger,
						12.0f,
						1.3f,
						12.0f,
						23.0f,
						14.0f,
						0.7f,
						4.5f,
						14.0f);
					break;
				case SyntheticRunewordStyle::kSelfShockCloak:
					ready = configureSelfBuff(
						spellVanillaShockCloak ? spellVanillaShockCloak : (spellBarrier ? spellBarrier : spellPhase),
						auraTrigger,
						12.0f,
						1.3f,
						12.0f,
						23.0f,
						14.0f,
						0.7f,
						4.5f,
						14.0f);
					break;
				case SyntheticRunewordStyle::kSelfOakflesh:
					ready = configureSelfBuff(
						spellVanillaOakflesh ? spellVanillaOakflesh : (spellWard ? spellWard : spellPhase),
						auraTrigger,
						14.0f,
						1.4f,
						14.0f,
						25.0f,
						16.0f,
						0.9f,
						6.0f,
						16.0f);
					break;
				case SyntheticRunewordStyle::kSelfStoneflesh:
					ready = configureSelfBuff(
						spellVanillaStoneflesh ? spellVanillaStoneflesh : (spellBarrier ? spellBarrier : spellPhase),
						auraTrigger,
						14.0f,
						1.4f,
						14.0f,
						25.0f,
						16.0f,
						0.9f,
						6.0f,
						16.0f);
					break;
				case SyntheticRunewordStyle::kSelfIronflesh:
					ready = configureSelfBuff(
						spellVanillaIronflesh ? spellVanillaIronflesh : (spellBarrier ? spellBarrier : spellPhase),
						auraTrigger,
						13.0f,
						1.4f,
						13.0f,
						24.0f,
						17.0f,
						0.9f,
						6.0f,
						17.0f);
					break;
				case SyntheticRunewordStyle::kSelfEbonyflesh:
					ready = configureSelfBuff(
						spellVanillaEbonyflesh ? spellVanillaEbonyflesh : (spellWard ? spellWard : spellPhase),
						auraTrigger,
						12.0f,
						1.2f,
						12.0f,
						22.0f,
						18.0f,
						1.0f,
						7.0f,
						18.0f);
					break;
				case SyntheticRunewordStyle::kSelfMuffle:
					ready = configureSelfBuff(
						spellVanillaMuffle ? spellVanillaMuffle : spellPhase,
						Trigger::kIncomingHit,
						13.0f,
						1.4f,
						13.0f,
						24.0f,
						20.0f,
						1.1f,
						8.0f,
						20.0f);
					break;
				case SyntheticRunewordStyle::kSelfInvisibility:
					ready = configureSelfBuff(
						spellCustomInvisibility ? spellCustomInvisibility : spellPhase,
						Trigger::kIncomingHit,
						8.0f,
						0.9f,
						8.0f,
						15.0f,
						28.0f,
						1.6f,
						14.0f,
						28.0f);
					break;
				case SyntheticRunewordStyle::kSoulTrap:
					ready = configureSingleTargetSpell(
						spellVanillaSoulTrap ? spellVanillaSoulTrap : spellChaosFragile,
						16.0f,
						1.4f,
						16.0f,
						27.0f,
						7.0f,
						0.7f,
						2.5f,
						7.0f,
						0.0f,
						0.0f,
						6.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kSummonFamiliar:
					ready = configureSummonSpell(
						spellSummonFamiliar,
						8.0f,
						0.8f,
						8.0f,
						14.0f,
						28.0f,
						1.6f,
						18.0f,
						30.0f);
					break;
				case SyntheticRunewordStyle::kSummonFlameAtronach:
					ready = configureSummonSpell(
						spellSummonFlameAtronach,
						7.0f,
						0.7f,
						7.0f,
						13.0f,
						30.0f,
						1.5f,
						20.0f,
						32.0f);
					break;
				case SyntheticRunewordStyle::kSummonFrostAtronach:
					ready = configureSummonSpell(
						spellSummonFrostAtronach,
						6.5f,
						0.7f,
						6.5f,
						12.0f,
						32.0f,
						1.5f,
						21.0f,
						34.0f);
					break;
				case SyntheticRunewordStyle::kSummonStormAtronach:
					ready = configureSummonSpell(
						spellSummonStormAtronach,
						6.0f,
						0.6f,
						6.0f,
						11.0f,
						34.0f,
						1.6f,
						22.0f,
						36.0f);
					break;
				case SyntheticRunewordStyle::kSummonDremoraLord:
					ready = configureSummonSpell(
						spellSummonDremoraLord,
						4.0f,
						0.5f,
						4.0f,
						8.0f,
						38.0f,
						2.0f,
						26.0f,
						40.0f);
					break;
				case SyntheticRunewordStyle::kLegacyFallback:
				default:
					ready = false;
					break;
				}

				if (!ready) {
					if (applyLegacyRunewordFallback(out, recipe, runeCount)) {
						ready = true;
						style = SyntheticRunewordStyle::kLegacyFallback;
					}
				}

					constexpr float kUnsetValue = -1.0f;
					constexpr std::int32_t kUnsetTrigger = -1;
					struct RecipeTuning
					{
						std::string_view id;
						float procPct;
						float icdSec;
						float perTargetIcdSec;
						float magnitudeMult;
						std::int32_t triggerOverride{ kUnsetTrigger };
						float lowHealthThresholdPct{ kUnsetValue };
						float lowHealthRearmPct{ kUnsetValue };
					};
					static constexpr RecipeTuning kRecipeTunings[] = {
						{ "rw_grief", 34.0f, 0.55f, kUnsetValue, 0.20f },
						{ "rw_infinity", 26.0f, 1.8f, 4.5f, kUnsetValue },
						{ "rw_enigma", 24.0f, 8.0f, kUnsetValue, kUnsetValue },
						{ "rw_call_to_arms", 28.0f, 7.0f, kUnsetValue, kUnsetValue },
						{ "rw_nadir", 18.0f, 10.0f, 12.0f, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 38.0f, 50.0f },
						{ "rw_steel", 30.0f, 0.85f, kUnsetValue, 0.16f },
						{ "rw_malice", 18.0f, 1.9f, 3.5f, kUnsetValue },
						{ "rw_stealth", 22.0f, 14.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 40.0f, 55.0f },
						{ "rw_leaf", 24.0f, 1.0f, kUnsetValue, 0.16f },
						{ "rw_ancients_pledge", 20.0f, 11.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 35.0f, 48.0f },
						{ "rw_holy_thunder", 22.0f, 7.8f, kUnsetValue, kUnsetValue },
						{ "rw_zephyr", 24.0f, 6.9f, kUnsetValue, kUnsetValue },
						{ "rw_pattern", 29.0f, 1.0f, kUnsetValue, 0.17f },
						{ "rw_kings_grace", 28.0f, 1.05f, kUnsetValue, 0.16f },
						{ "rw_strength", 27.0f, 0.95f, kUnsetValue, 0.15f },
						{ "rw_edge", 24.0f, 1.2f, kUnsetValue, 0.14f },
						{ "rw_lawbringer", 13.0f, 3.8f, 10.0f, kUnsetValue },
						{ "rw_wrath", 16.0f, 3.2f, 8.0f, kUnsetValue },
						{ "rw_kingslayer", 18.0f, 2.6f, 7.0f, kUnsetValue },
						{ "rw_principle", 14.0f, 4.2f, 9.0f, kUnsetValue },
						{ "rw_black", 14.0f, 2.4f, 3.5f, kUnsetValue },
						{ "rw_rift", 15.0f, 2.0f, 3.0f, kUnsetValue },
						{ "rw_plague", 14.0f, 2.2f, 4.0f, kUnsetValue },
						{ "rw_wealth", 24.0f, 4.8f, 5.0f, kUnsetValue },
						{ "rw_obedience", 22.0f, 5.2f, 5.0f, kUnsetValue },
						{ "rw_honor", 20.0f, 5.6f, 6.0f, kUnsetValue },
						{ "rw_eternity", 24.0f, 4.4f, 4.5f, kUnsetValue },
						{ "rw_smoke", 15.0f, 15.0f, kUnsetValue, kUnsetValue },
						{ "rw_treachery", 18.0f, 12.0f, kUnsetValue, kUnsetValue },
						{ "rw_gloom", 7.0f, 24.0f, kUnsetValue, kUnsetValue },
						{ "rw_delirium", 11.0f, 8.5f, 11.0f, kUnsetValue },
						{ "rw_beast", 26.0f, 3.0f, kUnsetValue, kUnsetValue },
						{ "rw_dragon", 16.0f, 10.0f, kUnsetValue, kUnsetValue },
						{ "rw_hand_of_justice", 18.0f, 8.5f, kUnsetValue, kUnsetValue },
						{ "rw_flickering_flame", 14.0f, 11.0f, kUnsetValue, kUnsetValue },
						{ "rw_temper", 17.0f, 8.8f, kUnsetValue, kUnsetValue },
						{ "rw_voice_of_reason", 16.0f, 9.5f, kUnsetValue, kUnsetValue },
						{ "rw_ice", 14.0f, 11.0f, kUnsetValue, kUnsetValue },
						{ "rw_pride", 14.0f, 12.5f, kUnsetValue, kUnsetValue },
						{ "rw_metamorphosis", 13.0f, 16.0f, kUnsetValue, kUnsetValue },
						{ "rw_destruction", 22.0f, 1.1f, kUnsetValue, 0.17f },
						{ "rw_hustle_w", 24.0f, 2.8f, kUnsetValue, kUnsetValue },
						{ "rw_harmony", 21.0f, 3.8f, kUnsetValue, kUnsetValue },
						{ "rw_unbending_will", 25.0f, 2.6f, kUnsetValue, kUnsetValue },
						{ "rw_stone", 18.0f, 13.0f, kUnsetValue, kUnsetValue },
						{ "rw_sanctuary", 17.0f, 13.0f, kUnsetValue, kUnsetValue },
						{ "rw_memory", 18.0f, 12.0f, kUnsetValue, kUnsetValue },
						{ "rw_wisdom", 17.0f, 13.0f, kUnsetValue, kUnsetValue },
						{ "rw_mist", 15.0f, 11.5f, kUnsetValue, kUnsetValue },
						{ "rw_myth", 19.0f, 11.5f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 37.0f, 50.0f },
						{ "rw_spirit", 22.0f, 9.5f, kUnsetValue, kUnsetValue },
						{ "rw_lore", 19.0f, 10.7f, kUnsetValue, kUnsetValue },
						{ "rw_radiance", 20.0f, 10.2f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 35.0f, 49.0f },
						{ "rw_insight", 23.0f, 8.5f, kUnsetValue, kUnsetValue },
						{ "rw_rhyme", 20.0f, 9.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 38.0f, 52.0f },
						{ "rw_peace", 16.0f, 10.5f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 39.0f, 53.0f },
						{ "rw_bulwark", 18.0f, 10.5f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 39.0f, 53.0f },
						{ "rw_cure", 18.0f, 10.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 40.0f, 54.0f },
						{ "rw_ground", 17.0f, 9.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 36.0f, 50.0f },
						{ "rw_hearth", 17.0f, 9.2f, kUnsetValue, kUnsetValue },
						{ "rw_white", 15.0f, 2.6f, 5.0f, kUnsetValue },
						{ "rw_melody", 20.0f, 9.8f, kUnsetValue, kUnsetValue },
						{ "rw_hustle_a", 21.0f, 8.8f, kUnsetValue, kUnsetValue },
						{ "rw_lionheart", 19.0f, 10.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 36.0f, 49.0f },
						{ "rw_passion", 26.0f, 2.6f, kUnsetValue, kUnsetValue },
						{ "rw_enlightenment", 18.0f, 11.0f, kUnsetValue, kUnsetValue },
						{ "rw_crescent_moon", 25.0f, 1.0f, kUnsetValue, 0.17f },
						{ "rw_duress", 18.0f, 11.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 37.0f, 50.0f },
						{ "rw_bone", 10.0f, 22.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 35.0f, 48.0f },
						{ "rw_venom", 16.0f, 1.9f, 3.8f, kUnsetValue },
						{ "rw_prudence", 17.0f, 12.0f, kUnsetValue, kUnsetValue },
						{ "rw_oath", 31.0f, 0.88f, kUnsetValue, 0.18f },
						{ "rw_rain", 16.0f, 9.5f, kUnsetValue, kUnsetValue },
						{ "rw_death", 19.0f, 2.2f, 6.0f, kUnsetValue },
						{ "rw_heart_of_the_oak", 24.0f, 2.2f, 4.2f, kUnsetValue },
						{ "rw_fortitude", 21.0f, 9.2f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 34.0f, 48.0f },
						{ "rw_bramble", 16.0f, 2.0f, 4.0f, kUnsetValue },
						{ "rw_chains_of_honor", 19.0f, 10.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 36.0f, 49.0f },
						{ "rw_fury", 27.0f, 2.2f, kUnsetValue, kUnsetValue },
						{ "rw_famine", 19.0f, 2.4f, 6.0f, kUnsetValue },
						{ "rw_wind", 20.0f, 6.6f, kUnsetValue, kUnsetValue },
						{ "rw_brand", 22.0f, 2.1f, 5.0f, kUnsetValue },
						{ "rw_dream", 23.0f, 1.0f, kUnsetValue, 0.17f },
						{ "rw_faith", 28.0f, 2.4f, kUnsetValue, kUnsetValue },
						{ "rw_last_wish", 17.0f, 2.9f, 6.0f, kUnsetValue },
						{ "rw_phoenix", 18.0f, 8.2f, kUnsetValue, kUnsetValue },
						{ "rw_doom", 21.0f, 1.2f, kUnsetValue, 0.16f },
						{ "rw_exile", 20.0f, 8.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 33.0f, 46.0f },
						{ "rw_chaos", 14.0f, 6.4f, 8.0f, kUnsetValue },
						{ "rw_mosaic", 14.0f, 2.4f, 4.8f, kUnsetValue },
						{ "rw_obsession", 13.0f, 2.8f, 5.5f, kUnsetValue },
						{ "rw_silence", 20.0f, 2.9f, 7.0f, kUnsetValue },
						{ "rw_splendor", 20.0f, 9.0f, kUnsetValue, kUnsetValue },
						{ "rw_breath_of_the_dying", 31.0f, 0.8f, kUnsetValue, 0.19f }
					};
				struct StyleTuning
				{
					SyntheticRunewordStyle style;
					float procPct;
					float icdSec;
				};
				static constexpr StyleTuning kSummonStyleTunings[] = {
					{ SyntheticRunewordStyle::kSummonDremoraLord, 5.0f, 34.0f },
					{ SyntheticRunewordStyle::kSummonStormAtronach, 7.0f, 26.0f },
					{ SyntheticRunewordStyle::kSummonFlameAtronach, 8.0f, 24.0f },
					{ SyntheticRunewordStyle::kSummonFrostAtronach, 8.0f, 24.0f },
					{ SyntheticRunewordStyle::kSummonFamiliar, 10.0f, 22.0f }
				};

					auto applyRunewordIndividualTuning = [&](std::string_view a_recipeId, SyntheticRunewordStyle a_style) {
						auto setProcIcd = [&](float a_procPct, float a_icdSec) {
							out.procChancePct = a_procPct;
							out.icd = toDurationMs(a_icdSec);
						};
					auto setPerTargetIcd = [&](float a_seconds) {
						out.perTargetIcd = toDurationMs(a_seconds);
					};
						auto setMagnitudeMult = [&](float a_mult) {
							if (out.action.magnitudeScaling.source != MagnitudeScaling::Source::kNone) {
								out.action.magnitudeScaling.mult = a_mult;
							}
						};
						auto setTrigger = [&](Trigger a_trigger) {
							out.trigger = a_trigger;
						};
						auto setLowHealthWindow = [&](float a_thresholdPct, float a_rearmPct) {
							out.lowHealthThresholdPct = std::clamp(a_thresholdPct, 1.0f, 95.0f);
							out.lowHealthRearmPct = std::clamp(a_rearmPct, out.lowHealthThresholdPct + 1.0f, 100.0f);
						};

						for (const auto& tuning : kRecipeTunings) {
							if (tuning.id != a_recipeId) {
								continue;
							}
						setProcIcd(tuning.procPct, tuning.icdSec);
						if (tuning.perTargetIcdSec > 0.0f) {
							setPerTargetIcd(tuning.perTargetIcdSec);
						}
							if (tuning.magnitudeMult > 0.0f) {
								setMagnitudeMult(tuning.magnitudeMult);
							}
							if (tuning.triggerOverride != kUnsetTrigger) {
								setTrigger(static_cast<Trigger>(tuning.triggerOverride));
							}
							if (tuning.lowHealthThresholdPct > 0.0f) {
								const float rearmPct =
									tuning.lowHealthRearmPct > 0.0f ?
									tuning.lowHealthRearmPct :
									(tuning.lowHealthThresholdPct + 10.0f);
								setLowHealthWindow(tuning.lowHealthThresholdPct, rearmPct);
							}
							break;
						}

					for (const auto& tuning : kSummonStyleTunings) {
						if (tuning.style != a_style) {
							continue;
						}
						setProcIcd(tuning.procPct, tuning.icdSec);
						break;
					}
				};

				if (ready) {
					if (const auto identityOverride = detail::ResolveRunewordIdentityOverride(recipe.id)) {
						if (identityOverride->actionKind == detail::RunewordIdentityActionKind::kCastSpellAdaptiveElement) {
							out.action.type = ActionType::kCastSpellAdaptiveElement;
							out.action.adaptiveMode = identityOverride->adaptiveMode;
							out.action.adaptiveFire = resolveIdentitySpell(identityOverride->adaptiveFire);
							out.action.adaptiveFrost = resolveIdentitySpell(identityOverride->adaptiveFrost);
							out.action.adaptiveShock = resolveIdentitySpell(identityOverride->adaptiveShock);
							out.action.applyToSelf = false;
							out.action.noHitEffectArt = identityOverride->noHitEffectArt;
						} else {
							if (auto* overrideSpell = resolveIdentitySpell(identityOverride->spell)) {
								out.action.type = ActionType::kCastSpell;
								out.action.spell = overrideSpell;
								out.action.applyToSelf = identityOverride->applyToSelf;
								out.action.noHitEffectArt = identityOverride->noHitEffectArt;
							}
						}
					}
					applyRunewordIndividualTuning(recipe.id, style);
				}

			if (!ready) {
				SKSE::log::warn(
					"CalamityAffixes: runeword result affix unresolved (recipe={}, resultToken={:016X}).",
					recipe.id,
					recipe.resultAffixToken);
				continue;
			}

				RegisterSynthesizedAffix(std::move(out), false);
				synthesizedRunewordAffixes += 1u;
				SKSE::log::warn(
					"CalamityAffixes: synthesized runeword runtime affix (recipe={}, style={}, resultToken={:016X}).",
					recipe.id,
					styleName(style),
					recipe.resultAffixToken);
		}

		if (synthesizedRunewordAffixes > 0u) {
			SKSE::log::info(
				"CalamityAffixes: synthesized {} runeword runtime affix definitions.",
				synthesizedRunewordAffixes);
		}
	}

}
