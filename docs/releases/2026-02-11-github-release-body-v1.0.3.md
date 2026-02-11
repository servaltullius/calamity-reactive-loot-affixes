# CalamityAffixes v1.0.3 (2026-02-11)

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

- MCM config에서 `shortNames` 배열을 제거하여 구버전 MCM Helper 호환성을 개선했습니다.
  - 일부 사용자 환경에서 MCM 진입 시 "Error loading config" → "invalid schema" 오류가 보고됨
  - 원인: `shortNames`는 최신 MCM Helper에서만 지원하는 기능으로, 구버전에서 스키마 검증 실패 유발 가능
  - 수정: `shortNames` 제거 → enum 드롭다운에 풀네임("English", "한국어", "English + 한국어") 표시

## 포함 파일

- `CalamityAffixes_MO2_2026-02-11.zip`

## 검증

- MCM config.json JSON 유효성 확인
- `build_mo2_zip.sh` 빌드 + lint 통과

## SHA256

- `CalamityAffixes_MO2_2026-02-11.zip`
  - `9b08061ebe99d81c26d3598c20025324ef3cbea2647681c8cb23a935df3ebc4c`
