## CalamityAffixes v1.2.17-rc46

`v1.2.17-rc45` 후속 프리릴리즈입니다. 외부 사용자 설정 저장 경로의 내구성/안전성을 강화하고, MCM 런타임 설정 저장을 디바운스 기반으로 최적화했습니다.

### 핵심 변경
- 런타임 설정 저장 디바운스 도입
  - MCM 연속 조작 시 즉시 다중 디스크 쓰기 대신 배치 저장
  - 동일 페이로드 중복 저장 스킵
  - 세이브 직전 강제 flush로 마지막 변경 유실 방지
- 사용자 설정 파일 원자적 저장 강화
  - 임시 파일 기록 + flush + 원자적 replace(`MoveFileExW`)
  - 부분 쓰기/중간 종료 시 손상 가능성 완화
- 파싱 실패 시 비파괴 보호
  - 기존 `user_settings.json` 파싱/IO 실패 상태에서 덮어쓰기 차단
  - 손상 파일을 빈 객체로 덮어쓰는 회귀 방지
- 회귀 방지 테스트 보강
  - `runtime_gate_store_checks`에 디바운스 저장 wiring/원자 저장 경로 검증 추가

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc46 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc46_2026-02-19.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
