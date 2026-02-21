## Calamity Reactive Loot & Affixes v1.2.18-rc4 (Pre-release)

이번 RC는 최근 보고된 두 가지 체감 이슈를 안정적으로 해결하는 데 집중했습니다.

### 핵심 수정
- **룬워드 변환 소모 트랜잭션화**
  - 변환 시 조각 소비를 먼저 한 뒤 적용 단계가 실패하면, 소비된 조각을 롤백(복구)하도록 보강했습니다.
  - 중간 소비 실패/룬 아이템 조회 실패/최종 적용 실패 모두 롤백 경로를 적용했습니다.
- **하이브리드 시체 드랍 정책 정합성 강화**
  - 하이브리드 모드에서 시체는 SPID 권한 경로만 사용하도록 고정했습니다.
  - 기존의 “태그 없는 시체 -> 런타임 fallback” 경로를 제거해, 시체 드랍에서 혼합 체감이 발생하지 않도록 정리했습니다.

### 품질/검증
- 런타임 게이트 정책 테스트에 회귀 가드 추가
  - 룬워드 변환 실패 후 복구 경로 가드
  - 시체 fallback 제거 정책 가드
- 검증 통과:
  - `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc4_<YYYY-MM-DD>.zip`
