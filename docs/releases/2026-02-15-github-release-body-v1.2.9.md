# CalamityAffixes v1.2.9 (2026-02-15)

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

## 패치노트 (v1.2.8 이후)

- (중요) 드랍 후 재획득 반복으로 어픽스를 리롤하던 꼼수를 차단했습니다.
  - 인스턴스 단위로 "이미 롤 처리됨" 상태를 저장하여, 같은 아이템은 다시 롤되지 않습니다.
  - 어픽스가 붙지 않은 결과(무옵)도 동일하게 재롤이 막히도록 처리했습니다.
- (중요) 드랍/이동 이후 특정 상황에서 어픽스가 사라졌는데 별(★) 마커만 남던 문제를 수정했습니다.
  - 어픽스 매핑이 없는 stale star 상태를 감지하면 별표를 정리해 표시 상태를 실제 데이터와 일치시킵니다.
- `TESUniqueIDChangeEvent` 처리 안정성을 강화했습니다.
  - 플레이어 관련 아이템으로 확인되는 이벤트만 키 리매핑을 수행하도록 가드를 추가해, 타 컨테이너/NPC 이벤트로 상태가 오염되는 위험을 줄였습니다.
- 롤 평가 캐시(`loot evaluated`)의 누적 관리 로직을 개선했습니다.
  - 최근 윈도우 + 실제 인벤/런타임 상태 기반 prune을 도입해 장기 세이브에서 불필요한 누적을 억제합니다.
  - 세이브 직전 prune을 수행해 코세이브 비대화도 완화했습니다.
- `uniqueID` 누락 fallback 경로에서 신규 아이템이 우선 처리되도록 순서를 조정했습니다.
  - 이미 평가된 후보보다 미평가 후보를 먼저 처리해, 획득 직후 아이템 롤 누락 가능성을 줄였습니다.

## 포함 파일

- `Data/SKSE/Plugins/CalamityAffixes.dll`
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `Data/MCM/Config/CalamityAffixes/config.json`
- `CalamityAffixes_MO2_v1.2.9_2026-02-15.zip`

## 검증

- `tools/release_verify.sh --fast`
- `tools/build_mo2_zip.sh`

## SHA256

- `CalamityAffixes_MO2_v1.2.9_2026-02-15.zip`
  - `438f02b9e1b877f75fd1da01f09f02c9383b7c701fdbc4404a2642950552e4b9`
