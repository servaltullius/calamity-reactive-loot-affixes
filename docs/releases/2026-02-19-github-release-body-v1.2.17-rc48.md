## CalamityAffixes v1.2.17-rc48

`v1.2.17-rc47` 후속 프리릴리즈입니다. MCM 확률 슬라이더 표시 포맷을 MCM Helper 문법에 맞게 수정해, 슬라이더 이동 시 현재 퍼센트를 정상 확인할 수 있도록 고쳤습니다.

### 핵심 변경
- MCM 확률 표시 포맷 수정
  - `Runeword Fragment Chance` / `Reforge Orb Chance` 슬라이더의 `formatString`을 `"{1}%"`로 수정
  - `Validation Interval`, `Proc Multiplier` 슬라이더 포맷도 MCM Helper 토큰 문법(`{0}`, `{1}`)으로 통일
- 회귀 방지 테스트 추가
  - `runtime_gate_store_checks`에 확률 슬라이더 포맷 검증 추가
  - 잘못된 printf 포맷(`%.1f%%`)이 재도입되면 테스트 실패하도록 가드

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc48 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc48_2026-02-19.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
