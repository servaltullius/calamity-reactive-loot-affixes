// Tests for EffectiveLootWeight gate:
//   return (lootWeight >= 0.0f) ? lootWeight : 0.0f;
// We replicate the exact expression here since AffixRuntime lives behind
// the RE/Skyrim.h wall and can't be included in lightweight static tests.

static constexpr float EffectiveLootWeight(float procChancePct, float lootWeight) noexcept
{
	(void)procChancePct;
	return (lootWeight >= 0.0f) ? lootWeight : 0.0f;
}

// Default lootWeight (-1) => not rollable.
static_assert(EffectiveLootWeight(25.0f, -1.0f) == 0.0f,
	"Default lootWeight should be excluded from loot-time rolling");

// Explicit lootWeight overrides procChancePct for eligibility gating.
static_assert(EffectiveLootWeight(0.0f, 15.0f) == 15.0f,
	"Explicit lootWeight should override procChancePct");

// lootWeight = 0 → excluded (weight 0, not falling back)
static_assert(EffectiveLootWeight(100.0f, 0.0f) == 0.0f,
	"lootWeight=0 should return 0 (exclude from loot), not fall back");

// Both zero → returns 0 (excluded)
static_assert(EffectiveLootWeight(0.0f, -1.0f) == 0.0f,
	"procChancePct=0 with no lootWeight should be excluded");

// Armor prefix pattern: procChancePct=0 (special-action semantics), lootWeight=20 enables loot eligibility.
static_assert(EffectiveLootWeight(0.0f, 20.0f) == 20.0f,
	"Armor prefix: procChancePct=0 with lootWeight=20 should use lootWeight");

// proc chance and loot chance are independent.
static_assert(EffectiveLootWeight(100.0f, -1.0f) == 0.0f,
	"Weapon prefix: procChancePct must not implicitly become loot weight");
