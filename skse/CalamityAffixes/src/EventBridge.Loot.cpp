#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
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
	}

	std::vector<std::uint64_t> EventBridge::CollectEquippedInstanceKeysForAffixToken(std::uint64_t a_affixToken) const
	{
		std::vector<std::uint64_t> keys;
		if (a_affixToken == 0u) {
			return keys;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return keys;
		}

		auto* changes = player->GetInventoryChanges();
		if (!changes || !changes->entryList) {
			return keys;
		}

		for (auto* entry : *changes->entryList) {
			if (!entry || !entry->extraLists) {
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
				if (!uid) {
					continue;
				}

				const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
				const auto it = _instanceAffixes.find(key);
				if (it == _instanceAffixes.end() || !it->second.HasToken(a_affixToken)) {
					continue;
				}

				keys.push_back(key);
			}
		}

		std::sort(keys.begin(), keys.end());
		keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
		return keys;
	}

		std::optional<std::uint64_t> EventBridge::ResolvePrimaryEquippedInstanceKey(std::uint64_t a_affixToken) const
		{
			auto keys = CollectEquippedInstanceKeysForAffixToken(a_affixToken);
			if (keys.empty()) {
				return std::nullopt;
			}
			return keys.front();
		}


	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESContainerChangedEvent* a_event,
		RE::BSTEventSource<RE::TESContainerChangedEvent>*)
	{
		if (!_configLoaded || !_runtimeEnabled) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (_loot.chancePercent <= 0.0f) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto playerId = player->GetFormID();
		const auto refHandle = static_cast<LootRerollGuard::RefHandle>(RE::ObjectRefHandle(a_event->reference).native_handle());

		// 1) Record player-dropped world references so re-picking them up can't "reroll" loot affixes.
		if (a_event->oldContainer == playerId &&
			a_event->newContainer == LootRerollGuard::kWorldContainer &&
			refHandle != 0) {
			std::uint64_t instanceKey = 0;
			if (a_event->reference) {
				if (auto ref = a_event->reference.get(); ref) {
					if (const auto* uid = ref->extraList.GetByType<RE::ExtraUniqueID>(); uid && uid->baseID != 0 && uid->uniqueID != 0) {
						instanceKey = MakeInstanceKey(uid->baseID, uid->uniqueID);
					}
				}
			}
			if (instanceKey == 0 && a_event->baseObj != 0 && a_event->uniqueID != 0) {
				instanceKey = MakeInstanceKey(a_event->baseObj, a_event->uniqueID);
			}

			_lootRerollGuard.NotePlayerDrop(
				playerId,
				a_event->oldContainer,
				a_event->newContainer,
				a_event->itemCount,
				refHandle,
				instanceKey);
			return RE::BSEventNotifyControl::kContinue;
		}

		// 2) Only roll when items move INTO the player.
		if (a_event->itemCount <= 0) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_event->newContainer != playerId) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// 3) Skip if this pickup is the player re-acquiring their own dropped reference.
		if (refHandle != 0 &&
			_lootRerollGuard.ConsumeIfPlayerDropPickup(
				playerId,
				a_event->oldContainer,
				a_event->newContainer,
				a_event->itemCount,
				refHandle)) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: skipping loot roll (player dropped + re-picked) (baseObj={:08X}, uniqueID={}).",
					a_event->baseObj,
					a_event->uniqueID);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

			const auto baseObj = a_event->baseObj;
			const auto count = a_event->itemCount;
			const auto uid = a_event->uniqueID;
			const auto allowRunewordFragmentRoll = IsLootSourceCorpseChestOrWorld(a_event->oldContainer, playerId);

			if (!allowRunewordFragmentRoll) {
				if (_loot.debugLog) {
					SKSE::log::debug(
						"CalamityAffixes: runeword fragment roll skipped (source filter) (oldContainer={:08X}, baseObj={:08X}, uniqueID={}).",
						a_event->oldContainer,
						baseObj,
						uid);
				}
			}

			SKSE::log::debug(
				"CalamityAffixes: loot event queued (baseObj={:08X}, count={}, uniqueID={}).",
			baseObj,
			count,
			uid);

			if (auto* task = SKSE::GetTaskInterface()) {
				task->AddTask([baseObj, count, uid, allowRunewordFragmentRoll]() {
					EventBridge::GetSingleton()->ProcessLootAcquired(baseObj, count, uid, allowRunewordFragmentRoll);
				});
			} else {
				ProcessLootAcquired(baseObj, count, uid, allowRunewordFragmentRoll);
			}
			return RE::BSEventNotifyControl::kContinue;
		}


}
