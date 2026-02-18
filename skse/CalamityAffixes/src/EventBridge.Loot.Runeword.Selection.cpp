#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "CalamityAffixes/RunewordUtil.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

		std::vector<std::uint64_t> EventBridge::CollectEquippedRunewordBaseCandidates(bool a_ensureUniqueId)
		{
			std::vector<std::uint64_t> keys;

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return keys;
			}

			auto* changes = player->GetInventoryChanges();
			if (!changes || !changes->entryList) {
				return keys;
			}

			for (auto* entry : *changes->entryList) {
				if (!entry || !entry->object || !entry->extraLists) {
					continue;
				}

				if (!IsWeaponOrArmor(entry->object)) {
					continue;
				}

				for (auto* xList : *entry->extraLists) {
					if (!xList) {
						continue;
					}

					const bool worn = xList->HasType<RE::ExtraWorn>() || xList->HasType<RE::ExtraWornLeft>();
					if (!worn) {
						continue;
					}

					auto* uid = xList->GetByType<RE::ExtraUniqueID>();
					if (!uid && a_ensureUniqueId) {
						auto* created = RE::BSExtraData::Create<RE::ExtraUniqueID>();
						created->baseID = entry->object->GetFormID();
						created->uniqueID = changes->GetNextUniqueID();
						xList->Add(created);
						uid = created;
					}

					if (!uid) {
						continue;
					}

					const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
					keys.push_back(key);
				}
			}

			std::sort(keys.begin(), keys.end());
			keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
			return keys;
		}

		const EventBridge::RunewordRecipe* EventBridge::ResolveCompletedRunewordRecipe(const InstanceAffixSlots& a_slots) const
		{
			const auto cappedCount = std::min<std::uint8_t>(a_slots.count, static_cast<std::uint8_t>(kMaxAffixesPerItem));
			for (std::uint8_t i = 0; i < cappedCount; ++i) {
				const auto token = a_slots.tokens[i];
				if (token == 0u) {
					continue;
				}

				const auto rwIt = _runewordRecipeIndexByResultAffixToken.find(token);
				if (rwIt == _runewordRecipeIndexByResultAffixToken.end() || rwIt->second >= _runewordRecipes.size()) {
					continue;
				}

				return std::addressof(_runewordRecipes[rwIt->second]);
			}

			return nullptr;
		}

		const EventBridge::RunewordRecipe* EventBridge::ResolveCompletedRunewordRecipe(std::uint64_t a_instanceKey) const
		{
			const auto it = _instanceAffixes.find(a_instanceKey);
			if (it == _instanceAffixes.end()) {
				return nullptr;
			}

			return ResolveCompletedRunewordRecipe(it->second);
		}

			bool EventBridge::SelectRunewordBase(std::uint64_t a_instanceKey)
			{
				if (!_configLoaded || a_instanceKey == 0u) {
					return false;
				}

			SanitizeRunewordState();

			RE::InventoryEntryData* entry = nullptr;
			RE::ExtraDataList* xList = nullptr;
			if (!ResolvePlayerInventoryInstance(a_instanceKey, entry, xList) || !entry || !entry->object) {
				return false;
			}

			if (!IsWeaponOrArmor(entry->object)) {
				return false;
			}
				const RunewordRecipe* completedRecipe = ResolveCompletedRunewordRecipe(a_instanceKey);

			_runewordSelectedBaseKey = a_instanceKey;

			const auto candidates = CollectEquippedRunewordBaseCandidates(true);
			if (const auto cursor = ResolveRunewordBaseCycleCursor(candidates, a_instanceKey); cursor) {
				_runewordBaseCycleCursor = *cursor;
			}

			std::optional<RunewordInstanceState> stateCopy;
			if (completedRecipe) {
				_runewordInstanceStates.erase(a_instanceKey);
			} else {
				auto& state = _runewordInstanceStates[a_instanceKey];
				if (state.recipeToken == 0u) {
					if (const auto* recipe = GetCurrentRunewordRecipe()) {
						state.recipeToken = recipe->token;
					}
				}
				stateCopy = state;
			}

			const std::string selectedName = ResolveInventoryDisplayName(entry, xList);

			const auto* recipe = completedRecipe;
			if (!recipe && stateCopy) {
				recipe = FindRunewordRecipeByToken(stateCopy->recipeToken);
			}
			if (!recipe) {
				recipe = GetCurrentRunewordRecipe();
			}

			std::string note = "Runeword Base: " + selectedName;
			if (recipe) {
				note.append(" | ");
				note.append(recipe->displayName);

				const auto baseType = ResolveInstanceLootType(a_instanceKey);
				if (recipe->recommendedBaseType && baseType && *recipe->recommendedBaseType != *baseType) {
					note.append(" (Recommended ");
					note.append(DescribeLootItemType(*recipe->recommendedBaseType));
					note.append(", current ");
					note.append(DescribeLootItemType(*baseType));
					note.append(")");
				}
			}

			RE::DebugNotification(note.c_str());
			return true;
		}

		std::vector<EventBridge::RunewordBaseInventoryEntry> EventBridge::GetRunewordBaseInventoryEntries()
		{
			std::vector<RunewordBaseInventoryEntry> entries;
			if (!_configLoaded) {
				return entries;
			}

			SanitizeRunewordState();
			const auto candidates = CollectEquippedRunewordBaseCandidates(true);
			entries.reserve(candidates.size());

			for (const auto key : candidates) {
				RE::InventoryEntryData* entry = nullptr;
				RE::ExtraDataList* xList = nullptr;
				if (!ResolvePlayerInventoryInstance(key, entry, xList) || !entry || !entry->object) {
					continue;
				}

				std::string displayName = ResolveInventoryDisplayName(entry, xList);
				if (const auto* completed = ResolveCompletedRunewordRecipe(key)) {
					displayName.append(" [Runeword: ");
					displayName.append(completed->displayName);
					displayName.push_back(']');
				}

				entries.push_back(RunewordBaseInventoryEntry{
					.instanceKey = key,
					.displayName = std::move(displayName),
					.selected = (_runewordSelectedBaseKey && *_runewordSelectedBaseKey == key)
				});
			}

			return entries;
		}

		std::vector<EventBridge::RunewordRecipeEntry> EventBridge::GetRunewordRecipeEntries()
		{
			std::vector<RunewordRecipeEntry> entries;
			if (!_configLoaded) {
				return entries;
			}

			SanitizeRunewordState();
			entries.reserve(_runewordRecipes.size());

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

			auto resolveRecommendedBaseKey = [&](const RunewordRecipe& a_recipe) -> std::string_view {
				const std::string_view id = a_recipe.id;

				if (idIsOneOf(id, { "rw_insight", "rw_infinity", "rw_pride", "rw_obedience" })) {
					return "polearm";
				}
				if (idIsOneOf(id, { "rw_faith", "rw_ice", "rw_mist", "rw_harmony", "rw_edge", "rw_melody", "rw_wrath", "rw_brand" })) {
					return "bow";
				}
				if (idIsOneOf(id, { "rw_leaf", "rw_memory", "rw_white", "rw_obsession" })) {
					return "staff_wand";
				}
				if (idIsOneOf(id, { "rw_chaos", "rw_mosaic" })) {
					return "claw";
				}
				if (idIsOneOf(id, { "rw_grief", "rw_lawbringer", "rw_oath", "rw_unbending_will", "rw_kingslayer" })) {
					return "sword";
				}
				if (idIsOneOf(id, { "rw_exile", "rw_sanctuary", "rw_ancients_pledge", "rw_rhyme", "rw_splendor" })) {
					return "shield";
				}
				if (idIsOneOf(id, { "rw_lore", "rw_nadir", "rw_delirium", "rw_bulwark", "rw_cure", "rw_ground", "rw_hearth", "rw_temper", "rw_flickering_flame", "rw_wisdom", "rw_metamorphosis" })) {
					return "helm";
				}
				if (idIsOneOf(id, { "rw_enigma", "rw_chains_of_honor", "rw_stealth", "rw_smoke", "rw_treachery", "rw_duress", "rw_gloom", "rw_stone", "rw_bramble", "rw_myth", "rw_peace", "rw_prudence", "rw_rain" })) {
					return "armor";
				}
				if (idIsOneOf(id, { "rw_spirit" })) {
					return "sword_shield";
				}
				if (idIsOneOf(id, { "rw_phoenix" })) {
					return "weapon_shield";
				}
				if (idIsOneOf(id, { "rw_dream" })) {
					return "helm_shield";
				}
				if (idIsOneOf(id, { "rw_dragon" })) {
					return "armor_shield";
				}

				if (!a_recipe.recommendedBaseType) {
					return "mixed";
				}
				return *a_recipe.recommendedBaseType == LootItemType::kArmor ? "armor" : "weapon";
			};

			auto resolveRecipeEffectSummaryKey = [&](const RunewordRecipe& a_recipe) -> std::string_view {
				const std::string_view id = a_recipe.id;

				if (idIsOneOf(id, { "rw_spirit", "rw_insight" })) {
					return "self_meditation";
				}
				if (idIsOneOf(id, { "rw_call_to_arms", "rw_fortitude" })) {
					return "self_ward";
				}
				if (idIsOneOf(id, { "rw_heart_of_the_oak", "rw_infinity", "rw_last_wish" })) {
					return "adaptive_exposure";
				}
				if (idIsOneOf(id, { "rw_exile" })) {
					return "self_barrier";
				}
				if (idIsOneOf(id, { "rw_grief", "rw_breath_of_the_dying" })) {
					return "adaptive_strike";
				}
				if (idIsOneOf(id, { "rw_chains_of_honor", "rw_enigma" })) {
					return "self_phase";
				}
				if (idIsOneOf(id, { "rw_dream" })) {
					return "shock_strike";
				}
				if (idIsOneOf(id, { "rw_faith" })) {
					return "self_haste";
				}
				if (idIsOneOf(id, { "rw_phoenix" })) {
					return "self_phoenix";
				}
				if (idIsOneOf(id, { "rw_doom" })) {
					return "frost_strike";
				}

				if (idIsOneOf(id, { "rw_dragon", "rw_hand_of_justice", "rw_flickering_flame", "rw_temper" })) {
					return "self_flame_cloak";
				}
				if (idIsOneOf(id, { "rw_ice", "rw_voice_of_reason", "rw_hearth" })) {
					return "self_frost_cloak";
				}
				if (idIsOneOf(id, { "rw_holy_thunder", "rw_zephyr", "rw_wind" })) {
					return "self_shock_cloak";
				}
				if (idIsOneOf(id, { "rw_bulwark", "rw_cure", "rw_ancients_pledge", "rw_lionheart", "rw_radiance" })) {
					return "self_oakflesh";
				}
				if (idIsOneOf(id, { "rw_sanctuary", "rw_duress", "rw_peace", "rw_myth" })) {
					return "self_stoneflesh";
				}
				if (idIsOneOf(id, { "rw_pride", "rw_stone", "rw_prudence", "rw_mist" })) {
					return "self_ironflesh";
				}
				if (idIsOneOf(id, { "rw_metamorphosis" })) {
					return "self_ebonyflesh";
				}
				if (idIsOneOf(id, { "rw_nadir" })) {
					return "curse_fear";
				}
				if (idIsOneOf(id, { "rw_delirium", "rw_chaos" })) {
					return "curse_frenzy";
				}
				if (idIsOneOf(id, { "rw_malice", "rw_venom", "rw_plague", "rw_bramble" })) {
					return "poison_bloom";
				}
				if (idIsOneOf(id, { "rw_black", "rw_rift" })) {
					return "tar_bloom";
				}
				if (idIsOneOf(id, { "rw_mosaic", "rw_obsession", "rw_white" })) {
					return "siphon_bloom";
				}
				if (idIsOneOf(id, { "rw_lawbringer", "rw_wrath", "rw_kingslayer", "rw_principle" })) {
					return "curse_fragile";
				}
				if (idIsOneOf(id, { "rw_death", "rw_famine" })) {
					return "curse_slow_attack";
				}
				if (idIsOneOf(id, { "rw_leaf", "rw_destruction" })) {
					return "fire_strike";
				}
				if (idIsOneOf(id, { "rw_crescent_moon" })) {
					return "shock_strike";
				}
				if (idIsOneOf(id, { "rw_beast", "rw_hustle_w", "rw_harmony", "rw_fury", "rw_unbending_will", "rw_passion" })) {
					return "self_haste";
				}
				if (idIsOneOf(id, { "rw_stealth", "rw_smoke", "rw_treachery" })) {
					return "self_muffle";
				}
				if (idIsOneOf(id, { "rw_gloom" })) {
					return "self_invisibility";
				}
				if (idIsOneOf(id, { "rw_wealth", "rw_obedience", "rw_honor", "rw_eternity" })) {
					return "soul_trap";
				}
				if (idIsOneOf(id, { "rw_memory", "rw_wisdom", "rw_lore", "rw_melody", "rw_enlightenment" })) {
					return "self_meditation";
				}
				if (idIsOneOf(id, { "rw_pattern", "rw_strength", "rw_kings_grace", "rw_edge", "rw_oath" })) {
					return "adaptive_strike";
				}
				if (idIsOneOf(id, { "rw_silence", "rw_brand" })) {
					return "adaptive_exposure";
				}
				if (idIsOneOf(id, { "rw_hustle_a", "rw_splendor", "rw_rhyme" })) {
					return "self_phase";
				}
				if (idIsOneOf(id, { "rw_rain", "rw_ground" })) {
					return "self_phoenix";
				}

				if (a_recipe.recommendedBaseType) {
					if (*a_recipe.recommendedBaseType == LootItemType::kWeapon) {
						switch (recipeBucket(id, 6u)) {
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
						switch (recipeBucket(id, 6u)) {
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

				return recipeBucket(id, 2u) == 0u ? "adaptive_exposure" : "adaptive_strike";
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

		EventBridge::RunewordPanelState EventBridge::GetRunewordPanelState()
		{
			RunewordPanelState panelState{};
			if (!_configLoaded) {
				return panelState;
			}

			SanitizeRunewordState();
			if (!_runewordSelectedBaseKey) {
				return panelState;
			}
			panelState.hasBase = true;

			// If the selected base is already a completed runeword, show status and block further crafting.
			if (const auto* completed = ResolveCompletedRunewordRecipe(*_runewordSelectedBaseKey)) {
				panelState.hasRecipe = true;
				panelState.isComplete = true;
				panelState.recipeName = completed->displayName;
				panelState.totalRunes = static_cast<std::uint32_t>(completed->runeTokens.size());
				panelState.insertedRunes = panelState.totalRunes;
				panelState.canInsert = false;
				return panelState;
			}

			const RunewordRecipe* recipe = nullptr;
			std::uint32_t inserted = 0u;

			const auto stateIt = _runewordInstanceStates.find(*_runewordSelectedBaseKey);
			if (stateIt != _runewordInstanceStates.end()) {
				inserted = stateIt->second.insertedRunes;
				recipe = FindRunewordRecipeByToken(stateIt->second.recipeToken);
			}

			if (!recipe) {
				recipe = GetCurrentRunewordRecipe();
			}
			if (!recipe || recipe->runeTokens.empty()) {
				return panelState;
			}

			panelState.hasRecipe = true;
			panelState.recipeName = recipe->displayName;
			panelState.totalRunes = static_cast<std::uint32_t>(recipe->runeTokens.size());
			panelState.insertedRunes = std::min(inserted, panelState.totalRunes);

			const auto applyBlockReason = ResolveRunewordApplyBlockReason(*_runewordSelectedBaseKey, *recipe);
			const bool canApplyResult = applyBlockReason == RunewordApplyBlockReason::kNone;

			if (panelState.insertedRunes >= panelState.totalRunes) {
				// Legacy: allow finalization only when result can actually be applied.
				panelState.canInsert = canApplyResult;
				if (!canApplyResult) {
					panelState.missingSummary = BuildRunewordApplyBlockMessage(applyBlockReason);
				}
				return panelState;
			}

			// Transmute-only UI: require all remaining rune fragments at once.
			const auto requiredCounts = BuildRuneTokenCounts<16>(
				std::span<const std::uint64_t>(recipe->runeTokens.data(), recipe->runeTokens.size()),
				panelState.insertedRunes);

			auto* player = RE::PlayerCharacter::GetSingleton();
			bool ready = true;
			bool firstMissingSet = false;
			std::string missingSummary;

			panelState.requiredRunes.reserve(requiredCounts.size);
			for (std::size_t i = 0; i < requiredCounts.size; ++i) {
				const auto token = requiredCounts.entries[i].token;
				const auto required = requiredCounts.entries[i].count;
				if (token == 0u || required == 0u) {
					continue;
				}

				std::string runeName = "Rune";
				if (const auto nameIt = _runewordRuneNameByToken.find(token); nameIt != _runewordRuneNameByToken.end()) {
					runeName = nameIt->second;
				}

				const auto owned = GetOwnedRunewordFragmentCount(player, _runewordRuneNameByToken, token);
				panelState.requiredRunes.push_back(RunewordRuneRequirement{
					.runeName = runeName,
					.required = required,
					.owned = owned,
				});
				if (owned >= required) {
					continue;
				}

				ready = false;
				const auto missing = required - owned;
				if (!missingSummary.empty()) {
					missingSummary.append(", ");
				}
				missingSummary.append(runeName);
				missingSummary.append(" x");
				missingSummary.append(std::to_string(missing));

				if (!firstMissingSet) {
					firstMissingSet = true;
					panelState.nextRuneName = runeName;
					panelState.nextRuneOwned = owned;
				}
			}

			if (!missingSummary.empty()) {
				panelState.missingSummary = std::move(missingSummary);
			} else if (!canApplyResult) {
				panelState.missingSummary = BuildRunewordApplyBlockMessage(applyBlockReason);
			}

			panelState.canInsert = ready && canApplyResult;
			return panelState;
		}

		bool EventBridge::SelectRunewordRecipe(std::uint64_t a_recipeToken)
		{
			if (!_configLoaded || a_recipeToken == 0u || _runewordRecipes.empty()) {
				return false;
			}

			SanitizeRunewordState();
			const auto it = _runewordRecipeIndexByToken.find(a_recipeToken);
			if (it == _runewordRecipeIndexByToken.end() || it->second >= _runewordRecipes.size()) {
				return false;
			}

			const auto idx = it->second;
			_runewordRecipeCycleCursor = static_cast<std::uint32_t>(idx);
			const auto& recipe = _runewordRecipes[idx];

				if (_runewordSelectedBaseKey) {
					const auto selectedKey = *_runewordSelectedBaseKey;
					const bool completedBase = ResolveCompletedRunewordRecipe(selectedKey) != nullptr;

					if (completedBase) {
						_runewordInstanceStates.erase(selectedKey);
					} else {
					auto& state = _runewordInstanceStates[selectedKey];
					if (state.recipeToken != recipe.token) {
						state.recipeToken = recipe.token;
						state.insertedRunes = 0u;
					}
				}
			}

			std::string runes;
			for (std::size_t i = 0; i < recipe.runeIds.size(); ++i) {
				if (i > 0) {
					runes.push_back('-');
				}
				runes.append(recipe.runeIds[i]);
			}

			std::string note = "Runeword Recipe: " + recipe.displayName + " [" + runes + "]";
			if (_runewordSelectedBaseKey) {
				const auto baseType = ResolveInstanceLootType(*_runewordSelectedBaseKey);
				if (recipe.recommendedBaseType && baseType && *recipe.recommendedBaseType != *baseType) {
					note.append(" (Recommended ");
					note.append(DescribeLootItemType(*recipe.recommendedBaseType));
					note.append(", current ");
					note.append(DescribeLootItemType(*baseType));
					note.append(")");
				}
			}

			RE::DebugNotification(note.c_str());
			return true;
		}
}
