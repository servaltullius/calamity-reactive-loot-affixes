#pragma once

#include <cstdint>
#include <string_view>

namespace CalamityAffixes::detail
{
	enum class WeaponSubtypeGroup : std::uint8_t
	{
		kUnknown = 0,
		kOneHandedMelee,
		kTwoHandedMelee,
		kBow,
		kCrossbow,
		kOther,
	};

	inline constexpr std::uint8_t kWeaponSubtypeOneHandedMelee = 1u << 0u;
	inline constexpr std::uint8_t kWeaponSubtypeTwoHandedMelee = 1u << 1u;
	inline constexpr std::uint8_t kWeaponSubtypeBow = 1u << 2u;
	inline constexpr std::uint8_t kWeaponSubtypeCrossbow = 1u << 3u;
	inline constexpr std::uint8_t kWeaponSubtypeInvalid = 1u << 7u;

	[[nodiscard]] constexpr WeaponSubtypeGroup ClassifyWeaponSubtype(std::uint8_t a_weaponType) noexcept
	{
		// RE::WEAPON_TYPE is stable across supported runtimes:
		// 1-4 one-handed melee, 5-6 two-handed melee, 7 bow, 8 staff, 9 crossbow.
		switch (a_weaponType) {
		case 1u:
		case 2u:
		case 3u:
		case 4u:
			return WeaponSubtypeGroup::kOneHandedMelee;
		case 5u:
		case 6u:
			return WeaponSubtypeGroup::kTwoHandedMelee;
		case 7u:
			return WeaponSubtypeGroup::kBow;
		case 9u:
			return WeaponSubtypeGroup::kCrossbow;
		case 8u:
			return WeaponSubtypeGroup::kOther;
		default:
			return WeaponSubtypeGroup::kUnknown;
		}
	}

	template <class WeaponT>
	[[nodiscard]] WeaponSubtypeGroup ResolveWeaponSubtype(const WeaponT* a_weapon) noexcept
	{
		if (!a_weapon) {
			return WeaponSubtypeGroup::kUnknown;
		}
		return ClassifyWeaponSubtype(static_cast<std::uint8_t>(a_weapon->GetWeaponType()));
	}

	[[nodiscard]] constexpr std::uint8_t WeaponSubtypeMaskForContractKey(std::string_view a_key) noexcept
	{
		if (a_key == "OneHandedMelee") {
			return kWeaponSubtypeOneHandedMelee;
		}
		if (a_key == "TwoHandedMelee") {
			return kWeaponSubtypeTwoHandedMelee;
		}
		if (a_key == "Bow") {
			return kWeaponSubtypeBow;
		}
		if (a_key == "Crossbow") {
			return kWeaponSubtypeCrossbow;
		}
		return 0u;
	}

	[[nodiscard]] constexpr std::uint8_t WeaponSubtypeMaskBit(WeaponSubtypeGroup a_subtype) noexcept
	{
		switch (a_subtype) {
		case WeaponSubtypeGroup::kOneHandedMelee:
			return kWeaponSubtypeOneHandedMelee;
		case WeaponSubtypeGroup::kTwoHandedMelee:
			return kWeaponSubtypeTwoHandedMelee;
		case WeaponSubtypeGroup::kBow:
			return kWeaponSubtypeBow;
		case WeaponSubtypeGroup::kCrossbow:
			return kWeaponSubtypeCrossbow;
		default:
			return 0u;
		}
	}

	[[nodiscard]] constexpr bool IsSuffixWeaponSubtypeEligible(
		std::uint8_t a_allowedMask,
		bool a_isWeapon,
		WeaponSubtypeGroup a_subtype) noexcept
	{
		if (!a_isWeapon) {
			return true;
		}
		if (a_allowedMask == 0u) {
			return true;
		}
		if ((a_allowedMask & kWeaponSubtypeInvalid) != 0u) {
			return false;
		}
		const auto subtypeBit = WeaponSubtypeMaskBit(a_subtype);
		return subtypeBit != 0u && (a_allowedMask & subtypeBit) != 0u;
	}

	enum class RunewordBaseKind : std::uint8_t
	{
		kUnknown = 0,
		kOneHandedMelee,
		kTwoHandedMelee,
		kBowOrCrossbow,
		kStaff,
		kShield,
		kHeavyArmor,
		kLightArmor,
		kClothing,
		kOtherArmor,
	};

	enum class SpecializedRunewordBase : std::uint8_t
	{
		kNone = 0,
		kOneHandedMelee,
		kTwoHandedMelee,
		kBowOrCrossbow,
		kHeavyArmor,
		kShield,
	};

	[[nodiscard]] constexpr RunewordBaseKind ResolveRunewordWeaponBaseKind(std::uint8_t a_weaponType) noexcept
	{
		switch (ClassifyWeaponSubtype(a_weaponType)) {
		case WeaponSubtypeGroup::kOneHandedMelee:
			return RunewordBaseKind::kOneHandedMelee;
		case WeaponSubtypeGroup::kTwoHandedMelee:
			return RunewordBaseKind::kTwoHandedMelee;
		case WeaponSubtypeGroup::kBow:
		case WeaponSubtypeGroup::kCrossbow:
			return RunewordBaseKind::kBowOrCrossbow;
		case WeaponSubtypeGroup::kOther:
			return RunewordBaseKind::kStaff;
		default:
			return RunewordBaseKind::kUnknown;
		}
	}

	[[nodiscard]] constexpr RunewordBaseKind ResolveRunewordArmorBaseKind(
		bool a_isShield,
		bool a_isHeavyArmor,
		bool a_isLightArmor,
		bool a_isClothing) noexcept
	{
		if (a_isShield) {
			return RunewordBaseKind::kShield;
		}
		if (a_isHeavyArmor) {
			return RunewordBaseKind::kHeavyArmor;
		}
		if (a_isLightArmor) {
			return RunewordBaseKind::kLightArmor;
		}
		if (a_isClothing) {
			return RunewordBaseKind::kClothing;
		}
		return RunewordBaseKind::kOtherArmor;
	}

	[[nodiscard]] constexpr SpecializedRunewordBase ResolveSpecializedRunewordBase(std::string_view a_recipeId) noexcept
	{
		if (a_recipeId == "rw_strength") {
			return SpecializedRunewordBase::kTwoHandedMelee;
		}
		if (a_recipeId == "rw_edge" || a_recipeId == "rw_zephyr") {
			return SpecializedRunewordBase::kBowOrCrossbow;
		}
		if (a_recipeId == "rw_oath") {
			return SpecializedRunewordBase::kOneHandedMelee;
		}
		if (a_recipeId == "rw_myth") {
			return SpecializedRunewordBase::kHeavyArmor;
		}
		if (a_recipeId == "rw_unbending_will") {
			return SpecializedRunewordBase::kShield;
		}
		return SpecializedRunewordBase::kNone;
	}

	[[nodiscard]] constexpr bool IsSpecializedRunewordBaseCompatible(
		SpecializedRunewordBase a_requirement,
		RunewordBaseKind a_actual) noexcept
	{
		switch (a_requirement) {
		case SpecializedRunewordBase::kNone:
			return true;
		case SpecializedRunewordBase::kOneHandedMelee:
			return a_actual == RunewordBaseKind::kOneHandedMelee;
		case SpecializedRunewordBase::kTwoHandedMelee:
			return a_actual == RunewordBaseKind::kTwoHandedMelee;
		case SpecializedRunewordBase::kBowOrCrossbow:
			return a_actual == RunewordBaseKind::kBowOrCrossbow;
		case SpecializedRunewordBase::kHeavyArmor:
			return a_actual == RunewordBaseKind::kHeavyArmor;
		case SpecializedRunewordBase::kShield:
			return a_actual == RunewordBaseKind::kShield;
		}
		return false;
	}

	[[nodiscard]] constexpr std::string_view DescribeSpecializedRunewordBaseEn(SpecializedRunewordBase a_requirement) noexcept
	{
		switch (a_requirement) {
		case SpecializedRunewordBase::kOneHandedMelee: return "one-handed melee weapon";
		case SpecializedRunewordBase::kTwoHandedMelee: return "two-handed melee weapon";
		case SpecializedRunewordBase::kBowOrCrossbow: return "bow or crossbow";
		case SpecializedRunewordBase::kHeavyArmor: return "heavy armor";
		case SpecializedRunewordBase::kShield: return "shield";
		default: return "compatible base";
		}
	}

	[[nodiscard]] constexpr std::string_view DescribeSpecializedRunewordBaseKo(SpecializedRunewordBase a_requirement) noexcept
	{
		switch (a_requirement) {
		case SpecializedRunewordBase::kOneHandedMelee: return "한손 근접 무기";
		case SpecializedRunewordBase::kTwoHandedMelee: return "양손 근접 무기";
		case SpecializedRunewordBase::kBowOrCrossbow: return "활 또는 석궁";
		case SpecializedRunewordBase::kHeavyArmor: return "중갑";
		case SpecializedRunewordBase::kShield: return "방패";
		default: return "호환 베이스";
		}
	}
}
