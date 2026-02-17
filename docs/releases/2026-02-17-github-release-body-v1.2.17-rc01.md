## CalamityAffixes v1.2.17-rc01

재련 오브(Reforge Orb) 기반 재련 흐름을 명확히 정리하고, 무어픽스 장비 재련 기회를 보장한 프리릴리즈입니다.

### 핵심 변경
- 재련 오브 기반 재련 경로 정리
  - 재련 버튼(`runeword.reforge`) 실행 시 재련 오브를 소모하여 선택 장비를 재련합니다.
  - 재련 오브가 없으면 재련이 실패하며 안내 메시지를 표시합니다.
- 무어픽스 장비(어픽스 0개) 재련 허용 고정
  - 어픽스가 없는 장비도 재련 시 1줄 목표 롤로 시도되어 “한 번의 기회”를 보장합니다.
- 재련 목표 슬롯 계산 공용화
  - 재련 타깃 개수 계산을 공용 헬퍼로 통일해 런타임 재련 경로와 테스트가 동일 규칙을 사용합니다.
- 재련 회귀 테스트 보강
  - `skse/CalamityAffixes/tests/test_reforge_rules.cpp` 추가
  - `skse/CalamityAffixes/tests/runtime_gate_store_checks.cpp`에 재련 경계 조건 검증 추가

### 검증
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j 8` 통과
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` 통과 (2/2)
- `tools/release_verify.sh --fast --with-mo2-zip` 통과

### 배포 파일
- `CalamityAffixes_MO2_v1.2.17-rc01_2026-02-17.zip`
