## Calamity Reactive Loot & Affixes v1.2.17-rc63 (Pre-release)

이번 RC는 **UserPatch 드랍 주입 안정성(로드오더 충돌 복원)** 개선에 초점을 맞췄습니다.

### 체감 이슈 원인/해결
- 원인:
  - 일부 환경에서 다른 모드가 바닐라 `DeathItem/LItem`을 뒤에서 오버라이드하면,
    기존 UserPatch가 해당 경로를 충분히 재주입하지 못해 드랍 체감이 낮아질 수 있었습니다.
- 해결:
  - UserPatch가 `affixes.json`의 명시 타깃(바닐라 FormKey 포함)까지 항상 패치 대상으로 포함하도록 보강했습니다.
  - 결과적으로 로드오더 충돌 상황에서도 통화 드랍 주입 유지력이 올라갑니다.

### 변경 사항
- UserPatch 타깃 해석 보강
  - `affixes.json`에 명시된 leveled-list 타깃을 **항상 포함**하도록 변경
  - 바닐라 FormKey(예: `Skyrim.esm|...`)도 제외하지 않고 패치 대상으로 유지
  - 효과: 다른 모드가 바닐라 LVLI를 뒤에서 덮어쓴 환경에서도 통화 드랍 주입을 재적용 가능
- UserPatch 오류/도움말 문구 정정
  - 대상 0건 시 원인을 `spec/loadorder` 기준으로 안내
  - 도움말에 명시 타깃 + 모드 DeathItem 자동탐지 동작을 명확히 표기
- 문서 갱신
  - `README.md`, `docs/NEXUS_DESCRIPTION.md`에 UserPatch 역할을
    - 로드오더 충돌 복원
    - 모드 추가 적 드랍 반영
    두 축으로 명확화(한/영 동기화)

### 변경되지 않은 사항
- 이번 RC는 **드랍 확률 수치 자체를 변경하지 않습니다**.
  - 기본값: 룬워드 조각 `6%`, 재련 오브 `2%`
  - `currencyDropMode=leveledList` 기준, MCM 값은 `CAFF_LItem_*`의 `ChanceNone` 동기화로 반영됩니다.

### 적용 안내
- 기존 유저도 rc63 적용 후 `CalamityAffixes_UserPatch.esp`를 다시 생성하면 반영 정확도가 올라갑니다.

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc63_2026-02-20.zip`
  - `Tools/UserPatch/CalamityAffixes.UserPatch.exe`
  - `Tools/UserPatch/build_user_patch_wizard.cmd`
  - `Tools/UserPatch/build_user_patch_wizard.ps1`
  - `Tools/UserPatch/affixes/affixes.json`

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc63 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (46/46)
  - UserPatch build: PASS
  - SKSE ctest: PASS (2/2)
  - MO2 ZIP 생성: PASS
