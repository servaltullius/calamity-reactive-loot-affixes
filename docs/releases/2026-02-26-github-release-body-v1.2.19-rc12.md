## Calamity Reactive Loot & Affixes v1.2.19-rc12 (Pre-release)

이번 RC는 멀티에이전트 심층 분석 결과를 반영한 런타임 안정화와 릴리즈 게이트 강화에 초점을 맞췄습니다.

---

### 변경 사항

- `GetFormEditorID` 사용 경로를 null-safe 유틸로 정리하고, 관련 회귀 테스트를 추가했습니다.
- 룬워드/UI 상태 접근 경로에 `_stateMutex` 잠금을 보강하고, RNG 접근은 `_rngMutex`로 동기화해 경쟁 상태 가능성을 낮췄습니다.
- `HandleHealthDamage` 훅의 후처리(CastOnCrit/OnHealthDamage/Conversion cast)를 task 경로로 분리해 훅 본문 부담을 줄였습니다.
- Trap tick 경로에서 락 범위를 축소해 고비용 구간이 전역 락을 오래 점유하지 않도록 개선했습니다.
- `OnFormDelete` fallback을 즉시 처리 대신 지연 큐 드레인 방식으로 바꿔 교착/재진입 리스크를 낮췄습니다.
- 이벤트 싱크 등록을 idempotent하게 보강해 중복 등록 가능성을 제거했습니다.
- `release_verify.sh`/CI 게이트를 강화했습니다.
  - strict 기본값 적용
  - `--no-strict` 옵션 추가
  - fast 경로에서도 SKSE static checks 빌드
  - runtime gate `ctest`에 `--no-tests=error` 적용

### 검증

- `CAFF_PACKAGE_VERSION=1.2.19-rc12 tools/release_verify.sh --fast --strict --with-mo2-zip` 통과
  - Generator 테스트 71/71
  - SKSE ctest 2/2
  - Papyrus 컴파일/패키징 포함 전체 성공

### 배포 파일

- `CalamityAffixes_MO2_v1.2.19-rc12_2026-02-26.zip`
- SHA256: `297291a7ce8eade4a7a123b7a72194eee4f0368adfe439b20f420a03d2a20034`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.19-rc11...v1.2.19-rc12
