## CalamityAffixes v1.2.17-rc42

`v1.2.17-rc41` 후속 프리릴리즈입니다. 생성기/검증 경로의 안정성 보강과 릴리즈 산출물 재생성을 반영했습니다.

### 핵심 변경
- 생성기 입력 안전성 강화
  - `modKey`를 파일명 전용으로 검증해 경로 탈출 입력(`../`, 절대경로, 구분자 포함)을 차단했습니다.
- 런타임 상태 초기화 대칭성 보강
  - `Revert`/`Load` 경로에서 summon corpse-explosion 상태/캐시 정리를 일관되게 적용했습니다.
- 검증 규칙 단일화
  - `tools/affix_validation_contract.json`을 추가하여 C# 로더와 Python lint가 동일한 trigger/action 타입 집합을 사용합니다.
- 런타임 검증 강화
  - `runtime.trigger`, `runtime.action.type`, `procChancePercent`, `icdSeconds`, `perTargetICDSeconds`의 하드 검증을 생성기 진입점에 추가했습니다.
- 회귀 테스트 보강
  - runtime 경계값/타입 오류 케이스를 Generator 테스트에 추가했습니다.

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc42 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc42_2026-02-19.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
