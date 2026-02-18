#include "CalamityAffixes/EventBridge.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	namespace
	{
		struct RecipeKeyOverride
		{
			std::string_view id;
			std::string_view key;
		};

		template <std::size_t N>
		[[nodiscard]] constexpr std::string_view FindKeyOverride(
			std::string_view a_id,
			const RecipeKeyOverride (&a_overrides)[N]) noexcept
		{
			for (const auto& entry : a_overrides) {
				if (entry.id == a_id) {
					return entry.key;
				}
			}
			return {};
		}

		[[nodiscard]] constexpr std::uint32_t RecipeBucket(std::string_view a_id, std::uint32_t a_modulo) noexcept
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

		static constexpr RecipeKeyOverride kRecommendedBaseOverrides[] = {
			{ "rw_insight", "polearm" },
			{ "rw_infinity", "polearm" },
			{ "rw_pride", "polearm" },
			{ "rw_obedience", "polearm" },
			{ "rw_faith", "bow" },
			{ "rw_ice", "bow" },
			{ "rw_mist", "bow" },
			{ "rw_harmony", "bow" },
			{ "rw_edge", "bow" },
			{ "rw_melody", "bow" },
			{ "rw_wrath", "bow" },
			{ "rw_brand", "bow" },
			{ "rw_leaf", "staff_wand" },
			{ "rw_memory", "staff_wand" },
			{ "rw_white", "staff_wand" },
			{ "rw_obsession", "staff_wand" },
			{ "rw_chaos", "claw" },
			{ "rw_mosaic", "claw" },
			{ "rw_grief", "sword" },
			{ "rw_lawbringer", "sword" },
			{ "rw_oath", "sword" },
			{ "rw_unbending_will", "sword" },
			{ "rw_kingslayer", "sword" },
			{ "rw_exile", "shield" },
			{ "rw_sanctuary", "shield" },
			{ "rw_ancients_pledge", "shield" },
			{ "rw_rhyme", "shield" },
			{ "rw_splendor", "shield" },
			{ "rw_lore", "helm" },
			{ "rw_nadir", "helm" },
			{ "rw_delirium", "helm" },
			{ "rw_bulwark", "helm" },
			{ "rw_cure", "helm" },
			{ "rw_ground", "helm" },
			{ "rw_hearth", "helm" },
			{ "rw_temper", "helm" },
			{ "rw_flickering_flame", "helm" },
			{ "rw_wisdom", "helm" },
			{ "rw_metamorphosis", "helm" },
			{ "rw_enigma", "armor" },
			{ "rw_chains_of_honor", "armor" },
			{ "rw_stealth", "armor" },
			{ "rw_smoke", "armor" },
			{ "rw_treachery", "armor" },
			{ "rw_duress", "armor" },
			{ "rw_gloom", "armor" },
			{ "rw_stone", "armor" },
			{ "rw_bramble", "armor" },
			{ "rw_myth", "armor" },
			{ "rw_peace", "armor" },
			{ "rw_prudence", "armor" },
			{ "rw_rain", "armor" },
			{ "rw_spirit", "sword_shield" },
			{ "rw_phoenix", "weapon_shield" },
			{ "rw_dream", "helm_shield" },
			{ "rw_dragon", "armor_shield" },
		};

		static constexpr RecipeKeyOverride kEffectSummaryOverrides[] = {
			{ "rw_spirit", "self_meditation" },
			{ "rw_insight", "self_meditation" },
			{ "rw_call_to_arms", "self_ward" },
			{ "rw_fortitude", "self_ward" },
			{ "rw_heart_of_the_oak", "adaptive_exposure" },
			{ "rw_infinity", "adaptive_exposure" },
			{ "rw_last_wish", "adaptive_exposure" },
			{ "rw_exile", "self_barrier" },
			{ "rw_grief", "adaptive_strike" },
			{ "rw_breath_of_the_dying", "adaptive_strike" },
			{ "rw_chains_of_honor", "self_phase" },
			{ "rw_enigma", "self_phase" },
			{ "rw_dream", "shock_strike" },
			{ "rw_faith", "self_haste" },
			{ "rw_phoenix", "self_phoenix" },
			{ "rw_doom", "frost_strike" },
			{ "rw_dragon", "self_flame_cloak" },
			{ "rw_hand_of_justice", "self_flame_cloak" },
			{ "rw_flickering_flame", "self_flame_cloak" },
			{ "rw_temper", "self_flame_cloak" },
			{ "rw_ice", "self_frost_cloak" },
			{ "rw_voice_of_reason", "self_frost_cloak" },
			{ "rw_hearth", "self_frost_cloak" },
			{ "rw_holy_thunder", "self_shock_cloak" },
			{ "rw_zephyr", "self_shock_cloak" },
			{ "rw_wind", "self_shock_cloak" },
			{ "rw_bulwark", "self_oakflesh" },
			{ "rw_cure", "self_oakflesh" },
			{ "rw_ancients_pledge", "self_oakflesh" },
			{ "rw_lionheart", "self_oakflesh" },
			{ "rw_radiance", "self_oakflesh" },
			{ "rw_sanctuary", "self_stoneflesh" },
			{ "rw_duress", "self_stoneflesh" },
			{ "rw_peace", "self_stoneflesh" },
			{ "rw_myth", "self_stoneflesh" },
			{ "rw_pride", "self_ironflesh" },
			{ "rw_stone", "self_ironflesh" },
			{ "rw_prudence", "self_ironflesh" },
			{ "rw_mist", "self_ironflesh" },
			{ "rw_metamorphosis", "self_ebonyflesh" },
			{ "rw_nadir", "curse_fear" },
			{ "rw_delirium", "curse_frenzy" },
			{ "rw_chaos", "curse_frenzy" },
			{ "rw_malice", "poison_bloom" },
			{ "rw_venom", "poison_bloom" },
			{ "rw_plague", "poison_bloom" },
			{ "rw_bramble", "poison_bloom" },
			{ "rw_black", "tar_bloom" },
			{ "rw_rift", "tar_bloom" },
			{ "rw_mosaic", "siphon_bloom" },
			{ "rw_obsession", "siphon_bloom" },
			{ "rw_white", "siphon_bloom" },
			{ "rw_lawbringer", "curse_fragile" },
			{ "rw_wrath", "curse_fragile" },
			{ "rw_kingslayer", "curse_fragile" },
			{ "rw_principle", "curse_fragile" },
			{ "rw_death", "curse_slow_attack" },
			{ "rw_famine", "curse_slow_attack" },
			{ "rw_leaf", "fire_strike" },
			{ "rw_destruction", "fire_strike" },
			{ "rw_crescent_moon", "shock_strike" },
			{ "rw_beast", "self_haste" },
			{ "rw_hustle_w", "self_haste" },
			{ "rw_harmony", "self_haste" },
			{ "rw_fury", "self_haste" },
			{ "rw_unbending_will", "self_haste" },
			{ "rw_passion", "self_haste" },
			{ "rw_stealth", "self_muffle" },
			{ "rw_smoke", "self_muffle" },
			{ "rw_treachery", "self_muffle" },
			{ "rw_gloom", "self_invisibility" },
			{ "rw_wealth", "soul_trap" },
			{ "rw_obedience", "soul_trap" },
			{ "rw_honor", "soul_trap" },
			{ "rw_eternity", "soul_trap" },
			{ "rw_memory", "self_meditation" },
			{ "rw_wisdom", "self_meditation" },
			{ "rw_lore", "self_meditation" },
			{ "rw_melody", "self_meditation" },
			{ "rw_enlightenment", "self_meditation" },
			{ "rw_pattern", "adaptive_strike" },
			{ "rw_strength", "adaptive_strike" },
			{ "rw_kings_grace", "adaptive_strike" },
			{ "rw_edge", "adaptive_strike" },
			{ "rw_oath", "adaptive_strike" },
			{ "rw_silence", "adaptive_exposure" },
			{ "rw_brand", "adaptive_exposure" },
			{ "rw_hustle_a", "self_phase" },
			{ "rw_splendor", "self_phase" },
			{ "rw_rhyme", "self_phase" },
			{ "rw_rain", "self_phoenix" },
			{ "rw_ground", "self_phoenix" },
		};
	}

	std::vector<EventBridge::RunewordRecipeEntry> EventBridge::GetRunewordRecipeEntries()
	{
		std::vector<RunewordRecipeEntry> entries;
		if (!_configLoaded) {
			return entries;
		}

		SanitizeRunewordState();
		entries.reserve(_runewordRecipes.size());

		auto resolveRecommendedBaseKey = [&](const RunewordRecipe& a_recipe) -> std::string_view {
			const std::string_view id = a_recipe.id;
			if (const auto overrideKey = FindKeyOverride(id, kRecommendedBaseOverrides); !overrideKey.empty()) {
				return overrideKey;
			}

			if (!a_recipe.recommendedBaseType) {
				return "mixed";
			}
			return *a_recipe.recommendedBaseType == LootItemType::kArmor ? "armor" : "weapon";
		};

		auto resolveRecipeEffectSummaryKey = [&](const RunewordRecipe& a_recipe) -> std::string_view {
			const std::string_view id = a_recipe.id;
			if (const auto overrideKey = FindKeyOverride(id, kEffectSummaryOverrides); !overrideKey.empty()) {
				return overrideKey;
			}

			if (a_recipe.recommendedBaseType) {
				if (*a_recipe.recommendedBaseType == LootItemType::kWeapon) {
					switch (RecipeBucket(id, 6u)) {
					case 0u:
						return "adaptive_strike";
					case 1u:
						return "adaptive_exposure";
					case 2u:
						return "fire_strike";
					case 3u:
						return "frost_strike";
					case 4u:
						return "shock_strike";
					default:
						return "self_haste";
					}
				}
				if (*a_recipe.recommendedBaseType == LootItemType::kArmor) {
					switch (RecipeBucket(id, 6u)) {
					case 0u:
						return "self_ward";
					case 1u:
						return "self_barrier";
					case 2u:
						return "self_meditation";
					case 3u:
						return "self_phase";
					case 4u:
						return "self_muffle";
					default:
						return "self_phoenix";
					}
				}
			}

			return RecipeBucket(id, 2u) == 0u ? "adaptive_exposure" : "adaptive_strike";
		};

		std::uint64_t selectedToken = 0u;
		if (const auto* currentRecipe = GetCurrentRunewordRecipe()) {
			selectedToken = currentRecipe->token;
		}
		if (_runewordSelectedBaseKey) {
			const auto selectedKey = *_runewordSelectedBaseKey;
			bool resolvedFromCompleted = false;
			if (const auto* completed = ResolveCompletedRunewordRecipe(selectedKey)) {
				selectedToken = completed->token;
				resolvedFromCompleted = true;
			}
			if (!resolvedFromCompleted) {
				if (const auto stateIt = _runewordInstanceStates.find(selectedKey);
					stateIt != _runewordInstanceStates.end() &&
					stateIt->second.recipeToken != 0u) {
					selectedToken = stateIt->second.recipeToken;
				}
			}
		}

		for (const auto& recipe : _runewordRecipes) {
			std::string runes;
			for (std::size_t i = 0; i < recipe.runeIds.size(); ++i) {
				if (i > 0) {
					runes.push_back('-');
				}
				runes.append(recipe.runeIds[i]);
			}

			entries.push_back(RunewordRecipeEntry{
				.recipeToken = recipe.token,
				.displayName = recipe.displayName,
				.runeSequence = std::move(runes),
				.effectSummaryKey = std::string(resolveRecipeEffectSummaryKey(recipe)),
				.recommendedBaseKey = std::string(resolveRecommendedBaseKey(recipe)),
				.selected = (selectedToken != 0u && selectedToken == recipe.token)
			});
		}

		return entries;
	}

}
