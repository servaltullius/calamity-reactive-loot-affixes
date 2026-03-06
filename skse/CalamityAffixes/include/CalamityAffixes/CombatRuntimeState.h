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

		void Reset() noexcept
		{
			ResetTransientState();
		}
	};
}
