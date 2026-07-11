#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/InstanceKeyTransferPolicy.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <unordered_set>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
		[[nodiscard]] bool IsPreviewRemapMenuContextOpen() noexcept
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui) {
				return false;
			}

			constexpr std::array<std::string_view, 3> kPreviewMenus{
				RE::BarterMenu::MENU_NAME,
				RE::ContainerMenu::MENU_NAME,
				RE::GiftMenu::MENU_NAME
			};

			for (const auto menuName : kPreviewMenus) {
				if (!IsPreviewItemSourceMenu(menuName)) {
					continue;
				}

				if (ui->IsMenuOpen(menuName.data())) {
					return true;
				}
			}

			return false;
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
	}

	const std::vector<std::uint64_t>* EventBridge::FindEquippedInstanceKeysForAffixTokenCached(std::uint64_t a_affixToken) const
	{
		if (a_affixToken == 0u || !_equippedTokenCacheReady) {
			return nullptr;
		}

		const auto it = _equippedInstanceKeysByToken.find(a_affixToken);
		if (it == _equippedInstanceKeysByToken.end()) {
			return nullptr;
		}

		return std::addressof(it->second);
	}

	std::vector<std::uint64_t> EventBridge::CollectEquippedInstanceKeysForAffixToken(std::uint64_t a_affixToken) const
	{
		std::vector<std::uint64_t> keys;
		if (a_affixToken == 0u) {
			return keys;
		}

		if (const auto* cached = FindEquippedInstanceKeysForAffixTokenCached(a_affixToken); cached) {
			return *cached;
		}
		if (_equippedTokenCacheReady) {
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
		if (const auto* cached = FindEquippedInstanceKeysForAffixTokenCached(a_affixToken); cached && !cached->empty()) {
			return cached->front();
		}

		auto keys = CollectEquippedInstanceKeysForAffixToken(a_affixToken);
		if (keys.empty()) {
			return std::nullopt;
		}
		return keys.front();
	}

	void EventBridge::MarkLootEvaluatedInstance(std::uint64_t a_instanceKey)
	{
		if (a_instanceKey == 0) {
			return;
		}

		_lootState.evaluatedRecent.push_back(a_instanceKey);
		if (_lootState.evaluatedInstances.insert(a_instanceKey).second) {
			++_lootState.evaluatedInsertionsSincePrune;
		}

		if (_lootState.evaluatedRecent.size() > (kLootEvaluatedRecentKeep * 4) ||
			_lootState.evaluatedInsertionsSincePrune >= kLootEvaluatedPruneEveryInserts) {
			PruneLootEvaluatedInstances();
		}
	}

	void EventBridge::ForgetLootEvaluatedInstance(std::uint64_t a_instanceKey)
	{
		if (a_instanceKey == 0) {
			return;
		}

		_lootState.evaluatedInstances.erase(a_instanceKey);
	}

	bool EventBridge::IsLootEvaluatedInstance(std::uint64_t a_instanceKey) const
	{
		if (a_instanceKey == 0) {
			return false;
		}

		return _lootState.evaluatedInstances.contains(a_instanceKey);
	}

	void EventBridge::PruneLootEvaluatedInstances()
	{
		if (_lootState.evaluatedInstances.empty()) {
			_lootState.evaluatedRecent.clear();
			_lootState.evaluatedInsertionsSincePrune = 0;
			return;
		}

		std::unordered_set<std::uint64_t> keep;
		keep.reserve(
			_instanceAffixes.size() +
			_runewordState.instanceStates.size() +
			kLootEvaluatedRecentKeep + 64);

		// Always keep explicit runtime states.
		for (const auto& [key, _] : _instanceAffixes) {
			keep.insert(key);
		}
		for (const auto& [key, _] : _runewordState.instanceStates) {
			keep.insert(key);
		}
		if (_runewordState.selectedBaseKey) {
			keep.insert(*_runewordState.selectedBaseKey);
		}

		// Keep keys that still exist in player inventory.
		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			if (auto* changes = player->GetInventoryChanges(); changes && changes->entryList) {
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
						keep.insert(MakeInstanceKey(uid->baseID, uid->uniqueID));
					}
				}
			}
		}

		// Keep a recent window to survive short out-of-inventory transitions.
		std::size_t recentKept = 0;
		for (auto it = _lootState.evaluatedRecent.rbegin();
		     it != _lootState.evaluatedRecent.rend() && recentKept < kLootEvaluatedRecentKeep;
		     ++it) {
			if (*it == 0) {
				continue;
			}
			keep.insert(*it);
			++recentKept;
		}

		for (auto it = _lootState.evaluatedInstances.begin(); it != _lootState.evaluatedInstances.end();) {
			if (!keep.contains(*it)) {
				it = _lootState.evaluatedInstances.erase(it);
			} else {
				++it;
			}
		}

		std::deque<std::uint64_t> compactedRecent;
		for (const auto key : _lootState.evaluatedRecent) {
			if (key != 0 && _lootState.evaluatedInstances.contains(key)) {
				compactedRecent.push_back(key);
			}
		}

		const std::size_t maxRecentEntries = kLootEvaluatedRecentKeep * 2;
		while (compactedRecent.size() > maxRecentEntries) {
			compactedRecent.pop_front();
		}

		_lootState.evaluatedRecent.swap(compactedRecent);
		_lootState.evaluatedInsertionsSincePrune = 0;
	}

	float EventBridge::ComputeActiveScrollNoConsumeChancePct() const
	{
		if (_affixes.empty() || _activeCounts.empty()) {
			return 0.0f;
		}

		const auto limit = std::min(_affixes.size(), _activeCounts.size());
		double totalChancePct = 0.0;
		for (std::size_t i = 0; i < limit; ++i) {
			const auto stacks = _activeCounts[i];
			if (stacks == 0) {
				continue;
			}

			const float perAffixChancePct = _affixes[i].scrollNoConsumeChancePct;
			if (perAffixChancePct <= 0.0f) {
				continue;
			}

			totalChancePct += static_cast<double>(perAffixChancePct) * static_cast<double>(stacks);
			if (totalChancePct >= 100.0) {
				return 100.0f;
			}
		}

		return static_cast<float>(std::clamp(totalChancePct, 0.0, 100.0));
	}

	std::int32_t EventBridge::RollScrollNoConsumeRestoreCount(
		std::int32_t a_consumedCount,
		float a_preserveChancePct)
	{
		if (a_consumedCount <= 0 || a_preserveChancePct <= 0.0f) {
			return 0;
		}
		if (a_preserveChancePct >= 100.0f) {
			return a_consumedCount;
		}

		std::int32_t restoreCount = 0;
		std::uniform_real_distribution<float> dist(0.0f, 100.0f);
		for (std::int32_t i = 0; i < a_consumedCount; ++i) {
			if (dist(_rng) < a_preserveChancePct) {
				restoreCount += 1;
			}
		}
		return restoreCount;
	}


	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESContainerChangedEvent* a_event,
		RE::BSTEventSource<RE::TESContainerChangedEvent>*)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!_configLoaded || !_runtimeSettings.enabled) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto playerId = player->GetFormID();
		const auto refHandle = static_cast<LootRerollGuard::RefHandle>(RE::ObjectRefHandle(a_event->reference).native_handle());
		RE::FormID referenceFormID = 0;
		std::uint64_t referenceUniqueKey = 0;
		if (a_event->reference) {
			if (auto ref = a_event->reference.get(); ref) {
				referenceFormID = ref->GetFormID();
				if (const auto* uid = ref->extraList.GetByType<RE::ExtraUniqueID>();
					uid && uid->baseID != 0u && uid->uniqueID != 0u) {
					referenceUniqueKey = MakeInstanceKey(uid->baseID, uid->uniqueID);
				}
			}
		}

		// Scroll preserve path:
		// - event shape: player inventory -> null container
		// - additive chance from equipped affixes (capped at 100)
		if (ShouldHandleScrollConsumePreservation(
				a_event->oldContainer,
				a_event->newContainer,
				a_event->itemCount,
				a_event->baseObj,
				refHandle,
				playerId)) {
			auto* consumed = RE::TESForm::LookupByID<RE::TESForm>(a_event->baseObj);
			auto* consumedScroll = consumed ? consumed->As<RE::ScrollItem>() : nullptr;
			if (consumedScroll) {
				const float preserveChancePct = ComputeActiveScrollNoConsumeChancePct();
				const auto restoreCount = RollScrollNoConsumeRestoreCount(a_event->itemCount, preserveChancePct);
				if (restoreCount > 0) {
					player->AddObjectToContainer(consumedScroll, nullptr, restoreCount, nullptr);
					if (_loot.debugLog) {
						SKSE::log::debug(
							"CalamityAffixes: restored consumed scroll (baseObj={:08X}, consumed={}, restored={}, chancePct={:.2f}).",
							a_event->baseObj,
							a_event->itemCount,
							restoreCount,
							preserveChancePct);
					}
				} else if (_loot.debugLog && preserveChancePct > 0.0f) {
					SKSE::log::debug(
						"CalamityAffixes: scroll preserve roll failed (baseObj={:08X}, consumed={}, chancePct={:.2f}).",
						a_event->baseObj,
						a_event->itemCount,
						preserveChancePct);
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		// 1a) Record player-dropped world references so re-picking them up can't "reroll" loot affixes.
		if (a_event->oldContainer == playerId &&
			a_event->newContainer == LootRerollGuard::kWorldContainer) {
			const auto detachedWorldKey = MakeDetachedWorldInstanceKey(referenceFormID);
			const auto playerEventKey = a_event->uniqueID != 0u ?
				MakeInstanceKey(playerId, a_event->uniqueID) :
				std::uint64_t{ 0 };
			const auto legacyEventKey = a_event->baseObj != 0u && a_event->uniqueID != 0u ?
				MakeInstanceKey(a_event->baseObj, a_event->uniqueID) :
				std::uint64_t{ 0 };
			const std::array dropSourceCandidates{
				playerEventKey,
				referenceUniqueKey,
				legacyEventKey
			};

			std::uint64_t dropSourceKey = 0;
			bool ambiguousMaterialSources = false;
			for (const auto candidateKey : dropSourceCandidates) {
				if (candidateKey == 0u ||
					candidateKey == detachedWorldKey ||
					!HasMaterialInstanceState(candidateKey)) {
					continue;
				}
				if (dropSourceKey == 0u) {
					dropSourceKey = candidateKey;
				} else if (dropSourceKey != candidateKey) {
					ambiguousMaterialSources = true;
				}
			}

			if (!ambiguousMaterialSources && dropSourceKey == 0u) {
				for (const auto candidateKey : dropSourceCandidates) {
					if (candidateKey != 0u &&
						candidateKey != detachedWorldKey &&
						IsInstanceKeyTracked(candidateKey)) {
						dropSourceKey = candidateKey;
						break;
					}
				}
			}

			if (ambiguousMaterialSources) {
				SKSE::log::warn(
					"CalamityAffixes: blocked ambiguous player-to-world state transfer; candidate states were discarded (baseObj={:08X}, ref={:08X}, uniqueID={}, detachedKey={:016X}).",
					a_event->baseObj,
					referenceFormID,
					a_event->uniqueID,
					detachedWorldKey);
				for (const auto candidateKey : dropSourceCandidates) {
					DiscardInstanceKeyState(candidateKey);
				}
				DiscardInstanceKeyState(detachedWorldKey);
				dropSourceKey = 0u;
			} else if (dropSourceKey != 0u && detachedWorldKey == 0u) {
				SKSE::log::warn(
					"CalamityAffixes: discarded player-to-world state because the dropped reference had no stable FormID (baseObj={:08X}, uniqueID={}, sourceKey={:016X}).",
					a_event->baseObj,
					a_event->uniqueID,
					dropSourceKey);
				DiscardInstanceKeyState(dropSourceKey);
				dropSourceKey = 0u;
			} else if (dropSourceKey != 0u && HasMaterialInstanceState(detachedWorldKey)) {
				if (HasMaterialInstanceState(dropSourceKey)) {
					SKSE::log::warn(
						"CalamityAffixes: blocked occupied player-to-world state transfer; both ambiguous states were discarded (baseObj={:08X}, ref={:08X}, sourceKey={:016X}, detachedKey={:016X}).",
						a_event->baseObj,
						referenceFormID,
						dropSourceKey,
						detachedWorldKey);
					DiscardInstanceKeyState(dropSourceKey);
					DiscardInstanceKeyState(detachedWorldKey);
				}
			} else if (dropSourceKey != 0u) {
				RemapInstanceKey(dropSourceKey, detachedWorldKey);
			}
			if (dropSourceKey == 0u) {
				if (auto* task = SKSE::GetTaskInterface()) {
					task->AddTask([]() {
						if (auto* bridge = EventBridge::GetSingleton(); bridge) {
							const std::scoped_lock lock(bridge->_stateMutex);
							const bool prunedOrphanedDropKeys = bridge->PruneOrphanedPlayerInstanceKeys();
							if (prunedOrphanedDropKeys) {
								bridge->RebuildActiveCounts();
							}
						}
					});
				} else {
					const bool prunedOrphanedDropKeys = PruneOrphanedPlayerInstanceKeys();
					if (prunedOrphanedDropKeys) {
						RebuildActiveCounts();
					}
				}
			}
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: reconciled player-to-world instance ownership (baseObj={:08X}, ref={:08X}, eventUid={}, referenceKey={:016X}, sourceKey={:016X}, detachedKey={:016X}).",
					a_event->baseObj,
					referenceFormID,
					a_event->uniqueID,
					referenceUniqueKey,
					dropSourceKey,
					detachedWorldKey);
			}

			if (refHandle != 0) {
				_lootState.rerollGuard.NotePlayerDrop(
					playerId,
					a_event->oldContainer,
					a_event->newContainer,
					a_event->itemCount,
					refHandle,
					detachedWorldKey);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		// 1b) Record items the player puts into any container (chest, barrel, etc.)
		//     so re-taking them can't reroll. Tracked by {containerID, baseObj} + count
		//     because UniqueID is unreliable across container transfers.
		if (a_event->oldContainer == playerId &&
			a_event->newContainer != 0 &&
			a_event->newContainer != playerId) {
			if (a_event->baseObj != 0) {
				const auto stashKey = std::make_pair(a_event->newContainer, a_event->baseObj);
				_lootState.playerContainerStash[stashKey] += a_event->itemCount;
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		// 2) Only roll when items move INTO the player.
		if (a_event->itemCount <= 0) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_event->newContainer != playerId) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// 3) Reattach state only when this is the exact world reference the player dropped.
		std::optional<std::uint64_t> guardedDropKey;
		if (refHandle != 0u) {
			guardedDropKey = _lootState.rerollGuard.ConsumePlayerDropPickupInstanceKey(
				playerId,
				a_event->oldContainer,
				a_event->newContainer,
				a_event->itemCount,
				refHandle);
		}

		const auto referenceDetachedWorldKey = MakeDetachedWorldInstanceKey(referenceFormID);
		if (guardedDropKey &&
			!IsDetachedWorldReferenceMatch(*guardedDropKey, referenceDetachedWorldKey)) {
			SKSE::log::warn(
				"CalamityAffixes: rejected recycled drop-reference handle during pickup (baseObj={:08X}, ref={:08X}, guardedKey={:016X}, referenceKey={:016X}).",
				a_event->baseObj,
				referenceFormID,
				*guardedDropKey,
				referenceDetachedWorldKey);
			guardedDropKey.reset();
		}
		std::uint64_t detachedWorldKey = guardedDropKey.value_or(0u);
		if ((!IsInstanceKeyTracked(detachedWorldKey)) &&
			IsInstanceKeyTracked(referenceDetachedWorldKey)) {
			detachedWorldKey = referenceDetachedWorldKey;
		}
		const bool trackedDetachedWorldState = IsInstanceKeyTracked(detachedWorldKey);
		if (guardedDropKey.has_value() || trackedDetachedWorldState) {
			const auto pickupKey = a_event->uniqueID != 0u ?
				MakeInstanceKey(playerId, a_event->uniqueID) :
				((referenceUniqueKey >> 16u) == playerId ? referenceUniqueKey : std::uint64_t{ 0 });
			if (trackedDetachedWorldState && pickupKey != 0u) {
				if (HasMaterialInstanceState(pickupKey)) {
					SKSE::log::warn(
						"CalamityAffixes: blocked occupied world-to-player state transfer; both ambiguous states were discarded (baseObj={:08X}, ref={:08X}, detachedKey={:016X}, pickupKey={:016X}).",
						a_event->baseObj,
						referenceFormID,
						detachedWorldKey,
						pickupKey);
					DiscardInstanceKeyState(detachedWorldKey);
					DiscardInstanceKeyState(pickupKey);
				} else {
					RemapInstanceKey(detachedWorldKey, pickupKey);
				}
			} else if (trackedDetachedWorldState) {
				SKSE::log::warn(
					"CalamityAffixes: retained detached world state because pickup had no stable player UID (baseObj={:08X}, ref={:08X}, detachedKey={:016X}).",
					a_event->baseObj,
					referenceFormID,
					detachedWorldKey);
			}
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: skipping loot roll (player dropped + re-picked) (baseObj={:08X}, uniqueID={}, detachedKey={:016X}, pickupKey={:016X}).",
					a_event->baseObj,
					a_event->uniqueID,
					detachedWorldKey,
					pickupKey);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		// 3b) Skip if this item was stashed in a container by the player.
		//     Tracked by {containerID, baseObj} count — UniqueID-independent.
		if (a_event->baseObj != 0 && a_event->oldContainer != 0) {
			const auto stashKey = std::make_pair(a_event->oldContainer, a_event->baseObj);
			if (auto stashIt = _lootState.playerContainerStash.find(stashKey);
				stashIt != _lootState.playerContainerStash.end()) {
				stashIt->second -= a_event->itemCount;
				if (stashIt->second <= 0) {
					_lootState.playerContainerStash.erase(stashIt);
				}
				if (_loot.debugLog) {
					SKSE::log::debug(
						"CalamityAffixes: skipping loot roll (player stashed + retrieved) (baseObj={:08X}, container={:08X}).",
						a_event->baseObj,
						a_event->oldContainer);
				}
				return RE::BSEventNotifyControl::kContinue;
			}
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

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: loot event queued (baseObj={:08X}, count={}, uniqueID={}).",
				baseObj,
				count,
				uid);
		}

		ProcessLootAcquired(baseObj, count, uid, a_event->oldContainer, allowRunewordFragmentRoll);
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESUniqueIDChangeEvent* a_event,
		RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto oldOwnerID = a_event->oldBaseID;
		const auto newOwnerID = a_event->newBaseID;
		const auto itemFormID = a_event->objectID;
		if (oldOwnerID == 0 || newOwnerID == 0 || itemFormID == 0) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: ignored incomplete TESUniqueIDChangeEvent owner boundary (oldOwner={:08X}, newOwner={:08X}, item={:08X}, oldUid={}, newUid={}).",
					oldOwnerID,
					newOwnerID,
					itemFormID,
					a_event->oldUniqueID,
					a_event->newUniqueID);
			}
			return RE::BSEventNotifyControl::kContinue;
		}
		if (a_event->oldUniqueID == 0 || a_event->newUniqueID == 0) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: ignored incomplete TESUniqueIDChangeEvent UID boundary (oldOwner={:08X}, newOwner={:08X}, item={:08X}, oldUid={}, newUid={}).",
					oldOwnerID,
					newOwnerID,
					itemFormID,
					a_event->oldUniqueID,
					a_event->newUniqueID);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto oldKey = MakeInstanceKey(oldOwnerID, a_event->oldUniqueID);
		const auto newKey = MakeInstanceKey(newOwnerID, a_event->newUniqueID);
		const auto legacyItemKey = MakeInstanceKey(itemFormID, a_event->oldUniqueID);
		const bool trackedOld = IsInstanceKeyTracked(oldKey);
		const bool trackedLegacyItemKey = legacyItemKey != oldKey && IsInstanceKeyTracked(legacyItemKey);
		const bool materialOld = HasMaterialInstanceState(oldKey);
		const bool materialLegacyItemKey =
			legacyItemKey != oldKey && HasMaterialInstanceState(legacyItemKey);
		const bool trackedNew = IsInstanceKeyTracked(newKey);
		const bool materialNew = HasMaterialInstanceState(newKey);
		const bool trackedPreview =
			FindLootPreviewSlots(oldKey) != nullptr ||
			(trackedLegacyItemKey && FindLootPreviewSlots(legacyItemKey) != nullptr);

		auto* player = RE::PlayerCharacter::GetSingleton();
		const auto playerId = player ? player->GetFormID() : 0u;
		const bool playerHasOldKey =
			PlayerHasInstanceKey(oldKey) ||
			(trackedLegacyItemKey && PlayerHasInstanceKey(legacyItemKey));
		const bool playerHasNewKey = PlayerHasInstanceKey(newKey);
		const bool previewMenuContextOpen = IsPreviewRemapMenuContextOpen();
		const bool previewRemapEvidence = ShouldAllowPreviewUniqueIdRemap(
			false,
			trackedPreview,
			previewMenuContextOpen);
		if (materialOld && materialLegacyItemKey) {
			SKSE::log::warn(
				"CalamityAffixes: blocked ambiguous canonical+legacy source collision; all involved states were discarded (oldOwner={:08X}, newOwner={:08X}, item={:08X}, oldUid={}, newUid={}, oldKey={:016X}, legacyKey={:016X}, newKey={:016X}).",
				oldOwnerID,
				newOwnerID,
				itemFormID,
				a_event->oldUniqueID,
				a_event->newUniqueID,
				oldKey,
				legacyItemKey,
				newKey);
			DiscardInstanceKeyState(oldKey);
			DiscardInstanceKeyState(legacyItemKey);
			if (materialNew) {
				DiscardInstanceKeyState(newKey);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		const bool policyOldTracked = materialOld || (!materialLegacyItemKey && trackedOld);
		const bool policyLegacyTracked =
			materialLegacyItemKey || (!policyOldTracked && trackedLegacyItemKey);
		const auto transferPlan = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
			.playerID = playerId,
			.oldOwnerID = oldOwnerID,
			.newOwnerID = newOwnerID,
			.itemFormID = itemFormID,
			.oldUniqueID = a_event->oldUniqueID,
			.newUniqueID = a_event->newUniqueID,
			.playerHasOldKey = playerHasOldKey || previewRemapEvidence,
			.playerHasNewKey = playerHasNewKey,
			.oldKeyTracked = policyOldTracked,
			.legacyItemKeyTracked = policyLegacyTracked,
			.newKeyTracked = materialNew
		});
		if (transferPlan.decision == InstanceKeyTransferDecision::kIgnore) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: ignored TESUniqueIDChangeEvent (oldOwner={:08X}, newOwner={:08X}, item={:08X}, oldUid={}, newUid={}, oldKey={:016X}, legacyKey={:016X}, newKey={:016X}, trackedOld={}, trackedLegacy={}, trackedNew={}, playerHasOld={}, playerHasNew={}, trackedPreview={}, previewMenuOpen={}).",
					oldOwnerID,
					newOwnerID,
					itemFormID,
					a_event->oldUniqueID,
					a_event->newUniqueID,
					oldKey,
					legacyItemKey,
					newKey,
					trackedOld,
					trackedLegacyItemKey,
					trackedNew,
					playerHasOldKey,
					playerHasNewKey,
					trackedPreview,
					previewMenuContextOpen);
			}
			return RE::BSEventNotifyControl::kContinue;
		}
		if (transferPlan.decision == InstanceKeyTransferDecision::kConflict) {
			SKSE::log::warn(
				"CalamityAffixes: blocked instance-key remap collision; both ambiguous states were discarded instead of merged (oldOwner={:08X}, newOwner={:08X}, item={:08X}, oldUid={}, newUid={}, oldKey={:016X}, newKey={:016X}).",
				oldOwnerID,
				newOwnerID,
				itemFormID,
				a_event->oldUniqueID,
				a_event->newUniqueID,
				transferPlan.oldKey,
				transferPlan.newKey);
			DiscardInstanceKeyState(transferPlan.oldKey);
			DiscardInstanceKeyState(transferPlan.newKey);
			return RE::BSEventNotifyControl::kContinue;
		}

		RemapInstanceKey(transferPlan.oldKey, transferPlan.newKey);
		return RE::BSEventNotifyControl::kContinue;
	}

	bool EventBridge::HasMaterialInstanceState(std::uint64_t a_instanceKey) const
	{
		if (a_instanceKey == 0u) {
			return false;
		}

		const bool hasRuntimeState = std::ranges::any_of(
			_instanceStates,
			[a_instanceKey](const auto& entry) {
				return entry.first.instanceKey == a_instanceKey;
			});
		return _instanceAffixes.contains(a_instanceKey) ||
		       hasRuntimeState ||
		       _runewordState.instanceStates.contains(a_instanceKey) ||
		       (_runewordState.selectedBaseKey && *_runewordState.selectedBaseKey == a_instanceKey);
	}

	bool EventBridge::IsInstanceKeyTracked(std::uint64_t a_instanceKey) const
	{
		return HasMaterialInstanceState(a_instanceKey) ||
		       IsLootEvaluatedInstance(a_instanceKey) ||
		       FindLootPreviewSlots(a_instanceKey) != nullptr;
	}

	void EventBridge::DiscardInstanceKeyState(std::uint64_t a_instanceKey)
	{
		if (a_instanceKey == 0u) {
			return;
		}

		_instanceAffixes.erase(a_instanceKey);
		ForgetLootEvaluatedInstance(a_instanceKey);
		ForgetLootPreviewSlots(a_instanceKey);
		EraseInstanceRuntimeStates(a_instanceKey);
		_runewordState.instanceStates.erase(a_instanceKey);
		if (_runewordState.selectedBaseKey && *_runewordState.selectedBaseKey == a_instanceKey) {
			_runewordState.selectedBaseKey.reset();
		}
		_equippedTokenCacheReady = false;
	}

	void EventBridge::RemapInstanceKey(std::uint64_t a_oldKey, std::uint64_t a_newKey)
	{
		if (a_oldKey == 0 || a_newKey == 0 || a_oldKey == a_newKey) {
			return;
		}
		if (HasMaterialInstanceState(a_newKey)) {
			SKSE::log::warn(
				"CalamityAffixes: refused occupied instance-key remap; destination state was preserved (old={:016X}, new={:016X}).",
				a_oldKey,
				a_newKey);
			return;
		}
		// Evaluation/preview markers do not own an item. Clear them so a material
		// source can move into the canonical destination without data loss.
		ForgetLootEvaluatedInstance(a_newKey);
		ForgetLootPreviewSlots(a_newKey);

		if (IsLootEvaluatedInstance(a_oldKey)) {
			ForgetLootEvaluatedInstance(a_oldKey);
			MarkLootEvaluatedInstance(a_newKey);
		}

		if (const auto* previewSlots = FindLootPreviewSlots(a_oldKey)) {
			const auto preview = *previewSlots;
			ForgetLootPreviewSlots(a_oldKey);
			RememberLootPreviewSlots(a_newKey, preview);
		}

		if (auto affixNode = _instanceAffixes.extract(a_oldKey); !affixNode.empty()) {
			affixNode.key() = a_newKey;
			_instanceAffixes.insert(std::move(affixNode));
		}

		std::vector<std::pair<InstanceStateKey, InstanceRuntimeState>> remappedStates;
		for (auto it = _instanceStates.begin(); it != _instanceStates.end();) {
			if (it->first.instanceKey != a_oldKey) {
				++it;
				continue;
			}

			remappedStates.emplace_back(it->first, it->second);
			it = _instanceStates.erase(it);
		}

		for (const auto& [oldStateKey, state] : remappedStates) {
			const auto newStateKey = MakeInstanceStateKey(a_newKey, oldStateKey.affixToken);
			_instanceStates.emplace(newStateKey, state);
		}

		if (auto rwNode = _runewordState.instanceStates.extract(a_oldKey); !rwNode.empty()) {
			if (!_runewordState.instanceStates.contains(a_newKey)) {
				rwNode.key() = a_newKey;
				_runewordState.instanceStates.insert(std::move(rwNode));
			}
		}

		if (_runewordState.selectedBaseKey && *_runewordState.selectedBaseKey == a_oldKey) {
			_runewordState.selectedBaseKey = a_newKey;
		}
		_equippedTokenCacheReady = false;
	}


}
