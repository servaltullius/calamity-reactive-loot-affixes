#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "CalamityAffixes/NonHostileFirstHitGate.h"
#include "CalamityAffixes/PerTargetCooldownStore.h"

namespace CalamityAffixes
{
	struct LastHitKey
	{
		bool outgoing{ false };
		std::uint32_t aggressor{ 0 };
		std::uint32_t target{ 0 };
		std::uint32_t source{ 0 };
	};

	struct LowHealthTriggerKey
	{
		std::uint64_t token{ 0 };
		std::uint32_t owner{ 0 };

		[[nodiscard]] bool operator==(const LowHealthTriggerKey& a_rhs) const noexcept
		{
			return token == a_rhs.token && owner == a_rhs.owner;
		}
	};

	struct LowHealthTriggerKeyHash
	{
		[[nodiscard]] std::size_t operator()(const LowHealthTriggerKey& a_key) const noexcept
		{
			const auto owner64 = static_cast<std::uint64_t>(a_key.owner);
			return static_cast<std::size_t>((a_key.token << 1) ^ (owner64 * 0x9E3779B185EBCA87ull));
		}
	};

	struct CorpseExplosionRuntimeState
	{
		std::chrono::steady_clock::time_point lastExplosionAt{};
		std::chrono::steady_clock::time_point chainAnchorAt{};
		std::chrono::steady_clock::time_point rateWindowStartAt{};
		std::uint32_t chainDepth{ 0u };
		std::uint32_t explosionsInWindow{ 0u };
	};

	struct CombatRuntimeState
	{
		std::unordered_map<std::uint64_t, std::chrono::steady_clock::time_point> dotCooldowns{};
		std::chrono::steady_clock::time_point dotCooldownsLastPruneAt{};
		std::unordered_set<std::uint32_t> dotObservedMagicEffects{};
		bool dotTagSafetyWarned{ false };
		bool dotObservedMagicEffectsCapWarned{ false };
		bool dotTagSafetySuppressed{ false };

		PerTargetCooldownStore perTargetCooldownStore{};
		NonHostileFirstHitGate nonHostileFirstHitGate{};

		std::unordered_map<std::uint32_t, std::chrono::steady_clock::time_point> recentOwnerHitAt{};
		std::unordered_map<std::uint32_t, std::chrono::steady_clock::time_point> recentOwnerKillAt{};
		std::unordered_map<std::uint32_t, std::chrono::steady_clock::time_point> recentOwnerIncomingHitAt{};
		std::unordered_map<std::uint32_t, std::chrono::steady_clock::time_point> outgoingHitPerTargetLastAt{};

		std::chrono::steady_clock::time_point playerCombatEvidenceExpiresAt{};
		std::uint64_t lastHealthDamageSignature{ 0 };
		std::chrono::steady_clock::time_point lastHealthDamageSignatureAt{};
		std::uint64_t triggerProcBudgetWindowStartMs{ 0u };
		std::uint32_t triggerProcBudgetConsumed{ 0u };
		std::chrono::steady_clock::time_point castOnCritNextAllowed{};
		std::size_t castOnCritCycleCursor{ 0u };

		std::atomic<std::uint32_t> procDepth{ 0 };
		bool healthDamageHookSeen{ false };
		std::chrono::steady_clock::time_point healthDamageHookLastAt{};
		std::chrono::steady_clock::time_point lastHitAt{};
		LastHitKey lastHit{};
		std::chrono::steady_clock::time_point lastPapyrusHitEventAt{};
		LastHitKey lastPapyrusHit{};
		std::unordered_map<LowHealthTriggerKey, bool, LowHealthTriggerKeyHash> lowHealthTriggerConsumed{};
		std::unordered_map<std::uint32_t, float> lowHealthLastObservedPct{};
		CorpseExplosionRuntimeState corpseExplosionState{};
		std::unordered_map<std::uint32_t, std::chrono::steady_clock::time_point> corpseExplosionSeenCorpses{};
		CorpseExplosionRuntimeState summonCorpseExplosionState{};
		std::unordered_map<std::uint32_t, std::chrono::steady_clock::time_point> summonCorpseExplosionSeenCorpses{};

		void ResetTransientState() noexcept
		{
			dotCooldowns.clear();
			dotCooldownsLastPruneAt = {};
			dotObservedMagicEffects.clear();
			dotTagSafetyWarned = false;
			dotObservedMagicEffectsCapWarned = false;
			dotTagSafetySuppressed = false;
			perTargetCooldownStore.Clear();
			nonHostileFirstHitGate.Clear();
			recentOwnerHitAt.clear();
			recentOwnerKillAt.clear();
			recentOwnerIncomingHitAt.clear();
			outgoingHitPerTargetLastAt.clear();
			playerCombatEvidenceExpiresAt = {};
			lastHealthDamageSignature = 0u;
			lastHealthDamageSignatureAt = {};
			triggerProcBudgetWindowStartMs = 0u;
			triggerProcBudgetConsumed = 0u;
			castOnCritNextAllowed = {};
			castOnCritCycleCursor = 0u;
			procDepth.store(0u, std::memory_order_relaxed);
			healthDamageHookSeen = false;
			healthDamageHookLastAt = {};
			lastHitAt = {};
			lastHit = {};
			lastPapyrusHitEventAt = {};
			lastPapyrusHit = {};
			lowHealthTriggerConsumed.clear();
			lowHealthLastObservedPct.clear();
		}

		void ResetCorpseExplosionState() noexcept
		{
			corpseExplosionState = {};
			corpseExplosionSeenCorpses.clear();
			summonCorpseExplosionState = {};
			summonCorpseExplosionSeenCorpses.clear();
		}

		void Reset() noexcept
		{
			ResetTransientState();
			ResetCorpseExplosionState();
		}
	};

	// RAII guard for CombatRuntimeState::procDepth.
	//
	// Proc actions re-enter the engine (spell casts, trap spawns, corpse
	// explosions) and the trigger path refuses to run while procDepth > 0, which
	// is what stops a proc from observing its own side effects.  Raising and
	// lowering the counter by hand around those calls means any early exit or
	// escaping exception leaks the depth permanently -- and because procDepth is
	// unsigned, an unbalanced decrement wraps to ~4e9 instead of going negative.
	// Either way every affix proc stays disabled for the rest of the session.
	class ScopedProcDepth
	{
	public:
		explicit ScopedProcDepth(CombatRuntimeState& a_state) noexcept :
			_state(&a_state)
		{
			_state->procDepth.fetch_add(1u, std::memory_order_relaxed);
		}

		~ScopedProcDepth() noexcept
		{
			// Compare-exchange rather than a bare fetch_sub so a concurrent
			// ResetTransientState (save/load, config reload) cannot make this
			// decrement underflow.
			auto current = _state->procDepth.load(std::memory_order_relaxed);
			while (current > 0u &&
				   !_state->procDepth.compare_exchange_weak(
					   current,
					   current - 1u,
					   std::memory_order_relaxed,
					   std::memory_order_relaxed)) {
			}
		}

		ScopedProcDepth(const ScopedProcDepth&) = delete;
		ScopedProcDepth(ScopedProcDepth&&) = delete;
		ScopedProcDepth& operator=(const ScopedProcDepth&) = delete;
		ScopedProcDepth& operator=(ScopedProcDepth&&) = delete;

	private:
		CombatRuntimeState* _state;
	};
}
