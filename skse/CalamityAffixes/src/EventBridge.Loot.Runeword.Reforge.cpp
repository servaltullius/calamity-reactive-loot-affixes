#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

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
		const auto* completedRunewordRecipe = ResolveCompletedRunewordRecipe(instanceKey);
		const bool rerollRuneword = completedRunewordRecipe != nullptr;

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

		auto rollRegularAffixSlots = [&](std::uint8_t a_rollCount) -> InstanceAffixSlots {
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

		auto pickReforgeRunewordRecipe = [&]() -> const RunewordRecipe* {
			if (_runewordRecipes.empty()) {
				return nullptr;
			}

			std::vector<const RunewordRecipe*> eligible;
			eligible.reserve(_runewordRecipes.size());

			for (const auto& recipe : _runewordRecipes) {
				if (recipe.resultAffixToken == 0u) {
					continue;
				}
				if (const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);
					affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
					continue;
				}

				eligible.push_back(std::addressof(recipe));
			}

			if (eligible.empty()) {
				return nullptr;
			}

			const auto currentToken = completedRunewordRecipe ? completedRunewordRecipe->resultAffixToken : 0u;
			std::uniform_int_distribution<std::size_t> pick(0, eligible.size() - 1u);
			const RunewordRecipe* selected = eligible[pick(_rng)];
			if (eligible.size() <= 1u || currentToken == 0u) {
				return selected;
			}

			// Try a few times to avoid returning the same completed runeword token.
			for (std::uint8_t retry = 0; retry < 3u; ++retry) {
				if (selected->resultAffixToken != currentToken) {
					return selected;
				}
				selected = eligible[pick(_rng)];
			}
			return selected;
		};

		InstanceAffixSlots newSlots{};
		const RunewordRecipe* rolledRunewordRecipe = nullptr;
		static constexpr std::uint8_t kReforgeMaxAttempts = 4;
		for (std::uint8_t attempt = 0; attempt < kReforgeMaxAttempts; ++attempt) {
			InstanceAffixSlots rolled{};
			const RunewordRecipe* attemptRunewordRecipe = nullptr;

			if (rerollRuneword) {
				attemptRunewordRecipe = pickReforgeRunewordRecipe();
				if (!attemptRunewordRecipe) {
					result.message = "Reforge failed: no eligible runeword effect.";
					return result;
				}

				rolled.AddToken(attemptRunewordRecipe->resultAffixToken);
				const std::uint8_t regularTargetCount = static_cast<std::uint8_t>(
					std::clamp<int>(RollAffixCount(), 1, static_cast<int>(kMaxRegularAffixesPerItem)));
				const auto regularSlots = rollRegularAffixSlots(regularTargetCount);
				for (std::uint8_t i = 0; i < regularSlots.count; ++i) {
					(void)rolled.AddToken(regularSlots.tokens[i]);
				}
			} else {
				const std::uint8_t targetCount = detail::ResolveReforgeTargetAffixCount(previousSlots.count);
				rolled = rollRegularAffixSlots(targetCount);
			}

			if (rolled.count == 0) {
				continue;
			}
			if (previousSlots.count > 0 && sameSlots(previousSlots, rolled) && attempt + 1 < kReforgeMaxAttempts) {
				continue;
			}
			newSlots = rolled;
			rolledRunewordRecipe = attemptRunewordRecipe;
			break;
		}

		if (newSlots.count == 0) {
			result.message = "Reforge failed: no eligible affix in current pool.";
			return result;
		}

		player->RemoveItem(orb, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

		EraseInstanceRuntimeStates(instanceKey);
		_runewordInstanceStates.erase(instanceKey);
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
		if (rerollRuneword && rolledRunewordRecipe) {
			result.message = "Reforged: " + itemName +
				" [Runeword: " + rolledRunewordRecipe->displayName + "] (Orbs: " + std::to_string(ownedAfter) + ")";
		} else {
			result.message = "Reforged: " + itemName + " (Orbs: " + std::to_string(ownedAfter) + ")";
		}
		return result;
	}

}
