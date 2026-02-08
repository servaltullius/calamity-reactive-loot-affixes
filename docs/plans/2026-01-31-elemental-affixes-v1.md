# Elemental Affixes v1 Implementation Plan

> **For Codex:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Add 6 “elemental” weapon affixes (Fire/Frost/Shock × bonus damage + resistance shred) using CK 0 record autogen (Spell + MagicEffect) and rebuild the MO2 zip.

**Architecture:** Extend the Mutagen generator spec to support MagicEffect `recover` (needed for temporary AV debuffs/buffs on non-Health AVs), then define 6 affixes in `affixes/affixes.json` with `records.magicEffect` + `records.spell` and `runtime.action` that casts the generated spell by `spellEditorId`.

**Tech Stack:** C# (`tools/CalamityAffixes.Generator`) + Mutagen, JSON spec/schema, SKSE runtime reads `Data/SKSE/Plugins/CalamityAffixes/affixes.json`.

---

### Task 1: Add RED test for `recover`

**Files:**
- Modify: `tools/CalamityAffixes.Generator.Tests/KeywordPluginBuilderTests.cs`

**Step 1: Write failing test**
- Build a JSON spec that includes `records.magicEffect.recover: true`, load it via `AffixSpecLoader.Load`, build plugin, assert `MagicEffect.Flag.Recover` is set.

**Step 2: Run test to verify it fails**
- Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj`
- Expected: FAIL (Recover flag missing)

---

### Task 2: Implement `recover` end-to-end

**Files:**
- Modify: `tools/CalamityAffixes.Generator/Spec/AffixSpec.cs`
- Modify: `tools/CalamityAffixes.Generator/Writers/KeywordPluginBuilder.cs`
- Modify: `affixes/affixes.schema.json`
- Modify: `doc/3.B_데이터주도_생성기_워크플로우.md`

**Step 1: Minimal implementation**
- Add optional `recover` to `MagicEffectRecordSpec` and add the flag in `AddMagicEffect`.

**Step 2: Run tests**
- Run: `dotnet test tools/CalamityAffixes.Tools.sln`
- Expected: PASS

---

### Task 3: Add 6 elemental affixes (spec only)

**Files:**
- Modify: `affixes/affixes.json`

**Step 1: Add affixes**
- Fire/Frost/Shock bonus damage (TargetActor, ActorValue=Health, hostile=true)
- Fire/Frost/Shock resistance shred (TargetActor, ActorValue=ResistFire/ResistFrost/ResistShock, hostile=true, recover=true)
- Each uses `runtime.action.type = CastSpell` with `spellEditorId` referencing generated spell.

**Step 2: JSON validation**
- Run: `python3 -m json.tool affixes/affixes.json > /dev/null`

---

### Task 4: Regenerate outputs + zip

**Files:**
- Outputs under: `Data/`
- Output zip: `dist/`

**Step 1: Regenerate `Data/`**
- Run: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`

**Step 2: Build MO2 zip**
- Run: `tools/build_mo2_zip.sh`

