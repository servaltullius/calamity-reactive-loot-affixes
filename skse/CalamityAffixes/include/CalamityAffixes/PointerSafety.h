#pragma once

#include <cstdint>

namespace CalamityAffixes
{
	inline constexpr std::uintptr_t kMinLikelyObjectAddress = 0x10000u;
	inline constexpr std::uintptr_t kMaxLikelyUserObjectAddress = 0x00007FFFFFFFFFFFu;

	[[nodiscard]] constexpr bool IsLikelyUserAddress(std::uintptr_t a_address) noexcept
	{
		if constexpr (sizeof(std::uintptr_t) >= 8) {
			return a_address <= kMaxLikelyUserObjectAddress;
		}
		return true;
	}

	[[nodiscard]] constexpr bool IsLikelyValidObjectAddress(std::uintptr_t a_address) noexcept
	{
		return a_address >= kMinLikelyObjectAddress &&
		       IsLikelyUserAddress(a_address) &&
		       (a_address % alignof(void*) == 0u);
	}

	[[nodiscard]] constexpr bool ShouldProcessHealthDamageHookPointers(
		std::uintptr_t a_targetAddress,
		std::uintptr_t a_attackerAddress) noexcept
	{
		return IsLikelyValidObjectAddress(a_targetAddress) &&
		       IsLikelyValidObjectAddress(a_attackerAddress);
	}

	template <class TTarget, class TAttacker>
	[[nodiscard]] inline bool ShouldProcessHealthDamageHookPointers(
		const TTarget* a_target,
		const TAttacker* a_attacker) noexcept
	{
		return ShouldProcessHealthDamageHookPointers(
			reinterpret_cast<std::uintptr_t>(a_target),
			reinterpret_cast<std::uintptr_t>(a_attacker));
	}

	template <class T>
	[[nodiscard]] inline bool IsLikelyValidObjectPointer(const T* a_ptr) noexcept
	{
		return IsLikelyValidObjectAddress(reinterpret_cast<std::uintptr_t>(a_ptr));
	}

	template <class T>
	[[nodiscard]] inline T* SanitizeObjectPointer(T* a_ptr) noexcept
	{
		return IsLikelyValidObjectPointer(a_ptr) ? a_ptr : nullptr;
	}
}
