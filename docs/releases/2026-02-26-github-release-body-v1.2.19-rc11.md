## Calamity Reactive Loot & Affixes v1.2.19-rc11 (Pre-release)

이번 RC는 런타임 선언 구조 분리와 검증 체인 보강을 중심으로,
릴리즈 전 회귀 누락 가능성을 줄이는 안정화 업데이트입니다.

---

### 변경 사항

- 런타임 헤더 구조 정리: `EventBridge.h`의 공개/비공개 선언 블록을 `detail/*.inl`로 분리해 유지보수 가독성을 개선했습니다.
- 안전장치 추가: `detail` 선언 조각 파일에 직접 include 금지 sentinel을 넣어 잘못된 include 경로로 인한 ODR/컴파일 리스크를 예방했습니다.
- 검증 체인 강화: `lint_affixes`가 generated 런타임 설정의 **ID 집합뿐 아니라 핵심 내용 드리프트**도 감지하도록 보강했습니다.
- CI 보강: `compose_affixes.py --check`와 SKSE static checks 빌드를 `ci-verify` 흐름에 포함해 릴리즈 전 회귀 누락 가능성을 낮췄습니다.

### 포함 커밋

- `b802c82` refactor(runtime): split EventBridge declarations into guarded detail includes
- `992aab4` ci: enforce compose freshness and skse static checks
- `e878f44` fix(tools): detect generated runtime content drift
- `63b1348` docs(runtime): document rc10 currency roll policy and release notes

### 검증

- `CAFF_PACKAGE_VERSION=1.2.19-rc11 tools/release_verify.sh --fast --strict --with-mo2-zip` 통과
  - compose/lint/json sanity
  - Generator 테스트 71/71
  - SKSE ctest 2/2
  - MO2 패키징 생성 완료

### 배포 파일

- `CalamityAffixes_MO2_v1.2.19-rc11_2026-02-26.zip`
- SHA256: `4ae9f8126d611757a36710559613eed3c85c59d43b558fbaed7efd12693ae837`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.19-rc10...v1.2.19-rc11

### 발행 링크

- https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v1.2.19-rc11
