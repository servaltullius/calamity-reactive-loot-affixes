## Calamity Reactive Loot & Affixes v1.2.20-rc20 (Pre-release)

이번 RC는 `v1.2.20-rc19` 후속 UI 안정화 프리릴리즈입니다.
Prisma control panel을 드래그할 때 위치 갱신이 무겁게 느껴지던 부분을 정리해
패널 이동이 더 부드럽고 예측 가능하게 보이도록 맞췄습니다.

---

### 변경 사항

- Prisma control panel 드래그 중 `left/top`를 매 이벤트마다 직접 갱신하던 흐름을 `requestAnimationFrame + translate3d(...)` 기반으로 전환했습니다.
- 패널 위치 고정 로직을 `setPanelAnchoredPosition(...)` helper로 통일해 drag / resize / layout restore 경로가 같은 좌표 규칙을 쓰도록 정리했습니다.
- 드래그 중 `will-change`/`transform` 시각 상태를 분리해 pointer release 시 최종 anchor 좌표만 확정하도록 맞췄습니다.

### 검증

- `CAFF_PACKAGE_VERSION=1.2.20-rc20 tools/release_verify.sh --fast --strict --with-mo2-zip` 통과
  - compose/lint/json sanity
  - tools unittest
  - runtime contract sync
  - Generator 테스트 71/71
  - SKSE ctest 3/3
  - MO2 패키징 생성 완료

### 배포 파일

- `CalamityAffixes_MO2_v1.2.20-rc20_2026-03-08.zip`
- SHA256: `5e80ad745dc6ecfeee1977541f79e41086b67d7e02fc27f10f862fd8857d5e63`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc19...v1.2.20-rc20
