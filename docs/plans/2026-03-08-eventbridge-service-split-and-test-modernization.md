# 2026-03-08 EventBridge Service Split And Test Modernization

## Goal

- Reduce `EventBridge::Load()` and `PrismaTooltip` command handling into clearer domain modules
- Add at least one behavior-based runtime test for extracted helper logic
- Keep existing runtime-gate extraction checks green while moving logic to new files

## Non-goals

- Full `EventBridge` class decomposition into separately constructed services
- New save format or record version changes
- Large UI feature changes

## Work order

1. Split serialization load record readers and post-load finalization into dedicated helper modules.
2. Add deterministic, behavior-testable serialization load state helpers.
3. Migrate Prisma tooltip command routing behind a controller object.
4. Update runtime-gate policies and stateful tests to cover the new module boundaries.

## Validation

- `python3 tools/ensure_skse_build.py --lane plugin --lane runtime-gate`
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel`
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Implemented

- `EventBridge::Load()` orchestration만 남기고 record별 읽기와 post-load 정리를 `EventBridge.Serialization.Load.Records.inl`로 분리
- `SerializationLoadState.h`에 shuffle bag resolve, evaluated/currency recent rebuild, shuffle bag sanitize 헬퍼 추가
- runtime-gate 상태 테스트에 새 헬퍼의 동작 기반 검증 추가
- `PrismaTooltip.HandleUiCommand.inl`을 `PrismaCommandController` 기반으로 재구성하고 기존 엔트리포인트는 얇은 wrapper로 유지
- trigger dispatch 본체와 전투 런타임 보조를 `EventBridge.Triggers.cpp` / `EventBridge.Triggers.Runtime.cpp`로 분리
- low-health trigger snapshot 구성을 `LowHealthTriggerSnapshot.h` 헬퍼로 추출하고 동작 기반 테스트를 추가
- `PrismaTooltip.ViewState.inl`에 view state/telemetry controller를 도입해 core 파일의 UI 상태 정리 책임을 추가 분리
- runeword selection/runtime-effect 판정을 `EventBridge.Loot.Runeword.Selection.cpp` helper로 공통화해 base/recipe/panel/catalog 흐름에서 재사용
- health-damage routing의 stale-window 억제를 `TriggerGuards.h` public helper 기반으로 검증하도록 보강
- `Runeword Transmute`의 fragment consume/rollback 절차를 `EventBridge.Loot.Runeword.Transmute.Fragments.cpp` helper로 분리
- `Reforge` regular-slot reroll 비교/재시도/보존 토큰 재삽입 절차를 `LootRollSelection.h` pure helper로 승격
- runtime-gate에 fake health-damage 이벤트 시퀀스 검증을 추가해 helper 조합 동작을 직접 확인
- 문자열 기반 정책 테스트를 새 파일 배치와 controller 구조를 기준으로 보정
