#include "CalamityAffixes/ItemSubtypePolicy.h"

using namespace CalamityAffixes::detail;

static_assert(ClassifyWeaponSubtype(1u) == WeaponSubtypeGroup::kOneHandedMelee);
static_assert(ClassifyWeaponSubtype(4u) == WeaponSubtypeGroup::kOneHandedMelee);
static_assert(ClassifyWeaponSubtype(5u) == WeaponSubtypeGroup::kTwoHandedMelee);
static_assert(ClassifyWeaponSubtype(6u) == WeaponSubtypeGroup::kTwoHandedMelee);
static_assert(ClassifyWeaponSubtype(7u) == WeaponSubtypeGroup::kBow);
static_assert(ClassifyWeaponSubtype(8u) == WeaponSubtypeGroup::kOther);
static_assert(ClassifyWeaponSubtype(9u) == WeaponSubtypeGroup::kCrossbow);
static_assert(ClassifyWeaponSubtype(0u) == WeaponSubtypeGroup::kUnknown);

static_assert(WeaponSubtypeMaskForContractKey("OneHandedMelee") == kWeaponSubtypeOneHandedMelee);
static_assert(WeaponSubtypeMaskForContractKey("TwoHandedMelee") == kWeaponSubtypeTwoHandedMelee);
static_assert(WeaponSubtypeMaskForContractKey("Bow") == kWeaponSubtypeBow);
static_assert(WeaponSubtypeMaskForContractKey("Crossbow") == kWeaponSubtypeCrossbow);
static_assert(WeaponSubtypeMaskForContractKey("Staff") == 0u);

static_assert(IsSuffixWeaponSubtypeEligible(0u, true, WeaponSubtypeGroup::kOther));
static_assert(IsSuffixWeaponSubtypeEligible(kWeaponSubtypeOneHandedMelee, false, WeaponSubtypeGroup::kUnknown));
static_assert(IsSuffixWeaponSubtypeEligible(kWeaponSubtypeOneHandedMelee, true, WeaponSubtypeGroup::kOneHandedMelee));
static_assert(!IsSuffixWeaponSubtypeEligible(kWeaponSubtypeOneHandedMelee, true, WeaponSubtypeGroup::kTwoHandedMelee));
static_assert(!IsSuffixWeaponSubtypeEligible(kWeaponSubtypeOneHandedMelee, true, WeaponSubtypeGroup::kBow));
static_assert(IsSuffixWeaponSubtypeEligible(kWeaponSubtypeBow | kWeaponSubtypeCrossbow, true, WeaponSubtypeGroup::kBow));
static_assert(IsSuffixWeaponSubtypeEligible(kWeaponSubtypeBow | kWeaponSubtypeCrossbow, true, WeaponSubtypeGroup::kCrossbow));
static_assert(!IsSuffixWeaponSubtypeEligible(kWeaponSubtypeBow | kWeaponSubtypeCrossbow, true, WeaponSubtypeGroup::kOneHandedMelee));
static_assert(!IsSuffixWeaponSubtypeEligible(kWeaponSubtypeInvalid, true, WeaponSubtypeGroup::kOneHandedMelee));

static_assert(ResolveSpecializedRunewordBase("rw_strength") == SpecializedRunewordBase::kTwoHandedMelee);
static_assert(ResolveSpecializedRunewordBase("rw_edge") == SpecializedRunewordBase::kBowOrCrossbow);
static_assert(ResolveSpecializedRunewordBase("rw_zephyr") == SpecializedRunewordBase::kBowOrCrossbow);
static_assert(ResolveSpecializedRunewordBase("rw_oath") == SpecializedRunewordBase::kOneHandedMelee);
static_assert(ResolveSpecializedRunewordBase("rw_myth") == SpecializedRunewordBase::kHeavyArmor);
static_assert(ResolveSpecializedRunewordBase("rw_unbending_will") == SpecializedRunewordBase::kShield);
static_assert(ResolveSpecializedRunewordBase("rw_dragon") == SpecializedRunewordBase::kNone);

static_assert(ResolveRunewordWeaponBaseKind(4u) == RunewordBaseKind::kOneHandedMelee);
static_assert(ResolveRunewordWeaponBaseKind(6u) == RunewordBaseKind::kTwoHandedMelee);
static_assert(ResolveRunewordWeaponBaseKind(7u) == RunewordBaseKind::kBowOrCrossbow);
static_assert(ResolveRunewordArmorBaseKind(true, true, false, false) == RunewordBaseKind::kShield);
static_assert(ResolveRunewordArmorBaseKind(false, true, false, false) == RunewordBaseKind::kHeavyArmor);
static_assert(ResolveRunewordArmorBaseKind(false, false, true, false) == RunewordBaseKind::kLightArmor);

static_assert(IsSpecializedRunewordBaseCompatible(SpecializedRunewordBase::kTwoHandedMelee, RunewordBaseKind::kTwoHandedMelee));
static_assert(!IsSpecializedRunewordBaseCompatible(SpecializedRunewordBase::kTwoHandedMelee, RunewordBaseKind::kOneHandedMelee));
static_assert(IsSpecializedRunewordBaseCompatible(SpecializedRunewordBase::kBowOrCrossbow, RunewordBaseKind::kBowOrCrossbow));
static_assert(IsSpecializedRunewordBaseCompatible(SpecializedRunewordBase::kOneHandedMelee, RunewordBaseKind::kOneHandedMelee));
static_assert(!IsSpecializedRunewordBaseCompatible(SpecializedRunewordBase::kOneHandedMelee, RunewordBaseKind::kTwoHandedMelee));
static_assert(IsSpecializedRunewordBaseCompatible(SpecializedRunewordBase::kHeavyArmor, RunewordBaseKind::kHeavyArmor));
static_assert(!IsSpecializedRunewordBaseCompatible(SpecializedRunewordBase::kHeavyArmor, RunewordBaseKind::kLightArmor));
static_assert(IsSpecializedRunewordBaseCompatible(SpecializedRunewordBase::kShield, RunewordBaseKind::kShield));
static_assert(!IsSpecializedRunewordBaseCompatible(SpecializedRunewordBase::kShield, RunewordBaseKind::kHeavyArmor));
