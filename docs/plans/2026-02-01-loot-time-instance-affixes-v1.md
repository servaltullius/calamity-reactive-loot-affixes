# Loot-time Instance Affixes v1 Implementation Plan

> **For Codex:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Roll 0–1 affix **per item instance** when the player acquires a weapon/armor (loot-time), persist it, and have the SKSE runtime proc system read from those per-instance affixes (not base-record keywords). Keep CK usage minimal/zero.

**Architecture:** Use `loot` config in `affixes/affixes.json` to control chance/rename/debug/nameFormat. The generator never emits “affix keywords distributed to base records” into `CalamityAffixes_KID.ini` (instance-only). The SKSE plugin listens for `TESContainerChangedEvent` (player acquisition), assigns an affix to the specific inventory instance via `InventoryChanges` + `ExtraUniqueID`, and persists the mapping via SKSE co-save serialization. Active affix counts are rebuilt by scanning worn `ExtraDataList`s (ExtraWorn/ExtraWornLeft) and looking up per-instance affixes.

**Tech Stack:** C# generator (Mutagen) + JSON spec/schema + C++ SKSE plugin (CommonLibSSE-NG).

---

### Task 1: Add RED tests for `loot` mode generator output

**Files:**
- Modify: `tools/CalamityAffixes.Generator.Tests/KidIniRendererTests.cs`

**Step 1: Write failing test**
- Create a minimal spec with `loot` section and 1+ `keywords.affixes`.
- Assert `KidIniWriter.Render(spec)`:
  - does **not** emit `ExclusiveGroup = ...` for affixes
  - does **not** emit any `Keyword = <affixEditorId>|...` lines
  - still emits any `keywords.kidRules` lines (tag rules must remain)

**Step 2: Run test to verify it fails**
- Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj`
- Expected: FAIL (generator currently always emits affix KID lines)

---

### Task 2: Implement `loot` spec + schema + KID writer behavior

**Files:**
- Modify: `affixes/affixes.schema.json`
- Modify: `tools/CalamityAffixes.Generator/Spec/AffixSpec.cs`
- Modify: `tools/CalamityAffixes.Generator/Writers/KidIniWriter.cs`

**Step 1: Minimal implementation**
- Add optional `loot` object to schema/spec with:
  - `chancePercent` (number)
  - `renameItem` (bool)
  - `nameFormat` (string)
- Update `KidIniWriter` to skip emitting `keywords.affixes[*].kid` lines (and `ExclusiveGroup`) always (instance-only).

**Step 2: Run tests**
- Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj`
- Expected: PASS

---

### Task 3: Add `loot` defaults to the example spec

**Files:**
- Modify: `affixes/affixes.json`

**Step 1: Add config**
- Add `loot` section with sane defaults (enabled/disabled based on current milestone).

**Step 2: JSON validation**
- Run: `python3 -m json.tool affixes/affixes.json > /dev/null`

---

### Task 4: Add per-instance affix support to the SKSE runtime (core)

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Modify: `skse/CalamityAffixes/src/main.cpp`

**Step 1: Config parsing**
- Parse `loot` section from runtime config:
  - chance percent
  - rename toggle + format

**Step 2: Event sinks**
- Register `TESContainerChangedEvent` and on `newContainer == Player` roll/apply affix for eligible weapons/armor.

**Step 3: Instance storage**
- Ensure target `ExtraDataList` has `ExtraUniqueID` (create if missing).
- Maintain a runtime map keyed by `(baseID, uniqueID)` → `affixId`.

**Step 4: Serialization**
- Add SKSE serialization callbacks:
  - Save: write all instance mappings
  - Load: read mappings (resolve base FormID) and repopulate map
  - Revert: clear map

**Step 5: Active counts**
- Rebuild active counts by scanning player `InventoryChanges->entryList`:
  - for each `ExtraDataList` that has `ExtraWorn` or `ExtraWornLeft`
  - look up `(baseID, uniqueID)` and increment matching affix
- Simplify `TESEquipEvent` handling to call `RebuildActiveCounts()` (per-instance deltas aren’t reliable with baseObject alone).

---

### Task 5: Display (CK 0)

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`

**Step 1: Rename on roll**
- If `loot.renameItem=true`, set/update `ExtraTextDisplayData` to include the affix name using `loot.nameFormat`.

---

### Task 6: Documentation updates

**Files:**
- Modify: `README.md`
- Modify: `doc/3.B_데이터주도_생성기_워크플로우.md`

**Step 1: Explain modes**
- Document that this project is instance-only:
  - no base-record affix keyword distribution
  - no LoreBox requirement (SKSE item card injection for tooltips)

---

### Task 7: Regenerate outputs + rebuild MO2 zip

**Files:**
- Outputs under: `Data/`
- Output zip: `dist/`

**Step 1: Regenerate `Data/`**
- Run: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`

**Step 2: Build MO2 zip**
- Run: `tools/build_mo2_zip.sh`
