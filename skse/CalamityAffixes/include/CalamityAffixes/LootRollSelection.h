#pragma once

#include "CalamityAffixes/InstanceAffixSlots.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <unordered_set>
#include <vector>

namespace CalamityAffixes::detail
{
	[[nodiscard]] constexpr std::uint8_t ResolveReforgeTargetAffixCount(std::uint8_t a_existingAffixCount) noexcept
	{
		constexpr std::uint8_t kMaxCount = static_cast<std::uint8_t>(kMaxAffixesPerItem);
		if (a_existingAffixCount == 0u) {
			return 1u;
		}

		return (a_existingAffixCount > kMaxCount) ? kMaxCount : a_existingAffixCount;
	}

	struct LootPrefixSuffixTargets
	{
		std::uint8_t prefixTarget{ 0u };
		std::uint8_t suffixTarget{ 0u };
	};

	[[nodiscard]] constexpr LootPrefixSuffixTargets DetermineLootPrefixSuffixTargets(std::uint8_t a_targetAffixCount) noexcept
	{
		// Soft-disable suffix drops for new loot rolls.
		// Keep suffix data/runtime handling for backward compatibility with existing items.
		constexpr std::uint8_t kMaxCount = static_cast<std::uint8_t>(kMaxAffixesPerItem);
		const std::uint8_t clamped = (a_targetAffixCount > kMaxCount) ? kMaxCount : a_targetAffixCount;
		return { clamped, 0u };
	}

	[[nodiscard]] constexpr bool ShouldConsumeSuffixRollForSingleAffixTarget(
		std::uint8_t a_targetAffixCount,
		std::uint8_t a_currentAssignedCount) noexcept
	{
		if (a_targetAffixCount != 1u) {
			return true;
		}

		return a_currentAssignedCount == 0u;
	}

	inline void SanitizeLootShuffleBagOrder(
		const std::vector<std::size_t>& a_pool,
		std::vector<std::size_t>& a_bag,
		std::size_t& a_cursor)
	{
		if (a_pool.empty()) {
			a_bag.clear();
			a_cursor = 0;
			return;
		}

		std::unordered_set<std::size_t> allowed;
		allowed.reserve(a_pool.size() * 2);
		for (const auto idx : a_pool) {
			allowed.insert(idx);
		}

		std::unordered_set<std::size_t> seen;
		seen.reserve(a_pool.size() * 2);
		std::vector<std::size_t> cleaned;
		cleaned.reserve(a_pool.size());

		for (const auto idx : a_bag) {
			if (!allowed.contains(idx)) {
				continue;
			}
			if (!seen.insert(idx).second) {
				continue;
			}
			cleaned.push_back(idx);
		}

		for (const auto idx : a_pool) {
			if (seen.insert(idx).second) {
				cleaned.push_back(idx);
			}
		}

		a_bag = std::move(cleaned);
		if (a_cursor > a_bag.size()) {
			a_cursor = a_bag.size();
		}
	}

	// Shared loot-roll picker used by both production and runtime checks.
	// Uniform across eligible entries; caller decides eligibility beforehand.
	template <class URBG>
	[[nodiscard]] inline std::optional<std::size_t> SelectUniformEligibleLootIndex(
		URBG& a_rng,
		const std::vector<std::size_t>& a_eligible)
	{
		if (a_eligible.empty()) {
			return std::nullopt;
		}
		if (a_eligible.size() == 1) {
			return a_eligible.front();
		}

		std::uniform_int_distribution<std::size_t> dist(0, a_eligible.size() - 1);
		return a_eligible[dist(a_rng)];
	}

	template <class URBG>
	inline void SyncLootShuffleBag(
		URBG& a_rng,
		const std::vector<std::size_t>& a_pool,
		std::vector<std::size_t>& a_bag,
		std::size_t& a_cursor)
	{
		if (a_bag.size() != a_pool.size()) {
			a_bag = a_pool;
			a_cursor = 0;
			if (a_bag.size() > 1) {
				std::shuffle(a_bag.begin(), a_bag.end(), a_rng);
			}
			return;
		}

		if (a_cursor > a_bag.size()) {
			a_cursor = a_bag.size();
		}
	}

	template <class URBG, class IsEligibleFn>
	[[nodiscard]] inline std::optional<std::size_t> SelectUniformEligibleLootIndexWithShuffleBag(
		URBG& a_rng,
		const std::vector<std::size_t>& a_pool,
		std::vector<std::size_t>& a_bag,
		std::size_t& a_cursor,
		IsEligibleFn&& a_isEligible)
	{
		if (a_pool.empty()) {
			return std::nullopt;
		}

		SyncLootShuffleBag(a_rng, a_pool, a_bag, a_cursor);
		if (a_bag.empty()) {
			return std::nullopt;
		}

		auto drawFromRemaining = [&]() -> std::optional<std::size_t> {
			if (a_cursor >= a_bag.size()) {
				return std::nullopt;
			}

			for (std::size_t pos = a_cursor; pos < a_bag.size(); ++pos) {
				if (!a_isEligible(a_bag[pos])) {
					continue;
				}
				if (pos != a_cursor) {
					std::swap(a_bag[pos], a_bag[a_cursor]);
				}
				return a_bag[a_cursor++];
			}

			return std::nullopt;
		};

		if (const auto picked = drawFromRemaining(); picked.has_value()) {
			return picked;
		}

		a_cursor = 0;
		if (a_bag.size() > 1) {
			std::shuffle(a_bag.begin(), a_bag.end(), a_rng);
		}
		return drawFromRemaining();
	}

	template <class URBG, class IsEligibleFn, class WeightFn>
	[[nodiscard]] inline std::optional<std::size_t> SelectWeightedEligibleLootIndexWithShuffleBag(
		URBG& a_rng,
		const std::vector<std::size_t>& a_pool,
		std::vector<std::size_t>& a_bag,
		std::size_t& a_cursor,
		IsEligibleFn&& a_isEligible,
		WeightFn&& a_weightOf)
	{
		if (a_pool.empty()) {
			return std::nullopt;
		}

		// Keep persisted shuffle-bag state structurally valid for serialization/backward compatibility,
		// but use true weighted sampling for each draw.
		SyncLootShuffleBag(a_rng, a_pool, a_bag, a_cursor);

		std::vector<std::size_t> eligible{};
		std::vector<std::size_t> weightedEligible{};
		std::vector<double> weightedValues{};
		eligible.reserve(a_pool.size());
		weightedEligible.reserve(a_pool.size());
		weightedValues.reserve(a_pool.size());

		for (const auto idx : a_pool) {
			if (!a_isEligible(idx)) {
				continue;
			}

			eligible.push_back(idx);
			const double weight = std::max(0.0, static_cast<double>(a_weightOf(idx)));
			if (weight > 0.0) {
				weightedEligible.push_back(idx);
				weightedValues.push_back(weight);
			}
		}

		if (eligible.empty()) {
			return std::nullopt;
		}

		// If all effective weights are 0, gracefully fall back to uniform among eligible entries.
		if (weightedEligible.empty()) {
			if (eligible.size() == 1) {
				return eligible.front();
			}
			std::uniform_int_distribution<std::size_t> dist(0, eligible.size() - 1);
			return eligible[dist(a_rng)];
		}

		if (weightedEligible.size() == 1) {
			return weightedEligible.front();
		}

		std::discrete_distribution<std::size_t> dist(weightedValues.begin(), weightedValues.end());
		return weightedEligible[dist(a_rng)];
	}
}
