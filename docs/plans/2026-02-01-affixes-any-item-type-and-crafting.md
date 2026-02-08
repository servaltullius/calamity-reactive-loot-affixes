# Affixes Any Item Type + Crafting Behavior Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Allow any affix to roll on both weapons and armor (in KID/base-form mode), and document how crafted items behave in this system.

**Architecture:** Keep affixes as item Keywords. In KID mode, generate duplicate `Keyword = ...` lines per affix so the same keyword can distribute to `Weapon` and `Armor` record types. This keeps CK usage at zero and doesn’t require rewriting the affix spec. Crafted items inherit base-form keywords, so behavior is deterministic per base item.

**Tech Stack:** .NET generator (`tools/CalamityAffixes.Generator`), xUnit tests, KID runtime distribution, SKSE runtime hook (unchanged for this feature).

---

### Task 1: RED — Add test expectation for Weapon+Armor KID lines

**Files:**
- Modify: `tools/CalamityAffixes.Generator.Tests/KidIniRendererTests.cs`
- Test: `tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj`

**Step 1: Write the failing test**
- In `Render_IncludesAffixAndRuleLines`, keep `kid.type = "Weapon"` in the spec, but assert that the rendered INI contains BOTH:
  - `Keyword = LoreBox_CAFF_AFFIX_HIT_ARC_LIGHTNING|Weapon|NONE|-E|2`
  - `Keyword = LoreBox_CAFF_AFFIX_HIT_ARC_LIGHTNING|Armor|NONE|-E|2`

**Step 2: Run the test to verify it fails**
Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
Expected: FAIL because `KidIniWriter` currently only emits the single `kid.type` line.

---

### Task 2: GREEN — Emit KID lines for both weapons and armor

**Files:**
- Modify: `tools/CalamityAffixes.Generator/Writers/KidIniWriter.cs`
- Test: `tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj`

**Step 1: Minimal implementation**
- Add a helper that expands `kid.type`:
  - If `kid.type` is `Weapon` or `Armor`, return both `Weapon` and `Armor`.
  - Otherwise, return the original type unchanged.
- Emit one `Keyword = ...` line per expanded type.

**Step 2: Run tests to verify they pass**
Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
Expected: PASS.

---

### Task 3: Regenerate Data outputs

**Files:**
- Modify (generated): `Data/*` (KID INI + keyword plugin outputs)

**Step 1: Run generator**
Run: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`

**Step 2: Quick sanity check**
- Confirm the generated `Data/*_KID.ini` includes both `|Weapon|` and `|Armor|` lines for at least one affix.

---

### Task 4: Package MO2 zip

**Files:**
- Modify (generated): `dist/*.zip`

**Step 1: Build zip**
Run: `tools/build_mo2_zip.sh`

---

### Task 5: Document crafted item behavior (Korean)

**Files:**
- Modify: `README.md`

**Step 1: Add a short “제작 아이템(대장간/제작)에서 어픽스가 어떻게 붙나?” section**
- Explain KID mode behavior: keywords are added to base forms at startup, so crafted items inherit them.
- Call out the important limitation: chance/selection is deterministic per base form (not re-rolled per craft), so true ARPG “파밍/리롤” requires instance/loot mode.

