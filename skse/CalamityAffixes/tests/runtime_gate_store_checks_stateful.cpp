#include "runtime_gate_store_checks_common.h"
#include "CalamityAffixes/LootRerollGuard.h"
#include "CalamityAffixes/SerializationLoadState.h"

namespace RuntimeGateStoreChecks
{
	bool CheckPerTargetCooldownStorePruning()
	{
		using namespace std::chrono;
		CalamityAffixes::PerTargetCooldownStore store{};
		const auto t0 = steady_clock::time_point(seconds(1000));

		// Phase 1: Fill 8192 entries that expire at t0+50ms.
		for (std::uint32_t i = 1; i <= 8192; ++i) {
			store.Commit(static_cast<std::uint64_t>(i), 0x1u, milliseconds(50), t0);
		}

		// Commit entry #8193 at t0+100ms with long ICD.
		// Insert makes size=8193>8192, first prune fires (epoch check).
		// All 8192 prior entries expired (a_now=t0+100ms >= _nextAllowed=t0+50ms) -> removed.
		// This entry's _nextAllowed=t0+5100ms -> survives.
		store.Commit(0xFFFFull, 0x2u, milliseconds(5000), t0 + milliseconds(100));

		// Verify expired entry was pruned.
		if (store.IsBlocked(500ull, 0x1u, t0 + milliseconds(200))) {
			std::cerr << "pruning: expected expired entry to be removed after prune\n";
			return false;
		}

		// Verify valid entry survives.
		if (!store.IsBlocked(0xFFFFull, 0x2u, t0 + milliseconds(200))) {
			std::cerr << "pruning: expected valid entry to survive prune\n";
			return false;
		}

		// Phase 2: Prune interval skip.
		// _lastPruneAt = t0+100ms. Fill again within 10s.
		for (std::uint32_t i = 1; i <= 8192; ++i) {
			store.Commit(static_cast<std::uint64_t>(i + 20000), 0x1u, milliseconds(50), t0 + milliseconds(150));
		}
		// After inserting 8192 more: total size = 1 (surviving) + 8192 = 8193.
		// The 8192th insert makes size=8193>8192, prune check fires.
		// But interval: t0+150ms - t0+100ms = 50ms < 10s, prune SKIPPED.

		// Phase 3: After prune interval, prune fires again.
		// Advance time past prune interval (10s) from _lastPruneAt(t0+100ms).
		store.Commit(0xAAAAull, 0x3u, milliseconds(5000), t0 + seconds(11));
		// size was 8193 (before insert). Insert makes 8194>8192, prune check fires.
		// Interval: t0+11s - t0+100ms = ~10.9s > 10s, prune fires.
		// Phase 2 entries expired, Phase 1 survivor expired, only new entry kept.

		if (!store.IsBlocked(0xAAAAull, 0x3u, t0 + seconds(12))) {
			std::cerr << "pruning: expected new entry after interval-elapsed prune to survive\n";
			return false;
		}

		// Phase 1 survivor should now be gone (pruned in Phase 3).
		if (store.IsBlocked(0xFFFFull, 0x2u, t0 + seconds(12))) {
			std::cerr << "pruning: expected originally valid entry to be pruned after interval-elapsed prune\n";
			return false;
		}

		return true;
	}

	bool CheckPerTargetCooldownStoreIndependence()
	{
		using namespace std::chrono;
		CalamityAffixes::PerTargetCooldownStore store{};
		const auto t0 = steady_clock::time_point(seconds(1000));

		// Same token, different targets: independent cooldowns.
		store.Commit(0xA5ull, 0x100u, milliseconds(500), t0);
		store.Commit(0xA5ull, 0x200u, milliseconds(1000), t0);

		// At t0+600ms: token 0xA5/target 0x100 expired (ICD=500ms), 0xA5/0x200 still blocked.
		if (store.IsBlocked(0xA5ull, 0x100u, t0 + milliseconds(600))) {
			std::cerr << "independence: same token, different target — first should be unblocked\n";
			return false;
		}
		if (!store.IsBlocked(0xA5ull, 0x200u, t0 + milliseconds(600))) {
			std::cerr << "independence: same token, different target — second should still be blocked\n";
			return false;
		}

		// Different tokens, same target: independent cooldowns.
		store.Clear();
		store.Commit(0xBBull, 0x300u, milliseconds(200), t0);
		store.Commit(0xCCull, 0x300u, milliseconds(800), t0);

		if (store.IsBlocked(0xBBull, 0x300u, t0 + milliseconds(300))) {
			std::cerr << "independence: different token, same target — first should be unblocked\n";
			return false;
		}
		if (!store.IsBlocked(0xCCull, 0x300u, t0 + milliseconds(300))) {
			std::cerr << "independence: different token, same target — second should still be blocked\n";
			return false;
		}

		// Commit overwrite: recommitting the same key extends the cooldown.
		store.Clear();
		store.Commit(0xDDull, 0x400u, milliseconds(200), t0);

		if (!store.IsBlocked(0xDDull, 0x400u, t0 + milliseconds(150))) {
			std::cerr << "independence: expected initial commit to block\n";
			return false;
		}

		// Recommit at t0+150ms with fresh ICD extends to t0+150ms+500ms = t0+650ms.
		store.Commit(0xDDull, 0x400u, milliseconds(500), t0 + milliseconds(150));

		// At t0+300ms (would have expired under original ICD): should still be blocked.
		if (!store.IsBlocked(0xDDull, 0x400u, t0 + milliseconds(300))) {
			std::cerr << "independence: expected recommit to extend cooldown\n";
			return false;
		}

		// At t0+650ms: should now expire.
		if (store.IsBlocked(0xDDull, 0x400u, t0 + milliseconds(650))) {
			std::cerr << "independence: expected extended cooldown to expire at new boundary\n";
			return false;
		}

		return true;
	}

	bool CheckNonHostileFirstHitGateTtlExpiry()
	{
		using namespace std::chrono;
		CalamityAffixes::NonHostileFirstHitGate gate{};
		const auto t0 = steady_clock::time_point(seconds(1000));

		// First non-hostile hit is allowed.
		if (!gate.Resolve(0x14u, 0x100u, true, false, false, t0)) {
			std::cerr << "ttl_expiry: expected first hit to be allowed\n";
			return false;
		}

		// Past reentry window (20ms): denied.
		if (gate.Resolve(0x14u, 0x100u, true, false, false, t0 + milliseconds(30))) {
			std::cerr << "ttl_expiry: expected hit past reentry window to be denied\n";
			return false;
		}

		// After TTL (120s): entry should be pruned, re-allowed.
		if (!gate.Resolve(0x14u, 0x100u, true, false, false, t0 + seconds(121))) {
			std::cerr << "ttl_expiry: expected hit after TTL to be re-allowed\n";
			return false;
		}

		// Verify selective pruning: expired vs. non-expired entries.
		gate.Clear();
		(void)gate.Resolve(0x14u, 0x200u, true, false, false, t0);              // entry A: t0
		(void)gate.Resolve(0x14u, 0x300u, true, false, false, t0 + seconds(100)); // entry B: t0+100s

		// At t0+121s: A expired (121s > 120s TTL), B within TTL (21s < 120s).
		// Prune fires (interval exceeded). A removed, B kept.
		if (!gate.Resolve(0x14u, 0x200u, true, false, false, t0 + seconds(121))) {
			std::cerr << "ttl_expiry: expected expired entry to be pruned and re-insertable\n";
			return false;
		}

		// B should still be denied (not expired, past reentry window).
		if (gate.Resolve(0x14u, 0x300u, true, false, false, t0 + seconds(121))) {
			std::cerr << "ttl_expiry: expected non-expired entry to remain blocked\n";
			return false;
		}

		return true;
	}

	bool CheckNonHostileFirstHitGateCapacityEviction()
	{
		using namespace std::chrono;
		CalamityAffixes::NonHostileFirstHitGate gate{};
		const auto t0 = steady_clock::time_point(seconds(1000));

		// Insert 8193 unique entries (first triggers epoch prune, total size grows to 8193).
		for (std::uint32_t i = 1; i <= 8193; ++i) {
			if (!gate.Resolve(0x14u, i, true, false, false, t0 + milliseconds(i))) {
				std::cerr << "capacity: initial insert " << i << " failed\n";
				return false;
			}
		}

		// Insert #8194 well past reentry window of all prior entries.
		// Before emplace: size=8193>8192, prune fires.
		// All entries within TTL, so only capacity eviction loop runs.
		// Oldest entry (target=1 at t0+1ms) gets evicted.
		if (!gate.Resolve(0x14u, 8194u, true, false, false, t0 + seconds(9))) {
			std::cerr << "capacity: insert at capacity limit failed\n";
			return false;
		}

		// The oldest entry (target=1) should have been evicted.
		// Re-resolving succeeds as a fresh insert.
		if (!gate.Resolve(0x14u, 1u, true, false, false, t0 + seconds(10))) {
			std::cerr << "capacity: expected evicted entry to be re-insertable\n";
			return false;
		}

		// A non-evicted entry should still be blocked (well past 20ms reentry window).
		if (gate.Resolve(0x14u, 8193u, true, false, false, t0 + seconds(11))) {
			std::cerr << "capacity: expected non-evicted entry to remain blocked\n";
			return false;
		}

		return true;
	}

	bool CheckLootRerollGuardLifecycle()
	{
		CalamityAffixes::LootRerollGuard guard{};
		constexpr CalamityAffixes::LootRerollGuard::FormID player = 0x14u;
		constexpr CalamityAffixes::LootRerollGuard::RefHandle ref1 = 0x1000u;
		constexpr CalamityAffixes::LootRerollGuard::RefHandle ref2 = 0x2000u;
		constexpr CalamityAffixes::LootRerollGuard::InstanceKey key1 = 0xDEAD1ull;
		constexpr CalamityAffixes::LootRerollGuard::InstanceKey key2 = 0xDEAD2ull;

		// Drop -> Pickup matching: skip reroll.
		guard.NotePlayerDrop(player, player, CalamityAffixes::LootRerollGuard::kWorldContainer, 1, ref1, key1);

		if (!guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, ref1)) {
			std::cerr << "lifecycle: expected drop-then-pickup to be consumed\n";
			return false;
		}

		// Double consume: should fail.
		if (guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, ref1)) {
			std::cerr << "lifecycle: expected double consume to fail\n";
			return false;
		}

		// Drop -> Delete: returns instanceKey.
		guard.NotePlayerDrop(player, player, CalamityAffixes::LootRerollGuard::kWorldContainer, 1, ref2, key2);
		const auto deleted = guard.ConsumeIfPlayerDropDeleted(ref2);
		if (!deleted.has_value() || *deleted != key2) {
			std::cerr << "lifecycle: expected delete to return instanceKey\n";
			return false;
		}

		// Double delete: should return nullopt.
		if (guard.ConsumeIfPlayerDropDeleted(ref2).has_value()) {
			std::cerr << "lifecycle: expected double delete to return nullopt\n";
			return false;
		}

		// After pickup-consume, delete should also return nullopt.
		guard.NotePlayerDrop(player, player, CalamityAffixes::LootRerollGuard::kWorldContainer, 1, ref1, key1);
		(void)guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, ref1);
		if (guard.ConsumeIfPlayerDropDeleted(ref1).has_value()) {
			std::cerr << "lifecycle: expected delete after pickup-consume to return nullopt\n";
			return false;
		}

		return true;
	}

	bool CheckLootRerollGuardCircularOverflow()
	{
		CalamityAffixes::LootRerollGuard guard{};
		constexpr CalamityAffixes::LootRerollGuard::FormID player = 0x14u;

		// Insert 513 entries: first entry should be overwritten by #513 (circular buffer, size 512).
		for (std::uint32_t i = 1; i <= 513; ++i) {
			guard.NotePlayerDrop(
				player, player,
				CalamityAffixes::LootRerollGuard::kWorldContainer,
				1,
				static_cast<CalamityAffixes::LootRerollGuard::RefHandle>(i),
				static_cast<CalamityAffixes::LootRerollGuard::InstanceKey>(i * 100));
		}

		// Entry #1 should be overwritten: lookup should fail.
		if (guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, 1u)) {
			std::cerr << "circular: expected overwritten entry #1 to be gone\n";
			return false;
		}

		// Entry #2 should be the oldest surviving entry.
		if (!guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, 2u)) {
			std::cerr << "circular: expected entry #2 to still exist\n";
			return false;
		}

		// Entry #513 (the newest) should exist.
		if (!guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, 513u)) {
			std::cerr << "circular: expected newest entry #513 to exist\n";
			return false;
		}

		// Entry #256 (mid-range) should also exist.
		if (!guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, 256u)) {
			std::cerr << "circular: expected mid-range entry #256 to exist\n";
			return false;
		}

		// Delete on overwritten entry should return nullopt.
		const auto deletedOverwritten = guard.ConsumeIfPlayerDropDeleted(1u);
		if (deletedOverwritten.has_value()) {
			std::cerr << "circular: expected delete of overwritten entry to return nullopt\n";
			return false;
		}

		return true;
	}

	bool CheckLootRerollGuardEdgeCases()
	{
		CalamityAffixes::LootRerollGuard guard{};
		constexpr CalamityAffixes::LootRerollGuard::FormID player = 0x14u;
		constexpr CalamityAffixes::LootRerollGuard::RefHandle validRef = 0x999u;
		constexpr CalamityAffixes::LootRerollGuard::InstanceKey validKey = 0xABCull;

		// ref==0: NotePlayerDrop should be silently ignored.
		guard.NotePlayerDrop(player, player, CalamityAffixes::LootRerollGuard::kWorldContainer, 1, 0u, validKey);
		if (guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, 0u)) {
			std::cerr << "edge: ref==0 should be ignored by NotePlayerDrop\n";
			return false;
		}

		// itemCount==0: NotePlayerDrop should be ignored.
		guard.NotePlayerDrop(player, player, CalamityAffixes::LootRerollGuard::kWorldContainer, 0, validRef, validKey);
		if (guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, validRef)) {
			std::cerr << "edge: itemCount==0 should be ignored by NotePlayerDrop\n";
			return false;
		}

		// Non-player oldContainer: NotePlayerDrop should be ignored.
		guard.NotePlayerDrop(player, 0xBEEFu, CalamityAffixes::LootRerollGuard::kWorldContainer, 1, validRef, validKey);
		if (guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, validRef)) {
			std::cerr << "edge: non-player oldContainer should be ignored by NotePlayerDrop\n";
			return false;
		}

		// Non-world newContainer: NotePlayerDrop should be ignored.
		guard.NotePlayerDrop(player, player, 0xDEADu, 1, validRef, validKey);
		if (guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, validRef)) {
			std::cerr << "edge: non-world newContainer should be ignored by NotePlayerDrop\n";
			return false;
		}

		// ConsumeIfPlayerDropPickup edge cases after a valid drop:
		guard.NotePlayerDrop(player, player, CalamityAffixes::LootRerollGuard::kWorldContainer, 1, validRef, validKey);

		// ref==0: should return false.
		if (guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, 0u)) {
			std::cerr << "edge: ref==0 should be ignored by ConsumeIfPlayerDropPickup\n";
			return false;
		}

		// itemCount==0 on pickup: should return false.
		if (guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 0, validRef)) {
			std::cerr << "edge: itemCount==0 should be ignored by ConsumeIfPlayerDropPickup\n";
			return false;
		}

		// Non-player newContainer on pickup: should return false.
		if (guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, 0xBEEFu, 1, validRef)) {
			std::cerr << "edge: non-player newContainer should be ignored by ConsumeIfPlayerDropPickup\n";
			return false;
		}

		// The valid drop should still be consumable after edge-case attempts.
		if (!guard.ConsumeIfPlayerDropPickup(player, CalamityAffixes::LootRerollGuard::kWorldContainer, player, 1, validRef)) {
			std::cerr << "edge: valid entry should still be consumable after edge-case attempts\n";
			return false;
		}

		// ConsumeIfPlayerDropDeleted with ref==0: should return nullopt.
		if (guard.ConsumeIfPlayerDropDeleted(0u).has_value()) {
			std::cerr << "edge: ref==0 should return nullopt for ConsumeIfPlayerDropDeleted\n";
			return false;
		}

		return true;
	}

	bool CheckSerializationLoadStateHelpers()
	{
		CalamityAffixes::LootRuntimeState state{};

		if (CalamityAffixes::SerializationLoadState::ResolveShuffleBag(state, 0u) != &state.prefixSharedBag ||
			CalamityAffixes::SerializationLoadState::ResolveShuffleBag(state, 5u) != &state.suffixArmorBag ||
			CalamityAffixes::SerializationLoadState::ResolveShuffleBag(state, 9u) != nullptr) {
			std::cerr << "serialization_load_state: shuffle bag resolver mapping regressed\n";
			return false;
		}

		state.evaluatedInstances = { 9u, 2u, 7u, 4u };
		CalamityAffixes::SerializationLoadState::RebuildEvaluatedRecent(state, 3u);
		if (state.evaluatedRecent.size() != 3u ||
			state.evaluatedRecent[0] != 4u ||
			state.evaluatedRecent[1] != 7u ||
			state.evaluatedRecent[2] != 9u ||
			state.evaluatedInsertionsSincePrune != 0u) {
			std::cerr << "serialization_load_state: evaluated recent rebuild/truncation is incorrect\n";
			return false;
		}

		state.currencyRollLedger = {
			{ 40u, 4u },
			{ 10u, 1u },
			{ 30u, 3u },
			{ 20u, 2u },
		};
		CalamityAffixes::SerializationLoadState::RebuildCurrencyLedgerRecent(state, 2u);
		if (state.currencyRollLedgerRecent.size() != 2u ||
			state.currencyRollLedgerRecent[0] != 30u ||
			state.currencyRollLedgerRecent[1] != 40u ||
			state.currencyRollLedger.size() != 2u ||
			state.currencyRollLedger.contains(10u) ||
			state.currencyRollLedger.contains(20u) ||
			!state.currencyRollLedger.contains(30u) ||
			!state.currencyRollLedger.contains(40u)) {
			std::cerr << "serialization_load_state: currency ledger recent rebuild/pruning is incorrect\n";
			return false;
		}

		CalamityAffixes::AffixRegistryState registry{};
		registry.lootSharedAffixes = { 3u, 7u, 9u };
		registry.lootWeaponAffixes = { 11u, 13u };
		registry.lootArmorAffixes = { 21u };
		registry.lootSharedSuffixes = { 31u, 33u };
		registry.lootWeaponSuffixes = { 41u };
		registry.lootArmorSuffixes = {};

		state.prefixSharedBag.order = { 7u, 7u, 999u, 3u };
		state.prefixSharedBag.cursor = 8u;
		state.prefixWeaponBag.order = { 13u };
		state.prefixWeaponBag.cursor = 0u;
		state.suffixArmorBag.order = { 55u };
		state.suffixArmorBag.cursor = 1u;

		CalamityAffixes::SerializationLoadState::SanitizeShuffleBagOrders(registry, state);
		if (state.prefixSharedBag.order != std::vector<std::size_t>({ 7u, 3u, 9u }) ||
			state.prefixSharedBag.cursor != 3u) {
			std::cerr << "serialization_load_state: shared prefix bag sanitize behavior regressed\n";
			return false;
		}
		if (state.prefixWeaponBag.order != std::vector<std::size_t>({ 13u, 11u }) ||
			state.prefixWeaponBag.cursor != 0u) {
			std::cerr << "serialization_load_state: weapon prefix bag sanitize behavior regressed\n";
			return false;
		}
		if (!state.suffixArmorBag.order.empty() || state.suffixArmorBag.cursor != 0u) {
			std::cerr << "serialization_load_state: empty suffix bag sanitize behavior regressed\n";
			return false;
		}

		return true;
	}

	bool CheckLowHealthTriggerSnapshotHelpers()
	{
		const auto inactive = CalamityAffixes::BuildLowHealthTriggerSnapshot(false, 0x14u, 35.0f, 62.0f);
		if (inactive.hasSnapshot ||
			inactive.ownerFormID != 0u ||
			inactive.currentPct != 100.0f ||
			inactive.previousPct != 100.0f) {
			std::cerr << "low_health_trigger_snapshot_helpers: inactive trigger should keep default sentinel snapshot\n";
			return false;
		}

		const auto missingOwner = CalamityAffixes::BuildLowHealthTriggerSnapshot(true, 0u, 35.0f, 62.0f);
		if (missingOwner.hasSnapshot || missingOwner.ownerFormID != 0u) {
			std::cerr << "low_health_trigger_snapshot_helpers: zero owner should not produce a snapshot\n";
			return false;
		}

		const auto active = CalamityAffixes::BuildLowHealthTriggerSnapshot(true, 0x14u, 35.0f, 62.0f);
		if (!active.hasSnapshot ||
			active.ownerFormID != 0x14u ||
			active.currentPct != 35.0f ||
			active.previousPct != 62.0f) {
			std::cerr << "low_health_trigger_snapshot_helpers: active trigger snapshot values regressed\n";
			return false;
		}

		return true;
	}
}
