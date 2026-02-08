# Instance Affix Tooltip (No LoreBox) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.
> **Status Note (2026-02-07):** This is a historical pre-Prisma migration plan. The Scaleform `UiHooks`/SkyUI ItemCard runtime path described below is no longer present in current code.

**Goal:** In instance/loot-time mode (instance-only), show (1) a short affix label in the item name, and (2) the full affix description appended to the right-side item card “effects/description” area in SkyUI-based menus (Inventory / Container / Barter / Crafting). Do not show anything for “crafting recipe list preview” items (no instance).

**Architecture:**
- Keep per-instance affix assignment as-is: `ExtraUniqueID` + `_instanceAffixes` saved via SKSE serialization.
- **Name label:** derive a short label from the existing `affix.name` string and use it for rename formatting when `loot.renameItem=true`.
- **Tooltip UI:** hook SkyUI `ItemCard`’s `itemInfo` setter (`__set__itemInfo`) via Scaleform and append our affix HTML to `aUpdateObj.effects` before calling the original setter.
- Use SKSE Scaleform inventory-extension callback to tag each list entry with the affix tooltip text (`CAFF_affixTooltip`), so the ItemCard hook can read it from the currently-selected entry.

**Tech Stack:** CommonLibSSE-NG + SKSE Scaleform hooks, SkyUI class layout (`_global.ItemCard.prototype`), existing SKSE plugin build.

---

### Task 1: Hook Scaleform LoadMovie and patch ItemCard setter

**Files:**
- Create: `skse/CalamityAffixes/include/CalamityAffixes/UiHooks.h`
- Create: `skse/CalamityAffixes/src/UiHooks.cpp`
- Modify: `skse/CalamityAffixes/src/main.cpp`

**Steps:**
1. Install a `BSScaleformManager::LoadMovie` call hook (pattern same as InventoryInjector’s MovieManager).
2. In `AddScaleformHooks`, locate `_global.ItemCard.prototype` and wrap `__set__itemInfo`:
   - Save original as `CAFF_originalSetItemInfo`
   - Replace `__set__itemInfo` with C++ handler that:
     - Finds currently selected entry (from menu root: `inventoryLists.itemList.selectedEntry` or `ItemList.selectedEntry`)
     - Reads `selectedEntry.CAFF_affixTooltip`
     - Appends it to `aUpdateObj.effects` with `<br>` separators and a header
     - Calls `CAFF_originalSetItemInfo` with the modified update object

---

### Task 2: Register inventory extension callback to attach per-entry tooltip text

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Modify: `skse/CalamityAffixes/src/main.cpp`
- Create/Modify: `skse/CalamityAffixes/src/UiHooks.cpp`

**Steps:**
1. Add a public `EventBridge` query method returning optional UI tooltip text for a given `RE::InventoryEntryData*` (only for real item instances).
2. Register `SKSE::GetScaleformInterface()->Register(RegInvCallback)` and in that callback:
   - If there is an affix tooltip, set `CAFF_affixTooltip` (string) on the list entry object.

---

### Task 3: Short label for renameItem

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`

**Steps:**
1. Add `AffixRuntime.label` computed at config-load time from `name` (split on first `:` or `(`; trim).
2. When `loot.renameItem=true`, pass `affix.label` to `FormatLootName()` instead of the full description.

---

### Task 4: Build + package + docs

**Files:**
- Modify: `Data/SKSE/Plugins/CalamityAffixes.dll`
- Modify: `README.md`
- Modify: `dist/*.zip`

**Steps:**
1. Build SKSE plugin (release) and copy to `Data/SKSE/Plugins/CalamityAffixes.dll`.
2. Update README: in instance mode, LoreBox is not required; tooltip is appended in SkyUI item card.
3. Rebuild MO2 zip: `tools/build_mo2_zip.sh`.
