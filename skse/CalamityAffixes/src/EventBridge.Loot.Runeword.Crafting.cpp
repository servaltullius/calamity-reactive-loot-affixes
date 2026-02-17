#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/RunewordUtil.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

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

		void EventBridge::MaybeGrantRandomReforgeOrb()
		{
			const float chance = std::clamp(_loot.reforgeOrbChancePercent, 0.0f, 100.0f);
			if (chance <= 0.0f) {
				return;
			}

			std::uniform_real_distribution<float> chanceDist(0.0f, 100.0f);
			if (chanceDist(_rng) >= chance) {
				return;
			}

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return;
			}

			auto* orb = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_Misc_ReforgeOrb");
			if (!orb) {
				SKSE::log::error("CalamityAffixes: reforge orb item missing (editorId=CAFF_Misc_ReforgeOrb).");
				return;
			}

			player->AddObjectToContainer(orb, nullptr, 1, nullptr);
			const auto owned = std::max(0, player->GetItemCount(orb));

			std::string note = "Reforge Orb (";
			note.append(std::to_string(owned));
			note.push_back(')');
			RE::DebugNotification(note.c_str());
		}

		EventBridge::OperationResult EventBridge::ReforgeSelectedRunewordBaseWithOrb()
		{
			OperationResult result{};
			if (!_configLoaded) {
				result.message = "Reforge unavailable: runtime config not loaded.";
				return result;
			}

			SanitizeRunewordState();
			if (!_runewordSelectedBaseKey) {
				result.message = "Reforge: select a base first.";
				return result;
			}

			RE::InventoryEntryData* entry = nullptr;
			RE::ExtraDataList* xList = nullptr;
			if (!ResolvePlayerInventoryInstance(*_runewordSelectedBaseKey, entry, xList) || !entry || !entry->object || !xList) {
				_runewordSelectedBaseKey.reset();
				result.message = "Reforge failed: selected base is no longer available.";
				return result;
			}

			if (!IsLootObjectEligibleForAffixes(entry->object)) {
				result.message = "Reforge failed: this item type is not eligible.";
				return result;
			}

			const auto instanceKey = *_runewordSelectedBaseKey;
			if (const auto it = _instanceAffixes.find(instanceKey); it != _instanceAffixes.end()) {
				const auto primaryToken = it->second.GetPrimary();
				if (const auto rwIt = _runewordRecipeIndexByResultAffixToken.find(primaryToken);
					rwIt != _runewordRecipeIndexByResultAffixToken.end() && rwIt->second < _runewordRecipes.size()) {
					result.message = "Reforge blocked: completed runeword base.";
					return result;
				}
			}

			const auto lootType = ResolveInstanceLootType(instanceKey);
			if (!lootType) {
				result.message = "Reforge failed: unable to resolve item type.";
				return result;
			}

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				result.message = "Reforge failed: player not available.";
				return result;
			}

			auto* orb = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_Misc_ReforgeOrb");
			if (!orb) {
				result.message = "Reforge failed: orb item missing.";
				SKSE::log::error("CalamityAffixes: reforge orb item missing (editorId=CAFF_Misc_ReforgeOrb).");
				return result;
			}

			const auto ownedBefore = std::max(0, player->GetItemCount(orb));
			if (ownedBefore <= 0) {
				result.message = "Reforge failed: no Reforge Orb.";
				return result;
			}

			InstanceAffixSlots previousSlots{};
			if (const auto it = _instanceAffixes.find(instanceKey); it != _instanceAffixes.end()) {
				previousSlots = it->second;
			}

			const std::uint8_t targetCount = detail::ResolveReforgeTargetAffixCount(previousSlots.count);

			auto sameSlots = [](const InstanceAffixSlots& a_left, const InstanceAffixSlots& a_right) {
				if (a_left.count != a_right.count) {
					return false;
				}
				for (std::uint8_t i = 0; i < a_left.count; ++i) {
					if (a_left.tokens[i] != a_right.tokens[i]) {
						return false;
					}
				}
				return true;
			};

			auto rollSlots = [&](std::uint8_t a_rollCount) -> InstanceAffixSlots {
				InstanceAffixSlots slots{};
				std::vector<std::size_t> chosenIndices;
				chosenIndices.reserve(a_rollCount);

				const auto targets = detail::DetermineLootPrefixSuffixTargets(a_rollCount);
				const auto prefixTarget = targets.prefixTarget;
				const auto suffixTarget = targets.suffixTarget;

				std::vector<std::size_t> chosenPrefixIndices;
				for (std::uint8_t p = 0; p < prefixTarget; ++p) {
					static constexpr std::uint8_t kMaxRetries = 3;
					bool found = false;
					for (std::uint8_t retry = 0; retry < kMaxRetries; ++retry) {
						const auto idx = RollLootAffixIndex(*lootType, &chosenPrefixIndices, /*a_skipChanceCheck=*/true);
						if (!idx) {
							break;
						}
						if (std::find(chosenPrefixIndices.begin(), chosenPrefixIndices.end(), *idx) != chosenPrefixIndices.end()) {
							continue;
						}
						if (*idx >= _affixes.size() || _affixes[*idx].id.empty()) {
							continue;
						}
						chosenPrefixIndices.push_back(*idx);
						chosenIndices.push_back(*idx);
						slots.AddToken(_affixes[*idx].token);
						found = true;
						break;
					}
					if (!found) {
						break;
					}
				}

				std::vector<std::string> chosenFamilies;
				for (std::uint8_t s = 0; s < suffixTarget; ++s) {
					if (!detail::ShouldConsumeSuffixRollForSingleAffixTarget(a_rollCount, slots.count)) {
						break;
					}

					const auto idx = RollSuffixIndex(*lootType, &chosenFamilies);
					if (!idx) {
						break;
					}
					if (*idx >= _affixes.size() || _affixes[*idx].id.empty()) {
						continue;
					}
					if (!_affixes[*idx].family.empty()) {
						chosenFamilies.push_back(_affixes[*idx].family);
					}
					chosenIndices.push_back(*idx);
					slots.AddToken(_affixes[*idx].token);
				}

				if (slots.count == 0) {
					if (const auto fallback = RollLootAffixIndex(*lootType, nullptr, /*a_skipChanceCheck=*/true);
						fallback && *fallback < _affixes.size() && !_affixes[*fallback].id.empty()) {
						slots.AddToken(_affixes[*fallback].token);
					}
				}

				return slots;
			};

			InstanceAffixSlots newSlots{};
			static constexpr std::uint8_t kReforgeMaxAttempts = 4;
			for (std::uint8_t attempt = 0; attempt < kReforgeMaxAttempts; ++attempt) {
				const auto rolled = rollSlots(targetCount);
				if (rolled.count == 0) {
					continue;
				}
				if (previousSlots.count > 0 && sameSlots(previousSlots, rolled) && attempt + 1 < kReforgeMaxAttempts) {
					continue;
				}
				newSlots = rolled;
				break;
			}

			if (newSlots.count == 0) {
				result.message = "Reforge failed: no eligible affix in current pool.";
				return result;
			}

			player->RemoveItem(orb, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

			EraseInstanceRuntimeStates(instanceKey);
			_instanceAffixes[instanceKey] = newSlots;
			MarkLootEvaluatedInstance(instanceKey);

			for (std::uint8_t i = 0; i < newSlots.count; ++i) {
				EnsureInstanceRuntimeState(instanceKey, newSlots.tokens[i]);
			}

			EnsureMultiAffixDisplayName(entry, xList, newSlots);
			RebuildActiveCounts();

			const auto ownedAfter = std::max(0, player->GetItemCount(orb));
			std::string itemName = ResolveInventoryDisplayName(entry, xList);
			if (itemName.empty()) {
				itemName = "Selected base";
			}

			result.success = true;
			result.message = "Reforged: " + itemName + " (Orbs: " + std::to_string(ownedAfter) + ")";
			return result;
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
			// Clean up runtime states for any previous affixes before replacing.
			EraseInstanceRuntimeStates(a_instanceKey);
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
