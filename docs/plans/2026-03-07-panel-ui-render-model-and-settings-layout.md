# Panel UI Render Model And Settings Layout Plan

Date: 2026-03-07
Status: completed

## Goal

칼라미티 패널 UI에서 남아 있던 두 개의 큰 구조 부채를 정리한다.

1. `PrismaTooltip.SettingsLayout.inl`의 설정 파싱, persistence, JS push 책임을 더 작은 helper로 분리한다.
2. `index.html`의 주요 render 함수에서 계산 로직과 DOM 반영 경계를 더 분명하게 만든다.

이번 차수는 기능/UX 의미를 바꾸지 않고, 다음 리팩터에서 각 책임을 더 쉽게 분리할 수 있게 만드는 것이 목표다.

## Non-Goals

- 새 UI 기능 추가
- 외부 JS/CSS 파일 분리
- MCM/PrismaUI 계약 변경
- 패널 시각 디자인 변경

## Affected Files

- Modify: `Data/PrismaUI/views/CalamityAffixes/index.html`
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.cpp`
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.SettingsLayout.inl`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_common.h`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_main.cpp`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_runeword_policy.cpp`

## Milestones

### 1. Settings layout extraction

- MCM settings path resolution helper 추출
- panel hotkey / UI language parser helper 추출
- layout load/push helper 추출

### 2. Render view-model helpers

- inventory list view-model helper 추출
- recipe list filter/view-model helper 추출
- runeword panel action state helper 추출

### 3. Regression guard

- settings layout extraction policy 테스트 추가
- render helper extraction policy 테스트 추가 또는 기존 Prisma panel UI 테스트 보강

## Validation

1. Lint command
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- Risk: render helper 추출 과정에서 버튼/힌트 문구 계산이 달라질 수 있다.
- Risk: settings helper 추출 과정에서 MCM fallback 경로가 바뀌면 초기값 동기화가 어긋날 수 있다.
- Rollback: helper를 제거하고 기존 inline 계산/파싱 흐름으로 되돌리면 된다.
