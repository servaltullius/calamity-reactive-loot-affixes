## Calamity Reactive Loot & Affixes v1.2.17-rc60 (Pre-release)

이번 RC는 **UserPatch ESP의 슬롯 점유 문제**를 개선합니다.

### 변경 사항
- `CalamityAffixes_UserPatch.esp`를 생성할 때 TES4 헤더에 ESL 플래그(ESPFE) 적용
  - 파일 확장자는 `.esp` 유지
  - 로드오더에서는 라이트 플러그인(ESL-flagged ESP)로 동작
  - 풀 ESP 슬롯 소모를 줄임
- 관련 단위 테스트 추가/보강
- README / Nexus 설명에 UserPatch ESPFE 동작 안내 추가

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc60_2026-02-20.zip`
  - `Tools/UserPatch/CalamityAffixes.UserPatch.exe`
  - `Tools/UserPatch/build_user_patch_wizard.cmd`
  - `Tools/UserPatch/build_user_patch_wizard.ps1`
  - `Tools/UserPatch/affixes/affixes.json`

### 검증
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` 통과 (46/46)
- `tools/build_mo2_zip.sh` 빌드 성공
