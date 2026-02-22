## Calamity Reactive Loot & Affixes v1.2.18-rc21 (Pre-release)

룬워드 조각/재련 오브가 “바닥에 생성(PlaceAtMe)”되는 경우, **소유 셀(영주 집 등)에서 `훔치기`로 뜨는 문제**를 완화했습니다.

### 핵심 변경
- 월드에 스폰되는 화폐 드랍(룬워드 조각/재련 오브)을 **플레이어 소유**로 설정
  - 목적: 소유 셀/실내에서 보상 드랍이 `Steal`로 표시되는 UX를 방지

### 검증
- `CAFF_PACKAGE_VERSION=1.2.18-rc21 tools/release_verify.sh --with-mo2-zip` 통과
  - Generator tests: PASS (54/54)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc21_2026-02-22.zip`
