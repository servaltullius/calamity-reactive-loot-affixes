#pragma once

#include "CalamityAffixes/InstanceAffixSlots.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace CalamityAffixes
{
	enum class ProcChanceDisplayMode : std::uint8_t
	{
		kCurrent,
		kEquippedAloneEstimate,
		kInactive
	};

	inline constexpr std::array<float, kMaxAffixesPerItem> kMultiAffixProcPenalty{
		1.0f,
		0.8f,
		0.65f,
		0.5f
	};

	// The tier input is the item's PROC-CAPABLE (non-suffix) slot count. The penalty
	// exists to damp proc-affix stacking on a single item; passive suffix slots on the
	// same item must not raise the tier (a lone proc prefix next to three passive
	// suffixes stays at 100%).
	[[nodiscard]] constexpr float ResolveMultiAffixProcPenalty(std::uint8_t a_slotCount) noexcept
	{
		if (a_slotCount == 0u) {
			return 1.0f;
		}

		const auto clampedCount = std::min<std::uint8_t>(
			a_slotCount,
			static_cast<std::uint8_t>(kMaxAffixesPerItem));
		return kMultiAffixProcPenalty[clampedCount - 1u];
	}

	// An affix occupies a proc-penalty slot only if it can actually proc. The
	// anti-stacking tier must ignore non-suffix affixes that never roll: passive
	// prefixes (runeword auras, scroll mastery tiers) and internal helper entries
	// carry procChancePct == 0 or a DebugNotify action and must not drag down the
	// real proc affixes sharing their item.
	[[nodiscard]] constexpr bool IsProcPenaltyEligible(
		bool a_isSuffixSlot,
		float a_procChancePct,
		bool a_isProcCapableAction) noexcept
	{
		return !a_isSuffixSlot && a_procChancePct > 0.0f && a_isProcCapableAction;
	}

	template <class IsProcCapableSlotFn>
	[[nodiscard]] constexpr std::uint8_t CountProcCapableSlots(
		std::uint8_t a_slotCount,
		IsProcCapableSlotFn&& a_isProcCapableSlot) noexcept
	{
		std::uint8_t procSlots = 0u;
		for (std::uint8_t i = 0u; i < a_slotCount; ++i) {
			if (a_isProcCapableSlot(i)) {
				++procSlots;
			}
		}
		return procSlots;
	}

	[[nodiscard]] constexpr float ResolveEffectiveProcChancePct(
		float a_baseChancePct,
		float a_runtimeMultiplier,
		float a_slotPenalty) noexcept
	{
		return std::clamp(a_baseChancePct * a_runtimeMultiplier * a_slotPenalty, 0.0f, 100.0f);
	}

	[[nodiscard]] constexpr bool ShouldShowAdjustedProcChance(
		float a_baseChancePct,
		float a_effectiveChancePct) noexcept
	{
		const float clampedBase = std::clamp(a_baseChancePct, 0.0f, 100.0f);
		const float difference = clampedBase >= a_effectiveChancePct ?
			clampedBase - a_effectiveChancePct :
			a_effectiveChancePct - clampedBase;
		return difference >= 0.05f;
	}

	[[nodiscard]] constexpr ProcChanceDisplayMode ResolveProcChanceDisplayMode(
		bool a_runtimeEnabled,
		bool a_candidateWorn,
		bool a_equippedTokenCacheReady,
		bool a_exactInstanceCached,
		bool a_affixActive) noexcept
	{
		if (!a_runtimeEnabled) {
			return ProcChanceDisplayMode::kInactive;
		}
		if (!a_candidateWorn) {
			return ProcChanceDisplayMode::kEquippedAloneEstimate;
		}
		if (a_equippedTokenCacheReady && a_exactInstanceCached && a_affixActive) {
			return ProcChanceDisplayMode::kCurrent;
		}
		return ProcChanceDisplayMode::kInactive;
	}

	[[nodiscard]] constexpr float ResolveDisplayedProcChancePct(
		ProcChanceDisplayMode a_displayMode,
		bool a_usesStandardTriggerProcLane,
		float a_baseChancePct,
		float a_runtimeMultiplier,
		float a_localSlotPenalty,
		float a_currentTriggerChancePct) noexcept
	{
		if (a_displayMode == ProcChanceDisplayMode::kCurrent && a_usesStandardTriggerProcLane) {
			return std::clamp(a_currentTriggerChancePct, 0.0f, 100.0f);
		}

		const float slotPenalty = a_usesStandardTriggerProcLane ? a_localSlotPenalty : 1.0f;
		return ResolveEffectiveProcChancePct(a_baseChancePct, a_runtimeMultiplier, slotPenalty);
	}
}
