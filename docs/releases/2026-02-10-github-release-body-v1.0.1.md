# CalamityAffixes v1.0.1 (2026-02-10)

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

- 인벤토리에서 항상 떠 있던 “패널 열기/닫기” 퀵런치 UI(오른쪽 상단 pill)를 기본 숨김 처리했습니다.
  - 패널 열기/닫기는 기존대로 단축키(기본: `F11`)로 사용합니다.

## 포함 파일

- `CalamityAffixes.dll`
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `CalamityAffixes_MO2_2026-02-10.zip`

## 검증

- `./tools/release_verify.sh --with-mo2-zip`

## SHA256

- `CalamityAffixes_MO2_2026-02-10.zip`
  - `675c3ec91c14156b8510ab71c904ae4843a32b17e2a7ac846bbf0eb0fe172384`
