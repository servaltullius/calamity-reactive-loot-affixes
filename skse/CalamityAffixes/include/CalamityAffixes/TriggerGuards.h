#pragma once

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
