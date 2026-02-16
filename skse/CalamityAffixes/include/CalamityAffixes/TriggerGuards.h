#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

namespace CalamityAffixes
{
	struct DuplicateHitKey
	{
		bool outgoing{ false };
		std::uint32_t aggressor{ 0 };
		std::uint32_t target{ 0 };
		std::uint32_t source{ 0 };

		[[nodiscard]] constexpr bool operator==(const DuplicateHitKey& a_rhs) const noexcept = default;
	};

	[[nodiscard]] constexpr bool ShouldSuppressDuplicateHitWindow(
		const DuplicateHitKey& a_current,
		const DuplicateHitKey& a_last,
		std::uint64_t a_nowMs,
		std::uint64_t a_lastMs,
		std::uint64_t a_windowMs) noexcept
	{
		if (a_windowMs == 0u) {
			return false;
		}
		if (a_nowMs < a_lastMs) {
			return false;
		}
		if ((a_nowMs - a_lastMs) >= a_windowMs) {
			return false;
		}

		return a_current == a_last;
	}

	[[nodiscard]] constexpr std::optional<std::uint64_t> ComputePerTargetCooldownNextAllowedMs(
		std::uint64_t a_nowMs,
		std::int64_t a_perTargetIcdMs,
		std::uint64_t a_token,
		std::uint32_t a_targetFormId) noexcept
	{
		if (a_perTargetIcdMs <= 0 || a_token == 0u || a_targetFormId == 0u) {
			return std::nullopt;
		}

		return a_nowMs + static_cast<std::uint64_t>(a_perTargetIcdMs);
	}

	[[nodiscard]] constexpr bool IsPerTargetCooldownBlockedMs(
		std::uint64_t a_nowMs,
		std::uint64_t a_nextAllowedMs) noexcept
	{
		return a_nowMs < a_nextAllowedMs;
	}

	// Shared "recently" window helper.
	// - a_windowMs == 0 means "disabled / always pass"
	// - a_lastEventMs == 0 with enabled window means "no history / fail"
	[[nodiscard]] constexpr bool IsWithinRecentlyWindowMs(
		std::uint64_t a_nowMs,
		std::uint64_t a_lastEventMs,
		std::uint64_t a_windowMs) noexcept
	{
		if (a_windowMs == 0u) {
			return true;
		}
		if (a_lastEventMs == 0u || a_nowMs < a_lastEventMs) {
			return false;
		}
		return (a_nowMs - a_lastEventMs) <= a_windowMs;
	}

	// "Not hit recently" helper.
	// - a_windowMs == 0 means "disabled / always pass"
	// - a_lastHitMs == 0 with enabled window means "never hit yet / pass"
	[[nodiscard]] constexpr bool IsOutsideRecentlyWindowMs(
		std::uint64_t a_nowMs,
		std::uint64_t a_lastHitMs,
		std::uint64_t a_windowMs) noexcept
	{
		if (a_windowMs == 0u) {
			return true;
		}
		if (a_lastHitMs == 0u) {
			return true;
		}
		if (a_nowMs < a_lastHitMs) {
			return false;
		}
		return (a_nowMs - a_lastHitMs) >= a_windowMs;
	}

	// Diablo-like lucky-hit shape:
	// effective chance = base chance * proc coefficient (clamped).
	[[nodiscard]] constexpr float ResolveLuckyHitEffectiveChancePct(
		float a_baseChancePct,
		float a_procCoefficient) noexcept
	{
		if (a_baseChancePct <= 0.0f || a_procCoefficient <= 0.0f) {
			return 0.0f;
		}

		const float clampedBase = std::clamp(a_baseChancePct, 0.0f, 100.0f);
		const float clampedCoefficient = std::clamp(a_procCoefficient, 0.0f, 5.0f);
		return std::clamp(clampedBase * clampedCoefficient, 0.0f, 100.0f);
	}

	// Shared fixed-window budget gate used to cap bursty proc execution.
	[[nodiscard]] constexpr bool TryConsumeFixedWindowBudget(
		std::uint64_t a_nowMs,
		std::uint64_t a_windowMs,
		std::uint32_t a_maxPerWindow,
		std::uint64_t& a_inOutWindowStartMs,
		std::uint32_t& a_inOutConsumed) noexcept
	{
		// 0 means "disabled / unlimited".
		if (a_windowMs == 0u || a_maxPerWindow == 0u) {
			return true;
		}

		const bool staleWindow =
			a_inOutWindowStartMs == 0u ||
			a_nowMs < a_inOutWindowStartMs ||
			(a_nowMs - a_inOutWindowStartMs) >= a_windowMs;
		if (staleWindow) {
			a_inOutWindowStartMs = a_nowMs;
			a_inOutConsumed = 0u;
		}

		if (a_inOutConsumed >= a_maxPerWindow) {
			return false;
		}

		a_inOutConsumed += 1u;
		return true;
	}

	// Safety guard for trigger actions that are configured as guaranteed proc with zero ICD.
	// This prevents runaway proc storms without changing non-guaranteed behavior.
	[[nodiscard]] constexpr std::int64_t ResolveTriggerProcCooldownMs(
		std::int64_t a_configuredIcdMs,
		bool a_hasPerTargetIcd,
		float a_procChancePct,
		std::int64_t a_zeroIcdSafetyGuardMs) noexcept
	{
		if (a_configuredIcdMs > 0) {
			return a_configuredIcdMs;
		}
		if (a_hasPerTargetIcd) {
			return 0;
		}
		if (a_procChancePct >= 100.0f && a_zeroIcdSafetyGuardMs > 0) {
			return a_zeroIcdSafetyGuardMs;
		}
		return 0;
	}

	[[nodiscard]] constexpr bool ShouldSendPlayerOwnedHitEvent(
		bool a_isPlayerOwnedAggressor,
		bool a_hasPlayerOwner,
		bool a_hostileEitherDirection,
		bool a_allowNonHostilePlayerOwnedOutgoing,
		bool a_targetIsPlayer) noexcept
	{
		if (!a_isPlayerOwnedAggressor || !a_hasPlayerOwner) {
			return false;
		}

		if (a_hostileEitherDirection) {
			return true;
		}

		return a_allowNonHostilePlayerOwnedOutgoing && !a_targetIsPlayer;
	}

	[[nodiscard]] constexpr bool ShouldResolveNonHostileOutgoingFirstHitAllowance(
		bool a_hasPlayerOwner,
		bool a_targetIsPlayer,
		bool a_allowNonHostilePlayerOwnedOutgoing) noexcept
	{
		return a_hasPlayerOwner && !a_targetIsPlayer && a_allowNonHostilePlayerOwnedOutgoing;
	}

	[[nodiscard]] constexpr bool ShouldProcessHealthDamageProcPath(
		bool a_hasTarget,
		bool a_hasAttacker,
		bool a_targetIsPlayer,
		bool a_attackerIsPlayerOwned,
		bool a_hasPlayerOwner,
		bool a_hostileEitherDirection,
		bool a_allowNonHostilePlayerOwnedOutgoing) noexcept
	{
		if (!a_hasTarget || !a_hasAttacker) {
			return false;
		}

		if (a_hostileEitherDirection) {
			if (a_targetIsPlayer) {
				return true;
			}

			return a_attackerIsPlayerOwned && a_hasPlayerOwner;
		}

		if (a_allowNonHostilePlayerOwnedOutgoing &&
			!a_targetIsPlayer &&
			a_attackerIsPlayerOwned &&
			a_hasPlayerOwner) {
			return true;
		}

		return false;
	}

	[[nodiscard]] constexpr bool ShouldHandleScrollConsumePreservation(
		std::uint32_t a_oldContainer,
		std::uint32_t a_newContainer,
		std::int32_t a_itemCount,
		std::uint32_t a_baseObj,
		std::uint32_t a_referenceHandle,
		std::uint32_t a_playerId) noexcept
	{
		// Scroll consume is modeled as player inventory -> null container with no world reference.
		// Player drop to world also uses newContainer == 0, but keeps a valid reference handle.
		if (a_playerId == 0u) {
			return false;
		}
		if (a_oldContainer != a_playerId || a_newContainer != 0u) {
			return false;
		}
		if (a_itemCount <= 0 || a_baseObj == 0u) {
			return false;
		}
		if (a_referenceHandle != 0u) {
			return false;
		}
		return true;
	}
}
