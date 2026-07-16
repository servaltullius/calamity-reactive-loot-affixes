#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/SuffixFamilySelection.h"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>


namespace CalamityAffixes
{
	void EventBridge::DeactivateRuntimeState()
	{
		ResetActiveCountsStateForRebuild();

		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			for (const auto& affix : _affixes) {
				if (affix.passiveSpell) {
					_appliedPassiveSpells.insert(affix.passiveSpell);
				}
			}
			ApplyDesiredPassiveSpells(player, {});
		} else {
			_appliedPassiveSpells.clear();
		}

		ClearTrapRuntimeState();
		_combatState.ResetTransientState();
		Hooks::ClearRuntimeState();
		_equipResync.nextAtMs = 0u;
		for (auto& affix : _affixes) {
			affix.nextAllowed = {};
		}
	}

	void EventBridge::ResetActiveCountsStateForRebuild()
	{
		_activeCounts.assign(_affixes.size(), 0);
		_lootState.activeSlotPenalty.assign(_affixes.size(), 0.0f);
		_activeCritDamageBonusPct = 0.0f;
		RebuildActiveTriggerIndexCaches();
		_equippedInstanceKeysByToken.clear();
		_equippedTokenCacheReady = false;
		_equippedInstanceKeysByToken.reserve(_affixRegistry.affixIndexByToken.size());
	}

	void EventBridge::RefreshInventoryInstanceActiveState(
		RE::InventoryEntryData* a_entry,
		RE::ExtraDataList* a_xList,
		std::unordered_set<RE::SpellItem*>& a_desiredPassives)
	{
		if (!a_entry || !a_xList) {
			return;
		}

		auto* uid = a_xList->GetByType<RE::ExtraUniqueID>();
		if (!uid) {
			return;
		}

		const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
		const auto it = _instanceAffixes.find(key);
		if (it == _instanceAffixes.end()) {
			return;
		}

		// Lazy cleanup: strip stale mappings for non-eligible items from older versions/saves.
		if (!IsLootObjectEligibleForAffixes(a_entry->object)) {
			if (_loot.cleanupInvalidLegacyAffixes) {
				CleanupInvalidLootInstance(a_entry, a_xList, key, "RebuildActiveCounts.ineligible");
			}
			return;
		}

		const auto& slots = it->second;
		for (std::uint8_t slot = 0; slot < slots.count; ++slot) {
			if (slots.tokens[slot] != 0u) {
				EnsureInstanceRuntimeState(key, slots.tokens[slot]);
			}
		}

		// Use primary affix for display name.
		if (slots.count > 0) {
			if (const auto idxIt = _affixRegistry.affixIndexByToken.find(slots.tokens[0]); idxIt != _affixRegistry.affixIndexByToken.end() && idxIt->second < _affixes.size()) {
				EnsureMultiAffixDisplayName(a_entry, a_xList, slots);
			}
		}

		const bool worn = a_xList->HasType<RE::ExtraWorn>() || a_xList->HasType<RE::ExtraWornLeft>();
		if (!worn) {
			return;
		}

		AccumulateEquippedAffixState(key, slots, a_desiredPassives);
	}

	void EventBridge::AccumulateEquippedAffixState(
		std::uint64_t a_instanceKey,
		const InstanceAffixSlots& a_slots,
		std::unordered_set<RE::SpellItem*>& a_desiredPassives)
	{
		if (a_slots.count == 0) {
			return;
		}

		const auto penalty = kMultiAffixProcPenalty[std::min<std::uint8_t>(a_slots.count, static_cast<std::uint8_t>(kMaxAffixesPerItem)) - 1];

		for (std::uint8_t slot = 0; slot < a_slots.count; ++slot) {
			const auto token = a_slots.tokens[slot];
			if (token != 0u) {
				_equippedInstanceKeysByToken[token].push_back(a_instanceKey);
			}

			const auto idxIt = _affixRegistry.affixIndexByToken.find(token);
			if (idxIt == _affixRegistry.affixIndexByToken.end()) {
				SKSE::log::warn(
					"CalamityAffixes: RebuildActiveCounts — worn token {:016X} not found in _affixRegistry.affixIndexByToken (instance={:016X}, slot={}).",
					token,
					a_instanceKey,
					slot);
				continue;
			}
			const auto affixIdx = idxIt->second;

			if (affixIdx < _activeCounts.size()) {
				_activeCounts[affixIdx] += 1;
			}

			// Family-less passives (including runewords) keep their existing behavior.
			// Tiered suffix families are selected once, after all worn items are counted.
			if (!_runtimeSettings.disablePassiveSuffixSpells &&
				affixIdx < _affixes.size() &&
				_affixes[affixIdx].passiveSpell) {
				const auto& affix = _affixes[affixIdx];
				const bool deferTieredSuffix =
					affix.slot == AffixSlot::kSuffix && !affix.family.empty();
				if (!deferTieredSuffix) {
					a_desiredPassives.insert(affix.passiveSpell);
				}
			}

			// Family-less suffix values keep their legacy additive behavior.
			// Tiered families are resolved once after all worn items are counted.
			if (affixIdx < _affixes.size()) {
				const auto& affix = _affixes[affixIdx];
				if (affix.slot == AffixSlot::kSuffix &&
					affix.family.empty() &&
					affix.critDamageBonusPct > 0.0f) {
					_activeCritDamageBonusPct += affix.critDamageBonusPct;
				}
			}

			// "Best Slot Wins" penalty only for prefixes.
			if (affixIdx < _affixes.size() && _affixes[affixIdx].slot != AffixSlot::kSuffix) {
				if (affixIdx < _lootState.activeSlotPenalty.size()) {
					_lootState.activeSlotPenalty[affixIdx] = std::max(_lootState.activeSlotPenalty[affixIdx], penalty);
				}
			}
		}
	}

	void EventBridge::CollectBestSuffixFamilyState(
		std::unordered_set<RE::SpellItem*>& a_desiredPassives)
	{
		std::unordered_map<std::string_view, detail::SuffixFamilyBestCandidate> bestAffixByFamily;
		for (std::size_t affixIdx = 0; affixIdx < _affixes.size() && affixIdx < _activeCounts.size(); ++affixIdx) {
			const auto& affix = _affixes[affixIdx];
			if (_activeCounts[affixIdx] == 0 ||
				affix.slot != AffixSlot::kSuffix ||
				affix.family.empty()) {
				continue;
			}

			bestAffixByFamily[affix.family].Consider(affix.id, affixIdx);
		}

		for (const auto& [_, best] : bestAffixByFamily) {
			if (!best.selected || best.index >= _affixes.size()) {
				continue;
			}

			const auto& affix = _affixes[best.index];
			_activeCritDamageBonusPct += affix.critDamageBonusPct;
			if (!_runtimeSettings.disablePassiveSuffixSpells && affix.passiveSpell) {
				a_desiredPassives.insert(affix.passiveSpell);
			}
		}
	}

	void EventBridge::LogActiveAffixListDebug() const
	{
		if (_loot.debugLog) {
			std::uint32_t shown = 0;
			for (std::size_t i = 0; i < _affixes.size() && i < _activeCounts.size(); i++) {
				if (_activeCounts[i] == 0) {
					continue;
				}
				SKSE::log::debug("CalamityAffixes: active affix (id={}, count={})", _affixes[i].id, _activeCounts[i]);
				shown += 1;
				if (shown >= 50) {
					break;
				}
			}
			if (shown == 0) {
				SKSE::log::debug("CalamityAffixes: active affix list is empty (no equipped affix instances detected).");
			}
		}
	}

	void EventBridge::ApplyDesiredPassiveSpells(
		RE::PlayerCharacter* a_player,
		const std::unordered_set<RE::SpellItem*>& a_desiredPassives,
		bool a_refreshConfiguredPassivesOnPostLoad)
	{
		if (!a_player) {
			return;
		}

		std::unordered_set<RE::SpellItem*> knownPassiveSpells = _appliedPassiveSpells;
		for (const auto& affix : _affixes) {
			if (affix.passiveSpell) {
				knownPassiveSpells.insert(affix.passiveSpell);
			}
		}
		for (auto* spell : a_desiredPassives) {
			if (spell) {
				knownPassiveSpells.insert(spell);
			}
		}
		std::unordered_set<RE::SpellItem*> refreshRequestedPassives;
		if (a_refreshConfiguredPassivesOnPostLoad) {
			for (const auto& affix : _affixes) {
				if (affix.refreshPassiveSpellOnPostLoad &&
					affix.passiveSpell &&
					a_desiredPassives.contains(affix.passiveSpell)) {
					refreshRequestedPassives.insert(affix.passiveSpell);
				}
			}
		}

		const bool passivesDisabled = _runtimeSettings.disablePassiveSuffixSpells;
		auto findPassiveAddFeedback = [this](RE::SpellItem* a_spell) -> const Action* {
			for (const auto& affix : _affixes) {
				if (affix.passiveSpell == a_spell &&
					affix.action.feedback.playOn == ActionFeedbackPlayOn::kPassiveAdd) {
					return std::addressof(affix.action);
				}
			}
			return nullptr;
		};
		for (auto* spell : knownPassiveSpells) {
			if (!spell) {
				continue;
			}

			const bool desired = a_desiredPassives.find(spell) != a_desiredPassives.end();
			const bool present = a_player->HasSpell(spell);
			const bool refreshRequested = refreshRequestedPassives.contains(spell);
			switch (detail::ResolvePassiveSpellReconcileAction(desired, present, passivesDisabled, refreshRequested)) {
			case detail::PassiveSpellReconcileAction::kAdd:
				a_player->AddSpell(spell);
				if (const auto* feedbackAction = findPassiveAddFeedback(spell)) {
					PlayActionFeedback(*feedbackAction, a_player, nullptr, ActionFeedbackPlayOn::kPassiveAdd);
				}
				SKSE::log::debug("CalamityAffixes: applied passive spell {:08X}.", spell->GetFormID());
				break;
			case detail::PassiveSpellReconcileAction::kRemove:
				a_player->RemoveSpell(spell);
				SKSE::log::debug("CalamityAffixes: removed stale passive spell {:08X}.", spell->GetFormID());
				break;
			case detail::PassiveSpellReconcileAction::kRefresh:
				a_player->RemoveSpell(spell);
				a_player->AddSpell(spell);
				SKSE::log::debug("CalamityAffixes: refreshed passive spell {:08X} after load.", spell->GetFormID());
				break;
			case detail::PassiveSpellReconcileAction::kKeep:
			default:
				break;
			}
		}

		_appliedPassiveSpells.clear();
		if (!passivesDisabled) {
			for (auto* spell : a_desiredPassives) {
				if (spell) {
					_appliedPassiveSpells.insert(spell);
				}
			}
		}
	}

	void EventBridge::LogRebuildActiveCountsDebugSummary(
		const std::unordered_set<RE::SpellItem*>& a_desiredPassives) const
	{
		if (_loot.debugLog) {
			std::uint32_t totalActive = 0;
			std::uint32_t totalWornInstances = 0;
			for (std::size_t i = 0; i < _activeCounts.size(); ++i) {
				if (_activeCounts[i] > 0) {
					++totalActive;
				}
			}
			for (const auto& [token, keys] : _equippedInstanceKeysByToken) {
				totalWornInstances += static_cast<std::uint32_t>(keys.size());
			}
			SKSE::log::debug(
				"CalamityAffixes: RebuildActiveCounts — {} active affixes, {} worn instances, {} desired passives, {} applied passives, {} total affixes loaded.",
				totalActive,
				totalWornInstances,
				a_desiredPassives.size(),
				_appliedPassiveSpells.size(),
				_affixes.size());
		}
	}

	void EventBridge::RebuildActiveCounts(bool a_refreshConfiguredPassivesOnPostLoad)
	{
		if (!_configLoaded) {
			return;
		}
		if (!_runtimeSettings.enabled) {
			DeactivateRuntimeState();
			return;
		}

		ResetActiveCountsStateForRebuild();
		std::unordered_set<RE::SpellItem*> desiredPassives;

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		auto* changes = player->GetInventoryChanges();
		if (!changes || !changes->entryList) {
			return;
		}

		for (auto* entry : *changes->entryList) {
			if (!entry || !entry->extraLists) {
				continue;
			}

			for (auto* xList : *entry->extraLists) {
				RefreshInventoryInstanceActiveState(entry, xList, desiredPassives);
			}
		}

		CollectBestSuffixFamilyState(desiredPassives);

		for (auto& [_, keys] : _equippedInstanceKeysByToken) {
			std::sort(keys.begin(), keys.end());
			keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
		}
		RebuildActiveTriggerIndexCaches();
		_equippedTokenCacheReady = true;

		LogActiveAffixListDebug();
		ApplyDesiredPassiveSpells(player, desiredPassives, a_refreshConfiguredPassivesOnPostLoad);
		LogRebuildActiveCountsDebugSummary(desiredPassives);
	}

}
