#pragma once

#include <cstddef>

namespace CalamityAffixes::detail
{
	// Squared distance between a trap and a candidate target.
	//
	// Squared throughout: the radius test is the only consumer and comparing
	// against radius * radius avoids a sqrt per (trap x actor) pair.
	[[nodiscard]] constexpr float TrapTargetDistanceSq(float a_dx, float a_dy, float a_dz) noexcept
	{
		return (a_dx * a_dx) + (a_dy * a_dy) + (a_dz * a_dz);
	}

	// Inclusive: an actor standing exactly on the radius is inside it.
	[[nodiscard]] constexpr bool IsWithinTrapRadiusSq(float a_distanceSq, float a_radiusSq) noexcept
	{
		return a_distanceSq <= a_radiusSq;
	}

	// Owner-independent half of the per-tick target filter.
	//
	// Split out from the radius test because the trap tick evaluates one
	// shared actor snapshot against many traps: this part depends on the trap
	// (its owner) while the snapshot itself does not.
	[[nodiscard]] constexpr bool IsTrapTickTargetEligible(
		bool a_isTrapOwner,
		bool a_isDead,
		bool a_hostileToOwner) noexcept
	{
		return !a_isTrapOwner && !a_isDead && a_hostileToOwner;
	}

	// Whether a trap may spend a_cost more casts this tick.
	//
	// A budget of 0 means "unmetered", matching the runtime setting where 0
	// disables the cap rather than banning all casts.
	[[nodiscard]] constexpr bool HasTrapCastBudget(
		std::size_t a_consumed,
		std::size_t a_budgetPerTick,
		std::size_t a_cost = 1u) noexcept
	{
		return a_budgetPerTick == 0u || (a_consumed + a_cost) <= a_budgetPerTick;
	}
}
