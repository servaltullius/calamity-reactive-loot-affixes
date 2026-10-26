#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace CalamityAffixes
{
	// The TESHitEvent fallback (when another mod owns the damage hook) only hears
	// about a hit after its damage, so the killing blow already finds the target
	// dead. The first hit on a dead target is that blow; later ones within the
	// memory window are corpse hits and fire nothing (v2.3.0).
	// When the death itself was observed (TESDeathEvent, any killer), a hit on
	// the dead target is the killing blow only if it came right after that
	// death; a later hit is a corpse hit however long the body lies there, and
	// whoever (or whatever DoT) killed it. Without a death record (died before
	// a load, or the death event is still queued behind the hit) the
	// first-hit memory below decides, as before.
	enum class DeadTargetHitVerdict : std::uint8_t
	{
		kKillingBlow,
		kCorpseHit,
		kUnknownDeath,
	};

	inline constexpr std::int64_t kKillingBlowAfterDeathWindowMs = 250;

	[[nodiscard]] constexpr DeadTargetHitVerdict ClassifyDeadTargetHit(
		bool a_hasDeathRecord,
		std::int64_t a_hitObservedMs,
		std::int64_t a_deathObservedMs) noexcept
	{
		if (!a_hasDeathRecord) {
			return DeadTargetHitVerdict::kUnknownDeath;
		}
		return a_hitObservedMs - a_deathObservedMs <= kKillingBlowAfterDeathWindowMs ?
			DeadTargetHitVerdict::kKillingBlow :
			DeadTargetHitVerdict::kCorpseHit;
	}

	class CorpseHitFilter
	{
	public:
		static constexpr std::size_t kCapacity = 32u;
		static constexpr std::int64_t kMemoryMs = 30000;

		// True for the first hit on this dead target (the killing blow).
		constexpr bool AdmitDeadTargetHit(std::uint32_t a_target, std::int64_t a_nowMs) noexcept
		{
			if (a_target == 0u) {
				return false;
			}
			for (const auto& entry : _entries) {
				if (entry.target == a_target && a_nowMs - entry.atMs < kMemoryMs) {
					return false;
				}
			}
			_entries[_next] = { a_target, a_nowMs };
			_next = (_next + 1u) % kCapacity;
			return true;
		}

		constexpr void Clear() noexcept
		{
			_entries = {};
			_next = 0u;
		}

	private:
		struct Entry
		{
			std::uint32_t target{ 0u };
			std::int64_t atMs{ 0 };
		};

		std::array<Entry, kCapacity> _entries{};
		std::size_t _next{ 0u };
	};
}
