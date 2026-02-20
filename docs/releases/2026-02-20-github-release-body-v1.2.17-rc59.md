## Calamity Reactive Loot & Affixes v1.2.17-rc59 (Pre-release)

이번 RC는 **UserPatch 위저드에서 MSB1009가 발생하던 배포 경로 문제**를 수정합니다.

### 수정 내용
- `build_user_patch_wizard.ps1` 개선:
  - 배포 ZIP 환경에서는 `dotnet run --project ...` 대신 동봉된 `CalamityAffixes.UserPatch.exe`를 우선 실행
  - 소스 저장소(개발 환경)에서는 기존 `dotnet run` 폴백 유지
  - Spec 경로 해석 로직 강화 (`affixes/affixes.json` 자동 탐색)
  - 번들 EXE도 없고 소스 루트도 없으면 명확한 오류 메시지 표시
- `build_mo2_zip.sh` 개선:
  - `Tools/UserPatch/affixes/affixes.json`를 ZIP에 함께 포함
- 문서 갱신:
  - 배포 ZIP 위저드는 동봉 EXE+spec를 자동 사용한다는 안내 추가

### 원인 요약
- 기존 배포 위저드는 소스 트리 전제를 가진 `dotnet run --project tools/CalamityAffixes.UserPatch`를 호출해,
  일반 사용자 환경(MO2 모드 폴더)에서 `MSB1009: 프로젝트 파일이 없습니다`가 발생했습니다.

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc59_2026-02-20.zip`
  - `Tools/UserPatch/CalamityAffixes.UserPatch.exe`
  - `Tools/UserPatch/build_user_patch_wizard.cmd`
  - `Tools/UserPatch/build_user_patch_wizard.ps1`
  - `Tools/UserPatch/affixes/affixes.json`

### 검증
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` 통과 (46/46)
- `tools/build_mo2_zip.sh` 빌드 성공
- ZIP 내부에 `Tools/UserPatch/affixes/affixes.json` 포함 확인
