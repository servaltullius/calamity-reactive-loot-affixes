## Calamity Reactive Loot & Affixes v1.2.17-rc70 (Pre-release)

이번 `rc70`는 심층 코드리뷰 후속 안정화 릴리즈입니다.

### 변경 사항
- 런타임 메모리 안전성 보강:
  - DoT 관측 MagicEffect ID 세트에 하드 캡(`4096`) 추가
  - 캡 도달 시 1회 경고 후 추가 ID 누적 방지
- 루팅 핫패스 비용 절감:
  - 월드가 아닌 소스(시체/컨테이너)에서는 불필요한 currency ledger 조회를 건너뛰도록 정리
  - 루팅 큐잉 디버그 로그를 `debugLog` 가드 하에만 출력하도록 수정
- 스펙 검증 체인 강화:
  - `tools/lint_affixes.py`에 JSON Schema 검증(`--schema`) 추가
  - CI `affix-lint` 잡에서 `jsonschema` 의존성 설치 단계 추가
  - 실데이터 형태(`nameEn/nameKo/family`, nullable fields, `archetype`, `spellType/castType`)를 스키마에 반영
- 정책 테스트 보강:
  - 런타임 사용자 설정 round-trip 필드 정책 체크 추가
  - 코드 줄바꿈 스타일에 덜 민감하도록 검사 로직 개선

### 검증
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `python3 tools/tests/test_compose_affixes.py`
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`

### 산출물
- `CalamityAffixes_MO2_v1.2.17-rc70_2026-02-21.zip`
