#pragma once

#include "CalamityAffixes/StatusLedger.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace CalamityAffixes
{
	// Build-up statuses (v2.3.0). Freeze, Bleed and Shock fill a per-target meter;
	// a full meter pays off once and empties. Burning stacks instead. All of it lives
	// in runtime memory only: a meter means nothing once the fight is over.
	inline constexpr std::size_t kBuildUpKindCount = 3u;

	[[nodiscard]] constexpr std::optional<std::size_t> BuildUpIndex(StatusKind a_kind) noexcept
	{
		switch (a_kind) {
		case StatusKind::kFreeze:
			return 0u;
		case StatusKind::kBleed:
			return 1u;
		case StatusKind::kShock:
			return 2u;
		default:
			return std::nullopt;
		}
	}

	[[nodiscard]] constexpr bool IsBuildUpStatus(StatusKind a_kind) noexcept
	{
		return BuildUpIndex(a_kind).has_value();
	}

	namespace BuildUpRules
	{
		inline constexpr float kThreshold = 100.0f;
		// Bosses, dragons and unique actors start higher and climb after each
		// payoff, so a long fight cannot chain freezes (PoE2's freeze threshold).
		inline constexpr float kToughThreshold = 150.0f;
		inline constexpr float kToughThresholdStep = 50.0f;
		inline constexpr float kToughThresholdCap = 300.0f;
		inline constexpr std::int64_t kDecayDelayMs = 3000;
		inline constexpr float kDecayPerSecond = 15.0f;

		inline constexpr std::uint8_t kBurningMaxStacks = 5u;
		inline constexpr std::int64_t kBurningDurationMs = 4000;
	}

	[[nodiscard]] constexpr float BuildUpThreshold(bool a_tough, std::uint8_t a_payoffs) noexcept
	{
		if (!a_tough) {
			return BuildUpRules::kThreshold;
		}
		const float raised = BuildUpRules::kToughThreshold + BuildUpRules::kToughThresholdStep * static_cast<float>(a_payoffs);
		return raised < BuildUpRules::kToughThresholdCap ? raised : BuildUpRules::kToughThresholdCap;
	}

	// The meter after decay: it holds for kDecayDelayMs after the last gain, then
	// drains at kDecayPerSecond.
	[[nodiscard]] constexpr float DecayedBuildUp(float a_value, std::int64_t a_lastGainMs, std::int64_t a_nowMs) noexcept
	{
		const auto idleMs = a_nowMs - a_lastGainMs - BuildUpRules::kDecayDelayMs;
		if (a_value <= 0.0f || idleMs <= 0) {
			return a_value < 0.0f ? 0.0f : a_value;
		}
		const float drained = a_value - BuildUpRules::kDecayPerSecond * static_cast<float>(idleMs) / 1000.0f;
		return drained > 0.0f ? drained : 0.0f;
	}

	// When a meter at a_value (just gained at a_nowMs) will have drained to zero.
	[[nodiscard]] constexpr std::int64_t BuildUpEmptiesAtMs(float a_value, std::int64_t a_nowMs) noexcept
	{
		if (a_value <= 0.0f) {
			return a_nowMs;
		}
		return a_nowMs + BuildUpRules::kDecayDelayMs +
		       static_cast<std::int64_t>(a_value / BuildUpRules::kDecayPerSecond * 1000.0f);
	}

	struct BuildUpGain
	{
		bool applied{ false };    // false while the meter is locked by its own payoff
		bool paidOff{ false };    // the meter filled and emptied on this gain
		float value{ 0.0f };      // the meter after this gain
		float threshold{ 0.0f };  // the threshold this gain was measured against
	};

	struct StatusMeterSnapshot
	{
		std::array<float, kBuildUpKindCount> values{};
		std::uint8_t burningStacks{ 0u };
	};

	class StatusMeters
	{
	public:
		static constexpr std::size_t kCapacity = 64u;

	private:
		struct Slot
		{
			std::uint32_t target{ 0u };
			std::array<float, kBuildUpKindCount> values{};
			std::array<std::int64_t, kBuildUpKindCount> lastGainMs{};
			std::array<std::int64_t, kBuildUpKindCount> lockedUntilMs{};
			std::array<std::uint8_t, kBuildUpKindCount> payoffs{};
			std::uint8_t burningStacks{ 0u };
			std::int64_t burningExpiresMs{ 0 };
			std::int64_t touchedMs{ 0 };
		};

		[[nodiscard]] constexpr const Slot* FindSlot(std::uint32_t a_target) const noexcept
		{
			for (std::size_t i = 0; i < _count; ++i) {
				if (_slots[i].target == a_target) {
					return &_slots[i];
				}
			}
			return nullptr;
		}

		// The target's slot, made on first use. A full table reuses the slot touched
		// longest ago.
		constexpr Slot& SlotFor(std::uint32_t a_target, std::int64_t a_nowMs) noexcept
		{
			for (std::size_t i = 0; i < _count; ++i) {
				if (_slots[i].target == a_target) {
					_slots[i].touchedMs = a_nowMs;
					return _slots[i];
				}
			}
			std::size_t index = _count;
			if (_count < kCapacity) {
				++_count;
			} else {
				index = 0u;
				for (std::size_t i = 1; i < _count; ++i) {
					if (_slots[i].touchedMs < _slots[index].touchedMs) {
						index = i;
					}
				}
			}
			_slots[index] = Slot{};
			_slots[index].target = a_target;
			_slots[index].touchedMs = a_nowMs;
			return _slots[index];
		}

	public:
		// Adds a_amount to the target's meter. Reaching the threshold empties the
		// meter, counts a payoff and locks the meter for a_lockMs so the payoff's
		// own duration cannot be refilled into a second one.
		constexpr BuildUpGain Gain(
			std::uint32_t a_target, StatusKind a_kind, float a_amount, std::int64_t a_nowMs, bool a_tough,
			std::int64_t a_lockMs) noexcept
		{
			const auto index = BuildUpIndex(a_kind);
			if (!index || a_target == 0u || a_amount <= 0.0f) {
				return {};
			}
			auto& slot = SlotFor(a_target, a_nowMs);
			const auto i = *index;
			BuildUpGain result{};
			result.threshold = BuildUpThreshold(a_tough, slot.payoffs[i]);
			if (slot.lockedUntilMs[i] > a_nowMs) {
				return result;
			}
			result.applied = true;
			result.value = DecayedBuildUp(slot.values[i], slot.lastGainMs[i], a_nowMs) + a_amount;
			slot.lastGainMs[i] = a_nowMs;
			if (result.value >= result.threshold) {
				result.paidOff = true;
				result.value = 0.0f;
				if (slot.payoffs[i] < 255u) {
					++slot.payoffs[i];
				}
				slot.lockedUntilMs[i] = a_nowMs + (a_lockMs > 0 ? a_lockMs : 0);
			}
			slot.values[i] = result.value;
			return result;
		}

		[[nodiscard]] constexpr float Value(std::uint32_t a_target, StatusKind a_kind, std::int64_t a_nowMs) const noexcept
		{
			const auto index = BuildUpIndex(a_kind);
			const auto* slot = FindSlot(a_target);
			if (!index || !slot) {
				return 0.0f;
			}
			return DecayedBuildUp(slot->values[*index], slot->lastGainMs[*index], a_nowMs);
		}

		// Adds Burning stacks (capped) and restarts its timer; returns the new count.
		constexpr std::uint8_t AddBurning(std::uint32_t a_target, std::uint8_t a_stacks, std::int64_t a_nowMs) noexcept
		{
			if (a_target == 0u || a_stacks == 0u) {
				return BurningStacks(a_target, a_nowMs);
			}
			auto& slot = SlotFor(a_target, a_nowMs);
			const std::uint32_t current = slot.burningExpiresMs > a_nowMs ? slot.burningStacks : 0u;
			const std::uint32_t added = current + a_stacks;
			slot.burningStacks = static_cast<std::uint8_t>(
				added < BuildUpRules::kBurningMaxStacks ? added : BuildUpRules::kBurningMaxStacks);
			slot.burningExpiresMs = a_nowMs + BuildUpRules::kBurningDurationMs;
			return slot.burningStacks;
		}

		[[nodiscard]] constexpr std::uint8_t BurningStacks(std::uint32_t a_target, std::int64_t a_nowMs) const noexcept
		{
			const auto* slot = FindSlot(a_target);
			return slot && slot->burningExpiresMs > a_nowMs ? slot->burningStacks : std::uint8_t{ 0u };
		}

		// The target's current meters and stacks, removing the target (a death).
		constexpr StatusMeterSnapshot Take(std::uint32_t a_target, std::int64_t a_nowMs) noexcept
		{
			StatusMeterSnapshot snapshot{};
			if (const auto* slot = FindSlot(a_target)) {
				for (std::size_t i = 0; i < kBuildUpKindCount; ++i) {
					snapshot.values[i] = DecayedBuildUp(slot->values[i], slot->lastGainMs[i], a_nowMs);
				}
				snapshot.burningStacks = BurningStacks(a_target, a_nowMs);
			}
			Erase(a_target);
			return snapshot;
		}

		constexpr void Erase(std::uint32_t a_target) noexcept
		{
			std::size_t kept = 0u;
			for (std::size_t i = 0; i < _count; ++i) {
				if (_slots[i].target != a_target) {
					_slots[kept++] = _slots[i];
				}
			}
			_count = kept;
		}

		constexpr void Clear() noexcept { _count = 0u; }
		[[nodiscard]] constexpr std::size_t Size() const noexcept { return _count; }

	private:
		std::array<Slot, kCapacity> _slots{};
		std::size_t _count{ 0u };
	};
}
