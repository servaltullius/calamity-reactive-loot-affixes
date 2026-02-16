#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/NonHostileFirstHitGate.h"
#include "CalamityAffixes/PerTargetCooldownStore.h"
#include "CalamityAffixes/TriggerGuards.h"

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

	bool CheckWeightedShuffleBagLootRollSelection()
	{
		std::mt19937 rng{ 0xC0FFEEu };
		const std::vector<std::size_t> pool{ 0u, 1u, 2u, 3u };
		const std::array<float, 4> weights{ 8.0f, 4.0f, 2.0f, 1.0f };
		std::array<std::size_t, 4> counts{};

		std::vector<std::size_t> bag{};
		std::size_t cursor = 0u;
		constexpr std::size_t kDraws = 200000u;

		// Long-run frequency should track configured weights.
		for (std::size_t draw = 0; draw < kDraws; ++draw) {
			const auto picked = CalamityAffixes::detail::SelectWeightedEligibleLootIndexWithShuffleBag(
				rng,
				pool,
				bag,
				cursor,
				[](std::size_t) { return true; },
				[&](std::size_t idx) {
					if (idx >= weights.size()) {
						return 0.0f;
					}
					return weights[idx];
				});
			if (!picked.has_value() || *picked >= counts.size()) {
				std::cerr << "weighted_shuffle_bag: invalid weighted draw\n";
				return false;
			}
			counts[*picked] += 1u;
		}

		const double weightSum = 15.0;
		const std::array<double, 4> expected{
			8.0 / weightSum,
			4.0 / weightSum,
			2.0 / weightSum,
			1.0 / weightSum
		};

		for (std::size_t i = 0; i < counts.size(); ++i) {
			const double observed = static_cast<double>(counts[i]) / static_cast<double>(kDraws);
			if (std::abs(observed - expected[i]) > 0.015) {
				std::cerr << "weighted_shuffle_bag: distribution drift on bucket " << i
				          << " (observed=" << observed << ", expected=" << expected[i] << ")\n";
				return false;
			}
		}

		// Zero-weight entries should never win while a positive-weight option exists.
		const std::array<float, 4> zeroHeavy{ 10.0f, 0.0f, 0.0f, 0.0f };
		for (std::size_t draw = 0; draw < 1000u; ++draw) {
			const auto picked = CalamityAffixes::detail::SelectWeightedEligibleLootIndexWithShuffleBag(
				rng,
				pool,
				bag,
				cursor,
				[](std::size_t) { return true; },
				[&](std::size_t idx) {
					if (idx >= zeroHeavy.size()) {
						return 0.0f;
					}
					return zeroHeavy[idx];
				});
			if (!picked.has_value() || *picked != 0u) {
				std::cerr << "weighted_shuffle_bag: zero-weight candidate selected unexpectedly\n";
				return false;
			}
		}

		// All-zero weights must still produce a valid eligible pick (uniform fallback).
		const std::array<float, 4> allZero{ 0.0f, 0.0f, 0.0f, 0.0f };
		for (std::size_t draw = 0; draw < 1000u; ++draw) {
			const auto picked = CalamityAffixes::detail::SelectWeightedEligibleLootIndexWithShuffleBag(
				rng,
				pool,
				bag,
				cursor,
				[](std::size_t) { return true; },
				[&](std::size_t idx) {
					if (idx >= allZero.size()) {
						return 0.0f;
					}
					return allZero[idx];
				});
			if (!picked.has_value() || *picked >= pool.size()) {
				std::cerr << "weighted_shuffle_bag: all-zero fallback failed\n";
				return false;
			}
		}

		return true;
	}

	bool CheckFixedWindowBudget()
	{
		std::uint64_t windowStartMs = 0u;
		std::uint32_t consumed = 0u;

		// Disabled budget settings should always pass.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 0u, 5u, windowStartMs, consumed)) {
			std::cerr << "budget: expected windowMs=0 to disable budget gate\n";
			return false;
		}
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 50u, 0u, windowStartMs, consumed)) {
			std::cerr << "budget: expected maxPerWindow=0 to disable budget gate\n";
			return false;
		}

		windowStartMs = 0u;
		consumed = 0u;

		// First two consumes pass, third in same window fails.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected first consume to pass\n";
			return false;
		}
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(120u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected second consume to pass within same window\n";
			return false;
		}
		if (CalamityAffixes::TryConsumeFixedWindowBudget(130u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected third consume in same window to fail\n";
			return false;
		}

		// New window should reset budget.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(151u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected consume to pass after window rollover\n";
			return false;
		}

		// Time rewind should also reset window safely.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(40u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected consume to pass after timestamp rewind reset\n";
			return false;
		}

		return true;
	}

	bool CheckRecentlyAndLuckyHitGuards()
	{
		// Recently window semantics.
		if (!CalamityAffixes::IsWithinRecentlyWindowMs(1200u, 1000u, 250u)) {
			std::cerr << "recently: expected event inside window to pass\n";
			return false;
		}
		if (CalamityAffixes::IsWithinRecentlyWindowMs(1301u, 1000u, 250u)) {
			std::cerr << "recently: expected event outside window to fail\n";
			return false;
		}
		if (!CalamityAffixes::IsOutsideRecentlyWindowMs(1300u, 1000u, 250u)) {
			std::cerr << "recently: expected not-hit-recently gate to pass for old hit\n";
			return false;
		}
		if (CalamityAffixes::IsOutsideRecentlyWindowMs(1199u, 1000u, 250u)) {
			std::cerr << "recently: expected not-hit-recently gate to fail for fresh hit\n";
			return false;
		}

		// Lucky-hit effective chance envelope.
		if (CalamityAffixes::ResolveLuckyHitEffectiveChancePct(25.0f, 0.5f) != 12.5f) {
			std::cerr << "lucky_hit: expected 25% * 0.5 = 12.5%\n";
			return false;
		}
		if (CalamityAffixes::ResolveLuckyHitEffectiveChancePct(80.0f, 2.0f) != 100.0f) {
			std::cerr << "lucky_hit: expected clamp to 100%\n";
			return false;
		}

		// Runtime-like stochastic sanity check for lucky-hit chance.
		std::mt19937 rng{ 0x1A2Bu };
		std::uniform_real_distribution<float> dist(0.0f, 100.0f);

		const float effectiveChance = CalamityAffixes::ResolveLuckyHitEffectiveChancePct(25.0f, 0.5f);  // 12.5%
		constexpr std::size_t kTrials = 120000u;
		std::size_t hits = 0u;
		for (std::size_t i = 0; i < kTrials; ++i) {
			if (dist(rng) < effectiveChance) {
				++hits;
			}
		}

		const double observedPct = static_cast<double>(hits) * 100.0 / static_cast<double>(kTrials);
		const double expectedPct = 12.5;
		const double tolerancePct = 0.8;  // wide enough to avoid flakes, tight enough to catch regressions.
		if (std::abs(observedPct - expectedPct) > tolerancePct) {
			std::cerr << "lucky_hit: stochastic distribution drifted (observed="
			          << observedPct << "%, expected=" << expectedPct << "%)\n";
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
	const bool weightedShuffleBagSelectionOk = CheckWeightedShuffleBagLootRollSelection();
	const bool shuffleBagConstraintsOk = CheckShuffleBagSanitizeAndRollConstraints();
	const bool fixedWindowBudgetOk = CheckFixedWindowBudget();
	const bool recentlyLuckyOk = CheckRecentlyAndLuckyHitGuards();
	return (gateOk && storeOk && lootSelectionOk && shuffleBagSelectionOk && weightedShuffleBagSelectionOk &&
	        shuffleBagConstraintsOk && fixedWindowBudgetOk && recentlyLuckyOk) ? 0 :
	                                                                               1;
}
