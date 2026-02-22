## Calamity Reactive Loot & Affixes v1.2.18-rc20 (Pre-release)

룬워드 조각/재련 오브 기본 드랍 확률을 상향 조정했습니다. (MCM에서 즉시 변경 가능)

### 핵심 변경
- 기본 확률 변경
  - 룬워드 조각: `8%` (기존 `5%`)
  - 재련 오브: `5%` (기존 `3%`)

### 검증
- `CAFF_PACKAGE_VERSION=1.2.18-rc20 tools/release_verify.sh --with-mo2-zip` 통과
  - Generator tests: PASS (54/54)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc20_2026-02-22.zip`
