## Calamity Reactive Loot & Affixes v1.2.18-rc18 (Pre-release)

이번 RC는 CoC 계열(썬더볼트 등) 프로크에서 **히트 이펙트 아트가 월드에 잔상처럼 남는 현상**을 완화하는 데 집중했습니다.

### 핵심 변경
- **CoC 바닐라 주문 프로크의 `noHitEffectArt`를 `true`로 고정**
  - 대상: `coc_firebolt`, `coc_ice_spike`, `coc_lightning_bolt`, `coc_thunderbolt`, `coc_icy_spear`, `coc_chain_lightning`, `coc_ice_storm`
  - 기대 효과: 잔상(바닥 번개 등) 고착으로 인해 “이후 발동이 막힌 것처럼 보이는” 체감 이슈를 줄입니다.
- **회귀 방지 테스트 추가**
  - `CastOnCrit` + `Skyrim.esm` 주문을 쓰는 경우 `noHitEffectArt == true`를 요구하도록 `RepoSpecRegressionTests`를 보강했습니다.

### 검증
- `CAFF_PACKAGE_VERSION=1.2.18-rc18 tools/release_verify.sh --with-mo2-zip` 통과
  - Generator tests: PASS (54/54)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc18_2026-02-22.zip`

