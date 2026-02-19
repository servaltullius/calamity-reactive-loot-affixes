## CalamityAffixes v1.2.17-rc47

`v1.2.17-rc46` 후속 프리릴리즈입니다. 런워드 계약 스냅샷 기반 로딩 경로를 도입하고, 런타임 게이트 검증 체인을 모듈화해 릴리즈 안정성을 높였습니다.

### 핵심 변경
- 런워드 계약 스냅샷 확장
  - Generator가 `runtime_contract.json`을 생성하며 `runewordCatalog`와 `runewordRuneWeights`를 함께 기록
  - 런워드 조각(룬) 레코드 생성도 동일 계약 데이터 기반으로 동기화
- SKSE 런워드 카탈로그 로딩 안정화
  - 런타임 계약 스냅샷에서 카탈로그/가중치를 우선 로드
  - 스냅샷이 비어 있거나 유효 레시피가 0개면 컴파일 내장 카탈로그로 자동 폴백
- 런타임 계약 검증/경로 관리 정리
  - 트리거/액션/런워드 계약 검증 경로를 단일화
  - 런타임 상대 경로 해석 공통 모듈(`RuntimePaths`)로 통합
- 사용자 설정 파일 내구성 보강
  - `user_settings.json` 파싱/IO 실패 시 파일을 즉시 덮어쓰지 않고 quarantine 후 재생성
- 런타임 게이트 테스트 모듈화
  - 단일 대형 테스트를 기능별 파일로 분리
  - 테스트 스크립트가 `runtime_contract.json` 부재 시 임시 생성 후 검증하도록 개선

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc47 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc47_2026-02-19.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
