#pragma once

#include <array>
#include <chrono>
#include <cstddef>

#include <RE/Skyrim.h>

#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes::Hooks::detail
{
	struct DeferredConversionCast
	{
		RE::FormID spellFormID{ 0u };
		float effectiveness{ 1.0f };
		bool noHitEffectArt{ false };
		float magnitudeOverride{ 0.0f };
	};

	struct DamageAdjustmentResult
	{
		float adjustedDamage{ 0.0f };
		float originalDamage{ 0.0f };
		static constexpr std::size_t kMaxConversions = CalamityAffixes::EventBridge::kMaxConversionsPerHit;
		std::array<DeferredConversionCast, kMaxConversions> conversions{};
		std::size_t conversionCount{ 0 };
	};

	[[nodiscard]] bool IsInProcDispatchGuard() noexcept;
	[[nodiscard]] const RE::HitData* ResolveStableHitDataForSpecialActions(
		const RE::HitData* a_hitData,
		const RE::Actor* a_target,
		RE::Actor* a_attacker) noexcept;
	[[nodiscard]] bool ShouldAllowProcDispatch(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_preHitData,
		float a_damage,
		std::chrono::steady_clock::time_point a_now) noexcept;
	[[nodiscard]] DamageAdjustmentResult AdjustDamageAndEvaluateSpecials(
		CalamityAffixes::EventBridge* a_bridge,
		RE::Actor* a_attacker,
		RE::Actor* a_target,
		const RE::HitData* a_preHitData,
		float a_damage) noexcept;
	void SchedulePostHealthDamageActions(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const DamageAdjustmentResult& a_adjustment,
		std::chrono::steady_clock::time_point a_now,
		const RE::HitData* a_preHitData) noexcept;
	void ClearDispatchRuntimeState() noexcept;
}
