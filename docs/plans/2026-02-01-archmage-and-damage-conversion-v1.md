# Archmage + Physical→Elemental Conversion (Tiered) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Add PoE-style *spell-focused* Archmage and *true replacement* Physical→Elemental conversion affixes (tiered) with minimal CK (prefer generator + SKSE).

**Architecture:** Generator (`affixes/affixes.json`) produces keywords + optional damage spells (MGEF/SPEL) + KID rules + LoreBox translations. SKSE plugin adds two behaviors: (1) Archmage on spell hit; (2) conversion via `Actor::HandleHealthDamage` vfunc hook.

**Tech Stack:** .NET 8 + Mutagen (plugin generation), SKSE + CommonLibSSE-NG (events + hooks), KID, LoreBox.

---

## Web / Reference Notes (why these fields/hooks)

- PoE “Archmage Support” reference: *Added Lightning Damage based on unreserved max mana* + *adds base mana cost equal to 5% unreserved max mana* (PoE Wiki).
- Skyrim enchanting interaction (UEXP): Augmented Flames/Frost/Shock perks affect elemental weapon enchant magnitudes; “on-elemental-damage” perks like Intense Flames do not trigger from enchantments. (Used as a design hint: we want “elemental scaling” without unintended perk procs.)
- Creation Kit Magic Effect “Resist Value” / “Magic Skill”: ResistValue selects which resistance applies to the effect; MagicSkill associates effect with a school. (Used to generate proper elemental damage effects without CK.)
- CommonLibSSE-NG `RE::HitData` contains `totalDamage`, `physicalDamage`, `resistedPhysicalDamage`, `resistedTypedDamage`, and `weapon` / `attackDataSpell`, and `Actor` has vfunc `HandleHealthDamage` at vtable slot `0x104` (hex). (Used to implement conversion as a damage-pipeline modifier rather than post-hit proc.)

---

## Implementation Plan

### Task 1: Extend generator spec to support elemental spell metadata

**Files:**
- Modify: `tools/CalamityAffixes.Generator/Spec/AffixSpec.cs`
- Modify: `tools/CalamityAffixes.Generator/Writers/KeywordPluginBuilder.cs`
- Test: `tools/CalamityAffixes.Generator.Tests/KeywordPluginBuilderTests.cs`

**Step 1: Write failing test (RED)**
- Add a test that loads an `AffixSpec` with `records.magicEffect.resistValue="ResistFire"` and `magicSkill="Destruction"` (new fields), runs `KeywordPluginBuilder.Build`, and asserts the created `MagicEffect` has `ResistValue == ActorValue.ResistFire` and `MagicSkill == ActorValue.Destruction`.

**Step 2: Run tests to verify failure**
- Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj`
- Expected: FAIL because fields don’t exist / aren’t applied.

**Step 3: Minimal implementation (GREEN)**
- Add optional `resistValue` + `magicSkill` fields to `MagicEffectRecordSpec`.
- Parse them in `KeywordPluginBuilder.AddMagicEffect` and set `mgef.ResistValue` / `mgef.MagicSkill`.
- Also apply `name` fields to `mgef.Name` and `spell.Name` if provided.

**Step 4: Run tests to verify pass**
- Same command, expected: PASS.

---

### Task 2: Add generic elemental damage spells for dynamic magnitude override

**Files:**
- Modify: `affixes/affixes.json`
- Test: `tools/CalamityAffixes.Generator.Tests/GeneratorRunnerTests.cs` (or add a new focused test)

**Step 1: Write failing test (RED)**
- Add a test that runs the generator runner against a spec containing `CAFF_SPEL_DMG_FIRE_DYNAMIC` (magnitude 0) and asserts the generated plugin contains that Spell and MGEF with correct `ResistValue`.

**Step 2: Verify test fails**
- Run the test, confirm it fails because spec not present or fields not emitted.

**Step 3: Implement spec entries (GREEN)**
- In `affixes/affixes.json`, add 3 records (Fire/Frost/Shock) in a “utility” affix block (or reuse an existing affix block) so generator emits:
  - `CAFF_MGEF_DMG_FIRE_DYNAMIC` (Health value modifier, Hostile, ResistFire, Destruction)
  - `CAFF_SPEL_DMG_FIRE_DYNAMIC` (TargetActor, effect magnitude 0)
  - and likewise for Frost/Shock.

**Step 4: Run tests**
- Expect PASS.

---

### Task 3: Add 12 conversion affixes (Fire/Frost/Shock × 25/50/75/100)

**Files:**
- Modify: `affixes/affixes.json`
- (Optional) Update: `docs/plans/2026-01-31-elemental-affixes-v1.md` or create a new UX note

**Design choice (default):** Conversion is *per hit weapon* (reads `HitData->weapon`), not “global equipped”.

**Implementation:**
- Add 12 weapon affixes with `runtime.action.type="ConvertDamage"` and:
  - `element`: `Fire|Frost|Shock`
  - `percent`: `25|50|75|100`
  - `spellEditorId`: one of the dynamic elemental spells from Task 2
- Set `procChancePercent` to `0` so they don’t execute as “post-hit proc”; they are applied by the hook.

---

### Task 4: Add Archmage affix (spell-hit) with tiering

**Files:**
- Modify: `affixes/affixes.json`
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Modify: `skse/CalamityAffixes/src/main.cpp`

**Design choice (default):**
- Trigger: spell hit (from `TESHitEvent` where `source` resolves to `SpellItem`).
- Cost gate: only apply bonus if `currentMagicka >= extraCost`.
- Bonus lightning damage: `% of max magicka` (PoE-inspired).

---

### Task 5: Implement conversion hook (Actor::HandleHealthDamage)

**Files:**
- Modify: `skse/CalamityAffixes/src/main.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/CMakeLists.txt` (if new files added)

**Implementation (log-driven verification):**
- Install vfunc hook at `RE::VTABLE_Actor[0]`, index `0x104`.
- In the hook:
  - Filter: attacker is player-owned, target is actor, and `lastHitData->weapon != nullptr`.
  - Identify conversion affix by checking `lastHitData->weapon->HasKeyword(affix.keyword)` among loaded `ConvertDamage` actions.
  - Compute `physicalDealt ≈ max(0, hitData->physicalDamage - hitData->resistedPhysicalDamage)`.
  - Convert `converted = physicalDealt * (percent / 100)`.
  - Call original with `a_damage - converted` (clamped).
  - Apply converted damage via `CastSpellImmediate` of the element’s dynamic spell with `magnitudeOverride = converted`.
  - Add recursion guard so internal spell damage doesn’t re-enter conversion.
- Add debug logs (spdlog debug) to print key values for users to validate formula.

---

### Task 6: Regenerate Data outputs + build MO2 zip

**Files/Commands:**
- Run generator: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`
- Build zip: `tools/build_mo2_zip.sh`

---

### Task 7: Verify

**Commands:**
- `dotnet test tools/CalamityAffixes.Tools.sln`

**In-game checklist (user-run):**
- Equip a weapon with a conversion affix and hit an armored target; compare damage feel vs non-conversion.
- Cast a spell with Archmage affix equipped; confirm extra lightning hits occur and magicka drains.
- Confirm LoreBox tooltip text shows the new affix descriptions.
