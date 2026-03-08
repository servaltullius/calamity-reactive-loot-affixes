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
		const std::scoped_lock lock(_stateMutex);
		OperationResult result{};
		if (!_configLoaded) {
			result.message = "Reforge unavailable: runtime config not loaded.";
			return result;
		}

		SanitizeRunewordState();
		std::uint64_t instanceKey = 0u;
		RE::InventoryEntryData* entry = nullptr;
		RE::ExtraDataList* xList = nullptr;
		std::string baseResolveFailure;
		if (!ResolveSelectedRunewordBaseInstance(instanceKey, entry, xList, &baseResolveFailure, true)) {
			result.message = "Reforge failed: " + baseResolveFailure;
			return result;
		}
		const auto* completedRunewordRecipe = ResolveCompletedRunewordRecipe(instanceKey);
		const std::uint64_t preservedRunewordToken =
			completedRunewordRecipe ? completedRunewordRecipe->resultAffixToken : 0u;

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

		const InstanceAffixSlots previousRegularSlots =
			detail::BuildRegularOnlyAffixSlots(previousSlots, preservedRunewordToken);

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
					// Skip the preserved runeword affix — it occupies its own slot.
					if (preservedRunewordToken != 0u && _affixes[*idx].token == preservedRunewordToken) {
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
					fallback && *fallback < _affixes.size() && !_affixes[*fallback].id.empty() &&
					(preservedRunewordToken == 0u || _affixes[*fallback].token != preservedRunewordToken)) {
					slots.AddToken(_affixes[*fallback].token);
				}
			}

			return slots;
		};

		InstanceAffixSlots newSlots{};
		static constexpr std::uint8_t kReforgeMaxAttempts = 4;
		for (std::uint8_t attempt = 0; attempt < kReforgeMaxAttempts; ++attempt) {
			// Re-roll affix count fresh (same 70/22/8% distribution as new loot drops).
			const std::uint8_t targetCount = std::max<std::uint8_t>(1u, RollAffixCount());
			InstanceAffixSlots rolled = rollRegularAffixSlots(targetCount);

			if (rolled.count == 0) {
				continue;
			}
			if (detail::ShouldRetryRegularAffixReforgeRoll(previousRegularSlots, rolled, attempt, kReforgeMaxAttempts)) {
				continue;
			}

			// Re-insert preserved runeword token as primary (slot 0).
			if (preservedRunewordToken != 0u) {
				if (!detail::TryPromotePreservedRunewordPrimary(rolled, preservedRunewordToken)) {
					SKSE::log::error(
						"CalamityAffixes: reforge runeword preserve failed — PromoteTokenToPrimary returned false "
						"(instance={:016X}, runewordToken={:016X}, rolledCount={}).",
						instanceKey,
						preservedRunewordToken,
						rolled.count);
					detail::ForcePreserveRunewordPrimary(rolled, preservedRunewordToken);
				}
			}

			newSlots = rolled;

			if (_loot.debugLog) {
				for (std::uint8_t di = 0; di < rolled.count; ++di) {
					const auto dit = _affixRegistry.affixIndexByToken.find(rolled.tokens[di]);
					const auto& did = (dit != _affixRegistry.affixIndexByToken.end() && dit->second < _affixes.size())
						? _affixes[dit->second].id : std::string{"?"};
					SKSE::log::info("CalamityAffixes: reforge rolled slot[{}] = {} (token={:016X}).",
						di, did, rolled.tokens[di]);
				}
			}
			break;
		}

		if (newSlots.count == 0) {
			result.message = "Reforge failed: no eligible affix in current pool.";
			return result;
		}

		player->RemoveItem(orb, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

		EraseInstanceRuntimeStates(instanceKey);
		if (preservedRunewordToken == 0u) {
			_runewordState.instanceStates.erase(instanceKey);
		}
		// Final safety: verify runeword token survived the roll pipeline.
		if (preservedRunewordToken != 0u && !newSlots.HasToken(preservedRunewordToken)) {
			SKSE::log::error(
				"CalamityAffixes: reforge runeword token {:016X} missing from newSlots after roll — forcing re-insertion (instance={:016X}, count={}).",
				preservedRunewordToken,
				instanceKey,
				newSlots.count);
			if (!detail::TryPromotePreservedRunewordPrimary(newSlots, preservedRunewordToken)) {
				if (newSlots.count > 0) {
					newSlots.tokens[newSlots.count - 1u] = 0u;
					--newSlots.count;
				}
				detail::ForcePreserveRunewordPrimary(newSlots, preservedRunewordToken);
			}
		}

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
		if (preservedRunewordToken != 0u && completedRunewordRecipe) {
			result.message = "Reforged: " + itemName +
				" [Runeword preserved: " + completedRunewordRecipe->displayName + "] (Orbs: " + std::to_string(ownedAfter) + ")";
		} else {
			result.message = "Reforged: " + itemName + " (Orbs: " + std::to_string(ownedAfter) + ")";
		}
		return result;
	}

}
