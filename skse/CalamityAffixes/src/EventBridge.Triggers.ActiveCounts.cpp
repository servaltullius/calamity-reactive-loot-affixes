#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/ProcChancePolicy.h"
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
			for (const auto& affix : _affixRuntimeState.affixes) {
				if (affix.passiveSpell) {
					_instanceTrackingState.appliedPassiveSpells.insert(affix.passiveSpell);
				}
			}
			ApplyDesiredPassiveSpells(player, {});
		} else {
			_instanceTrackingState.appliedPassiveSpells.clear();
		}

		ClearTrapRuntimeState();
		_combatState.ResetTransientState();
		Hooks::ClearRuntimeState();
		_equipResync.nextAtMs = 0u;
		for (auto& affix : _affixRuntimeState.affixes) {
			affix.nextAllowed = {};
		}
	}

	void EventBridge::ResetActiveCountsStateForRebuild()
	{
		_affixRuntimeState.activeCounts.assign(_affixRuntimeState.affixes.size(), 0);
		_lootState.activeSlotPenalty.assign(_affixRuntimeState.affixes.size(), 0.0f);
		_affixRuntimeState.activeCritDamageBonusPct = 0.0f;
		_affixRuntimeState.RebuildActiveTriggerIndexCaches();
		_instanceTrackingState.equippedInstanceKeysByToken.clear();
		_instanceTrackingState.equippedTokenCacheReady = false;
		_instanceTrackingState.equippedInstanceKeysByToken.reserve(_affixRuntimeState.affixRegistry.affixIndexByToken.size());
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
		const auto it = _instanceTrackingState.instanceAffixes.find(key);
		if (it == _instanceTrackingState.instanceAffixes.end()) {
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
			if (const auto idxIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(slots.tokens[0]); idxIt != _affixRuntimeState.affixRegistry.affixIndexByToken.end() && idxIt->second < _affixRuntimeState.affixes.size()) {
				EnsureMultiAffixDisplayName(a_entry, a_xList, slots);
			}
		}

		const bool worn = a_xList->HasType<RE::ExtraWorn>() || a_xList->HasType<RE::ExtraWornLeft>();
		if (!worn) {
			return;
		}

		AccumulateEquippedAffixState(key, slots, a_desiredPassives);
	}

	std::uint8_t EventBridge::CountProcPenaltySlots(const InstanceAffixSlots& a_slots) const
	{
		// The multi-affix proc penalty damps proc stacking, so its tier counts only
		// tokens that can actually proc (IsProcPenaltyEligible): suffixes, 0%-chance
		// passive prefixes, DebugNotify entries, and unresolved tokens must not drag
		// down the real proc affixes sharing the item.
		return CountProcCapableSlots(a_slots.count, [&](std::uint8_t a_slot) {
			const auto token = a_slots.tokens[a_slot];
			if (token == 0u) {
				return false;
			}
			const auto idxIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(token);
			if (idxIt == _affixRuntimeState.affixRegistry.affixIndexByToken.end() ||
				idxIt->second >= _affixRuntimeState.affixes.size()) {
				return false;
			}
			const auto& affix = _affixRuntimeState.affixes[idxIt->second];
			return IsProcPenaltyEligible(
				affix.slot == AffixSlot::kSuffix,
				affix.procChancePct,
				affix.action.type != ActionType::kDebugNotify);
		});
	}

	void EventBridge::AccumulateEquippedAffixState(
		std::uint64_t a_instanceKey,
		const InstanceAffixSlots& a_slots,
		std::unordered_set<RE::SpellItem*>& a_desiredPassives)
	{
		if (a_slots.count == 0) {
			return;
		}

		const auto penalty = ResolveMultiAffixProcPenalty(CountProcPenaltySlots(a_slots));

		for (std::uint8_t slot = 0; slot < a_slots.count; ++slot) {
			const auto token = a_slots.tokens[slot];
			if (token != 0u) {
				_instanceTrackingState.equippedInstanceKeysByToken[token].push_back(a_instanceKey);
			}

			const auto idxIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(token);
			if (idxIt == _affixRuntimeState.affixRegistry.affixIndexByToken.end()) {
				SKSE::log::warn(
					"CalamityAffixes: RebuildActiveCounts — worn token {:016X} not found in _affixRuntimeState.affixRegistry.affixIndexByToken (instance={:016X}, slot={}).",
					token,
					a_instanceKey,
					slot);
				continue;
			}
			const auto affixIdx = idxIt->second;

			if (affixIdx < _affixRuntimeState.activeCounts.size()) {
				_affixRuntimeState.activeCounts[affixIdx] += 1;
			}

			// Family-less passives (including runewords) keep their existing behavior.
			// Tiered suffix families are selected once, after all worn items are counted.
			if (!_runtimeSettings.disablePassiveSuffixSpells &&
				affixIdx < _affixRuntimeState.affixes.size() &&
				_affixRuntimeState.affixes[affixIdx].passiveSpell) {
				const auto& affix = _affixRuntimeState.affixes[affixIdx];
				const bool deferTieredSuffix = detail::ShouldDeferTieredSuffixFamily({
					.isSuffix = affix.slot == AffixSlot::kSuffix,
					.hasFamily = !affix.family.empty(),
				});
				if (!deferTieredSuffix) {
					a_desiredPassives.insert(affix.passiveSpell);
				}
			}

			// Family-less suffix values keep their legacy additive behavior.
			// Tiered families are resolved once after all worn items are counted.
			if (affixIdx < _affixRuntimeState.affixes.size()) {
				const auto& affix = _affixRuntimeState.affixes[affixIdx];
				if (detail::ShouldAccumulateFamilylessSuffixValue({
						.isSuffix = affix.slot == AffixSlot::kSuffix,
						.hasFamily = !affix.family.empty(),
					}) &&
					affix.critDamageBonusPct > 0.0f) {
					_affixRuntimeState.activeCritDamageBonusPct += affix.critDamageBonusPct;
				}
			}

			// "Best Slot Wins" penalty only for prefixes.
			if (affixIdx < _affixRuntimeState.affixes.size() && _affixRuntimeState.affixes[affixIdx].slot != AffixSlot::kSuffix) {
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
		for (std::size_t affixIdx = 0; affixIdx < _affixRuntimeState.affixes.size() && affixIdx < _affixRuntimeState.activeCounts.size(); ++affixIdx) {
			const auto& affix = _affixRuntimeState.affixes[affixIdx];
			if (_affixRuntimeState.activeCounts[affixIdx] == 0 ||
				affix.slot != AffixSlot::kSuffix ||
				affix.family.empty()) {
				continue;
			}

			bestAffixByFamily[affix.family].Consider(affix.id, affixIdx);
		}

		for (const auto& [_, best] : bestAffixByFamily) {
			if (!best.selected || best.index >= _affixRuntimeState.affixes.size()) {
				continue;
			}

			const auto& affix = _affixRuntimeState.affixes[best.index];
			_affixRuntimeState.activeCritDamageBonusPct += affix.critDamageBonusPct;
			if (!_runtimeSettings.disablePassiveSuffixSpells && affix.passiveSpell) {
				a_desiredPassives.insert(affix.passiveSpell);
			}
		}
	}

	void EventBridge::LogActiveAffixListDebug() const
	{
		if (_loot.debugLog) {
			std::uint32_t shown = 0;
			for (std::size_t i = 0; i < _affixRuntimeState.affixes.size() && i < _affixRuntimeState.activeCounts.size(); i++) {
				if (_affixRuntimeState.activeCounts[i] == 0) {
					continue;
				}
				SKSE::log::debug("CalamityAffixes: active affix (id={}, count={})", _affixRuntimeState.affixes[i].id, _affixRuntimeState.activeCounts[i]);
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

		std::unordered_set<RE::SpellItem*> knownPassiveSpells = _instanceTrackingState.appliedPassiveSpells;
		for (const auto& affix : _affixRuntimeState.affixes) {
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
			for (const auto& affix : _affixRuntimeState.affixes) {
				if (affix.refreshPassiveSpellOnPostLoad &&
					affix.passiveSpell &&
					a_desiredPassives.contains(affix.passiveSpell)) {
					refreshRequestedPassives.insert(affix.passiveSpell);
				}
			}
		}

		const bool passivesDisabled = _runtimeSettings.disablePassiveSuffixSpells;
		auto findPassiveAddFeedback = [this](RE::SpellItem* a_spell) -> const Action* {
			for (const auto& affix : _affixRuntimeState.affixes) {
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
			switch (detail::ResolvePassiveSpellReconcileAction(detail::PassiveSpellReconcileInput{
				.desired = desired,
				.present = present,
				.passivesDisabled = passivesDisabled,
				.refreshRequested = refreshRequested,
			})) {
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

		_instanceTrackingState.appliedPassiveSpells.clear();
		if (!passivesDisabled) {
			for (auto* spell : a_desiredPassives) {
				if (spell) {
					_instanceTrackingState.appliedPassiveSpells.insert(spell);
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
			for (std::size_t i = 0; i < _affixRuntimeState.activeCounts.size(); ++i) {
				if (_affixRuntimeState.activeCounts[i] > 0) {
					++totalActive;
				}
			}
			for (const auto& [token, keys] : _instanceTrackingState.equippedInstanceKeysByToken) {
				totalWornInstances += static_cast<std::uint32_t>(keys.size());
			}
			SKSE::log::debug(
				"CalamityAffixes: RebuildActiveCounts — {} active affixes, {} worn instances, {} desired passives, {} applied passives, {} total affixes loaded.",
				totalActive,
				totalWornInstances,
				a_desiredPassives.size(),
				_instanceTrackingState.appliedPassiveSpells.size(),
				_affixRuntimeState.affixes.size());
		}
	}

	void EventBridge::RebuildActiveCounts(bool a_refreshConfiguredPassivesOnPostLoad)
	{
		if (!_configLoaded) {
			return;
		}
		if (!_runtimeSettings.enabled.load(std::memory_order_relaxed)) {
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

		for (auto& [_, keys] : _instanceTrackingState.equippedInstanceKeysByToken) {
			std::sort(keys.begin(), keys.end());
			keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
		}
		_affixRuntimeState.RebuildActiveTriggerIndexCaches();
		_instanceTrackingState.equippedTokenCacheReady = true;

		LogActiveAffixListDebug();
		ApplyDesiredPassiveSpells(player, desiredPassives, a_refreshConfiguredPassivesOnPostLoad);
		LogRebuildActiveCountsDebugSummary(desiredPassives);
	}

}
