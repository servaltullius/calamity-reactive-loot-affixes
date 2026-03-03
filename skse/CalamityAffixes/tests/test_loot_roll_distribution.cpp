// Runtime test: verify affix count rolling distribution and reforge helper behavior.
// Tests that kAffixCountWeights { 70, 22, 8 } produces correct multi-affix ratios.
// Note: reforge now re-rolls affix count via RollAffixCount() (same 70/22/8% distribution),
// NOT preserving the existing count.  ResolveReforgeTargetAffixCount is kept as a utility
// but is no longer called from the reforge path.

#include "CalamityAffixes/InstanceAffixSlots.h"
#include "CalamityAffixes/LootRollSelection.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>

using CalamityAffixes::InstanceAffixSlots;
using CalamityAffixes::detail::DetermineLootPrefixSuffixTargets;
using CalamityAffixes::detail::ResolveReforgeTargetAffixCount;

static constexpr std::size_t kMaxRegular = CalamityAffixes::kMaxRegularAffixesPerItem;

// Simulate the same rolling logic used in BuildLootPreviewAffixSlots / RollAffixCount.
static constexpr std::array<float, kMaxRegular> kAffixCountWeights = { 70.0f, 22.0f, 8.0f };

static bool TestAffixCountDistribution()
{
	constexpr std::uint32_t kTrials = 100'000u;
	constexpr double kTolerance = 0.02; // 2% tolerance for statistical test

	std::mt19937 rng{ 42u };
	std::array<std::uint32_t, kMaxRegular> counts{};

	// Build weights vector (same as runtime code)
	std::vector<double> weights;
	for (const auto w : kAffixCountWeights) {
		weights.push_back(static_cast<double>(w));
	}

	std::discrete_distribution<int> dist(weights.begin(), weights.end());

	for (std::uint32_t i = 0; i < kTrials; ++i) {
		const int rolled = dist(rng);
		if (rolled < 0 || rolled >= static_cast<int>(kMaxRegular)) {
			std::fprintf(stderr, "FAIL: rolled out-of-range index %d\n", rolled);
			return false;
		}
		counts[static_cast<std::size_t>(rolled)] += 1;
	}

	// Expected ratios: 70%, 22%, 8%
	const double total = static_cast<double>(kTrials);
	const std::array<double, kMaxRegular> expectedRatios = { 0.70, 0.22, 0.08 };
	const char* labels[] = { "1-affix", "2-affix", "3-affix" };

	bool allPassed = true;
	for (std::size_t i = 0; i < kMaxRegular; ++i) {
		const double actual = static_cast<double>(counts[i]) / total;
		const double deviation = std::abs(actual - expectedRatios[i]);

		std::fprintf(stderr, "  %s: expected=%.2f%% actual=%.2f%% (n=%u) deviation=%.3f\n",
			labels[i],
			expectedRatios[i] * 100.0,
			actual * 100.0,
			counts[i],
			deviation);

		if (deviation > kTolerance) {
			std::fprintf(stderr, "FAIL: %s deviation %.3f exceeds tolerance %.3f\n",
				labels[i], deviation, kTolerance);
			allPassed = false;
		}
	}

	return allPassed;
}

static bool TestReforgePreservesMultiAffixCount()
{
	bool allPassed = true;

	// Test: reforge of item with N regular affixes should target N
	for (std::uint8_t n = 0; n <= 5; ++n) {
		const auto target = ResolveReforgeTargetAffixCount(n);
		const std::uint8_t expected = (n == 0u) ? 1u :
			(n > static_cast<std::uint8_t>(kMaxRegular)) ? static_cast<std::uint8_t>(kMaxRegular) : n;

		if (target != expected) {
			std::fprintf(stderr, "FAIL: ResolveReforgeTargetAffixCount(%u) = %u, expected %u\n",
				static_cast<unsigned>(n), static_cast<unsigned>(target), static_cast<unsigned>(expected));
			allPassed = false;
		}
	}

	return allPassed;
}

static bool TestReforgeWithRuneword()
{
	// Simulate: item has runeword(slot[0]) + 2 regular affixes = count 3
	InstanceAffixSlots slots{};
	const std::uint64_t runewordToken = 0x1000u;
	const std::uint64_t regularA = 0x2000u;
	const std::uint64_t regularB = 0x3000u;

	slots.AddToken(runewordToken);
	slots.AddToken(regularA);
	slots.AddToken(regularB);

	if (slots.count != 3) {
		std::fprintf(stderr, "FAIL: initial slots.count = %u, expected 3\n", static_cast<unsigned>(slots.count));
		return false;
	}

	// Remove runeword to get regular-only snapshot (same as reforge code line 79-81)
	InstanceAffixSlots regularSlots = slots;
	regularSlots.RemoveToken(runewordToken);

	if (regularSlots.count != 2) {
		std::fprintf(stderr, "FAIL: regularSlots.count = %u after RemoveToken, expected 2\n",
			static_cast<unsigned>(regularSlots.count));
		return false;
	}

	// Reforge target should be 2 (preserve regular count)
	const auto target = ResolveReforgeTargetAffixCount(regularSlots.count);
	if (target != 2u) {
		std::fprintf(stderr, "FAIL: ResolveReforgeTargetAffixCount(%u) = %u, expected 2\n",
			static_cast<unsigned>(regularSlots.count), static_cast<unsigned>(target));
		return false;
	}

	// DetermineLootPrefixSuffixTargets should give 2 prefixes, 0 suffixes
	const auto targets = DetermineLootPrefixSuffixTargets(target);
	if (targets.prefixTarget != 2u || targets.suffixTarget != 0u) {
		std::fprintf(stderr, "FAIL: DetermineLootPrefixSuffixTargets(%u) = { prefix=%u, suffix=%u }, expected { 2, 0 }\n",
			static_cast<unsigned>(target),
			static_cast<unsigned>(targets.prefixTarget),
			static_cast<unsigned>(targets.suffixTarget));
		return false;
	}

	std::fprintf(stderr, "  runeword+2 regular: target=%u prefix=%u suffix=%u -> OK\n",
		static_cast<unsigned>(target),
		static_cast<unsigned>(targets.prefixTarget),
		static_cast<unsigned>(targets.suffixTarget));
	return true;
}

static bool TestReforgeWithRunewordOnlyOneRegular()
{
	// Simulate: item has runeword(slot[0]) + 1 regular affix = count 2
	InstanceAffixSlots slots{};
	slots.AddToken(0x1000u); // runeword
	slots.AddToken(0x2000u); // regular

	InstanceAffixSlots regularSlots = slots;
	regularSlots.RemoveToken(0x1000u);

	if (regularSlots.count != 1u) {
		std::fprintf(stderr, "FAIL: regularSlots.count = %u, expected 1\n",
			static_cast<unsigned>(regularSlots.count));
		return false;
	}

	const auto target = ResolveReforgeTargetAffixCount(regularSlots.count);
	if (target != 1u) {
		std::fprintf(stderr, "FAIL: target = %u, expected 1\n", static_cast<unsigned>(target));
		return false;
	}

	std::fprintf(stderr, "  runeword+1 regular: target=%u -> OK (this is the '1 affix' case)\n",
		static_cast<unsigned>(target));
	return true;
}

static bool TestNewLootCanRollMultiple()
{
	// Verify kAffixCountWeights supports multi-affix
	double totalWeight = 0.0;
	for (const auto w : kAffixCountWeights) {
		totalWeight += static_cast<double>(w);
	}

	const double weight2Plus = static_cast<double>(kAffixCountWeights[1] + kAffixCountWeights[2]);
	const double ratio2Plus = weight2Plus / totalWeight;

	std::fprintf(stderr, "  kAffixCountWeights = { %.0f, %.0f, %.0f }\n",
		kAffixCountWeights[0], kAffixCountWeights[1], kAffixCountWeights[2]);
	std::fprintf(stderr, "  P(2+ affixes) = %.1f%%\n", ratio2Plus * 100.0);

	if (ratio2Plus < 0.20) {
		std::fprintf(stderr, "FAIL: P(2+ affixes) = %.1f%%, expected >= 20%%\n", ratio2Plus * 100.0);
		return false;
	}

	return true;
}

int main()
{
	int failures = 0;

	std::fprintf(stderr, "\n=== Test: Affix Count Distribution (100k trials) ===\n");
	if (!TestAffixCountDistribution()) { ++failures; }

	std::fprintf(stderr, "\n=== Test: Reforge Preserves Multi-Affix Count ===\n");
	if (!TestReforgePreservesMultiAffixCount()) { ++failures; }

	std::fprintf(stderr, "\n=== Test: Reforge with Runeword + 2 Regular ===\n");
	if (!TestReforgeWithRuneword()) { ++failures; }

	std::fprintf(stderr, "\n=== Test: Reforge with Runeword + 1 Regular ===\n");
	if (!TestReforgeWithRunewordOnlyOneRegular()) { ++failures; }

	std::fprintf(stderr, "\n=== Test: New Loot Can Roll Multiple Affixes ===\n");
	if (!TestNewLootCanRollMultiple()) { ++failures; }

	std::fprintf(stderr, "\n%s (%d failure%s)\n",
		failures == 0 ? "ALL PASSED" : "SOME FAILED",
		failures,
		failures == 1 ? "" : "s");

	return failures == 0 ? 0 : 1;
}
