## Calamity Reactive Loot & Affixes v1.2.17-rc68 (Pre-release)

이번 `rc68`은 **하이브리드 드랍의 시체 런타임 과발동 체감**을 줄이는 안정화 패치입니다.

### 변경 사항
- 하이브리드(`loot.currencyDropMode=hybrid`) 시체 분기 로직 조정:
  - `CAFF_TAG_CURRENCY_DEATH_DIST` 태그가 있는 시체는 런타임 롤을 스킵하고 SPID 경로를 우선 사용
  - 태그가 없는 시체만 런타임 폴백 대상
- 시체 인벤토리에서 통화 존재 여부를 스캔하던 보조 체크 제거:
  - 드랍 결과(성공/실패)와 무관하게 런타임 폴백이 자주 개입하던 경로를 정리
- 디버그 로그 메시지 정리:
  - `SPID-tagged corpse in hybrid mode` / `untagged corpse in hybrid mode`로 원인 추적이 명확해짐

### 의도된 동작(요약)
- `runtime` 모드: 기존처럼 런타임 롤 유지
- `hybrid` 모드: **시체=SPID 우선**, 월드/컨테이너=런타임, 태그 없는 시체만 런타임 폴백

### 산출물
- `CalamityAffixes_MO2_v1.2.17-rc68_2026-02-20.zip`
