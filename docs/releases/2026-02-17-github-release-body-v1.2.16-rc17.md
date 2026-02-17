## CalamityAffixes v1.2.16-rc17

서픽스 비활성화 정책 안정화 프리릴리즈입니다.

### 핵심 변경
- 신규 드랍 서픽스 차단 안정화
  - 롤링 경로뿐 아니라 `기존 인스턴스 매핑 재사용/폴백` 경로에서도 서픽스 토큰을 정규화해 제거하도록 보강했습니다.
- 로드/설정 순서 안전성 보강
  - 세이브 로드 시 인덱스가 준비된 경우에만 정규화를 수행하고,
  - `LoadConfig` 완료 직후 전체 추적 인스턴스를 다시 정규화하도록 보강했습니다.
- 운영 설정 명시화
  - `loot.stripTrackedSuffixSlots` 옵션을 추가/명시(`true`)해 런타임 동작을 고정했습니다.
- 회귀 테스트 보강
  - `LootSlotSanitizer` 정규화 로직(중복/비허용/손상 카운트 입력) 검증 테스트를 추가했습니다.

### 검증
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j 8` 통과
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` 통과 (2/2)

### 배포 파일
- `CalamityAffixes_MO2_v1.2.16_2026-02-17.zip`
