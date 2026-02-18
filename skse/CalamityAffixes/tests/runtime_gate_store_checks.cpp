#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/LootSlotSanitizer.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "CalamityAffixes/NonHostileFirstHitGate.h"
#include "CalamityAffixes/PerTargetCooldownStore.h"
#include "CalamityAffixes/RunewordUiPolicy.h"
#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
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
			if (CalamityAffixes::detail::ResolveReforgeTargetAffixCount(0u) != 1u) {
				std::cerr << "reforge: zero-affix bases should reroll with one target slot\n";
				return false;
			}
			if (CalamityAffixes::detail::ResolveReforgeTargetAffixCount(7u) !=
				static_cast<std::uint8_t>(CalamityAffixes::kMaxRegularAffixesPerItem)) {
				std::cerr << "reforge: corrupted high affix counts should clamp to max slots\n";
				return false;
			}

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

	bool CheckLootSlotSanitizer()
	{
		CalamityAffixes::InstanceAffixSlots slots{};
		slots.count = 3u;
		slots.tokens[0] = 101u;
		slots.tokens[1] = 202u;
		slots.tokens[2] = 101u;  // duplicate should be removed.

		std::array<std::uint64_t, CalamityAffixes::kMaxAffixesPerItem> removed{};
		std::uint8_t removedCount = 0u;
		const auto sanitized = CalamityAffixes::detail::BuildSanitizedInstanceAffixSlots(
			slots,
			[](std::uint64_t token) { return token != 202u; },
			&removed,
			&removedCount);

		if (sanitized.count != 1u || sanitized.tokens[0] != 101u) {
			std::cerr << "slot_sanitize: expected only token 101 to remain\n";
			return false;
		}
		if (removedCount != 2u) {
			std::cerr << "slot_sanitize: expected two removed tokens (disallowed + duplicate)\n";
			return false;
		}

		CalamityAffixes::InstanceAffixSlots corrupted{};
		corrupted.count = 7u;  // out-of-range count should be clamped internally.
		corrupted.tokens[0] = 0u;
		corrupted.tokens[1] = 301u;
		corrupted.tokens[2] = 302u;
		const auto sanitizedCorrupted = CalamityAffixes::detail::BuildSanitizedInstanceAffixSlots(
			corrupted,
			[](std::uint64_t) { return true; });
		if (sanitizedCorrupted.count != 2u ||
			sanitizedCorrupted.tokens[0] != 301u ||
			sanitizedCorrupted.tokens[1] != 302u) {
			std::cerr << "slot_sanitize: expected clamp+zero-filter behavior for corrupted slot input\n";
			return false;
		}

		return true;
	}

	bool CheckRunewordTooltipOverlayPolicy()
	{
		if (CalamityAffixes::ShouldShowRunewordTooltipInItemOverlay()) {
			std::cerr << "tooltip_policy: runeword text must stay panel-only\n";
			return false;
		}

		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path runtimeTooltipFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runtime.cpp";

		std::ifstream in(runtimeTooltipFile);
		if (!in.is_open()) {
			std::cerr << "tooltip_policy: failed to open runtime tooltip source: " << runtimeTooltipFile << "\n";
			return false;
		}

		std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("BuildRunewordTooltip(") != std::string::npos) {
			std::cerr << "tooltip_policy: runtime tooltip source still references runeword tooltip builder\n";
			return false;
		}

		return true;
	}

	bool CheckLootPreviewRuntimePolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path assignFile = repoRoot / "src" / "EventBridge.Loot.Assign.cpp";
		const fs::path runtimeFile = repoRoot / "src" / "EventBridge.Loot.Runtime.cpp";
		const fs::path lootFile = repoRoot / "src" / "EventBridge.Loot.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto assignText = loadText(assignFile);
		if (!assignText.has_value()) {
			std::cerr << "loot_preview_policy: failed to open assign source: " << assignFile << "\n";
			return false;
		}
			if (assignText->find("no random affix assignment on pickup") == std::string::npos ||
				assignText->find("!a_allowRunewordFragmentRoll || a_count <= 0") == std::string::npos ||
				assignText->find("ResolveLootCurrencySourceTier(") == std::string::npos ||
				assignText->find("_loot.bossContainerEditorIdAllowContains") == std::string::npos ||
				assignText->find("_loot.lootSourceChanceMultBossContainer") == std::string::npos ||
				assignText->find("MaybeGrantRandomRunewordFragment(sourceChanceMultiplier);") == std::string::npos ||
				assignText->find("MaybeGrantRandomReforgeOrb(sourceChanceMultiplier);") == std::string::npos ||
				assignText->find("const std::uint32_t rollCount = 1u;") == std::string::npos ||
				assignText->find("allowLegacyPickupAffixRoll") != std::string::npos) {
			std::cerr << "loot_preview_policy: pickup flow must remain orb/fragment-only, include source weighting, and avoid legacy affix roll branch\n";
			return false;
		}

		const auto runtimeText = loadText(runtimeFile);
		if (!runtimeText.has_value()) {
			std::cerr << "loot_preview_policy: failed to open runtime source: " << runtimeFile << "\n";
			return false;
		}
		if (runtimeText->find("const bool allowPreview = false &&") == std::string::npos) {
			std::cerr << "loot_preview_policy: runtime tooltip must keep synthetic preview disabled\n";
			return false;
		}

		const auto lootText = loadText(lootFile);
		if (!lootText.has_value()) {
			std::cerr << "loot_preview_policy: failed to open loot event source: " << lootFile << "\n";
			return false;
		}
		if (lootText->find("ProcessLootAcquired(baseObj, count, uid, oldContainer, allowRunewordFragmentRoll)") == std::string::npos ||
			lootText->find("_loot.chancePercent <= 0.0f") != std::string::npos) {
			std::cerr << "loot_preview_policy: pickup event path must always dispatch loot processing independent of loot chance\n";
			return false;
		}

		return true;
	}

	bool CheckLootRerollExploitGuardPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path lootFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.cpp";

		std::ifstream in(lootFile);
		if (!in.is_open()) {
			std::cerr << "loot_reroll_exploit_guard: failed to open loot source: " << lootFile << "\n";
			return false;
		}

		std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("IsLootSourceCorpseChestOrWorld(") == std::string::npos ||
			source.find("_lootRerollGuard.ConsumeIfPlayerDropPickup(") == std::string::npos ||
			source.find("_playerContainerStash[stashKey] += a_event->itemCount;") == std::string::npos ||
			source.find("skipping loot roll (player dropped + re-picked)") == std::string::npos ||
			source.find("skipping loot roll (player stashed + retrieved)") == std::string::npos) {
			std::cerr << "loot_reroll_exploit_guard: anti-reroll pickup guards are missing\n";
			return false;
		}

		return true;
	}

	bool CheckLootChanceMcmCleanupPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path triggerEventsFile = repoRoot / "src" / "EventBridge.Triggers.Events.cpp";
		const fs::path configFile = repoRoot / "src" / "EventBridge.Config.cpp";
		const fs::path headerFile = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto headerText = loadText(headerFile);
		if (!headerText.has_value()) {
			std::cerr << "loot_chance_mcm_cleanup: failed to open header file: " << headerFile << "\n";
			return false;
		}
		if (headerText->find("kMcmSetLootChanceEvent") != std::string::npos) {
			std::cerr << "loot_chance_mcm_cleanup: legacy loot-chance MCM event constant still exists in EventBridge.h\n";
			return false;
		}

		const auto triggerText = loadText(triggerEventsFile);
		if (!triggerText.has_value()) {
			std::cerr << "loot_chance_mcm_cleanup: failed to open trigger events source: " << triggerEventsFile << "\n";
			return false;
		}
		if (triggerText->find("eventName == kMcmSetLootChanceEvent") != std::string::npos ||
			triggerText->find("PersistLootChancePercentToMcmSettings(chancePct, true)") != std::string::npos) {
			std::cerr << "loot_chance_mcm_cleanup: trigger source still handles deprecated loot-chance MCM event\n";
			return false;
		}

		const auto configText = loadText(configFile);
		if (!configText.has_value()) {
			std::cerr << "loot_chance_mcm_cleanup: failed to open config source: " << configFile << "\n";
			return false;
		}
		if (configText->find("LoadLootChancePercentFromMcmSettings()") != std::string::npos ||
			configText->find("PersistLootChancePercentToMcmSettings(") != std::string::npos) {
			std::cerr << "loot_chance_mcm_cleanup: config source still contains deprecated loot-chance MCM sync helpers\n";
			return false;
		}

		return true;
	}

		bool CheckRunewordCompletedSelectionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path baseSelectionFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.BaseSelection.cpp";
			const fs::path recipeEntriesFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeEntries.cpp";
			const fs::path recipeUiFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeUi.cpp";

			std::ifstream recipeEntriesIn(recipeEntriesFile);
			if (!recipeEntriesIn.is_open()) {
				std::cerr << "runeword_completed_selection: failed to open source file: " << recipeEntriesFile << "\n";
				return false;
			}

			std::string recipeEntriesSource(
				(std::istreambuf_iterator<char>(recipeEntriesIn)),
				std::istreambuf_iterator<char>());

			std::ifstream recipeUiIn(recipeUiFile);
			if (!recipeUiIn.is_open()) {
				std::cerr << "runeword_completed_selection: failed to open source file: " << recipeUiFile << "\n";
				return false;
			}

			std::string recipeUiSource(
				(std::istreambuf_iterator<char>(recipeUiIn)),
				std::istreambuf_iterator<char>());

			std::ifstream baseIn(baseSelectionFile);
			if (!baseIn.is_open()) {
				std::cerr << "runeword_completed_selection: failed to open source file: " << baseSelectionFile << "\n";
				return false;
			}

			std::string baseSource(
				(std::istreambuf_iterator<char>(baseIn)),
				std::istreambuf_iterator<char>());

			// Completed runeword base must pin recipe highlight to completed recipe token.
			if (recipeEntriesSource.find("bool resolvedFromCompleted = false;") == std::string::npos ||
				recipeEntriesSource.find("selectedToken = completed->token;") == std::string::npos ||
				recipeEntriesSource.find("if (!resolvedFromCompleted)") == std::string::npos) {
				std::cerr << "runeword_completed_selection: completed recipe highlight guard is missing\n";
				return false;
			}

			// Completed runeword base must not persist mutable in-progress recipe state.
			if (recipeUiSource.find("const bool completedBase = ResolveCompletedRunewordRecipe(selectedKey) != nullptr;") ==
				    std::string::npos ||
				recipeUiSource.find("ShouldClearRunewordInProgressState(completedBase)") == std::string::npos ||
				recipeUiSource.find("_runewordInstanceStates.erase(selectedKey);") == std::string::npos) {
				std::cerr << "runeword_completed_selection: completed-base state write guard is missing\n";
				return false;
			}

			// Base selection should also clear mutable in-progress state for completed runeword bases.
			if (baseSource.find("if (completedRecipe)") == std::string::npos ||
				baseSource.find("_runewordInstanceStates.erase(a_instanceKey);") == std::string::npos) {
				std::cerr << "runeword_completed_selection: base-selection completed-state guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordRecipeEntriesMappingPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path recipeEntriesFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeEntries.cpp";

			std::ifstream recipeEntriesIn(recipeEntriesFile);
			if (!recipeEntriesIn.is_open()) {
				std::cerr << "runeword_recipe_entries_mapping: failed to open source file: " << recipeEntriesFile << "\n";
				return false;
			}

			std::string source(
				(std::istreambuf_iterator<char>(recipeEntriesIn)),
				std::istreambuf_iterator<char>());

			if (source.find("kRecommendedBaseOverrides") == std::string::npos ||
				source.find("kEffectSummaryOverrides") == std::string::npos ||
				source.find("FindKeyOverride(id, kRecommendedBaseOverrides)") == std::string::npos ||
				source.find("FindKeyOverride(id, kEffectSummaryOverrides)") == std::string::npos ||
				source.find("HasDuplicateOverrideIds(") == std::string::npos ||
				source.find("static_assert(!HasDuplicateOverrideIds(kRecommendedBaseOverrides)") == std::string::npos ||
				source.find("static_assert(!HasDuplicateOverrideIds(kEffectSummaryOverrides)") == std::string::npos ||
				source.find("{ \"rw_spirit\", \"sword_shield\" }") == std::string::npos ||
				source.find("{ \"rw_spirit\", \"self_meditation\" }") == std::string::npos) {
				std::cerr << "runeword_recipe_entries_mapping: recipe-entry table mapping guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordRecipeRuntimeEligibilityPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path recipeEntriesFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeEntries.cpp";
			const fs::path recipeUiFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeUi.cpp";
			const fs::path catalogFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.Catalog.cpp";
			const fs::path baseSelectionFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.BaseSelection.cpp";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto recipeEntriesText = loadText(recipeEntriesFile);
			if (!recipeEntriesText.has_value()) {
				std::cerr << "runeword_recipe_runtime_eligibility: failed to open source file: " << recipeEntriesFile << "\n";
				return false;
			}
			const auto recipeUiText = loadText(recipeUiFile);
			if (!recipeUiText.has_value()) {
				std::cerr << "runeword_recipe_runtime_eligibility: failed to open source file: " << recipeUiFile << "\n";
				return false;
			}
			const auto catalogText = loadText(catalogFile);
			if (!catalogText.has_value()) {
				std::cerr << "runeword_recipe_runtime_eligibility: failed to open source file: " << catalogFile << "\n";
				return false;
			}
			const auto baseSelectionText = loadText(baseSelectionFile);
			if (!baseSelectionText.has_value()) {
				std::cerr << "runeword_recipe_runtime_eligibility: failed to open source file: " << baseSelectionFile << "\n";
				return false;
			}

			const auto recipeUiGuardPos = recipeUiText->find("Runeword Recipe: runtime effect not available.");
			const auto recipeUiCursorUpdatePos = recipeUiText->find("_runewordRecipeCycleCursor = static_cast<std::uint32_t>(idx);");

			if (recipeEntriesText->find("if (const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);") == std::string::npos ||
				recipeEntriesText->find("affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size())") == std::string::npos ||
				recipeUiText->find("Runeword Recipe: runtime effect not available.") == std::string::npos ||
				recipeUiGuardPos == std::string::npos ||
				recipeUiCursorUpdatePos == std::string::npos ||
				recipeUiCursorUpdatePos < recipeUiGuardPos ||
				catalogText->find("const auto isEligible = [&](std::size_t a_idx)") == std::string::npos ||
				catalogText->find("if (!isEligible(cursor))") == std::string::npos ||
				catalogText->find("if (const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);") == std::string::npos ||
				baseSelectionText->find("const auto rwIt = _runewordRecipeIndexByResultAffixToken.find(token);") == std::string::npos ||
				baseSelectionText->find("if (const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);") == std::string::npos ||
				baseSelectionText->find("return std::addressof(recipe);") == std::string::npos) {
				std::cerr << "runeword_recipe_runtime_eligibility: unsupported recipe guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckSynthesizedAffixDisplayNameFallbackPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path synthesisFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.RunewordSynthesis.cpp";

			std::ifstream in(synthesisFile);
			if (!in.is_open()) {
				std::cerr << "synthesized_affix_displayname_fallback: failed to open source file: " << synthesisFile << "\n";
				return false;
			}

			std::string source(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());

			if (source.find("if (a_affix.displayName.empty()) {") == std::string::npos ||
				source.find("if (a_affix.displayNameEn.empty()) {") == std::string::npos ||
				source.find("if (a_affix.displayNameKo.empty()) {") == std::string::npos ||
				source.find("a_affix.displayNameEn = a_affix.displayName;") == std::string::npos ||
				source.find("a_affix.displayNameKo = a_affix.displayName;") == std::string::npos) {
				std::cerr << "synthesized_affix_displayname_fallback: synthesized affix tooltip-name fallback guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordTransmuteSafetyPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path recipeUiFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeUi.cpp";
			const fs::path panelStateFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.PanelState.cpp";
			const fs::path transmuteFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.Transmute.cpp";
			const fs::path policyFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.Policy.cpp";

			std::ifstream recipeUiIn(recipeUiFile);
			if (!recipeUiIn.is_open()) {
				std::cerr << "runeword_transmute_safety: failed to open recipe-ui source: " << recipeUiFile << "\n";
				return false;
			}
			std::string recipeUiSource(
				(std::istreambuf_iterator<char>(recipeUiIn)),
				std::istreambuf_iterator<char>());

			if (recipeUiSource.find("const bool completedBase = ResolveCompletedRunewordRecipe(selectedKey) != nullptr;") ==
				    std::string::npos ||
				recipeUiSource.find("_runewordInstanceStates.erase(selectedKey);") == std::string::npos) {
				std::cerr << "runeword_transmute_safety: completed-base recipe selection state guard is missing\n";
				return false;
			}

			std::ifstream panelIn(panelStateFile);
			if (!panelIn.is_open()) {
				std::cerr << "runeword_transmute_safety: failed to open panel-state source: " << panelStateFile << "\n";
				return false;
			}
			std::string panelSource(
				(std::istreambuf_iterator<char>(panelIn)),
				std::istreambuf_iterator<char>());

			if (panelSource.find("ResolveRunewordApplyBlockReason(*_runewordSelectedBaseKey, *recipe)") == std::string::npos ||
				panelSource.find("BuildRunewordApplyBlockMessage(applyBlockReason)") == std::string::npos ||
				panelSource.find("CanFinalizeRunewordFromPanel(") == std::string::npos ||
				panelSource.find("CanInsertRunewordFromPanel(") == std::string::npos) {
				std::cerr << "runeword_transmute_safety: panel canInsert/apply safety guard is missing\n";
				return false;
			}

			std::ifstream policyIn(policyFile);
			if (!policyIn.is_open()) {
				std::cerr << "runeword_transmute_safety: failed to open policy source: " << policyFile << "\n";
				return false;
			}
			std::string policySource(
				(std::istreambuf_iterator<char>(policyIn)),
				std::istreambuf_iterator<char>());

			if (policySource.find("Runeword result affix missing") == std::string::npos ||
				policySource.find("Affix slots full (max ") == std::string::npos) {
				std::cerr << "runeword_transmute_safety: policy messages for apply block reasons are missing\n";
				return false;
			}

			std::ifstream transmuteIn(transmuteFile);
			if (!transmuteIn.is_open()) {
				std::cerr << "runeword_transmute_safety: failed to open transmute source: " << transmuteFile << "\n";
				return false;
			}
			std::string transmuteSource(
				(std::istreambuf_iterator<char>(transmuteIn)),
				std::istreambuf_iterator<char>());

			if (transmuteSource.find("ResolveRunewordApplyBlockReason(a_instanceKey, a_recipe)") == std::string::npos ||
				transmuteSource.find("ResolveRunewordApplyBlockReason(*_runewordSelectedBaseKey, *recipe)") == std::string::npos ||
				transmuteSource.find("runeword result affix missing before transmute") == std::string::npos ||
				transmuteSource.find("note.append(BuildRunewordApplyBlockMessage(blockReason));") == std::string::npos) {
				std::cerr << "runeword_transmute_safety: transmute pre-consume safety guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordUiPolicyHelpers()
		{
			if (!CalamityAffixes::ShouldClearRunewordInProgressState(true) ||
				CalamityAffixes::ShouldClearRunewordInProgressState(false)) {
				std::cerr << "runeword_ui_policy: completed-base clear policy helper mismatch\n";
				return false;
			}

			if (!CalamityAffixes::CanFinalizeRunewordFromPanel(3u, 3u, true) ||
				CalamityAffixes::CanFinalizeRunewordFromPanel(2u, 3u, true) ||
				CalamityAffixes::CanFinalizeRunewordFromPanel(3u, 3u, false)) {
				std::cerr << "runeword_ui_policy: finalize helper mismatch\n";
				return false;
			}

			if (!CalamityAffixes::CanInsertRunewordFromPanel(true, true) ||
				CalamityAffixes::CanInsertRunewordFromPanel(true, false) ||
				CalamityAffixes::CanInsertRunewordFromPanel(false, true)) {
				std::cerr << "runeword_ui_policy: can-insert helper mismatch\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordReforgeSafetyPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path reforgeFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.Reforge.cpp";

			std::ifstream reforgeIn(reforgeFile);
			if (!reforgeIn.is_open()) {
				std::cerr << "runeword_reforge_safety: failed to open reforge source: " << reforgeFile << "\n";
				return false;
			}
			std::string reforgeSource(
				(std::istreambuf_iterator<char>(reforgeIn)),
				std::istreambuf_iterator<char>());

			if (reforgeSource.find("const bool rerollRuneword = completedRunewordRecipe != nullptr;") == std::string::npos ||
				reforgeSource.find("pickReforgeRunewordRecipe") == std::string::npos ||
				reforgeSource.find("rolled.AddToken(attemptRunewordRecipe->resultAffixToken);") == std::string::npos ||
				reforgeSource.find("kMaxRegularAffixesPerItem") == std::string::npos ||
				reforgeSource.find("_runewordInstanceStates.erase(instanceKey);") == std::string::npos ||
				reforgeSource.find("IsLootObjectEligibleForAffixes(entry->object)") == std::string::npos ||
				reforgeSource.find("Reforge blocked: completed runeword base.") != std::string::npos) {
				std::cerr << "runeword_reforge_safety: completed-base reroll integration guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaTooltipImmediateRefreshPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path prismaHandleFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.HandleUiCommand.inl";
			const fs::path prismaCoreFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.cpp";
			const fs::path runtimeTooltipFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runtime.cpp";
			const fs::path runewordPanelStateFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.PanelState.cpp";
			const fs::path prismaUiFile = testFile.parent_path().parent_path().parent_path().parent_path() /
				"Data" / "PrismaUI" / "views" / "CalamityAffixes" / "index.html";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto handleText = loadText(prismaHandleFile);
			if (!handleText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open handle source: " << prismaHandleFile << "\n";
				return false;
			}
			const auto coreText = loadText(prismaCoreFile);
			if (!coreText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open prisma source: " << prismaCoreFile << "\n";
				return false;
			}
			const auto uiText = loadText(prismaUiFile);
			if (!uiText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open ui source: " << prismaUiFile << "\n";
				return false;
			}
			const auto runtimeText = loadText(runtimeTooltipFile);
			if (!runtimeText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open runtime source: " << runtimeTooltipFile << "\n";
				return false;
			}
			const auto panelStateText = loadText(runewordPanelStateFile);
			if (!panelStateText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open runeword panel state source: " << runewordPanelStateFile << "\n";
				return false;
			}

			if (coreText->find("PushSelectedTooltipSnapshot(") == std::string::npos ||
				handleText->find("PushSelectedTooltipSnapshot(true);") == std::string::npos ||
				coreText->find("const bool selectionChanged =") == std::string::npos ||
				coreText->find("if (a_force || selectionChanged || !g_lastTooltip.empty())") == std::string::npos ||
				coreText->find("if (a_force || selectionChanged || next != g_lastTooltip)") == std::string::npos ||
				coreText->find("setRunewordAffixPreview") == std::string::npos ||
				handleText->find("SetRunewordAffixPreview(") == std::string::npos ||
				uiText->find("PrismaUI_Interop(\"setRunewordAffixPreview\"") == std::string::npos ||
				uiText->find("function setRunewordAffixPreview(raw)") == std::string::npos ||
				runtimeText->find("std::uint64_t a_preferredInstanceKey") == std::string::npos ||
				runtimeText->find("if (a_preferredInstanceKey != 0u)") == std::string::npos ||
				runtimeText->find("a_candidate.instanceKey == a_preferredInstanceKey") == std::string::npos ||
				panelStateText->find("GetInstanceAffixTooltip(") == std::string::npos ||
				panelStateText->find("*_runewordSelectedBaseKey") == std::string::npos) {
				std::cerr << "prisma_tooltip_refresh: immediate tooltip refresh guard is missing for runeword actions\n";
				return false;
			}

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
	const bool slotSanitizerOk = CheckLootSlotSanitizer();
	const bool fixedWindowBudgetOk = CheckFixedWindowBudget();
	const bool recentlyLuckyOk = CheckRecentlyAndLuckyHitGuards();
	const bool tooltipPolicyOk = CheckRunewordTooltipOverlayPolicy();
	const bool lootPreviewPolicyOk = CheckLootPreviewRuntimePolicy();
	const bool lootRerollExploitGuardOk = CheckLootRerollExploitGuardPolicy();
	const bool lootChanceMcmCleanupOk = CheckLootChanceMcmCleanupPolicy();
	const bool runewordCompletedSelectionOk = CheckRunewordCompletedSelectionPolicy();
	const bool runewordRecipeEntriesMappingOk = CheckRunewordRecipeEntriesMappingPolicy();
	const bool runewordRecipeRuntimeEligibilityOk = CheckRunewordRecipeRuntimeEligibilityPolicy();
	const bool synthesizedAffixDisplayNameFallbackOk = CheckSynthesizedAffixDisplayNameFallbackPolicy();
	const bool runewordUiPolicyHelpersOk = CheckRunewordUiPolicyHelpers();
	const bool runewordTransmuteSafetyOk = CheckRunewordTransmuteSafetyPolicy();
	const bool runewordReforgeSafetyOk = CheckRunewordReforgeSafetyPolicy();
	const bool prismaTooltipImmediateRefreshOk = CheckPrismaTooltipImmediateRefreshPolicy();
	return (gateOk && storeOk && lootSelectionOk && shuffleBagSelectionOk && weightedShuffleBagSelectionOk &&
	        shuffleBagConstraintsOk && slotSanitizerOk && fixedWindowBudgetOk && recentlyLuckyOk && tooltipPolicyOk &&
	        lootPreviewPolicyOk &&
	        lootRerollExploitGuardOk &&
	        lootChanceMcmCleanupOk && runewordCompletedSelectionOk && runewordRecipeEntriesMappingOk &&
	        runewordRecipeRuntimeEligibilityOk &&
	        synthesizedAffixDisplayNameFallbackOk &&
	        runewordUiPolicyHelpersOk &&
	        runewordTransmuteSafetyOk &&
	        runewordReforgeSafetyOk &&
	        prismaTooltipImmediateRefreshOk) ? 0 :
	                                                             1;
}
