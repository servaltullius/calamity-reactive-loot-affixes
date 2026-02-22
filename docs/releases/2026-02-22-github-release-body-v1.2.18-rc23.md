## Calamity Reactive Loot & Affixes v1.2.18-rc23 (Pre-release)

이번 프리릴리즈는 **시체 드랍 SPID 유지** 원칙을 유지하면서, 외부 모드 드랍과의 충돌 가능성을 줄이는 방향으로 정리했습니다.

### 핵심 변경
- 시체 화폐 드랍 정책 고정
  - 시체: SPID `DeathItem` 분배만 사용 (런타임 activation 롤은 계속 스킵)
  - 상자/월드: 기존 하이브리드 정책 유지
- SPID 규칙 단순화
  - `CAFF_TAG_CURRENCY_DEATH_DIST` 태그 주입 규칙 제거
  - `DeathItem` 라인에 ActorType 직접 필터 적용
- 런타임 코드 정리
  - 시체 경로 태그 의존 분기 제거
  - “SPID-owned corpse policy” 로그/게이트 검증 테스트 갱신

### 검증
- `CAFF_PACKAGE_VERSION=1.2.18-rc23 tools/release_verify.sh --with-mo2-zip` 통과
  - compose/lint: PASS
  - Generator tests: PASS (54/54)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - MO2 ZIP 생성: PASS

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc23_2026-02-22.zip`
