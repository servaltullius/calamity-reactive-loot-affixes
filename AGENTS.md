# Repo Agent Instructions
사용자와 한국어로 대화합니다.

## Required fields (repo-local AGENTS.md)

- Install command: `dotnet restore tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj`
- Dev server command: `N/A (게임 모드 프로젝트 특성상 상시 dev server 없음)`
- Unit test command: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release && ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
- Lint/format command: `python3 tools/compose_affixes.py --check && python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json && python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null && python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null`
- Typecheck/build command: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
- Primary entrypoint path: `skse/CalamityAffixes/src/main.cpp`

## Paths

- Repo root: `/home/kdw73/projects/Calamity - Reactive Loot & Affixes`
- Preferred symlink: `/home/kdw73/projects/calamity`
- SKSE project: `skse/CalamityAffixes/`
- Output DLL: `Data/SKSE/Plugins/CalamityAffixes.dll`

## Build And Generate

- Compose affixes: `python3 tools/compose_affixes.py`
- Generate data/runtime artifacts: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`
- Build SKSE DLL: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
- Run SKSE tests: `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
- Copy built DLL: `cp -f skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll Data/SKSE/Plugins/CalamityAffixes.dll`
- Verify DLL hashes: `sha256sum skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll Data/SKSE/Plugins/CalamityAffixes.dll`

## Packaging

- Build MO2 zip: `tools/build_mo2_zip.sh`
- Release verification: `tools/release_verify.sh`
- Output zip: `dist/CalamityAffixes_MO2_vX.Y.Z_<YYYY-MM-DD>.zip`
