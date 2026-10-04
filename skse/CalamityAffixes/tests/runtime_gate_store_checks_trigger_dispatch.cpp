#include "runtime_gate_store_checks_common.h"

#include "CalamityAffixes/CombatRuntimeState.h"
#include "CalamityAffixes/EchoStrikeState.h"
#include "CalamityAffixes/TriggerDispatchSnapshot.h"

namespace RuntimeGateStoreChecks
{
	namespace
	{
		// Stand-in for EventBridge::RebuildActiveTriggerIndexCaches().  The
		// clear() + reserve() + push_back() shape is what reallocates the
		// buffer and invalidates anything still pointing into it.
		void RebuildIndexCacheLikeRuntime(
			const std::vector<std::size_t>& a_source,
			std::vector<std::size_t>& a_out)
		{
			a_out.clear();
			a_out.reserve(a_source.size());
			for (const auto idx : a_source) {
				a_out.push_back(idx);
			}
		}
	}

	// The dispatch loop must iterate a private copy, so a re-entrant rebuild
	// of the live cache cannot change what the in-flight pass dispatches.
	bool CheckTriggerDispatchSnapshotIsolation()
	{
		std::vector<std::size_t> live{ 3u, 7u, 11u };
		std::vector<std::size_t> snapshot{};

		const auto copied = CalamityAffixes::detail::SnapshotTriggerIndices(&live, snapshot);
		if (copied != 3u || snapshot != std::vector<std::size_t>{ 3u, 7u, 11u }) {
			std::cerr << "trigger_dispatch_snapshot: snapshot did not copy the live indices\n";
			return false;
		}

		// Capture the storage the loop would have been walking, then rebuild
		// the live cache the way an equip event does mid-dispatch.
		const auto* liveStorageBefore = live.data();
		RebuildIndexCacheLikeRuntime({ 3u, 7u, 11u, 13u, 17u, 19u, 23u, 29u }, live);

		if (live.data() == liveStorageBefore) {
			// Not a product failure -- the rebuild has to actually move the
			// buffer for this check to mean anything.  If an allocator happens
			// to reuse the address the check would silently stop proving the
			// isolation, so fail loudly instead of passing vacuously.
			std::cerr << "trigger_dispatch_snapshot: rebuild did not reallocate; check is vacuous\n";
			return false;
		}

		if (snapshot != std::vector<std::size_t>{ 3u, 7u, 11u }) {
			std::cerr << "trigger_dispatch_snapshot: re-entrant rebuild mutated the in-flight dispatch set\n";
			return false;
		}

		return true;
	}

	// A trigger with no index cache (ResolveActiveTriggerIndices returns
	// nullptr for unmapped triggers) must dispatch nothing rather than
	// dereference the null source.
	bool CheckTriggerDispatchSnapshotNullSource()
	{
		std::vector<std::size_t> snapshot{ 1u, 2u, 3u };

		const auto copied = CalamityAffixes::detail::SnapshotTriggerIndices(nullptr, snapshot);
		if (copied != 0u || !snapshot.empty()) {
			std::cerr << "trigger_dispatch_snapshot: null source must clear the buffer\n";
			return false;
		}

		return true;
	}

	// The buffer is reused across dispatches, so a shorter follow-up must not
	// leave stale indices from the previous pass behind.
	bool CheckTriggerDispatchSnapshotBufferReuse()
	{
		std::vector<std::size_t> snapshot{};
		const std::vector<std::size_t> wide{ 1u, 2u, 3u, 4u, 5u };
		const std::vector<std::size_t> narrow{ 9u };

		(void)CalamityAffixes::detail::SnapshotTriggerIndices(&wide, snapshot);
		const auto copied = CalamityAffixes::detail::SnapshotTriggerIndices(&narrow, snapshot);

		if (copied != 1u || snapshot != std::vector<std::size_t>{ 9u }) {
			std::cerr << "trigger_dispatch_snapshot: reused buffer kept stale indices\n";
			return false;
		}

		const std::vector<std::size_t> empty{};
		if (CalamityAffixes::detail::SnapshotTriggerIndices(&empty, snapshot) != 0u || !snapshot.empty()) {
			std::cerr << "trigger_dispatch_snapshot: empty source must clear the buffer\n";
			return false;
		}

		return true;
	}

	// Shadow Boxer: the window gates echoes, each source hit echoes at most once,
	// due echoes leave the queue in order, and every reset path drops them.
	bool CheckEchoStrikeRuntimeState()
	{
		using namespace std::chrono;
		using CalamityAffixes::EchoStrikePending;
		using CalamityAffixes::EchoStrikeRuntimeState;

		const steady_clock::time_point t0{ milliseconds(10'000) };
		EchoStrikeRuntimeState state{};
		if (state.IsWindowOpen(t0)) {
			std::cerr << "echo_strike_state: a fresh state must have no open window\n";
			return false;
		}

		state.OpenWindow(0xABCull, t0 + milliseconds(8'000));
		if (!state.IsWindowOpen(t0 + milliseconds(7'999)) || state.IsWindowOpen(t0 + milliseconds(8'000))) {
			std::cerr << "echo_strike_state: the window must cover [open, open + duration)\n";
			return false;
		}

		if (!state.TryRememberSource(0x1111ull) || state.TryRememberSource(0x1111ull) || state.TryRememberSource(0u)) {
			std::cerr << "echo_strike_state: a source hit must echo once, and a missing hit never\n";
			return false;
		}
		// The dedupe ring forgets only after it wraps.
		for (std::uint64_t sig = 1u; sig <= EchoStrikeRuntimeState::kRecentSourceCount; ++sig) {
			(void)state.TryRememberSource(0x9000ull + sig);
		}
		if (!state.TryRememberSource(0x1111ull)) {
			std::cerr << "echo_strike_state: a source evicted from the ring must be accepted again\n";
			return false;
		}

		const auto queue = [&](std::uint32_t a_target, milliseconds a_at) {
			return state.Enqueue(EchoStrikePending{
				.affixToken = 0xABCull,
				.targetFormID = a_target,
				.magnitude = 10.0f,
				.fireAt = t0 + a_at,
			});
		};
		if (!queue(1u, milliseconds(250)) || !queue(2u, milliseconds(500)) || !queue(3u, milliseconds(250)) ||
			!state.hasPending.load()) {
			std::cerr << "echo_strike_state: enqueue must accept echoes and raise hasPending\n";
			return false;
		}

		std::vector<EchoStrikePending> due;
		if (state.TakeDue(t0 + milliseconds(100), due) != 0u || !state.hasPending.load()) {
			std::cerr << "echo_strike_state: nothing is due before its delay\n";
			return false;
		}
		if (state.TakeDue(t0 + milliseconds(250), due) != 2u ||
			due[0].targetFormID != 1u || due[1].targetFormID != 3u ||
			state.pending.size() != 1u || state.pending[0].targetFormID != 2u || !state.hasPending.load()) {
			std::cerr << "echo_strike_state: due echoes must leave in order and keep the rest queued\n";
			return false;
		}
		if (state.TakeDue(t0 + milliseconds(600), due) != 1u || !state.pending.empty() || state.hasPending.load()) {
			std::cerr << "echo_strike_state: draining the queue must clear hasPending\n";
			return false;
		}

		for (std::size_t i = 0; i < EchoStrikeRuntimeState::kMaxPending; ++i) {
			(void)queue(static_cast<std::uint32_t>(i), milliseconds(250));
		}
		if (queue(99u, milliseconds(250)) || state.pending.size() != EchoStrikeRuntimeState::kMaxPending) {
			std::cerr << "echo_strike_state: the queue must stop at kMaxPending\n";
			return false;
		}

		CalamityAffixes::CombatRuntimeState combat{};
		combat.echoStrike.OpenWindow(0xABCull, t0 + milliseconds(8'000));
		(void)combat.echoStrike.TryRememberSource(0x2222ull);
		(void)combat.echoStrike.Enqueue(EchoStrikePending{ .affixToken = 0xABCull, .targetFormID = 7u, .fireAt = t0 });
		combat.ResetTransientState();
		if (combat.echoStrike.IsWindowOpen(t0) || !combat.echoStrike.pending.empty() ||
			combat.echoStrike.hasPending.load() || !combat.echoStrike.TryRememberSource(0x2222ull)) {
			std::cerr << "echo_strike_state: combat transient reset must clear the window, queue, and sources\n";
			return false;
		}

		static_assert(CalamityAffixes::detail::IsEchoStrikeEligibleHit(true, true, false, false, false, false));
		static_assert(!CalamityAffixes::detail::IsEchoStrikeEligibleHit(false, true, false, false, false, false),
			"a summon's hit routed to the player is not the player's swing");
		static_assert(!CalamityAffixes::detail::IsEchoStrikeEligibleHit(true, true, true, false, false, false),
			"bow and crossbow hits never echo");
		static_assert(!CalamityAffixes::detail::IsEchoStrikeEligibleHit(true, true, false, true, false, false),
			"spell hits never echo");
		static_assert(!CalamityAffixes::detail::IsEchoStrikeEligibleHit(true, true, false, false, true, false),
			"bashes never echo");
		static_assert(!CalamityAffixes::detail::IsEchoStrikeEligibleHit(true, true, false, false, false, true),
			"explosions never echo");
		static_assert(!CalamityAffixes::detail::IsEchoStrikeEligibleHit(true, false, false, false, false, false),
			"a hit without melee evidence never echoes");
		static_assert(CalamityAffixes::detail::IsEchoStrikeActivationHit(true, false, false),
			"by default any eligible swing opens the window");
		static_assert(CalamityAffixes::detail::IsEchoStrikeActivationHit(true, true, false));
		static_assert(CalamityAffixes::detail::IsEchoStrikeActivationHit(true, true, true));
		static_assert(!CalamityAffixes::detail::IsEchoStrikeActivationHit(true, false, true),
			"requirePowerAttack keeps normal swings from opening the window");
		static_assert(!CalamityAffixes::detail::IsEchoStrikeActivationHit(false, true, false),
			"an ineligible hit never opens the window");
		return true;
	}
}
