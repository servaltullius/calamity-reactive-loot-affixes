## Calamity Reactive Loot & Affixes v1.2.18-rc17 (Pre-release)

이번 RC는 **룬워드 변환(Transmute)에서 “조각만 소모되고 완료가 안 되는” 체감 이슈**의 안전장치를 강화하는 데 집중했습니다.

### 핵심 변경
- **룬워드 변환 실패 시 조각 복구(rollback) 신뢰성 강화**
  - 조각 복구(`GrantRunewordFragments`)를 “요청 수량”이 아니라 **실제 인벤토리 증가량** 기준으로 판정합니다.
  - 복구가 부분 실패로 감지되면, 누락된 조각을 **플레이어 발밑에 스폰**해 완전 유실 가능성을 낮춥니다.
- **변환 실패 알림 개선**
  - 변환 실패 시 알림에 `reason` 태그를 포함해, 로그 없이도 실패 원인을 더 쉽게 좁힐 수 있게 했습니다.

### 문서 정합성
- README의 **시체 드랍(하이브리드) 설명**에서 “태그 없는 시체 런타임 폴백” 문구를 현재 정책(SPID-only)과 일치하도록 정정했습니다.

### 검증
- `CAFF_PACKAGE_VERSION=1.2.18-rc17 tools/release_verify.sh --with-mo2-zip` 통과
  - Generator tests: PASS (53/53)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc17_2026-02-22.zip`

