# CalamityAffixes v1.0.0 (2026-02-10)

## 선행 의존성

- 필수
  - SKSE64 (런타임 버전 일치): https://skse.silverlock.org/
  - Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444
  - Prisma UI (툴팁/조작 패널): https://www.nexusmods.com/skyrimspecialedition/mods/148718
- 권장
  - SkyUI: https://www.nexusmods.com/skyrimspecialedition/mods/12604
  - KID: https://www.nexusmods.com/skyrimspecialedition/mods/55728
  - SPID: https://www.nexusmods.com/skyrimspecialedition/mods/36869
- 선택
  - MCM Helper: https://www.nexusmods.com/skyrimspecialedition/mods/53000
  - I4: https://www.nexusmods.com/skyrimspecialedition/mods/85702

## 변경 요약

- `v0.1.0-beta.4` 기반 정식 릴리즈 태그입니다.
- SKSE 플러그인의 버전 표기를 `1.0.0`으로 상향했습니다. (동작 변경 없음)

## 포함 파일

- `CalamityAffixes.dll`
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `CalamityAffixes_MO2_2026-02-10.zip`

## 검증

- `./tools/release_verify.sh --with-mo2-zip`
- `python3 tools/lint_affixes.py --spec affixes/affixes.json`
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`

## SHA256

- `CalamityAffixes_MO2_2026-02-10.zip`
  - `3e63ff4dbaaa05d6fb187dd8ee1e0fa91250bc47661c1ac412e96f870a3de11b`

