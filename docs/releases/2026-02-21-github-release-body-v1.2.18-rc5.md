## Calamity Reactive Loot & Affixes v1.2.18-rc5 (Pre-release)

이번 RC는 룬워드 변환 실패 가능성을 더 낮추는 안정성 강화에 집중했습니다.

### 핵심 변경
- **룬워드 변환 preflight 강화**
  - 조각 소모 직전에 선택 베이스 유효성/적용 가능 여부를 재검증하도록 보강했습니다.
- **룬워드 변환 재진입 가드 추가**
  - 변환 중복 호출 시 동시 처리로 인한 경합을 방지합니다.
- **조각 소비 검증 강화**
  - 배치 제거 후 소비량을 사후 검증하고, 부분 소비/불일치 감지 시 즉시 롤백합니다.
- **기존 롤백 경로 유지**
  - 최종 적용 실패 시 조각 복구(rollback) 경로를 유지/강화했습니다.

### 검증
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` 통과
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` 통과

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc5_<YYYY-MM-DD>.zip`
