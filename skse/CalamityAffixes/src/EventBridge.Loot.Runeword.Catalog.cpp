#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RunewordContractSnapshot.h"
#include "CalamityAffixes/RuntimeContract.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

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

			const std::size_t count = _runewordRecipes.size();
			const auto start = static_cast<std::size_t>(_runewordRecipeCycleCursor % count);
			for (std::size_t offset = 0; offset < count; ++offset) {
				const auto idx = (start + offset) % count;
				const auto& recipe = _runewordRecipes[idx];
				if (const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);
					affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
					continue;
				}
				return std::addressof(recipe);
			}
			return nullptr;
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

			const auto parseRecommendedBaseType = [](const std::optional<bool>& a_recommendedBaseIsWeapon) {
				if (!a_recommendedBaseIsWeapon.has_value()) {
					return std::optional<LootItemType>{};
				}
				return *a_recommendedBaseIsWeapon ?
					std::optional<LootItemType>{ LootItemType::kWeapon } :
					std::optional<LootItemType>{ LootItemType::kArmor };
			};

			auto loadCompiledFallbackCatalog = [&]() {
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
			};

			RunewordContractSnapshot contractSnapshot{};
			bool loadedFromContract = LoadRunewordContractSnapshotFromRuntime(
				RuntimeContract::kRuntimeContractRelativePath,
				contractSnapshot);

			if (loadedFromContract) {
				for (const auto& row : contractSnapshot.recipes) {
					std::vector<std::string_view> runeViews;
					runeViews.reserve(row.runeNames.size());
					for (const auto& runeName : row.runeNames) {
						runeViews.push_back(runeName);
					}

					addRecipe(
						row.id,
						row.displayName,
						runeViews,
						row.resultAffixId,
						parseRecommendedBaseType(row.recommendedBaseIsWeapon));
				}

				if (_runewordRecipes.empty()) {
					SKSE::log::warn(
						"CalamityAffixes: runtime contract snapshot produced zero usable runeword recipes; falling back to compiled catalog.");
					loadCompiledFallbackCatalog();
					loadedFromContract = false;
				} else {
					SKSE::log::info(
						"CalamityAffixes: loaded {} runeword recipes from runtime contract snapshot.",
						_runewordRecipes.size());
				}
			} else {
				loadCompiledFallbackCatalog();
			}

			if (!_runewordRecipes.empty()) {
				if (!loadedFromContract) {
					SKSE::log::info(
						"CalamityAffixes: loaded {} runeword recipes from compiled fallback catalog.",
						_runewordRecipes.size());
				}
			} else {
				SKSE::log::info(
					"CalamityAffixes: runeword catalog is empty after contract/fallback load.");
			}

				if (_runewordRecipeCycleCursor >= _runewordRecipes.size()) {
					_runewordRecipeCycleCursor = 0;
				}

				_runewordRuneTokenWeights.reserve(_runewordRuneTokenPool.size());
				for (const auto token : _runewordRuneTokenPool) {
					double weight = 1.0;
					if (const auto nameIt = _runewordRuneNameByToken.find(token); nameIt != _runewordRuneNameByToken.end()) {
						if (loadedFromContract) {
							if (const auto weightIt = contractSnapshot.runeWeightsByName.find(nameIt->second);
								weightIt != contractSnapshot.runeWeightsByName.end() && weightIt->second > 0.0) {
								weight = weightIt->second;
							} else {
								weight = ResolveRunewordFragmentWeight(nameIt->second);
							}
						} else {
							weight = ResolveRunewordFragmentWeight(nameIt->second);
						}
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
			const std::size_t count = _runewordRecipes.size();
			auto cursor = static_cast<std::size_t>(_runewordRecipeCycleCursor % count);
			const auto isEligible = [&](std::size_t a_idx) {
				const auto& recipe = _runewordRecipes[a_idx];
				const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);
				return affixIt != _affixIndexByToken.end() && affixIt->second < _affixes.size();
			};

			for (std::size_t step = 0; step < count; ++step) {
				if (a_direction > 0) {
					cursor = (cursor + 1u) % count;
				} else {
					cursor = (cursor + count - 1u) % count;
				}
				if (!isEligible(cursor)) {
					continue;
				}
				_runewordRecipeCycleCursor = static_cast<std::uint32_t>(cursor);
				break;
			}

			const auto* recipe = GetCurrentRunewordRecipe();
			if (!recipe) {
				return;
			}
			(void)SelectRunewordRecipe(recipe->token);
		}
}
