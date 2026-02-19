## CalamityAffixes v1.2.17-rc45

`v1.2.17-rc44` 후속 프리릴리즈입니다. MCM/패널 설정을 외부 SKSE 설정 파일로 영속화해 새 게임/재시작 시 재설정 부담을 줄였습니다.

### 핵심 변경
- 외부 사용자 설정 파일 도입
  - `Data/SKSE/Plugins/CalamityAffixes/user_settings.json` 추가
  - 런타임(MCM) 값과 Prisma 패널 UI 값을 같은 파일에 저장
- 런타임 설정 영속화
  - `enabled`, `debugNotifications`, `validationIntervalSeconds`, `procChanceMultiplier`
  - `runewordFragmentChancePercent`, `reforgeOrbChancePercent`
  - `dotSafetyAutoDisable`, `allowNonHostileFirstHitProc`
  - MCM 변경 시 즉시 저장, 재로드 시 우선 복원
- Prisma 패널 설정 영속화
  - `panelHotkey`, `uiLanguageMode` 저장/복원
  - MCM 설정 파일 부재 시에도 외부 파일 기준으로 안전 폴백
- 코드리뷰 반영 안정화
  - `panelHotkey` uint 범위 가드 추가(오버플로/절단 방지)
  - `runtime_gate_store_checks`에서 이벤트별 persistence 호출 검증 강화

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc45 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc45_2026-02-19.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
