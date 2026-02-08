# CoC Spell Pool Expansion Implementation Plan

> **For Codex:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Expand the Cast-on-Crit (CoC) armor affix spell pool by adding 4 more vanilla spells, keeping the system data-driven and CK-minimal.

**Architecture:** No C++ changes. Add new `CastOnCrit` armor affixes in `affixes/affixes.json` and rely on the generator to update runtime data, KID distribution, and LoreBox tooltip metadata.

**Tech Stack:** SKSE (CommonLibSSE-NG), .NET generator, KID, LoreBox, xUnit.

---

### Task 1: Tighten regression test for CoC coverage (RED)

**Files:**
- Modify: `tools/CalamityAffixes.Generator.Tests/RepoSpecRegressionTests.cs`

**Step 1: Update test to require `CastOnCrit` affixes >= 7**
- Replace the current “exists” check with a count check and assert `>= 7`.

**Step 2: Run test to verify it fails**
- Run: `dotnet test tools/CalamityAffixes.Tools.sln -c Release`
- Expected: FAIL (because current spec only has 3 `CastOnCrit` affixes).

---

### Task 2: Add 4 new CoC armor affixes (GREEN)

**Files:**
- Modify: `affixes/affixes.json`

**Step 1: Add new affixes**
- Add these 4 armor affixes (all `runtime.action.type = "CastOnCrit"`):
  - `LoreBox_CAFF_AFFIX_HIT_COC_THUNDERBOLT` → `Skyrim.esm|0010F7EE`
  - `LoreBox_CAFF_AFFIX_HIT_COC_ICY_SPEAR` → `Skyrim.esm|0010F7EC`
  - `LoreBox_CAFF_AFFIX_HIT_COC_CHAIN_LIGHTNING` → `Skyrim.esm|00045F9D`
  - `LoreBox_CAFF_AFFIX_HIT_COC_ICE_STORM` → `Skyrim.esm|00045F9C`
- Keep the existing conventions:
  - `runtime.trigger = "Hit"`
  - `procChancePercent = 0.0` (triggering is handled by runtime CoC logic)
  - `noHitEffectArt = true`

**Step 2: Run tests to verify they pass**
- Run: `dotnet test tools/CalamityAffixes.Tools.sln -c Release`
- Expected: PASS

---

### Task 3: Regenerate Data outputs + MO2 zip

**Files:**
- Generated: `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- Generated: `Data/CalamityAffixes_KID.ini`
- Generated: `Data/SKSE/Plugins/InventoryInjector/CalamityAffixes_Keywords.json`
- Build output: `dist/CalamityAffixes_MO2_2026-02-01.zip`

**Step 1: Run generator**
- Run: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`

**Step 2: Build MO2 zip**
- Run: `tools/build_mo2_zip.sh`

**Step 3: Sanity-check outputs**
- Confirm the generated `affixes.json` contains the new `LoreBox_CAFF_AFFIX_HIT_COC_*` entries.

---

### Task 4: Update Korean docs for the new count and list

**Files:**
- Modify: `doc/4.현재_구현_어픽스_목록.md`

**Step 1: Update count and CoC list**
- Bump “현재 N개” by +4.
- Add the four new CoC entries under the CoC section.

---

### Task 5: Final verification (stay green)

**Step 1: Run full tool tests again**
- Run: `dotnet test tools/CalamityAffixes.Tools.sln -c Release`
- Expected: PASS

**Step 2: Quick grep check**
- Run: `rg -n "LoreBox_CAFF_AFFIX_HIT_COC_(THUNDERBOLT|ICY_SPEAR|CHAIN_LIGHTNING|ICE_STORM)" Data/SKSE/Plugins/CalamityAffixes/affixes.json`
