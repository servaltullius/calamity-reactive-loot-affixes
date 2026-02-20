## Calamity - Reactive Loot & Affixes v1.2.17-rc56 (프리릴리즈)

이번 RC는 `rc55` 피드백 반영으로, **자동패처를 ZIP에 바로 포함**했습니다.

### 변경 사항
- `tools/build_mo2_zip.sh`
  - `CalamityAffixes.UserPatch`를 `win-x64` 단일 EXE로 publish
  - MO2 ZIP에 아래 파일을 자동 포함:
    - `Tools/UserPatch/CalamityAffixes.UserPatch.exe`
    - `Tools/UserPatch/build_user_patch_wizard.cmd`
    - `Tools/UserPatch/build_user_patch_wizard.ps1`
- 생성물 갱신
  - `Data/CalamityAffixes_KID.ini`
  - `Data/CalamityAffixes_DISTR.ini`
  - `Data/Scripts/*.pex`

### 사용자 사용 방법
- ZIP 설치 후 모드 폴더에서 `Tools/UserPatch/build_user_patch_wizard.cmd` 실행
- 경로 선택 창에서:
  - Skyrim `Data` 폴더
  - `plugins.txt` 또는 `loadorder.txt`
  - 출력 `CalamityAffixes_UserPatch.esp`
  를 지정하면 UserPatch를 생성합니다.

### 검증
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
  - PASS (46/46)
- `tools/build_mo2_zip.sh` (`CAFF_PACKAGE_VERSION=1.2.17-rc56`)
  - PASS
  - `CalamityAffixes_MO2_v1.2.17-rc56_2026-02-20.zip` 생성 및 UserPatch EXE 포함 확인

