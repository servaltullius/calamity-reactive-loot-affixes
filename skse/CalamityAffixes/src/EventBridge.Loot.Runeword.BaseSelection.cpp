#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
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
}
