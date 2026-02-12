#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "CalamityAffixes/RunewordUtil.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CalamityAffixes
{
		namespace
		{
			std::string ToLowerAscii(std::string_view a_text)
		{
			std::string out(a_text);
			std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			return out;
		}

				[[nodiscard]] bool IsWeaponOrArmor(const RE::TESBoundObject* a_object)
				{
					if (!a_object) {
						return false;
					}

					return a_object->As<RE::TESObjectWEAP>() != nullptr || a_object->As<RE::TESObjectARMO>() != nullptr;
				}

				[[nodiscard]] std::string ResolveInventoryDisplayName(
					const RE::InventoryEntryData* a_entry,
					RE::ExtraDataList* a_xList)
				{
					if (!a_entry || !a_entry->object) {
						return "<Unknown>";
					}

					const char* displayRaw = a_xList ? a_xList->GetDisplayName(a_entry->object) : nullptr;
					std::string displayName = displayRaw ? displayRaw : "";
					if (displayName.empty()) {
						const char* objectNameRaw = a_entry->object->GetName();
						displayName = objectNameRaw ? objectNameRaw : "";
					}
					if (displayName.empty()) {
						displayName = "<Unknown>";
					}

					return displayName;
				}

				// Weighted rune fragment distribution to mimic D2 rune tier rarity.
				// Higher-tier runes intentionally have much lower weight.
				[[nodiscard]] double ResolveRunewordFragmentWeight(std::string_view a_runeName)
				{
					if (a_runeName.empty()) {
						return 1.0;
					}

					constexpr std::array<std::pair<std::string_view, double>, 33> kRuneWeights{ {
						{ "El", 1200.0 },
						{ "Eld", 1100.0 },
						{ "Tir", 1000.0 },
						{ "Nef", 900.0 },
						{ "Eth", 800.0 },
						{ "Ith", 700.0 },
						{ "Tal", 620.0 },
						{ "Ral", 560.0 },
						{ "Ort", 500.0 },
						{ "Thul", 450.0 },
						{ "Amn", 400.0 },
						{ "Sol", 340.0 },
						{ "Shael", 280.0 },
						{ "Dol", 230.0 },
						{ "Hel", 190.0 },
						{ "Io", 155.0 },
						{ "Lum", 125.0 },
						{ "Ko", 100.0 },
						{ "Fal", 80.0 },
						{ "Lem", 64.0 },
						{ "Pul", 50.0 },
						{ "Um", 39.0 },
						{ "Mal", 30.0 },
						{ "Ist", 23.0 },
						{ "Gul", 17.0 },
						{ "Vex", 12.0 },
						{ "Ohm", 8.5 },
						{ "Lo", 6.0 },
						{ "Sur", 4.2 },
						{ "Ber", 3.0 },
						{ "Jah", 2.2 },
						{ "Cham", 1.6 },
						{ "Zod", 1.0 },
					} };

					for (const auto& [name, weight] : kRuneWeights) {
						if (a_runeName == name) {
							return weight;
						}
					}

					return 25.0;
				}

				[[nodiscard]] bool IsLootSourceCorpseChestOrWorld(RE::FormID a_oldContainer, RE::FormID a_playerId)
				{
					if (a_oldContainer == LootRerollGuard::kWorldContainer) {
						return true;
					}
					if (a_oldContainer == a_playerId) {
						return false;
					}

					auto* sourceForm = RE::TESForm::LookupByID<RE::TESForm>(a_oldContainer);
					if (!sourceForm) {
						return false;
					}

					if (auto* sourceActor = sourceForm->As<RE::Actor>()) {
						return sourceActor->IsDead();
					}

					if (auto* sourceRef = sourceForm->As<RE::TESObjectREFR>()) {
						if (auto* sourceBase = sourceRef->GetBaseObject(); sourceBase && sourceBase->As<RE::TESObjectCONT>()) {
							return true;
						}
					}

					return sourceForm->As<RE::TESObjectCONT>() != nullptr;
				}

					constexpr std::string_view kRunewordFragmentEditorIdPrefix = "CAFF_RuneFrag_";

				RE::TESObjectMISC* LookupRunewordFragmentItem(
					const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
					std::uint64_t a_runeToken)
				{
					if (a_runeToken == 0u) {
						return nullptr;
					}

					static std::unordered_map<std::uint64_t, RE::TESObjectMISC*> cache;
					if (const auto cacheIt = cache.find(a_runeToken); cacheIt != cache.end()) {
						return cacheIt->second;
					}

					const auto nameIt = a_runeNameByToken.find(a_runeToken);
					if (nameIt == a_runeNameByToken.end() || nameIt->second.empty()) {
						return nullptr;
					}

					std::string editorId;
					editorId.reserve(kRunewordFragmentEditorIdPrefix.size() + nameIt->second.size());
					editorId.append(kRunewordFragmentEditorIdPrefix);
					editorId.append(nameIt->second);

					auto* item = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>(editorId);
					if (item) {
						cache.emplace(a_runeToken, item);
					}
					return item;
				}

				std::uint32_t GetOwnedRunewordFragmentCount(
					RE::PlayerCharacter* a_player,
					const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
					std::uint64_t a_runeToken)
				{
					if (!a_player) {
						return 0u;
					}

					auto* item = LookupRunewordFragmentItem(a_runeNameByToken, a_runeToken);
					if (!item) {
						return 0u;
					}

					const auto owned = a_player->GetItemCount(item);
					return owned > 0 ? static_cast<std::uint32_t>(owned) : 0u;
				}

				std::uint32_t GrantRunewordFragments(
					RE::PlayerCharacter* a_player,
					const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
					std::uint64_t a_runeToken,
					std::uint32_t a_amount)
				{
					if (!a_player || a_amount == 0u) {
						return 0u;
					}

					auto* item = LookupRunewordFragmentItem(a_runeNameByToken, a_runeToken);
					if (!item) {
						return 0u;
					}

					const auto maxGive = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
					const auto give = (a_amount > maxGive) ? maxGive : a_amount;
					a_player->AddObjectToContainer(item, nullptr, static_cast<std::int32_t>(give), nullptr);
					return give;
				}

				bool TryConsumeRunewordFragment(
					RE::PlayerCharacter* a_player,
					const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
					std::uint64_t a_runeToken)
				{
					if (!a_player) {
						return false;
					}

					auto* item = LookupRunewordFragmentItem(a_runeNameByToken, a_runeToken);
					if (!item) {
						return false;
					}

					if (a_player->GetItemCount(item) <= 0) {
						return false;
					}

					a_player->RemoveItem(item, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
					return true;
				}

			}

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

					keys.push_back(MakeInstanceKey(uid->baseID, uid->uniqueID));
				}
			}

			std::sort(keys.begin(), keys.end());
			keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
			return keys;
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

			_runewordSelectedBaseKey = a_instanceKey;

			const auto candidates = CollectEquippedRunewordBaseCandidates(true);
			if (const auto cursor = ResolveRunewordBaseCycleCursor(candidates, a_instanceKey); cursor) {
				_runewordBaseCycleCursor = *cursor;
			}

			auto& state = _runewordInstanceStates[a_instanceKey];
			if (state.recipeToken == 0u) {
				if (const auto* recipe = GetCurrentRunewordRecipe()) {
					state.recipeToken = recipe->token;
				}
			}

			const std::string selectedName = ResolveInventoryDisplayName(entry, xList);

			const auto* recipe = FindRunewordRecipeByToken(state.recipeToken);
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

				entries.push_back(RunewordBaseInventoryEntry{
					.instanceKey = key,
					.displayName = ResolveInventoryDisplayName(entry, xList),
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
				if (const auto stateIt = _runewordInstanceStates.find(*_runewordSelectedBaseKey);
					stateIt != _runewordInstanceStates.end() &&
					stateIt->second.recipeToken != 0u) {
					selectedToken = stateIt->second.recipeToken;
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
			if (const auto it = _instanceAffixes.find(*_runewordSelectedBaseKey); it != _instanceAffixes.end()) {
				const auto primaryToken = it->second.GetPrimary();
				if (const auto rwIt = _runewordRecipeIndexByResultAffixToken.find(primaryToken);
					rwIt != _runewordRecipeIndexByResultAffixToken.end() && rwIt->second < _runewordRecipes.size()) {
					const auto& completed = _runewordRecipes[rwIt->second];
					panelState.hasRecipe = true;
					panelState.isComplete = true;
					panelState.recipeName = completed.displayName;
					panelState.totalRunes = static_cast<std::uint32_t>(completed.runeTokens.size());
					panelState.insertedRunes = panelState.totalRunes;
					panelState.canInsert = false;
					return panelState;
				}
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

			if (panelState.insertedRunes >= panelState.totalRunes) {
				// Legacy: allow finalization when all runes have already been inserted.
				panelState.canInsert = true;
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

				panelState.missingSummary = std::move(missingSummary);
				panelState.canInsert = ready;
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
				auto& state = _runewordInstanceStates[*_runewordSelectedBaseKey];
				if (state.recipeToken != recipe.token) {
					state.recipeToken = recipe.token;
					state.insertedRunes = 0u;
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

		bool EventBridge::ResolvePlayerInventoryInstance(
			std::uint64_t a_instanceKey,
			RE::InventoryEntryData*& a_outEntry,
			RE::ExtraDataList*& a_outXList) const
		{
			a_outEntry = nullptr;
			a_outXList = nullptr;

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return false;
			}

			auto* changes = player->GetInventoryChanges();
			if (!changes || !changes->entryList) {
				return false;
			}

			for (auto* entry : *changes->entryList) {
				if (!entry || !entry->extraLists) {
					continue;
				}

				for (auto* xList : *entry->extraLists) {
					if (!xList) {
						continue;
					}

					const auto* uid = xList->GetByType<RE::ExtraUniqueID>();
					if (!uid) {
						continue;
					}

					if (MakeInstanceKey(uid->baseID, uid->uniqueID) != a_instanceKey) {
						continue;
					}

					a_outEntry = entry;
					a_outXList = xList;
					return true;
				}
			}

			return false;
		}

		std::optional<EventBridge::LootItemType> EventBridge::ResolveInstanceLootType(std::uint64_t a_instanceKey) const
		{
			RE::InventoryEntryData* entry = nullptr;
			RE::ExtraDataList* xList = nullptr;
			if (!ResolvePlayerInventoryInstance(a_instanceKey, entry, xList) || !entry || !entry->object) {
				return std::nullopt;
			}

			if (entry->object->As<RE::TESObjectWEAP>()) {
				return LootItemType::kWeapon;
			}
			if (entry->object->As<RE::TESObjectARMO>()) {
				return LootItemType::kArmor;
			}

			return std::nullopt;
		}

		const char* EventBridge::DescribeLootItemType(LootItemType a_type)
		{
			switch (a_type) {
			case LootItemType::kWeapon:
				return "Weapon";
			case LootItemType::kArmor:
				return "Armor";
			}

			return "Unknown";
		}

		const EventBridge::RunewordRecipe* EventBridge::FindRunewordRecipeByToken(std::uint64_t a_recipeToken) const
		{
			const auto it = _runewordRecipeIndexByToken.find(a_recipeToken);
			if (it == _runewordRecipeIndexByToken.end() || it->second >= _runewordRecipes.size()) {
				return nullptr;
			}

			return std::addressof(_runewordRecipes[it->second]);
		}

		const EventBridge::RunewordRecipe* EventBridge::GetCurrentRunewordRecipe() const
		{
			if (_runewordRecipes.empty()) {
				return nullptr;
			}

			const auto idx = static_cast<std::size_t>(_runewordRecipeCycleCursor % _runewordRecipes.size());
			return std::addressof(_runewordRecipes[idx]);
		}

		void EventBridge::InitializeRunewordCatalog()
		{
			_runewordRecipes.clear();
				_runewordRecipeIndexByToken.clear();
				_runewordRecipeIndexByResultAffixToken.clear();
				_runewordRuneNameByToken.clear();
				_runewordRuneTokenPool.clear();
				_runewordRuneTokenWeights.clear();

			auto addRecipe = [&](std::string_view a_recipeId,
				std::string_view a_displayName,
				const std::vector<std::string_view>& a_runes,
				std::string_view a_resultAffixId,
				std::optional<LootItemType> a_recommendedBaseType) {
				RunewordRecipe recipe{};
				recipe.id = std::string(a_recipeId);
				recipe.token = MakeAffixToken(recipe.id);
				recipe.displayName = std::string(a_displayName);
				recipe.resultAffixToken = MakeAffixToken(a_resultAffixId);
				recipe.recommendedBaseType = a_recommendedBaseType;

				for (const auto rune : a_runes) {
					if (rune.empty()) {
						continue;
					}

					recipe.runeIds.emplace_back(rune);
					const auto runeToken = MakeAffixToken(ToLowerAscii(rune));
					recipe.runeTokens.push_back(runeToken);

					_runewordRuneNameByToken.try_emplace(runeToken, std::string(rune));
					if (std::find(_runewordRuneTokenPool.begin(), _runewordRuneTokenPool.end(), runeToken) == _runewordRuneTokenPool.end()) {
						_runewordRuneTokenPool.push_back(runeToken);
					}
				}

				if (recipe.token == 0u || recipe.resultAffixToken == 0u || recipe.runeTokens.empty()) {
					return;
				}

				const auto idx = _runewordRecipes.size();
				_runewordRecipeIndexByToken.emplace(recipe.token, idx);
				_runewordRecipeIndexByResultAffixToken.emplace(recipe.resultAffixToken, idx);
				_runewordRecipes.push_back(std::move(recipe));
			};

			struct RunewordCatalogRow
			{
				std::string_view recipeId{};
				std::string_view displayName{};
				std::string_view runeCsv{};
				std::string_view resultAffixId{};
				std::optional<LootItemType> recommendedBaseType{};
			};

			constexpr std::array<RunewordCatalogRow, 94> kRunewordCatalogRows{ {
#include "RunewordCatalogRows.inl"
			} };

			auto splitRunes = [](std::string_view a_csv) {
				std::vector<std::string_view> runes;
				while (!a_csv.empty()) {
					const auto pos = a_csv.find(',');
					const auto token = (pos == std::string_view::npos) ? a_csv : a_csv.substr(0, pos);
					if (!token.empty()) {
						runes.push_back(token);
					}
					if (pos == std::string_view::npos) {
						break;
					}
					a_csv.remove_prefix(pos + 1);
				}
				return runes;
			};

			// Extended D2/D2R runeword catalog (94 recipes).
			for (const auto& row : kRunewordCatalogRows) {
				const auto runes = splitRunes(row.runeCsv);
				if (runes.empty()) {
					continue;
				}
				addRecipe(
					row.recipeId,
					row.displayName,
					runes,
					row.resultAffixId,
					row.recommendedBaseType);
			}

				if (_runewordRecipeCycleCursor >= _runewordRecipes.size()) {
					_runewordRecipeCycleCursor = 0;
				}

				_runewordRuneTokenWeights.reserve(_runewordRuneTokenPool.size());
				for (const auto token : _runewordRuneTokenPool) {
					double weight = 1.0;
					if (const auto nameIt = _runewordRuneNameByToken.find(token); nameIt != _runewordRuneNameByToken.end()) {
						weight = ResolveRunewordFragmentWeight(nameIt->second);
					}
					_runewordRuneTokenWeights.push_back(weight);
				}
			}

			void EventBridge::SanitizeRunewordState()
			{
				auto* player = RE::PlayerCharacter::GetSingleton();
				const auto canValidateInventory = (player && player->GetInventoryChanges() && player->GetInventoryChanges()->entryList);
				if (canValidateInventory) {
					// Legacy -> physical item migration: older saves stored fragments in SKSE serialization.
					// Convert those counters into real inventory items once, then clear the legacy map.
					if (!_runewordRuneFragments.empty()) {
						if (_runewordRuneNameByToken.empty()) {
							InitializeRunewordCatalog();
						}

						std::uint32_t migrated = 0u;
						for (auto it = _runewordRuneFragments.begin(); it != _runewordRuneFragments.end();) {
							const auto runeToken = it->first;
							const auto amount = it->second;
							if (runeToken == 0u || amount == 0u) {
								it = _runewordRuneFragments.erase(it);
								continue;
							}

							const auto given = GrantRunewordFragments(player, _runewordRuneNameByToken, runeToken, amount);
							if (given > 0u) {
								const auto maxVal = std::numeric_limits<std::uint32_t>::max();
								migrated = (migrated > maxVal - given) ? maxVal : (migrated + given);
								it = _runewordRuneFragments.erase(it);
							} else {
								++it;
							}
						}

						if (migrated > 0u) {
							RE::DebugNotification("Runeword: migrated legacy rune fragments to inventory.");
							SKSE::log::info(
								"CalamityAffixes: migrated {} legacy runeword fragments to inventory items.",
								migrated);
						}
					}

					for (auto it = _runewordInstanceStates.begin(); it != _runewordInstanceStates.end();) {
						if (!PlayerHasInstanceKey(it->first)) {
							it = _runewordInstanceStates.erase(it);
						} else {
							++it;
					}
				}

				if (_runewordSelectedBaseKey && !PlayerHasInstanceKey(*_runewordSelectedBaseKey)) {
					_runewordSelectedBaseKey.reset();
				}
			}

			if (_runewordRecipes.empty()) {
				_runewordRecipeCycleCursor = 0;
				return;
			}

			if (_runewordRecipeCycleCursor >= _runewordRecipes.size()) {
				_runewordRecipeCycleCursor = 0;
			}

			const auto* currentRecipe = GetCurrentRunewordRecipe();
			for (auto& [instanceKey, state] : _runewordInstanceStates) {
				auto* recipe = FindRunewordRecipeByToken(state.recipeToken);
				if (!recipe) {
					if (currentRecipe) {
						state.recipeToken = currentRecipe->token;
						recipe = currentRecipe;
					}
				}

				if (recipe && state.insertedRunes > recipe->runeTokens.size()) {
					state.insertedRunes = static_cast<std::uint32_t>(recipe->runeTokens.size());
				}
			}
		}

		void EventBridge::CycleRunewordBase(std::int32_t a_direction)
		{
			if (!_configLoaded || a_direction == 0) {
				return;
			}

			SanitizeRunewordState();
			auto candidates = CollectEquippedRunewordBaseCandidates(true);
			if (candidates.empty()) {
				RE::DebugNotification("Runeword: no equipped weapon/armor base found.");
				return;
			}

			std::size_t index = static_cast<std::size_t>(_runewordBaseCycleCursor % candidates.size());
			if (_runewordSelectedBaseKey) {
				const auto selectedIt = std::find(candidates.begin(), candidates.end(), *_runewordSelectedBaseKey);
				if (selectedIt != candidates.end()) {
					index = static_cast<std::size_t>(std::distance(candidates.begin(), selectedIt));
				}
			}

			if (a_direction > 0) {
				index = (index + 1u) % candidates.size();
			} else {
				index = (index + candidates.size() - 1u) % candidates.size();
			}

			const auto selectedKey = candidates[index];
			_runewordBaseCycleCursor = static_cast<std::uint32_t>(index);
			(void)SelectRunewordBase(selectedKey);
		}

		void EventBridge::CycleRunewordRecipe(std::int32_t a_direction)
		{
			if (!_configLoaded || a_direction == 0 || _runewordRecipes.empty()) {
				return;
			}

			SanitizeRunewordState();
			if (a_direction > 0) {
				_runewordRecipeCycleCursor = static_cast<std::uint32_t>((_runewordRecipeCycleCursor + 1u) % _runewordRecipes.size());
			} else {
				const auto current = static_cast<std::size_t>(_runewordRecipeCycleCursor % _runewordRecipes.size());
				const auto next = (current + _runewordRecipes.size() - 1u) % _runewordRecipes.size();
				_runewordRecipeCycleCursor = static_cast<std::uint32_t>(next);
			}

			const auto* recipe = GetCurrentRunewordRecipe();
			if (!recipe) {
				return;
			}
			(void)SelectRunewordRecipe(recipe->token);
		}

		void EventBridge::GrantNextRequiredRuneFragment(std::uint32_t a_amount)
		{
			if (!_configLoaded || a_amount == 0u) {
				return;
			}

			SanitizeRunewordState();
			const RunewordRecipe* recipe = nullptr;
			std::uint32_t nextIndex = 0u;
			if (_runewordSelectedBaseKey) {
				const auto stateIt = _runewordInstanceStates.find(*_runewordSelectedBaseKey);
				if (stateIt != _runewordInstanceStates.end()) {
					recipe = FindRunewordRecipeByToken(stateIt->second.recipeToken);
					nextIndex = stateIt->second.insertedRunes;
				}
			}

			if (!recipe) {
				recipe = GetCurrentRunewordRecipe();
			}
			if (!recipe || recipe->runeTokens.empty()) {
				return;
			}

				if (nextIndex >= recipe->runeTokens.size()) {
					nextIndex = 0u;
				}

				const auto runeToken = recipe->runeTokens[nextIndex];
				std::string runeName = "Rune";
				if (const auto nameIt = _runewordRuneNameByToken.find(runeToken); nameIt != _runewordRuneNameByToken.end()) {
					runeName = nameIt->second;
				}

				auto* player = RE::PlayerCharacter::GetSingleton();
				if (!player) {
					return;
				}

				const auto given = GrantRunewordFragments(player, _runewordRuneNameByToken, runeToken, a_amount);
				if (given == 0u) {
					std::string note = "Runeword fragment item missing: ";
					note.append(runeName);
					RE::DebugNotification(note.c_str());
					SKSE::log::error(
						"CalamityAffixes: runeword fragment item missing (runeToken={:016X}, runeName={}).",
						runeToken,
						runeName);
					return;
				}

				const auto owned = GetOwnedRunewordFragmentCount(player, _runewordRuneNameByToken, runeToken);
				const std::string note = "Runeword Fragment + " + runeName + " (" + std::to_string(owned) + ")";
				RE::DebugNotification(note.c_str());
			}

		void EventBridge::GrantCurrentRecipeRuneSet(std::uint32_t a_amount)
		{
			if (!_configLoaded || a_amount == 0u) {
				return;
			}

			const auto* recipe = GetCurrentRunewordRecipe();
				if (!recipe) {
					return;
				}

				auto* player = RE::PlayerCharacter::GetSingleton();
				if (!player) {
					return;
				}

				std::uint32_t totalGiven = 0u;
				for (const auto runeToken : recipe->runeTokens) {
					const auto given = GrantRunewordFragments(player, _runewordRuneNameByToken, runeToken, a_amount);
					const auto maxVal = std::numeric_limits<std::uint32_t>::max();
					totalGiven = (totalGiven > maxVal - given) ? maxVal : (totalGiven + given);
				}

				if (totalGiven == 0u) {
					RE::DebugNotification("Runeword fragments: missing rune fragment item records.");
					SKSE::log::error("CalamityAffixes: runeword fragment items missing (grant recipe set).");
					return;
				}

				const std::string note = "Runeword Fragments + Set: " + recipe->displayName;
				RE::DebugNotification(note.c_str());
			}

		void EventBridge::MaybeGrantRandomRunewordFragment()
		{
			if (_runewordRuneTokenPool.empty()) {
				return;
			}

			const float chance = std::clamp(_loot.runewordFragmentChancePercent, 0.0f, 100.0f);
			if (chance <= 0.0f) {
				return;
			}

			std::uniform_real_distribution<float> chanceDist(0.0f, 100.0f);
			if (chanceDist(_rng) >= chance) {
				return;
			}

				std::uint64_t runeToken = 0u;
				if (!_runewordRuneTokenWeights.empty() && _runewordRuneTokenWeights.size() == _runewordRuneTokenPool.size()) {
					std::discrete_distribution<std::size_t> pick(_runewordRuneTokenWeights.begin(), _runewordRuneTokenWeights.end());
					runeToken = _runewordRuneTokenPool[pick(_rng)];
				} else {
					std::uniform_int_distribution<std::size_t> pick(0, _runewordRuneTokenPool.size() - 1u);
					runeToken = _runewordRuneTokenPool[pick(_rng)];
				}

				std::string runeName = "Rune";
				if (const auto nameIt = _runewordRuneNameByToken.find(runeToken); nameIt != _runewordRuneNameByToken.end()) {
					runeName = nameIt->second;
				}

				auto* player = RE::PlayerCharacter::GetSingleton();
				if (!player) {
					return;
				}

				const auto given = GrantRunewordFragments(player, _runewordRuneNameByToken, runeToken, 1u);
				if (given == 0u) {
					SKSE::log::error(
						"CalamityAffixes: runeword fragment item missing (runeToken={:016X}, runeName={}).",
						runeToken,
						runeName);
					return;
				}

				const auto owned = GetOwnedRunewordFragmentCount(player, _runewordRuneNameByToken, runeToken);

				std::string note = "Runeword Fragment: ";
				note.append(runeName);
				note.append(" (");
				note.append(std::to_string(owned));
				note.push_back(')');
				RE::DebugNotification(note.c_str());
			}

		void EventBridge::ApplyRunewordResult(std::uint64_t a_instanceKey, const RunewordRecipe& a_recipe)
		{
			const auto affixIt = _affixIndexByToken.find(a_recipe.resultAffixToken);
			if (affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
				std::string note = "Runeword failed: missing affix ";
				note.append(a_recipe.displayName);
				RE::DebugNotification(note.c_str());
				SKSE::log::error(
					"CalamityAffixes: runeword result affix missing (recipe={}, resultToken={:016X}).",
					a_recipe.id,
					a_recipe.resultAffixToken);
				return;
			}

			// Runeword replaces ALL existing affixes (D2-style full replacement).
			auto& slots = _instanceAffixes[a_instanceKey];
			slots.ReplaceAll(a_recipe.resultAffixToken);

			EnsureInstanceRuntimeState(a_instanceKey, a_recipe.resultAffixToken);
			_runewordInstanceStates.erase(a_instanceKey);

			RE::InventoryEntryData* entry = nullptr;
			RE::ExtraDataList* xList = nullptr;
			if (ResolvePlayerInventoryInstance(a_instanceKey, entry, xList) && entry && xList) {
				EnsureMultiAffixDisplayName(entry, xList, slots);
			}

			RebuildActiveCounts();

			std::string note = "Runeword Complete: ";
			note.append(a_recipe.displayName);
			RE::DebugNotification(note.c_str());
			SKSE::log::info(
				"CalamityAffixes: runeword completed (recipe={}, resultAffix={}).",
				a_recipe.id,
				_affixes[affixIt->second].id);
		}

		void EventBridge::InsertRunewordRuneIntoSelectedBase()
		{
			if (!_configLoaded) {
				return;
			}

			SanitizeRunewordState();
			if (!_runewordSelectedBaseKey) {
				RE::DebugNotification("Runeword: select a base first.");
				return;
			}

			RE::InventoryEntryData* entry = nullptr;
			RE::ExtraDataList* xList = nullptr;
			if (!ResolvePlayerInventoryInstance(*_runewordSelectedBaseKey, entry, xList)) {
				RE::DebugNotification("Runeword: selected base is no longer available.");
				_runewordSelectedBaseKey.reset();
				return;
			}

			// If base already has a completed runeword, block further crafting.
			if (const auto it = _instanceAffixes.find(*_runewordSelectedBaseKey); it != _instanceAffixes.end()) {
				const auto primaryToken = it->second.GetPrimary();
				if (const auto rwIt = _runewordRecipeIndexByResultAffixToken.find(primaryToken);
					rwIt != _runewordRecipeIndexByResultAffixToken.end() && rwIt->second < _runewordRecipes.size()) {
					std::string note = "Runeword: already complete (";
					note.append(_runewordRecipes[rwIt->second].displayName);
					note.push_back(')');
					RE::DebugNotification(note.c_str());
					return;
				}
			}

			auto& state = _runewordInstanceStates[*_runewordSelectedBaseKey];
			if (state.recipeToken == 0u) {
				if (const auto* currentRecipe = GetCurrentRunewordRecipe()) {
					state.recipeToken = currentRecipe->token;
				}
			}

			const RunewordRecipe* recipe = FindRunewordRecipeByToken(state.recipeToken);
			if (!recipe) {
				recipe = GetCurrentRunewordRecipe();
				if (recipe) {
					state.recipeToken = recipe->token;
				}
			}
			if (!recipe || recipe->runeTokens.empty()) {
				RE::DebugNotification("Runeword: recipe is not set.");
				return;
			}

			const auto totalRunes = recipe->runeTokens.size();
			if (state.insertedRunes >= totalRunes) {
				// Legacy finalize path.
				ApplyRunewordResult(*_runewordSelectedBaseKey, *recipe);
				return;
			}

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return;
			}

			const auto requiredCounts = BuildRuneTokenCounts<16>(
				std::span<const std::uint64_t>(recipe->runeTokens.data(), recipe->runeTokens.size()),
				state.insertedRunes);

			bool ready = true;
			std::string missingSummary;
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
			}

			if (!ready) {
				std::string note = "Runeword missing fragments: ";
				note.append(missingSummary.empty() ? "?" : missingSummary);
				RE::DebugNotification(note.c_str());
				return;
			}

			// Consume all required fragments.
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

				auto* fragmentItem = LookupRunewordFragmentItem(_runewordRuneNameByToken, token);
				if (!fragmentItem) {
					std::string note = "Runeword fragment item missing: ";
					note.append(runeName);
					RE::DebugNotification(note.c_str());
					SKSE::log::error(
						"CalamityAffixes: runeword fragment item missing (requiredRuneToken={:016X}, runeName={}).",
						token,
						runeName);
					return;
				}

				for (std::uint32_t n = 0; n < required; ++n) {
					if (!TryConsumeRunewordFragment(player, _runewordRuneNameByToken, token)) {
						std::string note = "Runeword failed to consume fragment: ";
						note.append(runeName);
						RE::DebugNotification(note.c_str());
						SKSE::log::warn(
							"CalamityAffixes: runeword failed to consume fragment (runeToken={:016X}, runeName={}).",
							token,
							runeName);
						return;
					}
				}
			}

			ApplyRunewordResult(*_runewordSelectedBaseKey, *recipe);
		}

			std::string EventBridge::BuildRunewordTooltip(std::uint64_t a_instanceKey) const
			{
				std::string tooltip;
				const AffixRuntime* runewordAffix = nullptr;

				auto appendRunewordEffectSummary = [&](const AffixRuntime& a_affix) {
					auto readSpellName = [](const RE::SpellItem* a_spell) -> std::string {
						if (!a_spell) {
							return {};
						}
						const char* raw = a_spell->GetName();
						if (!raw || !*raw) {
							return {};
						}
						return std::string(raw);
					};

					std::string summary;
					switch (a_affix.action.type) {
					case ActionType::kCastSpell: {
						summary = readSpellName(a_affix.action.spell);
						if (summary.empty()) {
							summary = a_affix.action.applyToSelf ? "Self spell buff" : "Spell proc";
						}
						break;
					}
					case ActionType::kCastSpellAdaptiveElement: {
						std::vector<std::string> spellNames;
						auto addUnique = [&](const std::string& a_name) {
							if (a_name.empty()) {
								return;
							}
							if (std::find(spellNames.begin(), spellNames.end(), a_name) == spellNames.end()) {
								spellNames.push_back(a_name);
							}
						};
						addUnique(readSpellName(a_affix.action.adaptiveFire));
						addUnique(readSpellName(a_affix.action.adaptiveFrost));
						addUnique(readSpellName(a_affix.action.adaptiveShock));

						if (spellNames.empty()) {
							summary = "Adaptive elemental effect";
						} else {
							for (std::size_t i = 0; i < spellNames.size(); ++i) {
								if (i > 0) {
									summary.append(" / ");
								}
								summary.append(spellNames[i]);
							}
						}
						break;
					}
					case ActionType::kCastOnCrit:
						summary = "Cast-on-crit effect";
						break;
					case ActionType::kConvertDamage:
						summary = "Damage conversion effect";
						break;
					case ActionType::kArchmage:
						summary = "Archmage scaling proc";
						break;
					case ActionType::kCorpseExplosion:
						summary = "Corpse explosion proc";
						break;
					case ActionType::kSummonCorpseExplosion:
						summary = "Summon corpse explosion proc";
						break;
					case ActionType::kSpawnTrap:
						summary = "Trap spawn effect";
						break;
					case ActionType::kDebugNotify:
					default:
						break;
					}

					if (summary.empty()) {
						return;
					}

					if (!tooltip.empty()) {
						tooltip.push_back('\n');
					}
					tooltip.append("Runeword Effect: ");
					tooltip.append(summary);
				};

				if (const auto affixIt = _instanceAffixes.find(a_instanceKey); affixIt != _instanceAffixes.end()) {
					const auto primaryToken = affixIt->second.GetPrimary();
					if (const auto rwIt = _runewordRecipeIndexByResultAffixToken.find(primaryToken);
						rwIt != _runewordRecipeIndexByResultAffixToken.end() && rwIt->second < _runewordRecipes.size()) {
						tooltip = "Runeword Complete: ";
						tooltip.append(_runewordRecipes[rwIt->second].displayName);

						if (const auto idxIt = _affixIndexByToken.find(primaryToken);
							idxIt != _affixIndexByToken.end() && idxIt->second < _affixes.size()) {
							runewordAffix = std::addressof(_affixes[idxIt->second]);
						}
					}
				}

				if (runewordAffix) {
					appendRunewordEffectSummary(*runewordAffix);
				}

				const auto stateIt = _runewordInstanceStates.find(a_instanceKey);
				if (stateIt == _runewordInstanceStates.end()) {
					return tooltip;
				}

				const auto* recipe = FindRunewordRecipeByToken(stateIt->second.recipeToken);
				if (!recipe) {
					return tooltip;
				}

				if (!tooltip.empty()) {
					tooltip.push_back('\n');
				}

				tooltip.append("Runeword: ");
				tooltip.append(recipe->displayName);
				tooltip.append(" (");
				tooltip.append(std::to_string(stateIt->second.insertedRunes));
				tooltip.push_back('/');
				tooltip.append(std::to_string(recipe->runeTokens.size()));
				tooltip.push_back(')');

				if (stateIt->second.insertedRunes < recipe->runeTokens.size()) {
					const auto nextRuneToken = recipe->runeTokens[stateIt->second.insertedRunes];
					std::string runeName = "Rune";
					if (const auto nameIt = _runewordRuneNameByToken.find(nextRuneToken); nameIt != _runewordRuneNameByToken.end()) {
						runeName = nameIt->second;
					}
					auto* player = RE::PlayerCharacter::GetSingleton();
					const auto owned = GetOwnedRunewordFragmentCount(player, _runewordRuneNameByToken, nextRuneToken);

					tooltip.append(" / Next: ");
					tooltip.append(runeName);
					tooltip.append(" (owned ");
					tooltip.append(std::to_string(owned));
					tooltip.push_back(')');
				}

				if (recipe->recommendedBaseType) {
					const auto baseType = ResolveInstanceLootType(a_instanceKey);
					if (baseType && *baseType != *recipe->recommendedBaseType) {
						tooltip.append("\nRecommended Base: ");
						tooltip.append(DescribeLootItemType(*recipe->recommendedBaseType));
						tooltip.append(" (current ");
						tooltip.append(DescribeLootItemType(*baseType));
						tooltip.push_back(')');
					}
				}

				if (_runewordSelectedBaseKey && *_runewordSelectedBaseKey == a_instanceKey) {
					tooltip.append("\nRuneword Base: Selected");
				}

				return tooltip;
			}

		void EventBridge::LogRunewordStatus() const
		{
			std::string note = "Runeword Status: ";
			if (const auto* recipe = GetCurrentRunewordRecipe()) {
				note.append(recipe->displayName);
			} else {
				note.append("No recipe");
			}

			if (_runewordSelectedBaseKey) {
				note.append(" | Base selected");
				const auto stateIt = _runewordInstanceStates.find(*_runewordSelectedBaseKey);
				if (stateIt != _runewordInstanceStates.end()) {
					const auto* recipe = FindRunewordRecipeByToken(stateIt->second.recipeToken);
					if (recipe) {
						note.append(" | ");
						note.append(std::to_string(stateIt->second.insertedRunes));
						note.push_back('/');
						note.append(std::to_string(recipe->runeTokens.size()));
					}
				}
			}

				if (const auto* recipe = GetCurrentRunewordRecipe()) {
					note.append(" | Runes:");
					auto* player = RE::PlayerCharacter::GetSingleton();
					for (const auto token : recipe->runeTokens) {
						std::string runeName = "Rune";
						if (const auto nameIt = _runewordRuneNameByToken.find(token); nameIt != _runewordRuneNameByToken.end()) {
							runeName = nameIt->second;
						}
						const auto owned = GetOwnedRunewordFragmentCount(player, _runewordRuneNameByToken, token);
						note.push_back(' ');
						note.append(runeName);
						note.push_back('=');
						note.append(std::to_string(owned));
					}
				}

			RE::DebugNotification(note.c_str());
			SKSE::log::info("CalamityAffixes: {}", note);
		}


}
