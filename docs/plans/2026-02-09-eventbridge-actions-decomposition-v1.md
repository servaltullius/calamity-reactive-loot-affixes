# EventBridge Actions Decomposition V1 Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Split `EventBridge.Actions.cpp` into focused translation units while preserving runtime behavior for proc/special/corpse/trap actions.

**Architecture:** Keep class API unchanged in `EventBridge.h`, move out-of-line member definitions by concern into new `.cpp` files, and register all new sources in CMake. Use anonymous-namespace helpers per TU to keep internal linkage and avoid symbol collisions.

**Tech Stack:** C++23, CommonLibSSE-NG, SKSE plugin build via CMake + clang-cl/lld-link, dotnet generator tests, Python affix lint.

### Task 1: Baseline Snapshot And Split Boundaries

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Actions.cpp`
- Create: `/tmp/EventBridge.Actions.cpp.bak` (temporary backup)

**Step 1: Capture baseline and function map**

Run:
```bash
wc -l skse/CalamityAffixes/src/EventBridge.Actions.cpp
rg -n "^\\s*(EventBridge::|void EventBridge::|bool EventBridge::|std::uint32_t EventBridge::|RE::TESObjectREFR\\* EventBridge::|float EventBridge::)" skse/CalamityAffixes/src/EventBridge.Actions.cpp
cp skse/CalamityAffixes/src/EventBridge.Actions.cpp /tmp/EventBridge.Actions.cpp.bak
```

Expected: line count + stable function start list + backup created.

### Task 2: Extract Corpse Explosion Block

**Files:**
- Create: `skse/CalamityAffixes/src/EventBridge.Actions.Corpse.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.Actions.cpp`

**Step 1: Move corpse-only functions**

Move these functions into `EventBridge.Actions.Corpse.cpp`:
- `SelectBestCorpseExplosionAffix`
- `LogCorpseExplosionSelectionSkipped`
- `DescribeCorpseExplosionDenyReason`
- `LogCorpseExplosionBudgetDenied`
- `NotifyCorpseExplosionFired`
- `ProcessCorpseExplosionKill`
- `ProcessSummonCorpseExplosionDeath`
- `ProcessCorpseExplosionAction`
- `TryConsumeCorpseExplosionBudget`
- `ExecuteCorpseExplosion`

Include only needed headers and corpse-specific anonymous helpers.

**Step 2: Rebuild source file after removal**

Ensure `EventBridge.Actions.cpp` still compiles after corpse block removal (namespace/brace integrity).

### Task 3: Extract Special Proc Block

**Files:**
- Create: `skse/CalamityAffixes/src/EventBridge.Actions.Special.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.Actions.cpp`

**Step 1: Move special-proc functions**

Move these functions into `EventBridge.Actions.Special.cpp`:
- `EvaluateConversion`
- `EvaluateCastOnCrit`
- `SelectBestArchmageAction`
- `ResolveArchmageResourceUsage`
- `ExecuteArchmageCast`
- `ProcessArchmageSpellHit`

Add local helper functions required for proc chance and spell base magnitude in this TU.

**Step 2: Keep spell/trap dispatch path in main actions TU**

Leave these in `EventBridge.Actions.cpp`:
- Cast spell/adaptive cast functions
- Trap spawn/cap helpers
- `DispatchActionByType`
- `ExecuteActionWithProcDepthGuard`
- `ExecuteAction`

### Task 4: Wire CMake And Verify

**Files:**
- Modify: `skse/CalamityAffixes/CMakeLists.txt`

**Step 1: Register new sources**

Add:
- `src/EventBridge.Actions.Corpse.cpp`
- `src/EventBridge.Actions.Special.cpp`

**Step 2: Full verification**

Run:
```bash
cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j
ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure
dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release
python3 tools/lint_affixes.py --spec affixes/affixes.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json
```

Expected: build succeeds, CTest pass, dotnet tests pass, lint warning count unchanged (`0`).

### Task 5: Commit

**Files:**
- Modify: `skse/CalamityAffixes/CMakeLists.txt`
- Modify: `skse/CalamityAffixes/src/EventBridge.Actions.cpp`
- Create: `skse/CalamityAffixes/src/EventBridge.Actions.Corpse.cpp`
- Create: `skse/CalamityAffixes/src/EventBridge.Actions.Special.cpp`
- Create: `docs/plans/2026-02-09-eventbridge-actions-decomposition-v1.md`

**Step 1: Commit**

```bash
git add docs/plans/2026-02-09-eventbridge-actions-decomposition-v1.md \
  skse/CalamityAffixes/CMakeLists.txt \
  skse/CalamityAffixes/src/EventBridge.Actions.cpp \
  skse/CalamityAffixes/src/EventBridge.Actions.Corpse.cpp \
  skse/CalamityAffixes/src/EventBridge.Actions.Special.cpp
git commit -m "refactor(skse): split EventBridge actions into focused translation units"
git push origin main
```
