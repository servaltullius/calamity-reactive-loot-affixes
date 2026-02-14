# CalamityAffixes v1.2.10 (2026-02-15)

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

## 패치노트 (v1.2.9 이후)

- 툴팁 UI 커스터마이즈를 확장했습니다.
  - 고급 탭에 툴팁 글자 크기 조절(`Text +/-`)과 위치 이동(좌/우/상/하), 초기화 버튼을 추가했습니다.
  - 툴팁 글자 크기를 70%~180% 범위에서 조절할 수 있습니다.
- 마우스 드래그로 툴팁 위치를 직접 이동할 수 있게 했습니다.
  - 툴팁 타이틀을 드래그하면 즉시 위치가 갱신되고, 놓는 시점에 레이아웃이 저장됩니다.
- 툴팁 레이아웃 영속화를 추가했습니다.
  - `Data/SKSE/Plugins/CalamityAffixes/prisma_tooltip_layout.json`에 위치/폰트 스케일을 저장하고 재시작 시 복원합니다.
- UI 레이아웃 파싱/저장 로직을 `PrismaLayoutPersistence` 모듈로 분리해 유지보수성과 재사용성을 개선했습니다.
- 런타임 설정 로딩 경로를 모듈화(`LoadRuntimeConfigJson`, `ApplyLootConfigFromJson`, `ResolveAffixArray`)하여 스키마 오류 처리와 가독성을 개선했습니다.

## 포함 파일

- `Data/SKSE/Plugins/CalamityAffixes.dll`
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `Data/MCM/Config/CalamityAffixes/config.json`
- `CalamityAffixes_MO2_v1.2.10_2026-02-15.zip`

## 검증

- `tools/release_verify.sh --fast`
- `tools/build_mo2_zip.sh`

## SHA256

- `CalamityAffixes_MO2_v1.2.10_2026-02-15.zip`
  - `eee7829c41cee40f0dacc6e8014382193bdfccced8a9f0a7f81c7b2e9e8915b4`
