# Prisma Tooltip Lifecycle Refactor Plan

Date: 2026-03-07
Status: completed

## Goal

`PrismaTooltip.cpp`에서 view lifecycle, menu/input sink, worker scheduling 책임을 분리해서 코어 파일의 변경 반경을 줄인다.

이번 목표는 동작을 바꾸는 것이 아니라, 이미 존재하는 `.inl` 분리 패턴에 맞춰 lifecycle 축을 추가로 추출하는 것이다.

## Non-Goals

- Prisma UI 동작 의미 변경
- tooltip 내용/레이아웃/핫키 정책 변경
- runeword panel UX 변경
- 새로운 의존성 추가

## Affected Files

- Modify: `skse/CalamityAffixes/src/PrismaTooltip.cpp`
- Add: `skse/CalamityAffixes/src/PrismaTooltip.MenuInput.inl`
- Add: `skse/CalamityAffixes/src/PrismaTooltip.ViewLifecycle.inl`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_common.h`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_main.cpp`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_runeword_policy.cpp`

## Constraints

- 현재 public API (`Install`, `IsAvailable`, `SetControlPanelOpen`, `ToggleControlPanel`)는 유지한다.
- 기존 Prisma 회귀 테스트 의미를 유지한다.
- worker/menu sink 동작은 바꾸지 않고 소스 위치만 옮긴다.

## Milestones

### 1. Lifecycle 분리

- `EnsurePrismaApiAvailable`, `CreatePrismaView`, `EnsurePrisma`, `RegisterPrismaEventSinks`, `StartPrismaWorker`를 lifecycle inl로 이동

### 2. Menu/Input 분리

- hotkey/menu helper와 event sink class를 menu/input inl로 이동

### 3. Regression Guard

- 기존 Prisma worker policy 테스트가 새 inl 구성을 읽도록 갱신
- lifecycle extraction policy 테스트 추가

## Validation

1. Lint command
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- Risk: Prisma 관련 회귀 테스트가 소스 파일 결합 방식에 의존하므로, 새 inl 파일을 테스트 로더에 반영하지 않으면 false negative가 발생할 수 있다.
- Rollback: 새 inl include를 제거하고 정의를 `PrismaTooltip.cpp`로 되돌리면 된다.
