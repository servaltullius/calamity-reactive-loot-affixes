// Affix-count roll distribution and reforge target checks.
//
// These previously lived in tests/test_loot_roll_distribution.cpp, which ctest
// only *built* (the target is cross-compiled to a Windows PE and was registered
// with `cmake --build` as its test command), so none of the assertions below had
// ever executed.  They also simulated the roll with a private copy of the weight
// table instead of calling the shipping code.
//
// Both problems are fixed here: this translation unit runs in the host runtime
// gate, and it drives the same detail::RollAffixCountFromUnit that the loot
// preview path and EventBridge::RollAffixCount now share.

#include "runtime_gate_store_checks_common.h"

namespace RuntimeGateStoreChecks
{
	namespace
	{
		using CalamityAffixes::InstanceAffixSlots;
		using CalamityAffixes::kMaxRegularAffixesPerItem;
		using CalamityAffixes::detail::DetermineLootPrefixSuffixTargets;
		using CalamityAffixes::detail::kAffixCountWeights;
		using CalamityAffixes::detail::ResolveReforgeTargetAffixCount;
		using CalamityAffixes::detail::RollAffixCountFromUnit;
	}

	bool CheckAffixCountRollDistribution()
	{
		constexpr std::uint32_t kTrials = 100'000u;
		constexpr double kTolerance = 0.02;  // 2% absolute, ample for 100k trials

		// Drives the shipping roll helper, not a local reimplementation, so a
		// change to either the weights or the bucket walk shows up here.
		std::mt19937 rng{ 42u };
		std::uniform_real_distribution<double> unit(0.0, 1.0);
		std::array<std::uint32_t, kMaxRegularAffixesPerItem> counts{};

		for (std::uint32_t i = 0; i < kTrials; ++i) {
			const auto rolled = RollAffixCountFromUnit(unit(rng));
			if (rolled < 1u || rolled > static_cast<std::uint8_t>(kMaxRegularAffixesPerItem)) {
				std::cerr << "affix_count_distribution: rolled out-of-range count " << static_cast<unsigned>(rolled) << "\n";
				return false;
			}
			counts[static_cast<std::size_t>(rolled) - 1u] += 1u;
		}

		// Expected ratios follow directly from the shared weight table rather
		// than from hard-coded percentages, so rebalancing the weights adjusts
		// the expectation instead of silently invalidating the test.
		double totalWeight = 0.0;
		for (const auto weight : kAffixCountWeights) {
			totalWeight += static_cast<double>(weight);
		}
		if (totalWeight <= 0.0) {
			std::cerr << "affix_count_distribution: shared weight table sums to zero\n";
			return false;
		}

		bool allPassed = true;
		const double total = static_cast<double>(kTrials);
		for (std::size_t i = 0; i < kMaxRegularAffixesPerItem; ++i) {
			const double expected = static_cast<double>(kAffixCountWeights[i]) / totalWeight;
			const double actual = static_cast<double>(counts[i]) / total;
			const double deviation = std::abs(actual - expected);
			if (deviation > kTolerance) {
				std::cerr << "affix_count_distribution: bucket " << (i + 1u)
						  << " expected " << expected << " got " << actual
						  << " (deviation " << deviation << " > " << kTolerance << ")\n";
				allPassed = false;
			}
		}

		return allPassed;
	}

	bool CheckAffixCountRollUnitBounds()
	{
		// The helper is total over the closed unit interval and never escapes
		// [1, kMaxRegularAffixesPerItem], including for out-of-range input.
		constexpr std::array<double, 7> kSamples{ -1.0, 0.0, 0.25, 0.5, 0.75, 1.0, 2.0 };
		for (const auto sample : kSamples) {
			const auto rolled = RollAffixCountFromUnit(sample);
			if (rolled < 1u || rolled > static_cast<std::uint8_t>(kMaxRegularAffixesPerItem)) {
				std::cerr << "affix_count_unit_bounds: sample " << sample
						  << " produced out-of-range count " << static_cast<unsigned>(rolled) << "\n";
				return false;
			}
		}

		// Lowest and highest samples must land on the first and last buckets for
		// the currently shipped weights (all three are positive).
		if (RollAffixCountFromUnit(0.0) != 1u) {
			std::cerr << "affix_count_unit_bounds: unit 0.0 did not select the first bucket\n";
			return false;
		}
		if (RollAffixCountFromUnit(1.0) != static_cast<std::uint8_t>(kMaxRegularAffixesPerItem)) {
			std::cerr << "affix_count_unit_bounds: unit 1.0 did not select the last bucket\n";
			return false;
		}

		return true;
	}

	bool CheckReforgeTargetAffixCountBounds()
	{
		constexpr auto kMaxCount = static_cast<std::uint8_t>(kMaxRegularAffixesPerItem);
		for (std::uint8_t n = 0u; n <= 5u; ++n) {
			const auto target = ResolveReforgeTargetAffixCount(n);
			const std::uint8_t expected = (n == 0u) ? 1u : ((n > kMaxCount) ? kMaxCount : n);
			if (target != expected) {
				std::cerr << "reforge_target_bounds: ResolveReforgeTargetAffixCount(" << static_cast<unsigned>(n)
						  << ") = " << static_cast<unsigned>(target)
						  << ", expected " << static_cast<unsigned>(expected) << "\n";
				return false;
			}
		}

		return true;
	}

	bool CheckReforgeTargetWithRunewordSlots()
	{
		// Runeword occupies its own slot; reforge targets the regular affixes
		// that remain once the runeword token is removed.
		constexpr std::uint64_t kRunewordToken = 0x1000u;
		constexpr std::uint64_t kRegularA = 0x2000u;
		constexpr std::uint64_t kRegularB = 0x3000u;

		InstanceAffixSlots slots{};
		slots.AddToken(kRunewordToken);
		slots.AddToken(kRegularA);
		slots.AddToken(kRegularB);
		if (slots.count != 3u) {
			std::cerr << "reforge_runeword_slots: expected 3 tokens, got " << static_cast<unsigned>(slots.count) << "\n";
			return false;
		}

		InstanceAffixSlots regularSlots = slots;
		regularSlots.RemoveToken(kRunewordToken);
		if (regularSlots.count != 2u) {
			std::cerr << "reforge_runeword_slots: expected 2 regular tokens after removal, got "
					  << static_cast<unsigned>(regularSlots.count) << "\n";
			return false;
		}

		const auto target = ResolveReforgeTargetAffixCount(regularSlots.count);
		if (target != 2u) {
			std::cerr << "reforge_runeword_slots: target = " << static_cast<unsigned>(target) << ", expected 2\n";
			return false;
		}

		const auto targets = DetermineLootPrefixSuffixTargets(target);
		if (targets.prefixTarget != 1u || targets.suffixTarget != 1u) {
			std::cerr << "reforge_runeword_slots: DetermineLootPrefixSuffixTargets(2) = { prefix="
					  << static_cast<unsigned>(targets.prefixTarget) << ", suffix="
					  << static_cast<unsigned>(targets.suffixTarget) << " }, expected { 1, 1 }\n";
			return false;
		}

		// Runeword + a single regular affix collapses to the 1-affix case.
		InstanceAffixSlots singleRegular{};
		singleRegular.AddToken(kRunewordToken);
		singleRegular.AddToken(kRegularA);
		singleRegular.RemoveToken(kRunewordToken);
		if (singleRegular.count != 1u) {
			std::cerr << "reforge_runeword_slots: expected 1 regular token, got "
					  << static_cast<unsigned>(singleRegular.count) << "\n";
			return false;
		}
		if (ResolveReforgeTargetAffixCount(singleRegular.count) != 1u) {
			std::cerr << "reforge_runeword_slots: single-regular target did not collapse to 1\n";
			return false;
		}

		return true;
	}

	bool CheckAffixCountWeightsAllowMultiAffix()
	{
		// Guards the balance intent: multi-affix drops must stay reachable.
		// A rebalance that drives P(2+) below 20% is a deliberate design change
		// and should have to update this threshold explicitly.
		double totalWeight = 0.0;
		for (const auto weight : kAffixCountWeights) {
			if (weight < 0.0f) {
				std::cerr << "affix_count_weights: negative weight in shared table\n";
				return false;
			}
			totalWeight += static_cast<double>(weight);
		}
		if (totalWeight <= 0.0) {
			std::cerr << "affix_count_weights: shared weight table sums to zero\n";
			return false;
		}

		double weightMultiAffix = 0.0;
		for (std::size_t i = 1u; i < kAffixCountWeights.size(); ++i) {
			weightMultiAffix += static_cast<double>(kAffixCountWeights[i]);
		}

		const double ratioMultiAffix = weightMultiAffix / totalWeight;
		if (ratioMultiAffix < 0.20) {
			std::cerr << "affix_count_weights: P(2+ affixes) = " << ratioMultiAffix << ", expected >= 0.20\n";
			return false;
		}

		return true;
	}
}
