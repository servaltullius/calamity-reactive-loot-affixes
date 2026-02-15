## CalamityAffixes v1.2.13

유저 피드백 반영 핫픽스 릴리즈입니다.

### 핵심 변경
- 전투 상태/상호작용 이슈 완화
  - `HandleHealthDamage` 경로와 `TESHitEvent` 폴백 경로 모두에 **hostile 판정 가드**를 추가했습니다.
  - 비적대 대상(동료/중립/NPC 상호작용 상황)에서 히트 트리거가 불필요하게 처리되는 경우를 차단합니다.
- 어픽스 개수 표시 문자 호환성 개선
  - 아이템명 접두 표시를 유니코드 별(`★`)에서 ASCII(`*`, `**`, `***`)로 변경했습니다.
  - 기존 별 표식(유니코드/ASCII) 제거 로직은 모두 유지해 저장 데이터와 호환됩니다.
- 룬워드 완료 아이템 선택 방지
  - 이미 완성된 룬워드 아이템은 베이스 후보 수집 단계에서 제외됩니다.
  - 이미 선택된 베이스가 완료 상태로 바뀐 경우 sanitize 단계에서 선택을 자동 해제합니다.
  - 완료 아이템 선택 시 안내 알림을 표시하고 선택을 거부합니다.
- 드랍 확률/가중치 일관성 보정
  - 전역 `loot.chancePercent`를 아이템 단위에서 1회 게이트로 적용하도록 정리했습니다.
  - 레거시 데이터 호환을 위해 일부 prefix에 대해 `kid.chance` -> `lootWeight` 폴백을 복원했습니다.

### 검증
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` 통과
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` 통과
- `tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: 통과 (25/25)
  - SKSE static checks (`ctest`): 통과
  - Papyrus compile: 통과
  - lint_affixes: 통과

### 배포 파일
- `CalamityAffixes_MO2_v1.2.13_2026-02-15.zip`
