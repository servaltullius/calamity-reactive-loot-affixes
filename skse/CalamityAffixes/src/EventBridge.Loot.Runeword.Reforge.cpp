#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	EventBridge::OperationResult EventBridge::ReforgeSelectedRunewordBaseWithOrb()
	{
		return ReforgeSelectedRunewordBaseImpl(std::nullopt, std::nullopt);
	}

	EventBridge::OperationResult EventBridge::ReforgeSelectedRunewordBaseWithLockedAffix(
		std::uint64_t a_expectedInstanceKey,
		std::uint64_t a_lockedAffixToken)
	{
		if (a_expectedInstanceKey == 0u || a_lockedAffixToken == 0u) {
			return OperationResult{ .success = false, .message = "Locked reforge failed: invalid affix selection." };
		}
		return ReforgeSelectedRunewordBaseImpl(a_expectedInstanceKey, a_lockedAffixToken);
	}

	EventBridge::OperationResult EventBridge::ReforgeSelectedRunewordBaseImpl(
		std::optional<std::uint64_t> a_expectedInstanceKey,
		std::optional<std::uint64_t> a_lockedAffixToken)
	{
		const std::scoped_lock lock(_stateMutex);
		OperationResult result{};
		const bool lockedReforge = a_lockedAffixToken.has_value();
		const std::uint32_t orbCost = lockedReforge ?
			detail::kLockedReforgeOrbCost : detail::kStandardReforgeOrbCost;
		if (!_configLoaded) {
			result.message = "Reforge unavailable: runtime config not loaded.";
			return result;
		}
		if (_runewordState.affixExpansionInProgress) {
			result.message = "Reforge unavailable while affix expansion is in progress.";
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
		if (lockedReforge &&
			(!a_expectedInstanceKey || !detail::IsExpectedLockedReforgeInstance(*a_expectedInstanceKey, instanceKey))) {
			result.message = "Locked reforge failed: selected base changed; choose the protected affix again.";
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
		const auto weaponSubtype =
			detail::ResolveWeaponSubtype(entry->object->As<RE::TESObjectWEAP>());

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
		if (static_cast<std::uint32_t>(ownedBefore) < orbCost) {
			result.message = lockedReforge ?
				"Locked reforge failed: requires " + std::to_string(detail::kLockedReforgeOrbCost) + " Reforge Orbs." :
				"Reforge failed: no Reforge Orb.";
			return result;
		}

		InstanceAffixSlots previousSlots{};
		if (const auto it = _instanceTrackingState.instanceAffixes.find(instanceKey); it != _instanceTrackingState.instanceAffixes.end()) {
			previousSlots = it->second;
		}
		std::optional<InstanceRuntimeState> preservedRunewordRuntimeState;
		if (preservedRunewordToken != 0u) {
			if (const auto* state = FindInstanceRuntimeState(instanceKey, preservedRunewordToken)) {
				preservedRunewordRuntimeState = *state;
			}
		}

		const InstanceAffixSlots previousRegularSlots =
			detail::BuildRegularOnlyAffixSlots(previousSlots, preservedRunewordToken);
		const std::uint8_t targetRegularAffixCount =
			detail::ResolveReforgeTargetAffixCount(previousRegularSlots.count);

		std::size_t lockedAffixIndex = 0u;
		const AffixRuntime* lockedAffix = nullptr;
		std::optional<InstanceRuntimeState> preservedLockedAffixRuntimeState;
		if (lockedReforge) {
			std::uint8_t runewordResultCount = 0u;
			for (std::uint8_t i = 0; i < previousSlots.count; ++i) {
				const auto token = previousSlots.tokens[i];
				if (!_runewordState.recipeIndexByResultAffixToken.contains(token)) {
					continue;
				}
				++runewordResultCount;
				if (token != preservedRunewordToken) {
					result.message = "Locked reforge failed: selected base has conflicting runeword state.";
					return result;
				}
			}
			const auto expectedRunewordResultCount = preservedRunewordToken != 0u ? 1u : 0u;
			if (runewordResultCount != expectedRunewordResultCount) {
				result.message = "Locked reforge failed: selected base has corrupted runeword slots.";
				return result;
			}

			if (!detail::CanLockRegularAffixForReforge(previousRegularSlots.count)) {
				result.message = "Locked reforge failed: at least 2 regular affixes are required.";
				return result;
			}

			for (std::uint8_t i = 0; i < previousRegularSlots.count; ++i) {
				const auto token = previousRegularSlots.tokens[i];
				if (_runewordState.recipeIndexByResultAffixToken.contains(token)) {
					result.message = "Locked reforge failed: runeword effects cannot be locked.";
					return result;
				}
				const auto affixIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(token);
				if (affixIt == _affixRuntimeState.affixRegistry.affixIndexByToken.end() ||
					affixIt->second >= _affixRuntimeState.affixes.size()) {
					result.message = "Locked reforge failed: selected base has an unresolved regular affix.";
					return result;
				}
				const auto& affix = _affixRuntimeState.affixes[affixIt->second];
				if (affix.slot != AffixSlot::kPrefix && affix.slot != AffixSlot::kSuffix) {
					result.message = "Locked reforge failed: selected base has a non-regular affix slot.";
					return result;
				}
			}

			if (!previousRegularSlots.HasToken(*a_lockedAffixToken)) {
				result.message = "Locked reforge failed: selected affix is not on the current base.";
				return result;
			}
			const auto affixIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(*a_lockedAffixToken);
			if (affixIt == _affixRuntimeState.affixRegistry.affixIndexByToken.end() ||
				affixIt->second >= _affixRuntimeState.affixes.size() ||
				_runewordState.recipeIndexByResultAffixToken.contains(*a_lockedAffixToken)) {
				result.message = "Locked reforge failed: selected affix is not a regular affix.";
				return result;
			}
			lockedAffixIndex = affixIt->second;
			lockedAffix = std::addressof(_affixRuntimeState.affixes[lockedAffixIndex]);
			if (lockedAffix->slot != AffixSlot::kPrefix && lockedAffix->slot != AffixSlot::kSuffix) {
				result.message = "Locked reforge failed: selected affix is not a Prefix or Suffix.";
				return result;
			}
			if (const auto* state = FindInstanceRuntimeState(instanceKey, *a_lockedAffixToken)) {
				preservedLockedAffixRuntimeState = *state;
			}
		}

		const auto prefixSharedBagBefore = _lootState.prefixSharedBag;
		const auto prefixWeaponBagBefore = _lootState.prefixWeaponBag;
		const auto prefixArmorBagBefore = _lootState.prefixArmorBag;
		const auto suffixSharedBagBefore = _lootState.suffixSharedBag;
		const auto suffixWeaponBagBefore = _lootState.suffixWeaponBag;
		const auto suffixArmorBagBefore = _lootState.suffixArmorBag;
		auto restoreShuffleBags = [&]() {
			_lootState.prefixSharedBag = prefixSharedBagBefore;
			_lootState.prefixWeaponBag = prefixWeaponBagBefore;
			_lootState.prefixArmorBag = prefixArmorBagBefore;
			_lootState.suffixSharedBag = suffixSharedBagBefore;
			_lootState.suffixWeaponBag = suffixWeaponBagBefore;
			_lootState.suffixArmorBag = suffixArmorBagBefore;
		};

		auto rollRegularAffixSlots = [&](std::uint8_t a_targetCount) -> InstanceAffixSlots {
			InstanceAffixSlots slots{};
			std::vector<std::size_t> chosenIndices;
			chosenIndices.reserve(a_targetCount);
			std::vector<std::string> chosenFamilies;
			chosenFamilies.reserve(a_targetCount);
			if (lockedReforge) {
				chosenIndices.push_back(lockedAffixIndex);
				if (lockedAffix->slot == AffixSlot::kSuffix && !lockedAffix->family.empty()) {
					chosenFamilies.push_back(lockedAffix->family);
				}
			}

			const auto targets = lockedReforge ?
				detail::DetermineLockedReforgeRerollTargets(
					a_targetCount,
					lockedAffix->slot == AffixSlot::kPrefix) :
				detail::DetermineLootPrefixSuffixTargets(a_targetCount);
			const auto prefixTarget = targets.prefixTarget;
			const auto suffixTarget = targets.suffixTarget;

			std::vector<std::size_t> rolledPrefixIndices;
			std::vector<std::size_t> rolledSuffixIndices;
			for (std::uint8_t p = 0; p < prefixTarget; ++p) {
				static constexpr std::uint8_t kMaxRetries = 3;
				bool found = false;
				for (std::uint8_t retry = 0; retry < kMaxRetries; ++retry) {
					const auto idx = RollLootAffixIndex(*lootType, &chosenIndices, /*a_skipChanceCheck=*/true);
					if (!idx) {
						break;
					}
					if (*idx >= _affixRuntimeState.affixes.size() || _affixRuntimeState.affixes[*idx].id.empty()) {
						continue;
					}
					const auto& affix = _affixRuntimeState.affixes[*idx];
					if (affix.slot != AffixSlot::kPrefix ||
						_runewordState.recipeIndexByResultAffixToken.contains(affix.token)) {
						continue;
					}
					chosenIndices.push_back(*idx);
					rolledPrefixIndices.push_back(*idx);
					found = true;
					break;
				}
				if (!found) {
					break;
				}
			}

			for (std::uint8_t s = 0; s < suffixTarget; ++s) {
				static constexpr std::uint8_t kMaxRetries = 3;
				bool found = false;
				for (std::uint8_t retry = 0; retry < kMaxRetries; ++retry) {
					const auto idx = RollSuffixIndex(*lootType, weaponSubtype, &chosenFamilies, &chosenIndices);
					if (!idx) {
						break;
					}
					if (*idx >= _affixRuntimeState.affixes.size() || _affixRuntimeState.affixes[*idx].id.empty()) {
						continue;
					}
					const auto& affix = _affixRuntimeState.affixes[*idx];
					if (affix.slot != AffixSlot::kSuffix ||
						_runewordState.recipeIndexByResultAffixToken.contains(affix.token)) {
						continue;
					}
					if (!affix.family.empty()) {
						chosenFamilies.push_back(affix.family);
					}
					chosenIndices.push_back(*idx);
					rolledSuffixIndices.push_back(*idx);
					found = true;
					break;
				}
				if (!found) {
					break;
				}
			}

			// Canonical regular order is Prefix then Suffix. This keeps the primary
			// display affix stable when the locked affix is a Suffix.
			if (lockedReforge && lockedAffix->slot == AffixSlot::kPrefix) {
				(void)slots.AddToken(*a_lockedAffixToken);
			}
			for (const auto idx : rolledPrefixIndices) {
				(void)slots.AddToken(_affixRuntimeState.affixes[idx].token);
			}
			if (lockedReforge && lockedAffix->slot == AffixSlot::kSuffix) {
				(void)slots.AddToken(*a_lockedAffixToken);
			}
			for (const auto idx : rolledSuffixIndices) {
				(void)slots.AddToken(_affixRuntimeState.affixes[idx].token);
			}

			if (!lockedReforge && slots.count == 0) {
				if (const auto fallback = RollLootAffixIndex(*lootType, nullptr, /*a_skipChanceCheck=*/true);
					fallback && *fallback < _affixRuntimeState.affixes.size() && !_affixRuntimeState.affixes[*fallback].id.empty() &&
					_affixRuntimeState.affixes[*fallback].slot == AffixSlot::kPrefix &&
					!_runewordState.recipeIndexByResultAffixToken.contains(_affixRuntimeState.affixes[*fallback].token)) {
					slots.AddToken(_affixRuntimeState.affixes[*fallback].token);
				}
			}

			return slots;
		};

		auto hasUniqueSuffixFamilies = [&](const InstanceAffixSlots& a_slots) {
			std::unordered_set<std::string_view> families;
			for (std::uint8_t i = 0; i < a_slots.count; ++i) {
				const auto idxIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(a_slots.tokens[i]);
				if (idxIt == _affixRuntimeState.affixRegistry.affixIndexByToken.end() ||
					idxIt->second >= _affixRuntimeState.affixes.size()) {
					return false;
				}
				const auto& affix = _affixRuntimeState.affixes[idxIt->second];
				if (affix.slot == AffixSlot::kSuffix && !affix.family.empty() &&
					!families.insert(affix.family).second) {
					return false;
				}
			}
			return true;
		};

		InstanceAffixSlots newSlots{};
		bool rollAccepted = false;
		std::mt19937 rngBefore{};
		std::mt19937 rngAfter{};
		{
			std::lock_guard<std::mutex> rngLock(_rngMutex);
			rngBefore = _rng;
			static constexpr std::uint8_t kReforgeMaxAttempts = 4;
			for (std::uint8_t attempt = 0; attempt < kReforgeMaxAttempts; ++attempt) {
				InstanceAffixSlots rolledRegular = rollRegularAffixSlots(targetRegularAffixCount);

				if (!detail::HasCompleteRegularAffixReforgeRoll(targetRegularAffixCount, rolledRegular.count) ||
					!detail::HasUniqueAffixTokens(rolledRegular) ||
					!hasUniqueSuffixFamilies(rolledRegular)) {
					continue;
				}
				if (lockedReforge) {
					if (!detail::HasCompleteLockedRegularAffixReforgeRoll(
							targetRegularAffixCount,
							rolledRegular,
							*a_lockedAffixToken) ||
						detail::AreInstanceAffixTokenSetsEqual(previousRegularSlots, rolledRegular)) {
						continue;
					}
				} else if (detail::ShouldRetryRegularAffixReforgeRoll(
						previousRegularSlots,
						rolledRegular,
						attempt,
						kReforgeMaxAttempts)) {
					continue;
				}

				InstanceAffixSlots rolled = rolledRegular;
				if (preservedRunewordToken != 0u &&
					!detail::TryPromotePreservedRunewordPrimary(rolled, preservedRunewordToken)) {
					continue;
				}
				const auto expectedTotalCount = static_cast<std::uint8_t>(
					targetRegularAffixCount + (preservedRunewordToken != 0u ? 1u : 0u));
				if (rolled.count != expectedTotalCount || !detail::HasUniqueAffixTokens(rolled) ||
					(preservedRunewordToken != 0u &&
						(rolled.GetPrimary() != preservedRunewordToken || !rolled.HasToken(preservedRunewordToken)))) {
					continue;
				}

				newSlots = rolled;
				rollAccepted = true;

				if (_loot.debugLog) {
					for (std::uint8_t di = 0; di < rolled.count; ++di) {
					const auto dit = _affixRuntimeState.affixRegistry.affixIndexByToken.find(rolled.tokens[di]);
					const auto& did = (dit != _affixRuntimeState.affixRegistry.affixIndexByToken.end() && dit->second < _affixRuntimeState.affixes.size())
						? _affixRuntimeState.affixes[dit->second].id : std::string{"?"};
					SKSE::log::info("CalamityAffixes: reforge rolled slot[{}] = {} (token={:016X}).",
						di, did, rolled.tokens[di]);
					}
				}
				break;
			}
			if (!rollAccepted) {
				_rng = rngBefore;
			}
			rngAfter = _rng;
		}

		if (!rollAccepted) {
			restoreShuffleBags();
			result.message = lockedReforge ?
				"Locked reforge failed: no different complete affix combination is available." :
				"Reforge failed: no eligible affix in current pool.";
			return result;
		}

		auto restoreRollStateAfterEngineFailure = [&](std::string_view a_reason) {
			restoreShuffleBags();
			std::lock_guard<std::mutex> rngLock(_rngMutex);
			if (_rng == rngAfter) {
				_rng = rngBefore;
				return;
			}
			SKSE::log::warn(
				"CalamityAffixes: reforge {} left RNG advanced because another thread consumed randomness after candidate generation.",
				a_reason);
		};

		player->RemoveItem(
			orb,
			static_cast<std::int32_t>(orbCost),
			RE::ITEM_REMOVE_REASON::kRemove,
			nullptr,
			nullptr);
		const auto ownedAfter = std::max(0, player->GetItemCount(orb));
		if (!detail::DidConsumeExactInventoryCount(
				static_cast<std::uint32_t>(ownedBefore),
				static_cast<std::uint32_t>(ownedAfter),
				orbCost)) {
			const auto refundBefore = ownedAfter;
			auto refundAfter = refundBefore;
			const auto observedConsumed = detail::ResolveObservedInventoryConsumption(
				static_cast<std::uint32_t>(ownedBefore),
				static_cast<std::uint32_t>(ownedAfter));
			if (observedConsumed > 0u) {
				player->AddObjectToContainer(
					orb,
					nullptr,
					static_cast<std::int32_t>(observedConsumed),
					nullptr);
				refundAfter = std::max(0, player->GetItemCount(orb));
			}
			const bool refundConfirmed =
				refundAfter == ownedBefore &&
				(observedConsumed == 0u ||
					detail::DidRestoreExactInventoryCount(
						static_cast<std::uint32_t>(refundBefore),
						static_cast<std::uint32_t>(refundAfter),
						observedConsumed));
			restoreRollStateAfterEngineFailure("currency rollback");
			result.message = refundConfirmed
				? "Reforge failed: Reforge Orb consumption was not confirmed; inventory was restored."
				: "Reforge failed: Reforge Orb consumption was not confirmed; check inventory.";
			SKSE::log::error(
				"CalamityAffixes: reforge aborted because orb inventory delta did not match the exact cost "
				"(cost={}, before={}, after={}, observedConsumed={}, refundAfter={}, refundConfirmed={}, instance={:016X}).",
				orbCost,
				ownedBefore,
				ownedAfter,
				observedConsumed,
				refundAfter,
				refundConfirmed,
				instanceKey);
			return result;
		}

		EraseInstanceRuntimeStates(instanceKey);
		// Standard reforge keeps its legacy final safety net. Locked reforge has a
		// stronger pre-currency invariant and must never repair after charging.
		if (!lockedReforge && preservedRunewordToken != 0u && !newSlots.HasToken(preservedRunewordToken)) {
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

		_instanceTrackingState.instanceAffixes[instanceKey] = newSlots;
		MarkLootEvaluatedInstance(instanceKey);

		for (std::uint8_t i = 0; i < newSlots.count; ++i) {
			EnsureInstanceRuntimeState(instanceKey, newSlots.tokens[i]);
		}
		if (preservedRunewordRuntimeState && preservedRunewordToken != 0u) {
			EnsureInstanceRuntimeState(instanceKey, preservedRunewordToken) =
				*preservedRunewordRuntimeState;
		}
		if (lockedReforge && preservedLockedAffixRuntimeState) {
			EnsureInstanceRuntimeState(instanceKey, *a_lockedAffixToken) =
				*preservedLockedAffixRuntimeState;
		}

		EnsureMultiAffixDisplayName(entry, xList, newSlots);
		RebuildActiveCounts();

		std::string itemName = ResolveInventoryDisplayName(entry, xList);
		if (itemName.empty()) {
			itemName = "Selected base";
		}

		result.success = true;
		if (lockedReforge) {
			std::string lockedName = lockedAffix->displayNameEn;
			if (lockedName.empty()) {
				lockedName = lockedAffix->displayName.empty() ? lockedAffix->id : lockedAffix->displayName;
			}
			result.message = "Locked reforge: " + itemName + " [Kept: " + lockedName +
				"] (Orbs: " + std::to_string(ownedAfter) + ")";
		} else if (preservedRunewordToken != 0u && completedRunewordRecipe) {
			result.message = "Reforged: " + itemName +
				" [Runeword preserved: " + completedRunewordRecipe->displayName + "] (Orbs: " + std::to_string(ownedAfter) + ")";
		} else {
			result.message = "Reforged: " + itemName + " (Orbs: " + std::to_string(ownedAfter) + ")";
		}
		return result;
	}

	EventBridge::OperationResult EventBridge::ResetSelectedRunewordBaseCalamityState()
	{
		const std::scoped_lock lock(_stateMutex);
		OperationResult result{};
		if (!_configLoaded) {
			result.message = "Reset unavailable: runtime config not loaded.";
			return result;
		}
		if (_runewordState.transmuteInProgress) {
			result.message = "Reset unavailable while runeword transmutation is in progress.";
			return result;
		}
		if (_runewordState.affixExpansionInProgress) {
			result.message = "Reset unavailable while affix expansion is in progress.";
			return result;
		}

		SanitizeRunewordState();
		std::uint64_t instanceKey = 0u;
		RE::InventoryEntryData* entry = nullptr;
		RE::ExtraDataList* xList = nullptr;
		std::string baseResolveFailure;
		if (!ResolveSelectedRunewordBaseInstance(instanceKey, entry, xList, &baseResolveFailure, true)) {
			result.message = "Reset failed: " + baseResolveFailure;
			return result;
		}

		std::string itemName = ResolveStoredLootDisplayBaseName(entry, xList);
		if (itemName.empty()) {
			itemName = ResolveInventoryDisplayName(entry, xList);
		}
		if (itemName.empty()) {
			itemName = "Selected base";
		}

		const bool hadAffixes = _instanceTrackingState.instanceAffixes.erase(instanceKey) > 0u;
		const bool hadRuntimeState = std::ranges::any_of(
			_instanceTrackingState.instanceStates,
			[instanceKey](const auto& stateEntry) {
				return stateEntry.first.instanceKey == instanceKey;
			});
		const bool hadRunewordProgress = _runewordState.instanceStates.erase(instanceKey) > 0u;
		const bool hadPreview = FindLootPreviewSlots(instanceKey) != nullptr;

		EraseInstanceRuntimeStates(instanceKey);
		ForgetLootPreviewSlots(instanceKey);
		// Keep the evaluated marker so reset cannot become a free-reroll path if
		// automatic assignment is enabled again in a future version.
		if (!IsLootEvaluatedInstance(instanceKey)) {
			MarkLootEvaluatedInstance(instanceKey);
		}

		bool renamed = false;
		if (auto* text = xList->GetExtraTextDisplayData(); text && !itemName.empty()) {
			const char* currentRaw = xList->GetDisplayName(entry->object);
			const std::string_view currentName = currentRaw ? std::string_view(currentRaw) : std::string_view{};
			if (currentName != itemName) {
				text->SetName(itemName.c_str());
				renamed = true;
			}
		}

		if (_runewordState.selectedBaseKey && *_runewordState.selectedBaseKey == instanceKey) {
			_runewordState.selectedBaseKey.reset();
		}
		RebuildActiveCounts();

		result.success = true;
		const bool changed = hadAffixes || hadRuntimeState || hadRunewordProgress || hadPreview || renamed;
		result.message = changed ?
			"Reset complete: " + itemName + " (Calamity state removed; no material refund)." :
			"Already clear: " + itemName + ".";
		return result;
	}

}
