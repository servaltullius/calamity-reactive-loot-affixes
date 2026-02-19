## CalamityAffixes v1.2.17-rc49

`v1.2.17-rc48` 후속 프리릴리즈입니다. 룬워드 이름 한글 표기를 전체 카탈로그(94종) 기준으로 통일하고, 합성 런워드 툴팁의 효과 요약을 보강했습니다.

### 핵심 변경
- 룬워드 이름 표기 통일(94종)
  - 런워드 카탈로그에 recipe id -> 한국어 이름 매핑 테이블 추가
  - 레시피 표시명 기본값을 한국어로 고정해 Prisma 패널/레시피 목록에서 일관된 한글 표기 제공
  - 예: `Metamorphosis` -> `메타모포시스`
- 합성 런워드(`*_auto`) 이름 로컬라이징 정리
  - `displayNameEn`/`displayNameKo`를 분리 생성하고 한국어 표시명을 우선 사용
  - 기존처럼 일부가 영어로만 보이던 케이스를 축소
- 합성 런워드 툴팁 설명 보강
  - 이름만 출력되던 케이스에 대해 트리거/확률/ICD/효과 타입 요약을 한/영으로 노출
- 회귀 방지 테스트 강화
  - 런워드 이름 로컬라이징 테이블/합성 이름 생성/툴팁 요약 경로를 런타임 게이트 테스트에서 검증

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc49 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc49_2026-02-19.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
