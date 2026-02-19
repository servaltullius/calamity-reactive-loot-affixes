## CalamityAffixes v1.2.17-rc51

`v1.2.17-rc50` 후속 프리릴리즈입니다. 일부 룬워드에서 툴팁에 효과 설명이 누락되던 문제를 수정했습니다.

### 핵심 변경
- 룬워드 효과 설명 누락 수정
  - 합성 룬워드 판별 로직이 `runeword_*_auto`만 인식하던 부분을 보완해 `rw_*_auto`도 정상 인식하도록 수정
  - 아이템 툴팁에서 이름만 나오고 발동/확률/ICD 설명이 비던 케이스를 개선
- 회귀 방지 테스트 보강
  - 런타임 게이트 테스트에 합성 룬워드 ID 판별 조건(`rw_`/`runeword_` + `_auto`) 검증 추가

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc51 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc51_2026-02-20.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
