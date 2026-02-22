## Calamity Reactive Loot & Affixes v1.2.18-rc22 (Pre-release)

룬워드 조각/재련 오브가 “바닥(월드) 픽업” 경로에서 발밑에 스폰되던 동작을 제거했습니다.
런타임 화폐 드랍은 **컨테이너에만 적용**됩니다.

### 핵심 변경
- 런타임 화폐 드랍 정책 변경
  - 컨테이너: 기존과 동일하게 런타임 롤 적용 (컨테이너 인벤에 추가)
  - 월드(바닥) 픽업: **추가 드랍 없음** (바닥 스폰 제거)

### 검증
- `CAFF_PACKAGE_VERSION=1.2.18-rc22 tools/release_verify.sh --with-mo2-zip` 통과
  - Generator tests: PASS (54/54)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc22_2026-02-22.zip`
