#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
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
		if (ResolveCompletedRunewordRecipe(instanceKey)) {
			result.message = "Reforge blocked: completed runeword base.";
			return result;
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

}
