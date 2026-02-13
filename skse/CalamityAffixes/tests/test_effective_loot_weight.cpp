// Tests for EffectiveLootWeight logic:
//   return (lootWeight >= 0.0f) ? lootWeight : procChancePct;
// We replicate the exact expression here since AffixRuntime lives behind
// the RE/Skyrim.h wall and can't be included in lightweight static tests.

static constexpr float EffectiveLootWeight(float procChancePct, float lootWeight) noexcept
{
	return (lootWeight >= 0.0f) ? lootWeight : procChancePct;
}

// Default lootWeight (-1) → fall back to procChancePct
static_assert(EffectiveLootWeight(25.0f, -1.0f) == 25.0f,
	"Default lootWeight should fall back to procChancePct");

// Explicit lootWeight overrides procChancePct
static_assert(EffectiveLootWeight(0.0f, 15.0f) == 15.0f,
	"Explicit lootWeight should override procChancePct");

// lootWeight = 0 → excluded (weight 0, not falling back)
static_assert(EffectiveLootWeight(100.0f, 0.0f) == 0.0f,
	"lootWeight=0 should return 0 (exclude from loot), not fall back");

// Both zero → returns 0 (excluded)
static_assert(EffectiveLootWeight(0.0f, -1.0f) == 0.0f,
	"procChancePct=0 with no lootWeight should return 0");

// Armor prefix pattern: procChancePct=0 (always-proc), lootWeight=20
static_assert(EffectiveLootWeight(0.0f, 20.0f) == 20.0f,
	"Armor prefix: procChancePct=0 with lootWeight=20 should use lootWeight");

// Standard weapon prefix: procChancePct=100, no lootWeight override
static_assert(EffectiveLootWeight(100.0f, -1.0f) == 100.0f,
	"Weapon prefix: procChancePct=100 with default lootWeight should use procChancePct");
