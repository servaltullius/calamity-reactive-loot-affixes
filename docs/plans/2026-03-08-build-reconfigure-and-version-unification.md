# 2026-03-08 Build Reconfigure And Version Unification

## Goal

- 프로젝트 경로 이동 후에도 SKSE 빌드/CTest lane이 자동으로 현재 경로에 재configure되게 만든다.
- SKSE 플러그인 버전과 패키징/빌드 버전의 단일 진실원을 만든다.

## Non-Goals

- EventBridge/PrismaTooltip 구조 리팩터링
- 인게임 UX/밸런스 변경
- CI 전체 재설계

## Affected Files

- `tools/ensure_skse_build.py`
- `tools/build_mo2_zip.sh`
- `tools/release_verify.sh`
- `tools/release_build.py`
- `tools/tests/test_*.py`
- `skse/CalamityAffixes/CMakeLists.txt`
- `skse/CalamityAffixes/include/CalamityAffixes/Version.h.in`
- `skse/CalamityAffixes/src/main.cpp`
- `README.md`
- `AGENTS.md`

## Constraints

- 기존 `build.linux-clangcl-rel` cross lane을 유지한다.
- 경로에 공백이 있어도 동작해야 한다.
- stale cache는 감지 후 재configure하되, 사용자가 매번 수동 정리할 필요가 없게 한다.

## Milestones

1. stale CMake build/test tree 감지 및 재configure 헬퍼 추가
2. release/build/package 스크립트가 헬퍼를 선행 호출하도록 연결
3. CMake generated header로 SKSE plugin version 단일화
4. 관련 테스트/문서 갱신

## Validation

- `python3 -m unittest discover -s tools/tests -p 'test_*.py'`
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- `python3 tools/ensure_skse_build.py --lane plugin --lane runtime-gate`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- cross lane 재configure 인자가 부족하면 기존 WSL clang-cl 구성이 깨질 수 있다.
- 그 경우 `tools/ensure_skse_build.py` 호출만 되돌리고, 수동 configure 명령으로 복구 가능해야 한다.
