

# Repo Agent Instructions
사용자와 한국어로 대화합니다.

## Required fields (repo-local AGENTS.md)

- Install command: `dotnet restore tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj`
- Dev server command: `N/A (게임 모드 프로젝트 특성상 상시 dev server 없음)`
- Unit test command: `python3 tools/ensure_skse_build.py --lane plugin --lane runtime-gate && dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release && ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
- Lint/format command: `python3 tools/compose_affixes.py --check && python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json && python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null && python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null && python3 tools/audit_source_string_tests.py --max-brittle 353 && python3 tools/verify_commonlib_pin.py && python3 tools/verify_papyrus_pin.py && python3 tools/verify_prisma_view.py && python3 tools/verify_version_consistency.py`
- Typecheck/build command: `python3 tools/ensure_skse_build.py --lane plugin && cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
- Primary entrypoint path: `skse/CalamityAffixes/src/main.cpp`

## Paths

- Repo root: `/home/kdw73/Calamity - Reactive Loot & Affixes`
- Preferred symlink: `Optional no-space alias via CAFF_REPO_RUN_ROOT` (scripts auto-create a temporary alias when needed)
- SKSE project: `skse/CalamityAffixes/`
- Output DLL: `Data/SKSE/Plugins/CalamityAffixes.dll`

## Build And Generate

- Compose affixes: `python3 tools/compose_affixes.py`
- Generate data/runtime artifacts: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`
- Build SKSE DLL: `python3 tools/ensure_skse_build.py --lane plugin && cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
- Run SKSE tests: `python3 tools/ensure_skse_build.py --lane plugin --lane runtime-gate && ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
- Copy built DLL: `cp -f skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll Data/SKSE/Plugins/CalamityAffixes.dll`
- Verify DLL hashes: `sha256sum skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll Data/SKSE/Plugins/CalamityAffixes.dll`

## Papyrus (`Data/Scripts/*.psc`)

`PapyrusCompiler.exe` ships with the Creation Kit, so CI cannot compile these.
The `.pex` are committed and CI packages them, gated on
`tools/verify_papyrus_pin.py`. After editing any `.psc`, any stub under
`tools/papyrus-stubs/`, or the flags file, run:

```
python3 tools/verify_papyrus_pin.py --update
```

It recompiles first, then records the hashes; commit the regenerated `.pex`
together with the pin. Skipping it fails the lint command and the release.

Note that `.pex` output is not reproducible — the compiler emits its string
table in hash-map order and stamps a compile time — so recompiling always
produces a diff even when the source is unchanged. That is expected, and it is
why the pin cannot verify itself by recompiling.

## Prisma panel view (`Data/PrismaUI/views/CalamityAffixes/`)

`index.html` holds only markup; the CSS lives in `styles/*.css` and the JS in
`scripts/*.js`. The plugin passes one path to `CreateView` and knows nothing of
this structure, so a broken reference produces a successful log line and a panel
that is unstyled or empty in game. `tools/verify_prisma_view.py` is the only
check that catches it — it is in the lint command and in `ci-verify.yml`.

Two rules the split depends on:

- **Load order is the `<link>` / `<script src>` order in `index.html`**, and
  nothing else. Adding a file means adding its tag; the order of the tags is the
  cascade order for CSS and the execution order for JS.
- **Classic scripts only — never `type="module"`.** The files share one global
  lexical environment, which is what makes them equivalent to the single script
  they were extracted from. A module gets its own top-level scope and every
  cross-file reference breaks at once.

Test harnesses must read the view through `tools/prisma_view_source.py` (Python)
or `tools/prisma_view_source.js` (Node), never `index.html` directly. Reading
the file alone makes a negative assertion pass because the code it forbids moved
to another file — a silent loss of coverage rather than a failure.

## Packaging

- Build MO2 zip: `tools/build_mo2_zip.sh`
- Release verification: `tools/release_verify.sh`
- Output zip: `dist/CalamityAffixes_MO2_vX.Y.Z_<YYYY-MM-DD>.zip`
