#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "CalamityAffixes/AffixRegistryState.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/LootRuntimeState.h"

namespace CalamityAffixes::SerializationLoadState
{
	[[nodiscard]] inline LootShuffleBagState* ResolveShuffleBag(
		LootRuntimeState& a_state,
		std::uint8_t a_id) noexcept
	{
		switch (a_id) {
		case 0u:
			return &a_state.prefixSharedBag;
		case 1u:
			return &a_state.prefixWeaponBag;
		case 2u:
			return &a_state.prefixArmorBag;
		case 3u:
			return &a_state.suffixSharedBag;
		case 4u:
			return &a_state.suffixWeaponBag;
		case 5u:
			return &a_state.suffixArmorBag;
		default:
			return nullptr;
		}
	}

	inline void RebuildEvaluatedRecent(
		LootRuntimeState& a_state,
		std::size_t a_maxRecentEntries)
	{
		std::vector<std::uint64_t> keys;
		keys.reserve(a_state.evaluatedInstances.size());
		for (const auto key : a_state.evaluatedInstances) {
			if (key != 0u) {
				keys.push_back(key);
			}
		}

		std::sort(keys.begin(), keys.end());
		a_state.evaluatedRecent.clear();
		for (const auto key : keys) {
			a_state.evaluatedRecent.push_back(key);
		}
		while (a_state.evaluatedRecent.size() > a_maxRecentEntries) {
			a_state.evaluatedRecent.pop_front();
		}
		a_state.evaluatedInsertionsSincePrune = 0;
	}

	inline void RebuildCurrencyLedgerRecent(
		LootRuntimeState& a_state,
		std::size_t a_maxRecentEntries)
	{
		std::vector<std::uint64_t> keys;
		keys.reserve(a_state.currencyRollLedger.size());
		for (const auto& [key, _] : a_state.currencyRollLedger) {
			if (key != 0u) {
				keys.push_back(key);
			}
		}

		std::sort(keys.begin(), keys.end());
		a_state.currencyRollLedgerRecent.clear();
		for (const auto key : keys) {
			a_state.currencyRollLedgerRecent.push_back(key);
		}
		while (a_state.currencyRollLedgerRecent.size() > a_maxRecentEntries) {
			const auto oldest = a_state.currencyRollLedgerRecent.front();
			a_state.currencyRollLedgerRecent.pop_front();
			a_state.currencyRollLedger.erase(oldest);
		}
	}

	inline void RebuildCorpseCurrencyLedgerRecent(
		LootRuntimeState& a_state,
		std::size_t a_maxRecentEntries)
	{
		std::vector<std::pair<std::uint32_t, std::uint32_t>> chronologicalEntries;
		chronologicalEntries.reserve(a_state.corpseCurrencyRollLedger.size());
		for (const auto& [formId, entry] : a_state.corpseCurrencyRollLedger) {
			if (formId != 0u) {
				chronologicalEntries.emplace_back(entry.dayStamp, formId);
			}
		}

		std::sort(chronologicalEntries.begin(), chronologicalEntries.end());
		a_state.corpseCurrencyRollLedgerRecent.clear();
		for (const auto& [_, formId] : chronologicalEntries) {
			a_state.corpseCurrencyRollLedgerRecent.push_back(formId);
		}
		while (a_state.corpseCurrencyRollLedgerRecent.size() > a_maxRecentEntries) {
			const auto oldest = a_state.corpseCurrencyRollLedgerRecent.front();
			a_state.corpseCurrencyRollLedgerRecent.pop_front();
			a_state.corpseCurrencyRollLedger.erase(oldest);
		}
	}

	inline void SanitizeShuffleBagOrders(
		const AffixRegistryState& a_affixRegistry,
		LootRuntimeState& a_state)
	{
		detail::SanitizeLootShuffleBagOrder(
			a_affixRegistry.lootSharedAffixes,
			a_state.prefixSharedBag.order,
			a_state.prefixSharedBag.cursor);
		detail::SanitizeLootShuffleBagOrder(
			a_affixRegistry.lootWeaponAffixes,
			a_state.prefixWeaponBag.order,
			a_state.prefixWeaponBag.cursor);
		detail::SanitizeLootShuffleBagOrder(
			a_affixRegistry.lootArmorAffixes,
			a_state.prefixArmorBag.order,
			a_state.prefixArmorBag.cursor);
		detail::SanitizeLootShuffleBagOrder(
			a_affixRegistry.lootSharedSuffixes,
			a_state.suffixSharedBag.order,
			a_state.suffixSharedBag.cursor);
		detail::SanitizeLootShuffleBagOrder(
			a_affixRegistry.lootWeaponSuffixes,
			a_state.suffixWeaponBag.order,
			a_state.suffixWeaponBag.cursor);
		detail::SanitizeLootShuffleBagOrder(
			a_affixRegistry.lootArmorSuffixes,
			a_state.suffixArmorBag.order,
			a_state.suffixArmorBag.cursor);
	}
}
