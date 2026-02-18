# Deep Review - 2026-02-18

## Scope
- Branch/commit target: `main`
- Areas reviewed:
  - SKSE runtime (`skse/CalamityAffixes/src`, `skse/CalamityAffixes/include`)
  - Generator (`tools/CalamityAffixes.Generator`)
  - Generator tests (`tools/CalamityAffixes.Generator.Tests`)
  - Papyrus bridge scripts (`Data/Scripts/Source`)

## Findings (ordered by severity)

### S1 - Path traversal / out-of-tree write risk in generator output path
- Evidence:
  - `tools/CalamityAffixes.Generator/Spec/AffixSpecLoader.cs:28`
  - `tools/CalamityAffixes.Generator/Spec/AffixSpecLoader.cs:32`
  - `tools/CalamityAffixes.Generator/Writers/GeneratorRunner.cs:26`
  - `tools/CalamityAffixes.Generator/Writers/GeneratorRunner.cs:28`
- Detail:
  - `modKey` validation only checks extension (`.esp/.esm/.esl`) and does not block path separators (`../`, absolute path, rooted path).
  - `GeneratorRunner` builds output path as `Path.Combine(dataDir, spec.ModKey)` and writes binary directly.
  - A crafted spec can escape `dataDir` and write outside expected staging tree.
- Impact:
  - Unexpected overwrite of arbitrary files reachable by current user permissions.
  - CI/local build integrity risk when non-repo spec is processed.
- Recommendation:
  - Enforce `modKey` as filename-only (`Path.GetFileName(modKey) == modKey` + reject rooted paths + reject directory traversal segments).
  - Add explicit negative tests for `../foo.esp`, absolute paths, and path-separator input.

### S1 - Incomplete runtime state reset on `Revert` for summon corpse-explosion state
- Evidence:
  - `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h:695`
  - `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h:696`
  - `skse/CalamityAffixes/src/EventBridge.Serialization.cpp:754`
  - `skse/CalamityAffixes/src/EventBridge.Serialization.cpp:756`
  - `skse/CalamityAffixes/src/EventBridge.Config.cpp:170`
  - `skse/CalamityAffixes/src/EventBridge.Config.cpp:171`
- Detail:
  - `Revert()` resets `_corpseExplosionSeenCorpses` and `_corpseExplosionState`, but does not reset `_summonCorpseExplosionSeenCorpses` / `_summonCorpseExplosionState`.
  - The config reload reset path *does* clear both summon fields, but `Revert()` itself is asymmetric.
- Impact:
  - Cross-save/session transitions can retain stale summon-corpse dedupe/rate-limit state and cause false suppression or inconsistent proc behavior.
- Recommendation:
  - In `Revert()`, also clear `_summonCorpseExplosionSeenCorpses` and reset `_summonCorpseExplosionState`.
  - Add a regression test covering `Revert()` state symmetry for both normal/summon corpse explosion branches.

### S2 - Validation split: generator entrypoint bypasses stronger runtime lint checks
- Evidence:
  - `tools/CalamityAffixes.Generator/Program.cs:45`
  - `tools/CalamityAffixes.Generator/Spec/AffixSpecLoader.cs:26`
  - `tools/lint_affixes.py:174`
  - `.github/workflows/ci-verify.yml:29`
- Detail:
  - CLI generator path (`dotnet run ...Generator`) uses `AffixSpecLoader` only.
  - Strong runtime/action validation lives in `tools/lint_affixes.py`, not in generator loader.
  - CI runs lint, but direct generator execution path can still accept structurally weak runtime blocks that lint would reject.
- Impact:
  - Local/manual generation may succeed with invalid runtime semantics, leading to downstream runtime mismatch.
- Recommendation:
  - Either invoke lint-equivalent validation from generator CLI or harden `AffixSpecLoader.Validate` to enforce critical runtime constraints.
  - At minimum, fail generator when required runtime/action keys are missing or invalid.

### S3 - Test gaps for hard-failure and safety boundaries
- Evidence:
  - `tools/CalamityAffixes.Generator.Tests/AffixSpecLoaderTests.cs:8`
  - `tools/CalamityAffixes.Generator.Tests/GeneratorRunnerTests.cs:11`
- Detail:
  - Current tests mostly cover happy paths and repo-spec existence checks.
  - Missing negative tests for:
    - unsafe `modKey` path input
    - partial-write/deletion safety behavior
    - revert-state symmetry for summon corpse-explosion caches
- Impact:
  - Regressions in safety guards can pass CI undetected.
- Recommendation:
  - Add targeted negative/boundary tests for the above three safety-critical paths.

## Implementation status (this workspace)
- `S1 modKey path traversal`: fixed in `tools/CalamityAffixes.Generator/Spec/AffixSpecLoader.cs` with filename-only validation and invalid-char guards.
- `S1 Revert reset asymmetry`: fixed in `skse/CalamityAffixes/src/EventBridge.Serialization.cpp` by clearing summon corpse-explosion caches/states in `Load` and `Revert`.
- Added regression tests:
  - `tools/CalamityAffixes.Generator.Tests/AffixSpecLoaderTests.cs` (`Load_WhenModKeyContainsPathSegments_Throws`, `Load_WhenModKeyIsRootedPath_Throws`)

## No critical findings in Papyrus active build path
- Checked active compile target declarations:
  - `tools/compile_papyrus.sh:95`
  - `tools/compile_papyrus.sh:99`
- Notes:
  - Active Papyrus build is limited to lightweight bridge scripts.
  - Legacy manager/alias scripts are present but not compiled by default pipeline.

## Verification commands executed
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release --nologo`
  - Result: PASS (`33 passed, 0 failed`)
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`
  - Result: PASS (`2/2`)
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json`
  - Result: PASS (`Lint OK: 0 warning(s).`)
- `python3 scripts/vibe.py doctor --full`
  - Result: PASS (no boundary violations, doctor report generated)

## Reviewer notes
- A potential trap-thread race was considered, but SKSE task-interface thread-affinity semantics require explicit confirmation before elevating it to a defect. It is tracked as residual risk, not a confirmed finding.
