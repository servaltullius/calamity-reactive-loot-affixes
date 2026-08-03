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
