## Calamity - Reactive Loot & Affixes v1.2.17-rc55 (프리릴리즈)

이번 RC는 **유저 환경(로드오더)별 월드/시체 드랍 패치 자동화**에 초점을 맞췄습니다.

### 주요 변경
- `loot.currencyLeveledListAutoDiscoverDeathItems` 옵션 추가
  - leveled-list 생성 시 `DeathItem*` 타깃 자동 탐지를 지원합니다.
- `CalamityAffixes.UserPatch` CLI 추가
  - 유저별 `plugins.txt/loadorder.txt` + 실제 `Data` 경로를 입력받아
  - `CalamityAffixes_UserPatch.esp`를 생성합니다.
  - Synthesis 호환 인자(`-d/-l/-o`) 지원.
- Windows 더블클릭 Wizard 추가
  - `tools\build_user_patch_wizard.cmd`
  - CLI를 몰라도 경로 선택 창으로 UserPatch 생성 가능.
- 릴리즈 검증 스크립트 강화
  - `tools/release_verify.sh`에 UserPatch CLI 빌드 검증 단계 추가.

### 사용자 안내
- 기본 모드 설치 후, 모드 추가 적 드랍까지 반영하려면 UserPatch를 생성해 활성화하세요.
- 권장: `CalamityAffixes_UserPatch.esp`는 `CalamityAffixes.esp`보다 아래(뒤)에 로드.

### 검증
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
  - PASS (46/46)
- `dotnet build tools/CalamityAffixes.UserPatch/CalamityAffixes.UserPatch.csproj -c Release`
  - PASS
- `tools/build_mo2_zip.sh`
  - PASS (`CalamityAffixes_MO2_v1.2.17-rc55_2026-02-20.zip` 생성)

