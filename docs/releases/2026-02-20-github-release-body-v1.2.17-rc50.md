## CalamityAffixes v1.2.17-rc50

`v1.2.17-rc49` 후속 프리릴리즈입니다. 룬워드 고유 시그니처 효과 범위를 넓히고, 저체력 트리거(LowHealth) 동작을 런타임/계약/테스트까지 일관되게 보강했습니다.

### 핵심 변경
- 룬워드 시그니처 효과 확장
  - 핵심 4종(`Grief`, `Infinity`, `Enigma`, `Call to Arms`)에 전용 합성 스타일/튜닝 추가
  - 추가 시그니처 키(`signature_*`)를 확장해 툴팁/패널에서 룬워드별 개성이 더 분명하게 보이도록 정리
  - Prisma UI 효과 요약 문구(한/영) 매핑을 확장
- 저체력 트리거(`LowHealth`) 지원 추가
  - 런타임 계약(`runtime_contract.json`, `RuntimeContract.h`) 및 Generator 검증 계약에 `LowHealth` 트리거 추가
  - 이벤트 라우팅에서 저체력 트리거를 별도 인덱싱/처리하도록 확장
  - 저체력 게이트를 "단일 스냅샷 판정 + 재무장(rearm)" 방식으로 안정화해 다중 어픽스 동시 판정 시 일관성 개선
- 런타임 게이트 테스트 강화
  - 룬워드 카탈로그(94종) 기준 스타일/튜닝/요약 오버라이드 누락 검사 추가
  - 저체력 스냅샷 게이트 회귀 방지 정책 테스트 추가

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc50 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc50_2026-02-20.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
