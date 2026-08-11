#pragma once

#include <chrono>
#include <cstdint>

namespace CalamityAffixes::detail
{
	inline constexpr std::uint8_t kMaxTrapMarkerAnimationAttempts = 8u;
	inline constexpr auto kTrapMarkerAnimationRetryDelay = std::chrono::milliseconds(250);
	inline constexpr auto kTrapWorldMarkerProbeLifetime = std::chrono::seconds(3);
	inline constexpr auto kTrapWorldMarkerProbeArmAfterExpiry = std::chrono::milliseconds(1);

	struct TrapWorldMarkerProbeWindow
	{
		std::chrono::steady_clock::time_point expiresAt{};
		std::chrono::steady_clock::time_point armedAt{};
	};

	[[nodiscard]] constexpr TrapWorldMarkerProbeWindow BuildTrapWorldMarkerProbeWindow(
		std::chrono::steady_clock::time_point a_now) noexcept
	{
		const auto expiresAt = a_now + kTrapWorldMarkerProbeLifetime;
		return {
			expiresAt,
			expiresAt + kTrapWorldMarkerProbeArmAfterExpiry,
		};
	}

	[[nodiscard]] constexpr bool IsTrapMarkerAnimationGraphReady(
		bool a_has3D,
		bool a_hasGraphManager,
		bool a_hasLoadedGraph) noexcept
	{
		return a_has3D && a_hasGraphManager && a_hasLoadedGraph;
	}

	[[nodiscard]] constexpr bool ShouldRetryTrapMarkerAnimation(
		bool a_referenceUsable,
		bool a_accepted,
		std::uint8_t a_attempt) noexcept
	{
		return a_referenceUsable && !a_accepted &&
			a_attempt < kMaxTrapMarkerAnimationAttempts;
	}

	[[nodiscard]] constexpr std::chrono::steady_clock::time_point ExtendTrapArmedAtForAcceptedOpenAnimation(
		std::chrono::steady_clock::time_point a_armedAt,
		std::chrono::steady_clock::time_point a_acceptedAt,
		std::chrono::milliseconds a_openGate) noexcept
	{
		const auto gateEndsAt = a_acceptedAt + a_openGate;
		return a_armedAt < gateEndsAt ? gateEndsAt : a_armedAt;
	}

	[[nodiscard]] constexpr bool ShouldStartTrapMarkerRearmAnimation(
		std::chrono::steady_clock::time_point a_now,
		std::chrono::steady_clock::time_point a_armedAt,
		std::chrono::milliseconds a_openGate) noexcept
	{
		return a_now + a_openGate >= a_armedAt;
	}

	[[nodiscard]] constexpr bool ShouldBlockTrapCastForMarkerAnimation(
		std::chrono::steady_clock::time_point a_now,
		std::chrono::steady_clock::time_point a_armedAt,
		bool a_requiredAnimationEventPending) noexcept
	{
		return a_now < a_armedAt || a_requiredAnimationEventPending;
	}
}
