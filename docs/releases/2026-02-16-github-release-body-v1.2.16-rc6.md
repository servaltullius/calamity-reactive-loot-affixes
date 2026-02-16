## CalamityAffixes v1.2.16-rc6

v1.2.16-rc5 후속 성능 핫픽스 프리릴리즈입니다.

### 핵심 변경
- 군중 전투 성능 경로 최적화
  - 비적대 1타 프로크 옵션이 꺼져 있을 때, non-hostile first-hit gate 조회를 핫패스에서 바로 스킵하도록 가드를 추가했습니다.
  - 적용 경로: HandleHealthDamage 훅, HealthDamage/TESHitEvent 트리거, 특수 액션(변환/CoC) 경로.
- non-hostile 재진입 윈도우 조정
  - `NonHostileFirstHitGate` 재진입 허용 시간을 `100ms -> 20ms`로 축소해 군중 상황에서 연속 트리거 스팸을 완화했습니다.
- 회귀 테스트 보강
  - fast-path 가드 정적 검증 추가.
  - gate 재진입 창 축소에 맞춰 런타임 체크 테스트 갱신.

### 검증
- `tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: 통과 (26/26)
  - SKSE static checks (`ctest`): 통과 (2/2)
  - Papyrus compile: 통과
  - lint_affixes: 통과

### 배포 파일
- `CalamityAffixes_MO2_v1.2.16_2026-02-16.zip`

