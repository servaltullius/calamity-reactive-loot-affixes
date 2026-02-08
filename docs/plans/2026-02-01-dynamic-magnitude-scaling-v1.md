# Dynamic Magnitude Scaling (CastOnCrit) — Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Keep proc spell damage relevant at high level without CK work by adding configurable, runtime hit-based magnitude scaling for `CastOnCrit`.

**Architecture:** Add an optional `magnitudeScaling` object to runtime `action` configs. When present, compute a `magnitudeOverride` from hit damage (`HitData`) using a simple affine formula + optional clamps, optionally using the spell’s base magnitude as a minimum. Pass the computed value into `RE::MagicCaster::CastSpellImmediate(..., a_magnitudeOverride, ...)`.

**Tech Stack:** C++23 (`skse/CalamityAffixes`), CommonLibSSE-NG, `nlohmann_json`, existing generator + MO2 zip workflow.

---

## Runtime JSON (new, optional)

### `action.magnitudeScaling`

```json
{
  "source": "HitPhysicalDealt",
  "mult": 0.3,
  "add": 0.0,
  "min": 0.0,
  "max": 0.0,
  "spellBaseAsMin": true
}
```

**Semantics**
- `source`:
  - `HitPhysicalDealt`: `max(0, hit.physicalDamage - hit.resistedPhysicalDamage)`
  - `HitTotalDealt`: `max(0, hit.totalDamage - hit.resistedPhysicalDamage - hit.resistedTypedDamage)`
- `scaled = add + mult * sourceValue`
- Clamp:
  - if `spellBaseAsMin`: `scaled = max(scaled, spellBaseMagnitude)`
  - if `min > 0`: `scaled = max(scaled, min)`
  - if `max > 0`: `scaled = min(scaled, max)`
- If `magnitudeScaling` is missing or invalid → keep existing behavior (`magnitudeOverride` passthrough).

---

## Tasks

### Task 1: Add pure scaling helper + failing unit tests

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`
- Create: `skse/CalamityAffixes/tests/test_magnitude_scaling.cpp` (small C++ unit test)
- Modify: `skse/CalamityAffixes/CMakeLists.txt`
- Modify: `skse/CalamityAffixes/vcpkg.json` (add Catch2 for tests)

**Test cases (examples):**
- `spellBaseAsMin=true` preserves baseline when hit damage is low.
- Scaling uses `HitPhysicalDealt` and ignores negative/zero dealt.
- `min`/`max` clamps apply correctly.

### Task 2: Parse `magnitudeScaling` from JSON for `CastOnCrit`

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`

**Step:**
- Extend action parsing for `CastOnCrit` to read `magnitudeScaling` (if present).

### Task 3: Apply scaling in `EvaluateCastOnCrit`

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`

**Step:**
- When a picked `CastOnCrit` action has `magnitudeScaling.source != None`, compute and return computed `magnitudeOverride`.

### Task 4: Enable scaling for current CoC affixes

**Files:**
- Modify: `affixes/affixes.json`

**Step:**
- Add `magnitudeScaling` blocks for CoC affixes with initial tuning (start with `mult=0.3`, `spellBaseAsMin=true`).

### Task 5: Build + package

**Commands:**
- Build DLL: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel`
- Copy output to `Data/SKSE/Plugins/CalamityAffixes.dll` (if build doesn’t already).
- Regenerate Data: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`
- Build zip: `tools/build_mo2_zip.sh`

