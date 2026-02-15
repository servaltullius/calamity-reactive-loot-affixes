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
}
