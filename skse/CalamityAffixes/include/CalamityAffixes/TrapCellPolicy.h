#pragma once

#include <cstddef>

namespace CalamityAffixes::detail
{
	inline constexpr std::size_t kMaxPlacedTrapMarkers = 48u;

	struct PlacedTrapMarkerCleanupPolicy
	{
		bool disable{ false };
		bool markForDeletion{ false };
	};

	[[nodiscard]] constexpr bool IsTrapCellUsable(
		bool a_hasCell,
		bool a_cellAttached) noexcept
	{
		return a_hasCell && a_cellAttached;
	}

	[[nodiscard]] constexpr bool CanSpawnPlacedTrapMarker(
		std::size_t a_activeMarkerCount,
		std::size_t a_pendingCleanupCount = 0u) noexcept
	{
		return a_activeMarkerCount < kMaxPlacedTrapMarkers &&
			a_pendingCleanupCount < kMaxPlacedTrapMarkers - a_activeMarkerCount;
	}

	[[nodiscard]] constexpr bool CanReserveLogicalTrapSlots(
		std::size_t a_activeTrapCount,
		std::size_t a_configuredCap,
		std::size_t a_requestedSlots) noexcept
	{
		return a_configuredCap == 0u ||
			(a_activeTrapCount <= a_configuredCap &&
				a_requestedSlots <= a_configuredCap - a_activeTrapCount);
	}

	[[nodiscard]] constexpr bool ShouldDeferUnresolvedPlacedTrapMarker(
		bool a_handleAllocated,
		bool a_referenceResolved) noexcept
	{
		return a_handleAllocated && !a_referenceResolved;
	}

	[[nodiscard]] constexpr bool ShouldReusePlacedTrapMarker(
		bool a_referenceResolved,
		bool a_disabled,
		bool a_markedForDeletion) noexcept
	{
		return a_referenceResolved && !a_disabled && !a_markedForDeletion;
	}

	[[nodiscard]] constexpr PlacedTrapMarkerCleanupPolicy ResolvePlacedTrapMarkerCleanupPolicy(
		bool a_referenceResolved,
		bool a_hasParentCell,
		bool a_parentCellAttached) noexcept
	{
		return {
			a_referenceResolved && (!a_hasParentCell || a_parentCellAttached),
			a_referenceResolved
		};
	}
}
