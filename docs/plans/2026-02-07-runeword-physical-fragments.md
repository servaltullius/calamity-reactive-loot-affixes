# Runeword Physical Rune Fragments Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the invisible runeword fragment counters with real inventory items (rune fragments) that players can collect and that the runeword UI/logic consumes.

**Architecture:** Generate `TESObjectMISC` records in `CalamityAffixes.esp` for each rune fragment. In the SKSE plugin, treat player inventory count as the single source of truth. Keep the legacy `_runewordRuneFragments` map only for save migration (read old saves, grant equivalent items once, then clear).

**Tech Stack:** Mutagen (`Mutagen.Bethesda.Skyrim`) generator + CommonLibSSE-NG SKSE plugin + Prisma UI overlay.

## Task 1: Add Failing Generator Test (RED)

**Files:**
- Modify: `tools/CalamityAffixes.Generator.Tests/KeywordPluginBuilderTests.cs`

**Step 1: Write the failing test**
- Add a new `[Fact]` that builds a minimal `AffixSpec` and asserts that `mod.MiscItems` contains EditorIDs:
  - `CAFF_RuneFrag_Jah`, `CAFF_RuneFrag_Ith`, `CAFF_RuneFrag_Ber`, `CAFF_RuneFrag_Mal`, `CAFF_RuneFrag_Ist`,
  - `CAFF_RuneFrag_El`, `CAFF_RuneFrag_Sol`, `CAFF_RuneFrag_Dol`, `CAFF_RuneFrag_Lo`,
  - `CAFF_RuneFrag_Ral`, `CAFF_RuneFrag_Tir`, `CAFF_RuneFrag_Tal`.

**Step 2: Run tests to verify failure**
- Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- Expected: FAIL because rune fragment misc items do not exist yet.

## Task 2: Generate Rune Fragment MiscItems (GREEN)

**Files:**
- Modify: `tools/CalamityAffixes.Generator/Writers/KeywordPluginBuilder.cs`

**Step 1: Implement minimal generator support**
- Add `AddRunewordRuneFragments(SkyrimMod mod)` that:
  - Creates one `MiscItem` per rune (EditorID `CAFF_RuneFrag_<Rune>`).
  - Sets `Name` to a bilingual string e.g. `Rune Fragment (룬 조각): Jah`.
  - Sets `Weight = 0` and low `Value` (e.g. 0 or 1) to avoid economy impact.

**Step 2: Run tests to verify pass**
- Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- Expected: PASS.

## Task 3: Switch Runeword Fragment Logic to Inventory Items

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.cpp`
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`

**Step 1: Add inventory helpers**
- Add private helpers (or internal static funcs) to:
  - Resolve fragment item from rune token via EditorID lookup:
    - Token -> rune name via `_runewordRuneNameByToken`
    - EditorID format: `CAFF_RuneFrag_<RuneName>`
    - Lookup: `RE::TESForm::LookupByEditorID<RE::TESObjectMISC>(editorID)`
  - Read owned count: `player->GetItemCount(item)`
  - Consume 1: `player->RemoveItem(item, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr)`
  - Grant: `player->AddObjectToContainer(item, nullptr, count, nullptr)`

**Step 2: Replace all `_runewordRuneFragments` runtime reads/writes**
- Update:
  - `MaybeGrantRandomRunewordFragment()`
  - `GrantNextRequiredRuneFragment()`
  - `GrantCurrentRecipeRuneSet()`
  - `InsertRunewordRuneIntoSelectedBase()`
  - `BuildRunewordTooltip()`
  - `LogRunewordStatus()`
  - `GetRunewordPanelState()`

**Step 3: Add a safety guard**
- If a fragment item cannot be resolved (missing EditorID), log an error and do not grant/consume.

## Task 4: Migrate Legacy Saved Counters Once

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.cpp`

**Step 1: Add migration method**
- On first opportunity where player inventory is available, convert each entry in `_runewordRuneFragments` into inventory items, then clear the map.
- Ensure migration runs once and cannot duplicate grants.

## Task 5: UX Clarification (Optional but Recommended)

**Files:**
- Modify: `Data/PrismaUI/views/CalamityAffixes/index.html`

**Step 1: Avoid “insert without materials” confusion**
- When `insertedRunes >= totalRunes`, change insert button label to “Finalize Runeword / 룬워드 완성”.
- Otherwise keep “Insert Next Rune / 다음 룬 삽입”.

## Task 6: Build + Package Verification

**Step 1: Build generator outputs**
- Run: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`

**Step 2: Build SKSE plugin**
- Run: `cmake --build --preset wsl-release` (or repo’s documented build preset)

**Step 3: Package MO2 zip**
- Run: `tools/build_mo2_zip.sh`
- Expected: `dist/CalamityAffixes_MO2_<YYYY-MM-DD>.zip` includes updated `CalamityAffixes.esp` and `CalamityAffixes.dll`.

