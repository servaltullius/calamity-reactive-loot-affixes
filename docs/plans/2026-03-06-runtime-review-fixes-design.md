# Runtime Review Fixes Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** config reload/reset, release verification, and packaging scripts that were flagged in the review behave safely and leave reproducible evidence.

**Architecture:** keep the runtime fix local to `ResetRuntimeStateForConfigReload()` so the public reload path matches serialization reset behavior, and keep script fixes local to `tools/release_verify.sh` / `tools/build_mo2_zip.sh` by routing composed specs through temporary files instead of tracked outputs. Add regression coverage first so each change has a red-green proof.

**Tech Stack:** C++, CMake/CTest runtime gate checks, Python unittest, Bash release scripts.

---

### Task 1: Config Reload Transient State Reset

**Files:**
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_common.h`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_settings_policy.cpp`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_main.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.Config.Reset.cpp`

**Steps:**
1. Add a failing runtime gate test that inspects `ResetRuntimeStateForConfigReload()` and requires the same transient dedup/proc state reset as serialization/revert paths.
2. Run the runtime gate checks and confirm that new test fails for the current source.
3. Update `ResetRuntimeStateForConfigReload()` to reset the missing transient fields.
4. Re-run the runtime gate checks and confirm the new test passes.

### Task 2: Release/Packaging Script Workspace Safety

**Files:**
- Modify: `tools/tests/test_release_verify.py`
- Modify: `tools/release_verify.sh`
- Modify: `tools/build_mo2_zip.sh`

**Steps:**
1. Add failing tests that require the scripts to avoid composing directly into tracked `affixes/affixes.json`, and require `release_verify.sh --fast` to still compile the plugin target.
2. Run the Python tools test suite and confirm those new checks fail against the current scripts.
3. Change both scripts to compose into a temporary spec path and consume that path downstream.
4. Change `release_verify.sh` fast mode to build `CalamityAffixes` before running tests.
5. Re-run the Python tools tests and confirm the new checks pass.

### Task 3: Final Verification

**Files:**
- Verify only

**Steps:**
1. Run generator tests.
2. Run tools workflow tests.
3. Run runtime contract sync.
4. Run CTest runtime gate checks.
5. Run `cmake --build ... --target CalamityAffixes`.
