#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/NonHostileFirstHitGate.h"
#include "CalamityAffixes/PerTargetCooldownStore.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
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

		// Reentry within the window stays allowed.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(50))) {
			std::cerr << "gate: expected reentry within window to be allowed\n";
			return false;
		}

		// Reentry beyond the window is denied.
		if (gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(150))) {
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
}

int main()
{
	const bool gateOk = CheckNonHostileFirstHitGate();
	const bool storeOk = CheckPerTargetCooldownStore();
	const bool lootSelectionOk = CheckUniformLootRollSelection();
	return (gateOk && storeOk && lootSelectionOk) ? 0 : 1;
}
