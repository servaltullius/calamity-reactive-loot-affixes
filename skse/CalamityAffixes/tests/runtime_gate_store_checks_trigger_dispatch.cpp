#include "runtime_gate_store_checks_common.h"

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
}
