## Calamity Reactive Loot & Affixes v1.2.17-rc62 (Pre-release)

이번 RC는 **문서 정리/보강**에 초점을 맞췄습니다.

### 문서 갱신
- README UserPatch 섹션 정리
  - 실행 경로(`build_user_patch_wizard.cmd`) 권장 흐름 명확화
  - MO2 프로필 기반 기본 경로 자동 탐지 동작 설명 보강
  - ESPFE(ESL 플래그) 동작 명시
- README에 `UserPatch 트러블슈팅` 섹션 추가
  - `No arguments were provided`
  - `MSB1009: 프로젝트 파일이 없습니다`
  - 특정 모드만 반영될 때 확인 포인트(`TargetsByMod`, 활성 MO2 프로필)
- Nexus 설명서(`docs/NEXUS_DESCRIPTION.md`) 한/영 섹션 동기화
  - 동일 트러블슈팅/동작 안내 반영

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc62_2026-02-20.zip`
  - `Tools/UserPatch/CalamityAffixes.UserPatch.exe`
  - `Tools/UserPatch/build_user_patch_wizard.cmd`
  - `Tools/UserPatch/build_user_patch_wizard.ps1`
  - `Tools/UserPatch/affixes/affixes.json`
  - `Docs/README.md` (최신 문서 반영)

### 검증
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` 통과 (46/46)
- `tools/build_mo2_zip.sh` 빌드 성공
