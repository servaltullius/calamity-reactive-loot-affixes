# Calamity A2-2 (No-CK) Spell+MGEF Autogen Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Make CK optional by generating **Magic Effects (MGEF)** and **Spells (SPEL)** from `affixes/affixes.json`, so new affixes can be added by editing JSON + running the generator.

**Architecture:** Extend the existing Mutagen-based generator to also create `MagicEffect` (Value Modifier only, for now) and `Spell` records inside the same generated plugin (`spec.modKey`, currently `CalamityAffixes_Keywords.esp`). SKSE runtime continues to execute procs by casting spells (via `spellEditorId`) and KID/SPID remain ini-driven.

**Tech Stack:** Mutagen.Bethesda.Skyrim (.NET), existing generator/tests, SKSE runtime (CommonLibSSE-NG) reads `Data/SKSE/Plugins/CalamityAffixes/affixes.json`.

---

### Task 1: Extend spec (JSON + C# model + schema)

**Files:**
- Modify: `affixes/affixes.schema.json`
- Modify: `tools/CalamityAffixes.Generator/Spec/AffixSpec.cs`
- Modify: `tools/CalamityAffixes.Generator/Spec/AffixSpecLoader.cs` (optional validation)
- Modify: `affixes/affixes.json` (example updated to generated spell)

**Step 1: Add optional `records` to each affix**

Add a new optional block under `keywords.affixes[]`:
- `records.magicEffect` (Value Modifier only)
- `records.spell` (Self/TargetActor only)

**Step 2: Update schema**

Schema should validate at minimum:
- `records.magicEffect.editorId`
- `records.magicEffect.actorValue`
- `records.spell.editorId`
- `records.spell.delivery` in `{Self, TargetActor}`
- numeric `magnitude`, `duration`, `area`

**Step 3: Update example**

Change the sample affix runtime action to:
- `CastSpell` with `spellEditorId: "<generated spell editorId>"`

---

### Task 2: RED — Add failing tests for record generation

**Files:**
- Modify: `tools/CalamityAffixes.Generator.Tests/KeywordPluginBuilderTests.cs` (or new test file)

**Step 1: Write a test spec containing generated records**

Spec should define:
- 1 MagicEffect (Value Modifier, actorValue = Health, hostile true)
- 1 Spell (delivery TargetActor) that uses that MagicEffect with magnitude/duration/area

**Step 2: Run tests (verify RED)**

Run:
```bash
dotnet test tools/CalamityAffixes.Tools.sln
```

Expected: FAIL, because the generator doesn’t yet create `MagicEffect` / `Spell` records.

---

### Task 3: GREEN — Implement Spell+MGEF generation (Mutagen)

**Files:**
- Modify: `tools/CalamityAffixes.Generator/Writers/KeywordPluginBuilder.cs`

**Step 1: Create `MagicEffect` records**

For each affix with `records.magicEffect`:
- `mod.MagicEffects.AddNew()`
- set `EditorID`
- set “Value Modifier” archetype and `ActorValue`
- set flags (`Hostile` etc.) based on spec

**Step 2: Create `Spell` records**

For each affix with `records.spell`:
- `mod.Spells.AddNew()`
- set `EditorID` + `Name`
- set delivery to `Self` or `TargetActor`
- add one spell effect entry pointing to the generated magic effect (by EditorID) and set magnitude/duration/area

**Step 3: Run tests (verify GREEN)**

Run:
```bash
dotnet test tools/CalamityAffixes.Tools.sln
```

Expected: PASS.

---

### Task 4: Regenerate outputs + package for MO2

**Files:**
- Modify (generated): `Data/CalamityAffixes_Keywords.esp`
- Modify (generated): `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- Modify (generated): `Data/CalamityAffixes_KID.ini`
- Modify (generated): `Data/CalamityAffixes_DISTR.ini`
- Modify: `doc/3.B_데이터주도_생성기_워크플로우.md`
- Modify: `tools/build_mo2_zip.sh` (if docs included)

**Step 1: Run generator**

Run:
```bash
dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data
```

Expected: updated plugin + ini + runtime config.

**Step 2: Build MO2 zip**

Run:
```bash
tools/build_mo2_zip.sh
```

Expected: `dist/CalamityAffixes_MO2_2026-01-31.zip` updated.

---

### Task 5: Verification checklist (evidence)

Run:
```bash
dotnet test tools/CalamityAffixes.Tools.sln
python3 -m json.tool affixes/affixes.json >/dev/null
python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null
python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null
```

Expected: all exit code 0.

