## Calamity Reactive Loot & Affixes v1.2.17-rc58 (Pre-release)

이번 RC는 **UserPatch 실행 시 즉시 종료되는 사용성 문제**를 해결했습니다.

### 변경 사항
- `CalamityAffixes.UserPatch.exe`를 인자 없이 직접 실행했을 때:
  - 즉시 종료 대신 안내 메시지 출력
  - 일반 사용자에게 `build_user_patch_wizard.cmd` 실행 경로 안내
  - Windows 대화형 실행에서 `Press Enter to close...` 대기 추가
- `build_user_patch_wizard.cmd` 개선:
  - `powershell` 미설치 사전 체크
  - 실패 시 `pause`로 오류 확인 가능
- 문서 안내 보강:
  - README / Nexus 설명에 `exe`는 CLI 인자용, 일반 사용자는 wizard.cmd 사용 권장 문구 추가

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc58_2026-02-20.zip`
  - `Tools/UserPatch/CalamityAffixes.UserPatch.exe`
  - `Tools/UserPatch/build_user_patch_wizard.cmd`
  - `Tools/UserPatch/build_user_patch_wizard.ps1`

### 검증
- `dotnet build tools/CalamityAffixes.UserPatch/CalamityAffixes.UserPatch.csproj -c Release` 통과
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` 통과 (46/46)
- `tools/build_mo2_zip.sh`로 MO2 ZIP 빌드 완료
