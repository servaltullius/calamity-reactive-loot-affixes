#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/NonHostileFirstHitGate.h"
#include "CalamityAffixes/PerTargetCooldownStore.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
	bool CheckNonHostileFirstHitGate()
	{
		using namespace std::chrono;

		CalamityAffixes::NonHostileFirstHitGate gate{};
		const auto now = steady_clock::now();

		// First non-hostile hit on a fresh pair is allowed.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now)) {
			std::cerr << "gate: expected first non-hostile hit to be allowed\n";
			return false;
		}

		// Reentry within the short window stays allowed.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(10))) {
			std::cerr << "gate: expected reentry within window to be allowed\n";
			return false;
		}

		// Reentry beyond the window is denied.
		if (gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(30))) {
			std::cerr << "gate: expected reentry after window to be denied\n";
			return false;
		}

		// Hostile transition clears and denies this hit.
		if (gate.Resolve(0x14u, 0x1234u, true, true, false, now + milliseconds(200))) {
			std::cerr << "gate: expected hostile transition to be denied\n";
			return false;
		}

		// After hostile clear, first non-hostile hit can be allowed again.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(250))) {
			std::cerr << "gate: expected first non-hostile hit after clear to be allowed\n";
			return false;
		}

		// Guard rails.
		if (gate.Resolve(0x14u, 0x2234u, false, false, false, now)) {
			std::cerr << "gate: expected disabled setting to deny\n";
			return false;
		}
		if (gate.Resolve(0x14u, 0x3234u, true, false, true, now)) {
			std::cerr << "gate: expected player target to deny\n";
			return false;
		}

		return true;
	}

	bool CheckPerTargetCooldownStore()
	{
		using namespace std::chrono;

		CalamityAffixes::PerTargetCooldownStore store{};
		const auto now = steady_clock::now();

		// Fresh key is not blocked.
		if (store.IsBlocked(0xA5u, 0x99u, now)) {
			std::cerr << "store: expected fresh key to be unblocked\n";
			return false;
		}

		// Commit establishes blocking interval.
		store.Commit(0xA5u, 0x99u, milliseconds(200), now);
		if (!store.IsBlocked(0xA5u, 0x99u, now + milliseconds(100))) {
			std::cerr << "store: expected key to be blocked before ICD expiry\n";
			return false;
		}
		if (store.IsBlocked(0xA5u, 0x99u, now + milliseconds(200))) {
			std::cerr << "store: expected key to unblock at ICD boundary\n";
			return false;
		}

		// Invalid commit inputs must not create blocking state.
		store.Commit(0u, 0x99u, milliseconds(200), now);
		if (store.IsBlocked(0u, 0x99u, now + milliseconds(1))) {
			std::cerr << "store: expected zero-token commit to be ignored\n";
			return false;
		}

		// Clear removes existing state.
		store.Clear();
		if (store.IsBlocked(0xA5u, 0x99u, now + milliseconds(1))) {
			std::cerr << "store: expected clear to remove cooldown state\n";
			return false;
		}

		return true;
	}

	bool CheckUniformLootRollSelection()
	{
		std::mt19937 rng{ 0xCAFFu };

		{
			const std::vector<std::size_t> empty{};
			if (CalamityAffixes::detail::SelectUniformEligibleLootIndex(rng, empty).has_value()) {
				std::cerr << "loot_select: expected empty candidate set to return nullopt\n";
				return false;
			}
		}

		{
			const std::vector<std::size_t> single{ 42u };
			const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndex(rng, single);
			if (!picked.has_value() || *picked != 42u) {
				std::cerr << "loot_select: expected single candidate set to return that element\n";
				return false;
			}
		}

		const std::vector<std::size_t> candidates{ 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u };
		std::array<std::size_t, 8> counts{};
		constexpr std::size_t kDraws = 100000u;

		for (std::size_t i = 0; i < kDraws; ++i) {
			const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndex(rng, candidates);
			if (!picked.has_value() || *picked >= counts.size()) {
				std::cerr << "loot_select: invalid pick while sampling uniform distribution\n";
				return false;
			}
			counts[*picked] += 1u;
		}

		const double expected = static_cast<double>(kDraws) / static_cast<double>(counts.size());
		const double tolerance = expected * 0.05;  // wide enough to avoid flake; tight enough to catch weighting regressions.
		for (std::size_t i = 0; i < counts.size(); ++i) {
			const double diff = std::abs(static_cast<double>(counts[i]) - expected);
			if (diff > tolerance) {
				std::cerr << "loot_select: bucket " << i << " outside tolerance (count="
				          << counts[i] << ", expected=" << expected << ", tolerance=" << tolerance << ")\n";
				return false;
			}
		}

		const auto [minIt, maxIt] = std::minmax_element(counts.begin(), counts.end());
		if (static_cast<double>(*maxIt - *minIt) > expected * 0.09) {
			std::cerr << "loot_select: spread too wide (min=" << *minIt << ", max=" << *maxIt << ")\n";
			return false;
		}

		return true;
	}

	bool CheckShuffleBagLootRollSelection()
	{
		std::mt19937 rng{ 0xBA6Au };
		const std::vector<std::size_t> pool{ 0u, 1u, 2u, 3u, 4u, 5u };
		std::vector<std::size_t> bag{};
		std::size_t cursor = 0u;

		// Full-eligible draws should behave as "without replacement" per cycle.
		for (int cycle = 0; cycle < 4; ++cycle) {
			std::array<std::size_t, 6> seen{};
			for (std::size_t draw = 0; draw < pool.size(); ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					pool,
					bag,
					cursor,
					[](std::size_t) { return true; });
				if (!picked.has_value() || *picked >= seen.size()) {
					std::cerr << "shuffle_bag: invalid draw in full-eligible cycle\n";
					return false;
				}
				seen[*picked] += 1u;
			}
			for (std::size_t i = 0; i < seen.size(); ++i) {
				if (seen[i] != 1u) {
					std::cerr << "shuffle_bag: expected exactly one hit per bucket in a cycle (bucket="
					          << i << ", count=" << seen[i] << ")\n";
					return false;
				}
			}
		}

		// Temporary ineligibility must not consume that entry from the bag.
		bag.clear();
		cursor = 0u;

		const auto first = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
			rng,
			pool,
			bag,
			cursor,
			[](std::size_t) { return true; });
		if (!first.has_value()) {
			std::cerr << "shuffle_bag: expected first draw to succeed\n";
			return false;
		}

		if (cursor >= bag.size()) {
			std::cerr << "shuffle_bag: invalid cursor after first draw\n";
			return false;
		}

		const auto temporarilyExcluded = bag[cursor];
		const auto second = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
			rng,
			pool,
			bag,
			cursor,
			[temporarilyExcluded](std::size_t idx) { return idx != temporarilyExcluded; });
		if (!second.has_value()) {
			std::cerr << "shuffle_bag: expected second draw to succeed with one temporary exclusion\n";
			return false;
		}

		const auto third = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
			rng,
			pool,
			bag,
			cursor,
			[](std::size_t) { return true; });
		if (!third.has_value()) {
			std::cerr << "shuffle_bag: expected third draw to succeed\n";
			return false;
		}
		if (*third != temporarilyExcluded) {
			std::cerr << "shuffle_bag: temporarily excluded entry was consumed unexpectedly\n";
			return false;
		}

		return true;
	}

	bool CheckShuffleBagSanitizeAndRollConstraints()
	{
		struct MockAffix
		{
			float effectiveLootWeight{ 0.0f };
			bool isSuffix{ false };
			std::string family{};
		};

		std::mt19937 rng{ 0x5EEDu };
		std::vector<MockAffix> affixes{
			{ 1.0f, false, "" },      // 0 weapon prefix
			{ 1.0f, false, "" },      // 1 weapon prefix
			{ 0.0f, false, "" },      // 2 weapon prefix (disabled)
			{ 1.0f, false, "" },      // 3 armor prefix
			{ 1.0f, false, "" },      // 4 armor prefix
			{ 1.0f, true, "life" },   // 5 suffix
			{ 1.0f, true, "mana" },   // 6 suffix
			{ 1.0f, true, "life" },   // 7 suffix
		};

		const std::vector<std::size_t> weaponPrefixes{ 0u, 1u, 2u };
		const std::vector<std::size_t> armorPrefixes{ 3u, 4u };
		const std::vector<std::size_t> sharedPrefixes{ 0u, 1u, 2u, 3u, 4u };
		const std::vector<std::size_t> sharedSuffixes{ 5u, 6u, 7u };

		std::vector<std::size_t> prefixWeaponBag{};
		std::size_t prefixWeaponCursor = 0u;
		std::vector<std::size_t> prefixSharedBag{};
		std::size_t prefixSharedCursor = 0u;
		std::vector<std::size_t> suffixSharedBag{};
		std::size_t suffixSharedCursor = 0u;

		{
			std::vector<std::size_t> dirtyBag{ 99u, 2u, 2u, 1u };
			std::size_t dirtyCursor = 99u;
			CalamityAffixes::detail::SanitizeLootShuffleBagOrder(sharedPrefixes, dirtyBag, dirtyCursor);
			const std::vector<std::size_t> expected{ 2u, 1u, 0u, 3u, 4u };
			if (dirtyBag != expected || dirtyCursor != expected.size()) {
				std::cerr << "shuffle_bag: sanitize did not normalize bag as expected\n";
				return false;
			}
		}

		{
			const auto one = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(1u);
			if (one.prefixTarget != 1u || one.suffixTarget != 0u) {
				std::cerr << "shuffle_bag: target=1 composition should be prefix-only (1/0)\n";
				return false;
			}
			const auto two = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(2u);
			if (two.prefixTarget != 2u || two.suffixTarget != 0u) {
				std::cerr << "shuffle_bag: target=2 composition should be prefix-only (2/0)\n";
				return false;
			}
			const auto three = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(3u);
			if (three.prefixTarget != 3u || three.suffixTarget != 0u) {
				std::cerr << "shuffle_bag: target=3 composition should be prefix-only (3/0)\n";
				return false;
			}

			if (!CalamityAffixes::detail::ShouldConsumeSuffixRollForSingleAffixTarget(1u, 0u)) {
				std::cerr << "shuffle_bag: single-affix suffix fallback should be allowed before prefix assignment\n";
				return false;
			}
			if (CalamityAffixes::detail::ShouldConsumeSuffixRollForSingleAffixTarget(1u, 1u)) {
				std::cerr << "shuffle_bag: single-affix suffix fallback should be blocked after prefix assignment\n";
				return false;
			}
			if (!CalamityAffixes::detail::ShouldConsumeSuffixRollForSingleAffixTarget(2u, 1u)) {
				std::cerr << "shuffle_bag: multi-affix suffix roll should remain enabled\n";
				return false;
			}
		}

		{
			// Non-shared weapon pool must never pick armor indices and must skip disabled index 2.
			for (std::size_t draw = 0; draw < 40; ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					weaponPrefixes,
					prefixWeaponBag,
					prefixWeaponCursor,
					[&](std::size_t idx) {
						return idx < affixes.size() && affixes[idx].effectiveLootWeight > 0.0f;
					});
				if (!picked.has_value()) {
					std::cerr << "shuffle_bag: weapon-prefix draw unexpectedly failed\n";
					return false;
				}
				if (*picked >= 3u) {
					std::cerr << "shuffle_bag: weapon-only draw leaked into armor pool (idx=" << *picked << ")\n";
					return false;
				}
				if (*picked == 2u) {
					std::cerr << "shuffle_bag: disabled prefix was selected\n";
					return false;
				}
			}
		}

		{
			// Shared prefix pool with exclusions should converge to the only allowed prefix (idx=4).
			const std::vector<std::size_t> exclude{ 0u, 1u, 2u, 3u };
			for (std::size_t draw = 0; draw < 10; ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					sharedPrefixes,
					prefixSharedBag,
					prefixSharedCursor,
					[&](std::size_t idx) {
						if (idx >= affixes.size() || affixes[idx].effectiveLootWeight <= 0.0f) {
							return false;
						}
						return std::find(exclude.begin(), exclude.end(), idx) == exclude.end();
					});
				if (!picked.has_value() || *picked != 4u) {
					std::cerr << "shuffle_bag: shared-prefix exclusion constraint broken\n";
					return false;
				}
			}
		}

		{
			// Suffix family exclusion should never pick family='life' (idx=5,7).
			const std::vector<std::string> excludeFamilies{ "life" };
			for (std::size_t draw = 0; draw < 10; ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					sharedSuffixes,
					suffixSharedBag,
					suffixSharedCursor,
					[&](std::size_t idx) {
						if (idx >= affixes.size()) {
							return false;
						}
						const auto& affix = affixes[idx];
						if (!affix.isSuffix || affix.effectiveLootWeight <= 0.0f) {
							return false;
						}
						if (!affix.family.empty()) {
							return std::find(excludeFamilies.begin(), excludeFamilies.end(), affix.family) == excludeFamilies.end();
						}
						return true;
					});
				if (!picked.has_value() || *picked != 6u) {
					std::cerr << "shuffle_bag: suffix family exclusion constraint broken\n";
					return false;
				}
			}
		}

		(void)armorPrefixes;  // documents that armor pool exists in this integration-like scenario.
		return true;
	}
}

int main()
{
	const bool gateOk = CheckNonHostileFirstHitGate();
	const bool storeOk = CheckPerTargetCooldownStore();
	const bool lootSelectionOk = CheckUniformLootRollSelection();
	const bool shuffleBagSelectionOk = CheckShuffleBagLootRollSelection();
	const bool shuffleBagConstraintsOk = CheckShuffleBagSanitizeAndRollConstraints();
	return (gateOk && storeOk && lootSelectionOk && shuffleBagSelectionOk && shuffleBagConstraintsOk) ? 0 : 1;
}
