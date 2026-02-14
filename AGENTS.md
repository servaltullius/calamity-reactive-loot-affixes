# Repo Agent Instructions

<!-- skills-scout:start -->
## Skills (Auto-Pinned by skills-scout)

This section is generated. Re-run pinning to update.

### Available skills
- (none matched this repo)
<!-- skills-scout:end -->

## vibe-kit (에이전트 컨텍스트)

- 먼저 실행: `python3 scripts/vibe.py doctor --full`
- 최신 컨텍스트: `.vibe/context/LATEST_CONTEXT.md`
- (선택) SKSE 컨텍스트 팩: `python3 scripts/vibe.py pack --scope path --path skse/CalamityAffixes --out .vibe/context/PACK_SKSE.md --max-kb 256`

## 프로젝트 목표 (요약)

- Skyrim SE/AE용 “Diablo/PoE 스타일 어픽스 + 프로크 + ICD” 시스템
- CK 작업을 **최소화(가능하면 0)** 하고, 데이터(= `affixes/affixes.json`) 중심으로 생성/빌드/패키징 반복

## 중요 경로

- Repo root(공백 포함): `/home/kdw73/projects/Calamity - Reactive Loot & Affixes`
- 공백 회피용 심볼릭 링크(권장): `/home/kdw73/projects/calamity`
  - `clang-cl`/CMake cross build에서 공백 경로가 깨지는 케이스가 있어 링크를 만들어 사용합니다.

## “데이터 → 생성 → 패키징” 워크플로우

- 스펙 편집: `affixes/affixes.json`
- 생성(esp/ini/runtime/i4): `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`
- 생성기 테스트: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- MO2 배포 ZIP: `tools/build_mo2_zip.sh` → `dist/CalamityAffixes_MO2_vX.Y.Z_<YYYY-MM-DD>.zip`

## 툴팁 (기본: SKSE DLL / LoreBox 없음)

- 본 프로젝트는 **LoreBox 의존성을 제거**했습니다.
- 기본 UX는 **Prisma UI 오버레이**로 “선택 중인 아이템”의 어픽스 설명을 표시합니다. (Norden UI 등 UI 테마와 무관)
  - 뷰 파일: `Data/PrismaUI/views/CalamityAffixes/index.html`
- Prisma UI가 없으면 툴팁 UI는 표시되지 않습니다. (현재 런타임에는 SkyUI ItemCard 폴백 훅이 없습니다)
- ExtraUniqueID 기반 **인스턴스 어픽스**만 지원합니다.
- (선택) `loot.renameItem=true`: 아이템 이름에 짧은 어픽스 라벨을 붙여(좌측 리스트) 빠르게 식별합니다.

## SKSE 플러그인 빌드

### 권장(안정): Windows + Visual Studio 2022

- 프로젝트: `skse/CalamityAffixes/`
- 출력 DLL: `Data/SKSE/Plugins/CalamityAffixes.dll`

### Linux에서 cross (고급/실험)

이 레포는 WSL/Linux에서 cross로 `CalamityAffixes.dll`을 만들 수 있게 설정되어 있습니다.

핵심 요약:
- Windows SDK/MSVC 라이브러리: `xwin`으로 `skse/CalamityAffixes/.xwin`에 준비
- 컴파일러: `clang-19` 기반 `clang-cl` wrapper 사용
- 링크: `lld-link`
- MSVC/Windows SDK 라이브러리 검색 경로: `LIB` 환경변수로 제공(세미콜론 구분)

주의사항(중요):
- vcpkg로 내려받은 `CommonLibSSE.lib`는 Windows/MSVC로 빌드된 아카이브이며, Linux cross `clang-cl` 환경에서 링크 단계에서 문제가 날 수 있습니다.
- 그래서 `skse/CalamityAffixes/extern/CommonLibSSE-NG/`(소스)에서 **동일 toolchain으로 CommonLibSSE를 먼저 빌드**한 뒤, 플러그인 빌드에 사용합니다.
- SE 1.5.97 전용 빌드가 필요하면 CommonLibSSE-NG를 `ENABLE_SKYRIM_AE=OFF`로 빌드하세요.
  - 설치 후 `CommonLibSSEConfig.cmake`가 `CommonLibSSE.cmake`를 include 하므로, install tree에 `CommonLibSSE.cmake`가 없으면 아래 경로로 복사해야 합니다:
    - `skse/CalamityAffixes/extern/CommonLibSSE-NG/cmake/CommonLibSSE.cmake`
      → `skse/CalamityAffixes/extern/CommonLibSSE-NG/install.<...>/lib/cmake/CommonLibSSE/CommonLibSSE.cmake`

## 프로젝트 전용 스킬

- `calamity-skyrim-affix-devkit` 스킬을 우선 사용합니다:
  - Papyrus 이벤트 패턴/ICD 가드
  - SPID·KID 템플릿
  - MCM Helper 설정/디버그 툴링
