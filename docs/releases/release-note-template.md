# Release Note Template (CalamityAffixes)

> 목적: 매 릴리즈 본문에서 변경점/범위/검증 정보를 동일 구조로 유지하기 위한 템플릿

## 제목

`CalamityAffixes vX.Y.Z (YYYY-MM-DD)`

## 변경 요약

- [핵심 변경 1]
- [핵심 변경 2]
- [핵심 변경 3]
- [재련 오브(Reforge Orb) 관련 변경: 획득/소모/대상/예외 규칙]

## 적용 범위

- 지원 대상:
  - [예: 플레이어 장비/인벤토리]
  - [예: player-owned summon]
- 미지원/제한:
  - [예: 일반 팔로워/NPC 독립 런타임 미지원]

## 호환성/마이그레이션

- 세이브/직렬화:
  - [예: runtime-state v5, v1~v4 마이그레이션 지원]
- 의존성:
  - [필수/권장 의존성 링크]

## 테스트 범위 (정적)

- 빌드 lane 확인(실수 방지):
  - 테스트/정적체크: `linux-release-commonlibsse` (g++)
  - 실제 DLL 산출: `build.linux-clangcl-rel` (clang-cl + lld-link + xwin)
  - `fatal error: Windows.h: No such file or directory` 발생 시 DLL 빌드 lane이 잘못된 경우가 대부분입니다.
- (원샷) 로컬 검증 스크립트:
  - `tools/release_verify.sh`
- 빌드:
  - `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j`
- 정적 체크:
  - `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`
- DLL 최신 반영 확인:
  - `cp -f skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll Data/SKSE/Plugins/CalamityAffixes.dll`
  - `sha256sum skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll Data/SKSE/Plugins/CalamityAffixes.dll`
- 데이터/생성기:
  - `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
  - `python3 tools/lint_affixes.py --spec affixes/affixes.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json`

결과:
- [예: all pass]

## 테스트 범위 (인게임)

- 체크리스트 기준:
  - `docs/releases/2026-02-09-ingame-qa-smoke-checklist.md`
- (옵션) 로그 시그니처 체크:
  - `python3 tools/qa_skse_logcheck.py --log "<CalamityAffixes.log 경로>"`
- 실행 환경:
  - [런타임/모드셋/세이브 조건]
- 결과:
  - A Loot-time: PASS/FAIL
  - B Tooltip: PASS/FAIL
  - C Runeword: PASS/FAIL
  - D Suppression/ICD: PASS/FAIL
  - E DotApply: PASS/FAIL
  - F Scope: PASS/FAIL
  - G Reforge Orb: PASS/FAIL (오브 획득/소모/무어픽스 재련/완성 룬워드 차단)

## 알려진 이슈 / 다음 단계

- [Known issue 1]
- [Known issue 2]
- [Next step]

## 다운로드 / 문서 링크

- 릴리즈:
  - [GitHub release URL]
- 배포/게시 가이드:
  - `docs/releases/2026-02-09-nexus-upload-playbook.md`
