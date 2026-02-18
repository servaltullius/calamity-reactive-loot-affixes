#include "CalamityAffixes/EventBridge.h"
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

		_lootEvaluatedRecent.push_back(a_instanceKey);
		if (_lootEvaluatedInstances.insert(a_instanceKey).second) {
			++_lootEvaluatedInsertionsSincePrune;
		}

		if (_lootEvaluatedRecent.size() > (kLootEvaluatedRecentKeep * 4) ||
			_lootEvaluatedInsertionsSincePrune >= kLootEvaluatedPruneEveryInserts) {
			PruneLootEvaluatedInstances();
		}
	}

	void EventBridge::ForgetLootEvaluatedInstance(std::uint64_t a_instanceKey)
	{
		if (a_instanceKey == 0) {
			return;
		}

		_lootEvaluatedInstances.erase(a_instanceKey);
	}

	bool EventBridge::IsLootEvaluatedInstance(std::uint64_t a_instanceKey) const
	{
		if (a_instanceKey == 0) {
			return false;
		}

		return _lootEvaluatedInstances.contains(a_instanceKey);
	}

	void EventBridge::PruneLootEvaluatedInstances()
	{
		if (_lootEvaluatedInstances.empty()) {
			_lootEvaluatedRecent.clear();
			_lootEvaluatedInsertionsSincePrune = 0;
			return;
		}

		std::unordered_set<std::uint64_t> keep;
		keep.reserve(
			_instanceAffixes.size() +
			_runewordInstanceStates.size() +
			kLootEvaluatedRecentKeep + 64);

		// Always keep explicit runtime states.
		for (const auto& [key, _] : _instanceAffixes) {
			keep.insert(key);
		}
		for (const auto& [key, _] : _runewordInstanceStates) {
			keep.insert(key);
		}
		if (_runewordSelectedBaseKey) {
			keep.insert(*_runewordSelectedBaseKey);
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
		for (auto it = _lootEvaluatedRecent.rbegin();
		     it != _lootEvaluatedRecent.rend() && recentKept < kLootEvaluatedRecentKeep;
		     ++it) {
			if (*it == 0) {
				continue;
			}
			keep.insert(*it);
			++recentKept;
		}

		for (auto it = _lootEvaluatedInstances.begin(); it != _lootEvaluatedInstances.end();) {
			if (!keep.contains(*it)) {
				it = _lootEvaluatedInstances.erase(it);
			} else {
				++it;
			}
		}

		std::deque<std::uint64_t> compactedRecent;
		for (const auto key : _lootEvaluatedRecent) {
			if (key != 0 && _lootEvaluatedInstances.contains(key)) {
				compactedRecent.push_back(key);
			}
		}

		const std::size_t maxRecentEntries = kLootEvaluatedRecentKeep * 2;
		while (compactedRecent.size() > maxRecentEntries) {
			compactedRecent.pop_front();
		}

		_lootEvaluatedRecent.swap(compactedRecent);
		_lootEvaluatedInsertionsSincePrune = 0;
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
		if (!_configLoaded || !_runtimeEnabled) {
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

		if (_loot.chancePercent <= 0.0f) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// 1a) Record player-dropped world references so re-picking them up can't "reroll" loot affixes.
		if (a_event->oldContainer == playerId &&
			a_event->newContainer == LootRerollGuard::kWorldContainer) {
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

			if (refHandle != 0) {
				_lootRerollGuard.NotePlayerDrop(
					playerId,
					a_event->oldContainer,
					a_event->newContainer,
					a_event->itemCount,
					refHandle,
					instanceKey);
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
				_playerContainerStash[stashKey] += a_event->itemCount;
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

		// 3b) Skip if this item was stashed in a container by the player.
		//     Tracked by {containerID, baseObj} count — UniqueID-independent.
		if (a_event->baseObj != 0 && a_event->oldContainer != 0) {
			const auto stashKey = std::make_pair(a_event->oldContainer, a_event->baseObj);
			if (auto stashIt = _playerContainerStash.find(stashKey); stashIt != _playerContainerStash.end()) {
				stashIt->second -= a_event->itemCount;
				if (stashIt->second <= 0) {
					_playerContainerStash.erase(stashIt);
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

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESUniqueIDChangeEvent* a_event,
		RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_event->oldBaseID == 0 || a_event->newBaseID == 0) {
			return RE::BSEventNotifyControl::kContinue;
		}
		if (a_event->oldUniqueID == 0 || a_event->newUniqueID == 0) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto oldKey = MakeInstanceKey(a_event->oldBaseID, a_event->oldUniqueID);
		const auto newKey = MakeInstanceKey(a_event->newBaseID, a_event->newUniqueID);
		if (oldKey == newKey) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const bool trackedPreview = FindLootPreviewSlots(oldKey) != nullptr;
		const bool trackedOld =
			_instanceAffixes.contains(oldKey) ||
			IsLootEvaluatedInstance(oldKey) ||
			trackedPreview ||
			_runewordInstanceStates.contains(oldKey) ||
			(_runewordSelectedBaseKey && *_runewordSelectedBaseKey == oldKey);
		if (!trackedOld) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		const auto playerId = player ? player->GetFormID() : 0u;
		const bool ownerIsPlayer = (playerId != 0u && a_event->objectID == playerId);
		const bool playerOwnsEither = ownerIsPlayer || PlayerHasInstanceKey(oldKey) || PlayerHasInstanceKey(newKey);
		const bool previewMenuContextOpen = IsPreviewRemapMenuContextOpen();
		if (!ShouldAllowPreviewUniqueIdRemap(playerOwnsEither, trackedPreview, previewMenuContextOpen)) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: ignored TESUniqueIDChangeEvent not associated with player-tracked item (obj={:08X}, old={:016X}, new={:016X}, trackedPreview={}, previewMenuOpen={}).",
					a_event->objectID,
					oldKey,
					newKey,
					trackedPreview,
					previewMenuContextOpen);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		RemapInstanceKey(oldKey, newKey);
		return RE::BSEventNotifyControl::kContinue;
	}

	void EventBridge::RemapInstanceKey(std::uint64_t a_oldKey, std::uint64_t a_newKey)
	{
		if (a_oldKey == 0 || a_newKey == 0 || a_oldKey == a_newKey) {
			return;
		}

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
			if (auto it = _instanceAffixes.find(a_newKey); it != _instanceAffixes.end()) {
				for (std::uint8_t slot = 0; slot < affixNode.mapped().count; ++slot) {
					it->second.AddToken(affixNode.mapped().tokens[slot]);
				}
			} else {
				affixNode.key() = a_newKey;
				_instanceAffixes.insert(std::move(affixNode));
			}
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

		if (auto rwNode = _runewordInstanceStates.extract(a_oldKey); !rwNode.empty()) {
			if (!_runewordInstanceStates.contains(a_newKey)) {
				rwNode.key() = a_newKey;
				_runewordInstanceStates.insert(std::move(rwNode));
			}
		}

		if (_runewordSelectedBaseKey && *_runewordSelectedBaseKey == a_oldKey) {
			_runewordSelectedBaseKey = a_newKey;
		}
	}


}
