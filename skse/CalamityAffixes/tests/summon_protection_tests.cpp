#include "CalamityAffixes/SummonProtectionSerialization.h"
#include "CalamityAffixes/HealthDamageBoundary.h"
#include "test_hostile_effect_damage_policy.cpp"

#include <cstring>
#include <stdexcept>

namespace Wire = CalamityAffixes::SummonProtectionSerialization;

void Require(bool value)
{
	if (!value) {
		throw std::runtime_error("summon protection regression");
	}
}

int main()
{
	// Exercise the actual hook boundary: an engine callback can synchronously
	// cause friendly damage while the outer hit is still processing. Protection
	// must run in both recursion guards, without dispatching extra proc work.
	using CalamityAffixes::detail::DispatchHealthDamageBoundary;
	bool inHook = false;
	unsigned engineCalls = 0u;
	unsigned procCalls = 0u;
	unsigned protectedChecks = 0u;
	auto block = [&] { ++protectedChecks; return true; };
	auto allow = [] { return false; };
	auto engine = [&] { ++engineCalls; };
	auto proc = [&] { ++procCalls; };
	DispatchHealthDamageBoundary(inHook, false, allow, engine, [&] {
		++procCalls;
		Require(inHook);
		DispatchHealthDamageBoundary(inHook, false, block, engine, proc);
		Require(inHook && engineCalls == 0u && procCalls == 1u);
		// Enemy damage nested in the same callback still reaches the engine.
		DispatchHealthDamageBoundary(inHook, false, allow, engine, proc);
		Require(inHook && engineCalls == 1u && procCalls == 1u);
	});
	Require(!inHook);
	DispatchHealthDamageBoundary(inHook, true, block, engine, proc);
	DispatchHealthDamageBoundary(inHook, false, block, engine, proc);
	Require(!inHook && protectedChecks == 3u && engineCalls == 1u && procCalls == 1u);
	DispatchHealthDamageBoundary(inHook, true, allow, engine, proc);
	Require(!inHook && engineCalls == 2u && procCalls == 1u);
	try {
		DispatchHealthDamageBoundary(inHook, false, allow, engine, [] { throw 1; });
	} catch (int) {}
	Require(!inHook);

	// Distinct dynamic references with the same NPC base remain distinct: only
	// the identities in this co-save record may be restored by the game adapter.
	const std::uint32_t ids[]{ 0xFF001234u, 0xFF005678u };
	unsigned reads = 0u;
	auto reader = [&](void* out, std::uint32_t bytes) {
		++reads;
		Require(bytes <= sizeof(ids));
		std::memcpy(out, ids, bytes);
		return bytes;
	};
	const auto restored = Wire::Read(1u, sizeof(ids), reader);
	Require(restored && restored->count == 2u);
	Require(restored->formIDs[0] == ids[0] && restored->formIDs[1] == ids[1]);
	Require(reads == 1u);

	Require(!Wire::Read(2u, sizeof(ids), reader));
	Require(!Wire::Read(1u, 3u, reader));
	Require(!Wire::Read(1u, (Wire::kMaxSummons + 1u) * 4u, reader));
	Require(reads == 1u);  // Invalid records never reach the reader.
	const auto empty = Wire::Read(1u, 0u, reader);
	Require(empty && empty->count == 0u && reads == 1u);

	// Every possible truncation rejects the complete record, even if the first
	// FormID was readable; do not grant partial protection after a short read.
	for (std::uint32_t bytes = 0u; bytes < sizeof(ids); ++bytes) {
		Require(!Wire::Read(1u, sizeof(ids), [&](void* out, std::uint32_t) {
			std::memcpy(out, ids, bytes);
			return bytes;
		}));
	}
	const auto full = Wire::Read(1u, Wire::kMaxSummons * 4u, [](void* out, std::uint32_t bytes) {
		std::memset(out, 0, bytes);
		return bytes;
	});
	Require(full && full->count == Wire::kMaxSummons);
}
