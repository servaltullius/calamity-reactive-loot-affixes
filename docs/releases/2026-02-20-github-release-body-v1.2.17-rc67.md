## Calamity Reactive Loot & Affixes v1.2.17-rc67 (Pre-release)

이번 RC는 **하이브리드 드랍 기본화 + 시체 드랍 안정성 보강**이 핵심입니다.

### 핵심 변경
- **기본 드랍 모드 변경**
  - `loot.currencyDropMode` 기본값을 `runtime` -> `hybrid`로 전환
  - 기본 철학: **시체=SPID, 월드/컨테이너=런타임**
- **시체 드랍 안정성 보강 (중점)**
  - 시체에 `CAFF_TAG_CURRENCY_DEATH_DIST` 태그가 있어도,
    실제 시체 인벤토리에 Calamity 통화(룬조각/재련오브)가 없으면
    하이브리드에서 런타임 활성화 롤 1회를 폴백 허용
  - SPID 통화가 이미 주입된 시체는 기존처럼 런타임 롤 스킵(중복 방지)
- **문서 갱신**
  - README/NEXUS 설명을 `hybrid` 기본 기준으로 정리
  - UserPatch 권장 조건을 `leveledList` 고정 운영/충돌 보정 중심으로 명확화

### 테스트/검증
- `tools/release_verify.sh --fast`: PASS
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`: PASS
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`: PASS

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc67_2026-02-20.zip`
