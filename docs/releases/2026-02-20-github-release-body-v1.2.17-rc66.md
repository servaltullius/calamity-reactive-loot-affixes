## Calamity Reactive Loot & Affixes v1.2.17-rc66 (Pre-release)

이번 RC는 **런타임 드랍 경로 디커플링 + 룬워드 계약(Contract) 파이프라인 안정화**에 중점을 둔 업데이트입니다.

### 핵심 변경
- **통화 드랍 롤 공용화(디커플링)**
  - `activation`/`pickup`에 중복되던 룬워드 조각/재련 오브 롤 코드를 `ExecuteCurrencyDropRolls(...)`로 통합
  - 기존 정책(월드 픽업 1회 롤, 시체/컨테이너 활성화 시점 롤, ledger 가드)은 유지
- **룬워드 계약 SSOT 강화**
  - 신규 계약 파일: `affixes/runeword.contract.json`
  - Generator는 계약 JSON을 우선 사용해 룬워드 카탈로그/룬 가중치를 로드
  - 런타임 배포 스냅샷(`runtime_contract.json`)과의 정합성 경로를 명확화
- **빌드/스크립트 정합성 개선**
  - `build_user_patch.sh`에서 compose 출력 spec과 실제 `--spec` 입력 경로 불일치 가능성 수정
  - `SyncLeveledListCurrencyDropChances` 시그니처 단순화(미사용 플래그 제거)
  - `Assembly.Location` 의존 제거로 단일파일 배포 경로 경고(IL3000 계열) 리스크 완화
- **검증 체인 보강**
  - `compose_affixes.py` 워크플로 전용 테스트 추가
  - `tools/release_verify.sh --fast`에 compose 워크플로 테스트 포함
  - SKSE 정책 테스트를 공용 롤 헬퍼 패턴도 허용하도록 갱신

### 테스트/검증
- `tools/release_verify.sh --fast`: PASS
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`: PASS
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`: PASS
- `CAFF_PACKAGE_VERSION=1.2.17-rc66 tools/build_mo2_zip.sh`: PASS

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc66_2026-02-20.zip`
