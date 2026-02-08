# Code Health Review & Refactor Implementation Plan

> **For Codex:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Improve maintainability/decoupling of the SKSE plugin without changing gameplay behavior.

**Architecture:** Keep the current runtime behavior (loot-time instance affixes + Prisma UI tooltip + hit/DoT event bridge), but extract shared utilities and reduce “god file” complexity in `EventBridge.cpp` / `Hooks.cpp`.

**Tech Stack:** C++23, CommonLibSSE-NG, SKSE, nlohmann/json, spdlog

---

## Scope (what “done” means)

- No functional changes to affix behavior, loot rolling rules, proc/ICD logic, or Prisma overlay UX.
- Easier-to-read code with less duplication and clearer module boundaries.
- Minimal unit/compile tests added for any new pure helpers.
- Build succeeds on the repo’s WSL clang-cl configuration and MO2 zip is regenerated.

---

## Task 1: Baseline verification (no code changes)

**Commands:**
- Build: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j`
- Tests (compile-time objects): `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixesTests -j`

**Expected:** Both succeed.

---

## Task 2: Extract shared hit-data utilities (decoupling)

**Files:**
- Create: `skse/CalamityAffixes/include/CalamityAffixes/HitDataUtil.h`
- Modify: `skse/CalamityAffixes/src/Hooks.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`

**Steps (TDD-ish):**
1. Add header with `inline` helper(s) shared by both call-sites.
2. Replace duplicated `GetLastHitData` in both translation units with the shared helper.
3. Build + tests.

---

## Task 3: Fix formatting / brace clarity in hot files (refactor)

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Modify: `skse/CalamityAffixes/src/Hooks.cpp`
- (Historical) `skse/CalamityAffixes/src/UiHooks.cpp` was referenced in older drafts, but this file/path is removed in current runtime.

**Rules:**
- No behavior changes; only readability.
- Avoid large rewrites; do small extractions + guard clauses where safe.

**Verification:**
- Build + tests.

---

## Task 4: Optional next step (bigger decoupling)

If desired after review:
- Split `EventBridge.cpp` into focused units (e.g. `ConfigLoad.cpp`, `Loot.cpp`, `Triggers.cpp`, `Actions.cpp`) while keeping `EventBridge` as the public façade.
- Add small unit tests for new pure parsing helpers (string parsing, label derivation).

---

## Task 5: Repack MO2 zip

**Command:**
- `bash tools/build_mo2_zip.sh`

**Expected:** new `dist/CalamityAffixes_MO2_2026-02-02.zip`
