# EventBridge Module Split (v1) — Implementation Plan

> **For Codex:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Split `skse/CalamityAffixes/src/EventBridge.cpp` into focused translation units (Config/Loot/Triggers/Actions/Serialization) while preserving behavior.

**Architecture:** `EventBridge` remains the façade (public API stays in `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`). Only the method definitions move.

**Tech Stack:** C++23, CommonLibSSE-NG, SKSE, nlohmann/json, spdlog

---

## Task 1: Add new translation units

**Files:**
- Create: `skse/CalamityAffixes/src/EventBridge.Config.cpp`
- Create: `skse/CalamityAffixes/src/EventBridge.Loot.cpp`
- Create: `skse/CalamityAffixes/src/EventBridge.Triggers.cpp`
- Create: `skse/CalamityAffixes/src/EventBridge.Actions.cpp`
- Create: `skse/CalamityAffixes/src/EventBridge.Serialization.cpp`
- Modify: `skse/CalamityAffixes/CMakeLists.txt`

**Step:** Add the new `.cpp` files to `sources` and ensure build still succeeds.

**Verify:** `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j`

---

## Task 2: Move serialization

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.Serialization.cpp`

**Move:** `Save/Load/Revert` method bodies.

**Verify:**
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j`
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixesTests -j`

---

## Task 3: Move loot-time affix system

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.cpp`

**Move (loot/instance):**
- `ProcessEvent(TESContainerChangedEvent)`
- `ProcessLootAcquired`, `RollLootAffixIndex`, `ApplyChosenAffix`, `ApplyAffixToInstance`
- `GetInstanceAffixTooltip`, `MakeInstanceKey`, `ParseLootItemType`, `FormatLootName`

**Verify:** same as Task 2.

---

## Task 4: Move actions (execution)

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.Actions.cpp`

**Move:**
- `ExecuteAction`
- `ProcessArchmageSpellHit`
- `ProcessCorpseExplosionKill`, `TryConsumeCorpseExplosionBudget`, `ExecuteCorpseExplosion`
- (Optional to keep in same module) `EvaluateConversion`, `EvaluateCastOnCrit`

**Verify:** same as Task 2.

---

## Task 5: Move triggers + resync helpers

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.Triggers.cpp`

**Move:**
- `OnHealthDamage`, `ProcessTrigger`
- `ProcessEvent(TESHitEvent/TESEquipEvent/TESMagicEffectApplyEvent/TESDeathEvent)`
- `IsPlayerOwned`, `GetPlayerOwner`, `SendModEvent`
- `MaybeResyncEquippedAffixes`, `RebuildActiveCounts`, `ShouldSuppressDuplicateHit`

**Verify:** same as Task 2.

---

## Task 6: Clean up core file

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`

**Goal:** Leave only truly core glue (typically `GetSingleton` + `Register`) and remove now-unused local helpers/includes.

**Verify:** same as Task 2.

---

## Task 7: Sync + repack MO2 zip

**Files:**
- Modify: `Data/SKSE/Plugins/CalamityAffixes.dll` (copy from build output)

**Commands:**
- `cp -f skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll Data/SKSE/Plugins/CalamityAffixes.dll`
- `bash tools/build_mo2_zip.sh`

