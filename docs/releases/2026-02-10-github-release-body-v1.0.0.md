# CalamityAffixes v1.0.0 (2026-02-10)

## 선행 의존성

- 필수
  - SKSE64 (런타임 버전 일치): https://skse.silverlock.org/
  - Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444
  - Prisma UI (툴팁/조작 패널): https://www.nexusmods.com/skyrimspecialedition/mods/148718
- 권장
  - SkyUI: https://www.nexusmods.com/skyrimspecialedition/mods/12604
  - KID: https://www.nexusmods.com/skyrimspecialedition/mods/55728
  - SPID: https://www.nexusmods.com/skyrimspecialedition/mods/36869
- 선택
  - MCM Helper: https://www.nexusmods.com/skyrimspecialedition/mods/53000
  - I4: https://www.nexusmods.com/skyrimspecialedition/mods/85702

## 변경 요약

- `v0.1.0-beta.4` 기반 정식 릴리즈 태그입니다.
- SKSE 플러그인의 버전 표기를 `1.0.0`으로 상향했습니다. (동작 변경 없음)

## 패치노트 (v0.1.0-beta.1 이후)

### v0.1.0-beta.2 (2026-02-09)

- 코드 구조 리팩터링(동작 보존 목적):
  - `EventBridge.Triggers`를 핵심 트리거 로직과 이벤트 핸들러로 분리
  - `EventBridge.Actions`를 Special/Corpse/Dispatch 단위로 분리
- 릴리즈 운영 문서 보강:
  - 인게임 QA 스모크 체크리스트 추가
  - 릴리즈 노트 템플릿 및 업로드 플레이북 정리
- 플레이 영향: 의도된 게임플레이 변경은 없습니다.

### v0.1.0-beta.3 (2026-02-09)

- 속성 계열 히트 이펙트가 기본값에서 보이도록 조정했습니다.
- `CastOnCrit`, `ConvertDamage`, `Archmage`의 `noHitEffectArt` 기본값을 `false`로 변경했습니다.
- 런워드 자동 합성(적응형 원소 계열)에서도 히트 이펙트가 기본 표시되도록 반영했습니다.

### v0.1.0-beta.4 (2026-02-10)

- 개발/QA 자동화 보강(플레이 영향 없음):
  - vibe-kit 기반 레포 컨텍스트/경계 검사 도구 부트스트랩 (MO2 ZIP 미포함)
  - 릴리즈 전 검증 스크립트 `tools/release_verify.sh` 추가
  - 인게임 로그 스모크체크 보조 `tools/qa_skse_logcheck.py` 추가
  - SKSE 플러그인 EventBridge 구현 파일 물리 분리(동작 변경 없음)

### v1.0.0 (2026-02-10)

- beta 프리릴리즈 라벨을 제거하고 `v1.0.0` 정식 릴리즈로 전환했습니다.
- SKSE 플러그인/빌드 메타데이터 버전 표기를 `1.0.0`으로 상향했습니다. (동작 변경 없음)

## 포함 파일

- `CalamityAffixes.dll`
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `CalamityAffixes_MO2_2026-02-10.zip`

## 검증

- `./tools/release_verify.sh --with-mo2-zip`
- `python3 tools/lint_affixes.py --spec affixes/affixes.json`
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`

## SHA256

- `CalamityAffixes_MO2_2026-02-10.zip`
  - `3e63ff4dbaaa05d6fb187dd8ee1e0fa91250bc47661c1ac412e96f870a3de11b`
