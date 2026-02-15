## CalamityAffixes v1.2.14

v1.2.13 후속 핫픽스입니다. 코드리뷰에서 확인된 리스크를 모두 보완했습니다.

### 핵심 변경
- 전투 상태/상호작용 고착 재발 방지
  - `CalamityAffixes_Hit` ModEvent 발행에 적대성 가드를 추가했습니다.
  - C++ 트리거 가드와 Papyrus 경로가 동일 기준으로 동작합니다.
- 별 접두(`*`) 처리 안전화
  - ASCII 접두는 `* ` / `** ` / `*** ` 패턴일 때만 loot 마커로 인식합니다.
  - 원래 이름이 `*`로 시작하는 아이템(`*Iron Sword` 등)은 더 이상 훼손되지 않습니다.
  - UTF-8 별(`★`) 마커 제거 호환은 유지됩니다.
- 회귀 테스트 보강
  - hit event 발행 가드 테스트 추가
  - loot 별 접두 파싱/제거 테스트 추가

### 검증
- `tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: 통과 (25/25)
  - SKSE static checks (`ctest`): 통과
  - Papyrus compile: 통과
  - lint_affixes: 통과

### 배포 파일
- `CalamityAffixes_MO2_v1.2.14_2026-02-15.zip`
