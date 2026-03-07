# Panel UI Batching And Command Delegation Plan

Date: 2026-03-07
Status: completed

## Goal

칼라미티 패널 UI의 setter가 직접 DOM render를 연쇄 호출하던 경로를 `requestAnimationFrame` 기반 batching으로 정리하고, 정적/동적 버튼 클릭 처리를 command delegation으로 통합한다.

이번 차수에서는 패널 UX 의미를 바꾸지 않고, 같은 상태 변경이 짧은 시간에 여러 번 들어올 때 DOM 갱신이 더 예측 가능하도록 만드는 것이 핵심 목표다.

## Non-Goals

- 패널 레이아웃/시각 디자인 변경
- runeword/tooltip 기능 의미 변경
- 외부 JS 파일 분리
- Prisma UI API 버전 변경

## Affected Files

- Modify: `Data/PrismaUI/views/CalamityAffixes/index.html`
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.cpp`
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.PanelData.inl`
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.SettingsLayout.inl`
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.ViewLifecycle.inl`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_runeword_policy.cpp`

## Milestones

### 1. Render batching

- UI dirty section 상수화
- `schedulePanelRender` / `flushPanelRender` 도입
- interop setter가 직접 render 대신 dirty mark를 사용하도록 변경

### 2. Command delegation

- 정적 버튼을 root-level delegation으로 처리
- inventory/recipe 동적 버튼도 `data-cmd` 기반으로 통합
- preview invalidation 규칙 helper 추출

### 3. Interop contract constants

- JS interop method 이름 상수화
- C++ Prisma interop method 이름 상수화

### 4. Regression guard

- panel batching/delegation/interop constant 구조 회귀 테스트 추가 또는 보강

## Validation

1. Lint command
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- Risk: batching 순서가 기존 즉시 render 순서를 바꾸면 패널 텍스트/툴팁이 한 프레임 늦게 보일 수 있다.
- Risk: command delegation이 잘못 연결되면 동적 리스트 클릭이 무반응이 될 수 있다.
- Rollback: setter direct render 호출과 개별 button listener 방식으로 되돌리면 된다.
