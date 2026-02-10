# CalamityAffixes v0.1.0-beta.4 (2026-02-10)

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

- (내부) vibe-kit 기반 레포 컨텍스트/경계 검사 도구를 부트스트랩했습니다. (MO2 ZIP에는 포함되지 않습니다)
- (내부) 릴리즈 전 검증 스크립트 `tools/release_verify.sh` 와 인게임 로그 스모크체크 보조 `tools/qa_skse_logcheck.py`를 추가했습니다.
- (내부) SKSE 플러그인의 EventBridge 구현을 파일 단위로 물리 분리했습니다. (동작 변경 없음, 빌드/테스트 통과)

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
  - `861e16fc2da7df78bcb3541f790b8e0b45648c872fd96decc645dbe4c0203485`

