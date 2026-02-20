## Calamity Reactive Loot & Affixes v1.2.17-rc61 (Pre-release)

이번 RC는 **모드 몬스터 드랍 대상 누락 문제**를 보강합니다.

### 변경 사항
- UserPatch 자동 탐지 범위 확장:
  - 기존: 모드 `LVLI` 중 `EditorID`가 `DeathItem*`인 리스트만 탐지
  - 변경: 위 조건 + **모드 NPC가 실제 `DeathItem`으로 참조하는 LVLI**까지 탐지
- UserPatch 완료 로그에 `TargetsByMod` 추가
  - 어떤 모드에서 몇 개의 드랍 리스트를 패치했는지 즉시 확인 가능
- 문서 갱신:
  - 자동 탐지 설명을 `DeathItem*` + `NPC DeathItem 참조` 기준으로 업데이트

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc61_2026-02-20.zip`
  - `Tools/UserPatch/CalamityAffixes.UserPatch.exe`
  - `Tools/UserPatch/build_user_patch_wizard.cmd`
  - `Tools/UserPatch/build_user_patch_wizard.ps1`
  - `Tools/UserPatch/affixes/affixes.json`

### 검증
- `dotnet build tools/CalamityAffixes.UserPatch/CalamityAffixes.UserPatch.csproj -c Release` 통과
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` 통과 (46/46)
- `tools/build_mo2_zip.sh` 빌드 성공
