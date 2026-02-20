## Calamity Reactive Loot & Affixes v1.2.17-rc69 (Pre-release)

이번 `rc69`는 드랍 정책 단순화와 확률 체감 안정화를 위한 정리 릴리즈입니다.

### 변경 사항
- 드랍 모드를 `hybrid` 단일 모드로 고정:
  - 런타임이 `runtime`/`leveledList` 레거시 값을 받아도 내부적으로 `hybrid`를 강제
  - 관련 분기/로그 정리로 유지보수 복잡도 감소
- 시체 활성화 드랍 분기 단순화:
  - `CAFF_TAG_CURRENCY_DEATH_DIST` 태그 시체는 SPID 경로 우선(런타임 스킵)
  - 태그 없는 시체만 런타임 폴백
- 기본 드랍 확률 하향:
  - 룬워드 조각: `5%`
  - 재련 오브: `3%`
- 스키마/문서 갱신:
  - `currencyDropMode` 스키마 허용값을 `hybrid`로 제한
  - README/Nexus 설명을 하이브리드 고정 기준으로 정리

### 산출물
- `CalamityAffixes_MO2_v1.2.17-rc69_2026-02-20.zip`
