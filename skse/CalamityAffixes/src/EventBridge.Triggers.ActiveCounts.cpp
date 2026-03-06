#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <cstdint>
#include <unordered_set>


namespace CalamityAffixes
{
	void EventBridge::RebuildActiveCounts()
	{
		if (!_configLoaded) {
			return;
		}

		_activeCounts.assign(_affixes.size(), 0);
		_lootState.activeSlotPenalty.assign(_affixes.size(), 0.0f);
		_activeCritDamageBonusPct = 0.0f;
		RebuildActiveTriggerIndexCaches();
		_equippedInstanceKeysByToken.clear();
		_equippedTokenCacheReady = false;
		_equippedInstanceKeysByToken.reserve(_affixRegistry.affixIndexByToken.size());
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
				if (!xList) {
					continue;
				}

				auto* uid = xList->GetByType<RE::ExtraUniqueID>();
				if (!uid) {
					continue;
				}

				const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
				const auto it = _instanceAffixes.find(key);
				if (it == _instanceAffixes.end()) {
					continue;
				}

				// Lazy cleanup: strip stale mappings for non-eligible items from older versions/saves.
				if (!IsLootObjectEligibleForAffixes(entry->object)) {
					if (_loot.cleanupInvalidLegacyAffixes) {
						CleanupInvalidLootInstance(entry, xList, key, "RebuildActiveCounts.ineligible");
					}
					continue;
				}

				const auto& slots = it->second;
				for (std::uint8_t slot = 0; slot < slots.count; ++slot) {
					if (slots.tokens[slot] != 0u) {
						EnsureInstanceRuntimeState(key, slots.tokens[slot]);
					}
				}

				// Use primary affix for display name
				if (slots.count > 0) {
					if (const auto idxIt = _affixRegistry.affixIndexByToken.find(slots.tokens[0]); idxIt != _affixRegistry.affixIndexByToken.end() && idxIt->second < _affixes.size()) {
						EnsureMultiAffixDisplayName(entry, xList, slots);
					}
				}

				const bool worn = xList->HasType<RE::ExtraWorn>() || xList->HasType<RE::ExtraWornLeft>();
				if (!worn) {
					continue;
				}

				// Count each unique affix token as active, track best slot penalty
				for (std::uint8_t slot = 0; slot < slots.count; ++slot) {
					const auto token = slots.tokens[slot];
					if (token != 0u) {
						_equippedInstanceKeysByToken[token].push_back(key);
					}

						const auto idxIt = _affixRegistry.affixIndexByToken.find(token);
						if (idxIt == _affixRegistry.affixIndexByToken.end()) {
							SKSE::log::warn(
								"CalamityAffixes: RebuildActiveCounts — worn token {:016X} not found in _affixRegistry.affixIndexByToken (instance={:016X}, slot={}).",
								token, key, slot);
						continue;
					}
					const auto affixIdx = idxIt->second;

					if (affixIdx < _activeCounts.size()) {
						_activeCounts[affixIdx] += 1;
					}

					// Collect passive spells for equipped affixes (any slot)
					if (!_runtimeSettings.disablePassiveSuffixSpells &&
						affixIdx < _affixes.size() &&
						_affixes[affixIdx].passiveSpell) {
						desiredPassives.insert(_affixes[affixIdx].passiveSpell);
					}

					// Accumulate crit damage bonus for equipped suffixes.
					// Intentionally stacks across multiple equipped items (same suffix on sword + shield = additive).
					if (affixIdx < _affixes.size() && _affixes[affixIdx].critDamageBonusPct > 0.0f) {
						_activeCritDamageBonusPct += _affixes[affixIdx].critDamageBonusPct;
					}

					// "Best Slot Wins" penalty only for prefixes
					if (affixIdx < _affixes.size() && _affixes[affixIdx].slot != AffixSlot::kSuffix) {
						const float penalty = kMultiAffixProcPenalty[std::min<std::uint8_t>(slots.count, static_cast<std::uint8_t>(kMaxAffixesPerItem)) - 1];
						if (affixIdx < _lootState.activeSlotPenalty.size()) {
							_lootState.activeSlotPenalty[affixIdx] = std::max(_lootState.activeSlotPenalty[affixIdx], penalty);
						}
					}
				}
			}
		}

		for (auto& [_, keys] : _equippedInstanceKeysByToken) {
			std::sort(keys.begin(), keys.end());
			keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
		}
		RebuildActiveTriggerIndexCaches();
		_equippedTokenCacheReady = true;

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

		// Apply/remove passive suffix spells
		if (player) {
			if (_runtimeSettings.disablePassiveSuffixSpells) {
				for (auto* spell : _appliedPassiveSpells) {
					player->RemoveSpell(spell);
					SKSE::log::info(
						"CalamityAffixes: removed passive suffix spell {:08X} (passives disabled).",
						spell ? spell->GetFormID() : 0u);
				}
				_appliedPassiveSpells.clear();
			} else {
				// Remove spells no longer needed
				for (auto it = _appliedPassiveSpells.begin(); it != _appliedPassiveSpells.end(); ) {
					if (desiredPassives.find(*it) == desiredPassives.end()) {
						player->RemoveSpell(*it);
						SKSE::log::debug("CalamityAffixes: removed passive suffix spell {:08X}.", (*it)->GetFormID());
						it = _appliedPassiveSpells.erase(it);
					} else {
						++it;
					}
				}
				// Add new spells
				for (auto* spell : desiredPassives) {
					if (_appliedPassiveSpells.find(spell) == _appliedPassiveSpells.end()) {
						player->AddSpell(spell);
						_appliedPassiveSpells.insert(spell);
						SKSE::log::debug("CalamityAffixes: applied passive suffix spell {:08X}.", spell->GetFormID());
					}
				}
			}

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
					totalActive, totalWornInstances, desiredPassives.size(), _appliedPassiveSpells.size(), _affixes.size());
			}
		}
	}

}
