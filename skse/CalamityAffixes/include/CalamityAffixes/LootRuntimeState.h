#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "CalamityAffixes/InstanceAffixSlots.h"
#include "CalamityAffixes/LootRerollGuard.h"
#include "CalamityAffixes/LootShuffleBagState.h"

namespace CalamityAffixes
{
	struct LootRuntimeState
	{
		LootShuffleBagState prefixSharedBag{};
		LootShuffleBagState prefixWeaponBag{};
		LootShuffleBagState prefixArmorBag{};
		LootShuffleBagState suffixSharedBag{};
		LootShuffleBagState suffixWeaponBag{};
		LootShuffleBagState suffixArmorBag{};

		std::unordered_map<std::uint64_t, InstanceAffixSlots> previewAffixes{};
		std::deque<std::uint64_t> previewRecent{};

		std::unordered_set<std::uint64_t> evaluatedInstances{};
		std::deque<std::uint64_t> evaluatedRecent{};
		std::size_t evaluatedInsertionsSincePrune{ 0 };

		std::unordered_map<std::uint64_t, std::uint32_t> currencyRollLedger{};
		std::deque<std::uint64_t> currencyRollLedgerRecent{};

		std::uint32_t lootChanceEligibleFailStreak{ 0 };
		std::uint32_t runewordFragmentFailStreak{ 0 };
		std::uint32_t reforgeOrbFailStreak{ 0 };

		std::vector<float> activeSlotPenalty{};
		LootRerollGuard rerollGuard{};
		std::vector<LootRerollGuard::RefHandle> pendingDroppedRefDeletes{};
		std::atomic_bool dropDeleteDrainScheduled{ false };
		std::map<std::pair<LootRerollGuard::FormID, LootRerollGuard::FormID>, std::int32_t> playerContainerStash{};

		void ResetForConfigReload() noexcept
		{
			prefixSharedBag = {};
			prefixWeaponBag = {};
			prefixArmorBag = {};
			suffixSharedBag = {};
			suffixWeaponBag = {};
			suffixArmorBag = {};
			previewAffixes.clear();
			previewRecent.clear();
			lootChanceEligibleFailStreak = 0;
			runewordFragmentFailStreak = 0;
			reforgeOrbFailStreak = 0;
			activeSlotPenalty.clear();
			rerollGuard.Reset();
			playerContainerStash.clear();
		}

		void ResetForLoadOrRevert() noexcept
		{
			ResetForConfigReload();
			evaluatedInstances.clear();
			evaluatedRecent.clear();
			evaluatedInsertionsSincePrune = 0;
			currencyRollLedger.clear();
			currencyRollLedgerRecent.clear();
		}
	};
}
