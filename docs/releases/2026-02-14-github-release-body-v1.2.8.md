# CalamityAffixes v1.2.8 (2026-02-14)

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

## 패치노트 (v1.2.7 이후)

- (중요) 룬워드 설정 로딩 중 드물게 발생하던 CTD를 수정했습니다.
  - 원인: 런워드 설정 파싱 경로에서 문자열 수명 종료 후 포인터를 참조할 수 있는 댕글링 포인터 이슈
  - 수정: 설정 로드 경로를 안전한 소유/복사 방식으로 정리하여 크래시 가능성 제거
- Wrye Bash에서 `CalamityAffixes.esp` 병합/검사 시 발생하던 FNAM 오류를 수정했습니다.
  - MCM Quest Alias 생성 로직을 조정해 도구 호환성을 개선했습니다.
- MCM 진입 시 `"Error loading config (Click to view)"`만 표시되던 스키마 오류를 수정했습니다.
  - `config.json`의 `cursorFillMode`를 페이지 내부가 아닌 루트 필드로 이동해 MCM Helper 스키마와 정합성을 맞췄습니다.
- Nexus 업로드용 설명 문서(`docs/NEXUS_DESCRIPTION.md`)를 보강했습니다.

## 포함 파일

- `Data/SKSE/Plugins/CalamityAffixes.dll`
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `Data/MCM/Config/CalamityAffixes/config.json`
- `CalamityAffixes_MO2_2026-02-14.zip`

## 검증

- `tools/release_verify.sh --fast --with-mo2-zip`

## SHA256

- `CalamityAffixes_MO2_2026-02-14.zip`
  - `3fa8756961f88cb1df49d790ee147eb1e3525cfb5e22c0d9a26baef652db2bbc`
