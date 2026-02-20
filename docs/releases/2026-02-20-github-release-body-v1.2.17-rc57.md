## Calamity Reactive Loot & Affixes v1.2.17-rc57 (Pre-release)

이번 RC는 **UserPatch 위저드의 MO2 인식 정확도 개선**에 집중했습니다.

### 변경 사항
- `build_user_patch_wizard.ps1`에서 MO2 `ModOrganizer.ini`를 읽어 기본 경로를 자동 탐지하도록 개선
- MO2가 감지되면 `selected_profile` 기준으로 해당 프로필의 `loadorder.txt`/`plugins.txt`를 우선 선택
- `@ByteArray(...)`, `%BASE_DIR%`, 환경변수 형태 경로를 해석하도록 보강
- 다중 MO2 설치/인스턴스 환경에서 점수 기반으로 가장 타당한 컨텍스트를 선택하도록 개선
- README/Nexus 설명서에 위 동작을 반영

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc57_2026-02-20.zip`
  - `Tools/UserPatch/CalamityAffixes.UserPatch.exe`
  - `Tools/UserPatch/build_user_patch_wizard.cmd`
  - `Tools/UserPatch/build_user_patch_wizard.ps1`

### 검증
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` 통과 (46/46)
- `tools/build_mo2_zip.sh`로 MO2 ZIP 빌드 완료

### 참고
- 위저드가 기본 경로를 잘 제안해도, 최종 선택은 사용자 환경 기준으로 직접 확인 후 진행해 주세요.
