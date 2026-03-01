// Micro-benchmarks for core CalamityAffixes operations.
// No SKSE/CommonLibSSE-NG dependency — pure C++ logic only.
//
// Usage:
//   cmake --build build.linux-clangcl-rel --target bench_core_ops
//   ./build.linux-clangcl-rel/bench_core_ops

#include "CalamityAffixes/AffixToken.h"
#include "CalamityAffixes/InstanceAffixSlots.h"
#include "CalamityAffixes/InstanceStateKey.h"
#include "CalamityAffixes/LootCurrencyLedger.h"
#include "CalamityAffixes/MagnitudeScaling.h"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

struct BenchResult
{
	std::string name;
	std::uint64_t iterations;
	double totalMs;
	double nsPerOp;
};

template <typename Fn>
BenchResult Bench(const char* name, std::uint64_t iters, Fn&& fn)
{
	// Warmup
	for (std::uint64_t i = 0; i < std::min<std::uint64_t>(iters / 10, 1000); ++i) {
		fn(i);
	}

	auto start = Clock::now();
	for (std::uint64_t i = 0; i < iters; ++i) {
		fn(i);
	}
	auto end = Clock::now();

	const double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
	const double nsPerOp = (totalMs * 1e6) / static_cast<double>(iters);

	return BenchResult{
		.name = name,
		.iterations = iters,
		.totalMs = totalMs,
		.nsPerOp = nsPerOp
	};
}

static volatile std::uint64_t g_sink = 0;

int main()
{
	using namespace CalamityAffixes;

	std::vector<BenchResult> results;

	// 1. AffixToken (FNV-1a hash)
	{
		static constexpr const char* kIds[] = {
			"prefix_firestorm", "prefix_thunderbolt", "suffix_max_health_t1",
			"prefix_corpse_explosion", "suffix_fire_resist_t3"
		};
		results.push_back(Bench("MakeAffixToken (FNV-1a)", 5'000'000, [](std::uint64_t i) {
			g_sink = MakeAffixToken(kIds[i % 5]);
		}));
	}

	// 2. InstanceStateKey hash
	results.push_back(Bench("HashInstanceStateKey", 5'000'000, [](std::uint64_t i) {
		g_sink = HashInstanceStateKey(0x000A'0001'0000'0042ull + i, 0xCBF29CE484222325ull + i);
	}));

	// 3. InstanceAffixSlots::AddToken
	results.push_back(Bench("InstanceAffixSlots::AddToken (x3)", 2'000'000, [](std::uint64_t i) {
		InstanceAffixSlots slots{};
		slots.AddToken(0xABCD'0001ull + i);
		slots.AddToken(0xABCD'0002ull + i);
		slots.AddToken(0xABCD'0003ull + i);
		g_sink = slots.count;
	}));

	// 4. InstanceAffixSlots::RemoveToken
	results.push_back(Bench("InstanceAffixSlots::RemoveToken", 2'000'000, [](std::uint64_t i) {
		InstanceAffixSlots slots{};
		slots.AddToken(0x1111ull);
		slots.AddToken(0x2222ull);
		slots.AddToken(0x3333ull);
		slots.RemoveToken(0x2222ull);
		g_sink = slots.count;
	}));

	// 5. InstanceAffixSlots::PromoteTokenToPrimary
	results.push_back(Bench("InstanceAffixSlots::PromoteTokenToPrimary", 2'000'000, [](std::uint64_t i) {
		InstanceAffixSlots slots{};
		slots.AddToken(0x1111ull);
		slots.AddToken(0x2222ull);
		slots.AddToken(0x3333ull + i);
		slots.PromoteTokenToPrimary(0x3333ull + i);
		g_sink = slots.tokens[0];
	}));

	// 6. LootCurrencyLedger key building
	results.push_back(Bench("BuildLootCurrencyLedgerKey", 5'000'000, [](std::uint64_t i) {
		g_sink = detail::BuildLootCurrencyLedgerKey(
			detail::LootCurrencySourceTier::kCorpse,
			static_cast<std::uint32_t>(0x000ABC01u + i),
			0u, 0u,
			static_cast<std::uint32_t>(10u + (i % 365)));
	}));

	// 7. unordered_map lookup (InstanceStateKey)
	{
		std::unordered_map<InstanceStateKey, int, InstanceStateKeyHash> stateMap;
		for (std::uint64_t i = 0; i < 500; ++i) {
			stateMap[MakeInstanceStateKey(i * 1000, MakeAffixToken("prefix_firestorm"))] = static_cast<int>(i);
		}
		results.push_back(Bench("unordered_map<InstanceStateKey> lookup (500 entries)", 2'000'000, [&](std::uint64_t i) {
			auto it = stateMap.find(MakeInstanceStateKey((i % 500) * 1000, MakeAffixToken("prefix_firestorm")));
			g_sink = (it != stateMap.end()) ? static_cast<std::uint64_t>(it->second) : 0;
		}));
	}

	// 8. ResolveMagnitudeOverride compute
	results.push_back(Bench("ResolveMagnitudeOverride (hitTotal)", 2'000'000, [](std::uint64_t i) {
		MagnitudeScaling scaling{
			.source = MagnitudeScaling::Source::kHitTotalDealt,
			.mult = 0.5f,
			.add = 10.0f,
			.min = 5.0f,
			.max = 200.0f,
			.spellBaseAsMin = true
		};
		g_sink = static_cast<std::uint64_t>(ResolveMagnitudeOverride(
			0.0f, 20.0f,
			static_cast<float>(80 + (i % 50)),
			static_cast<float>(120 + (i % 80)),
			scaling));
	}));

	// Print results
	std::cout << "\n";
	std::cout << std::left << std::setw(55) << "Benchmark"
			  << std::right << std::setw(12) << "Iterations"
			  << std::setw(12) << "Total(ms)"
			  << std::setw(12) << "ns/op"
			  << "\n";
	std::cout << std::string(91, '-') << "\n";
	for (const auto& r : results) {
		std::cout << std::left << std::setw(55) << r.name
				  << std::right << std::setw(12) << r.iterations
				  << std::setw(12) << std::fixed << std::setprecision(1) << r.totalMs
				  << std::setw(12) << std::setprecision(1) << r.nsPerOp
				  << "\n";
	}
	std::cout << "\n";

	return 0;
}
