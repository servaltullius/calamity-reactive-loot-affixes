# Panel UI Structure Refactor Plan

Date: 2026-03-07
Status: completed

## Goal

칼라미티 패널 계층의 1차 구조 정리를 진행해서, HTML/JS bootstrap 경로와 C++ 패널 브리지의 핵심 책임을 더 작은 helper 단위로 분리한다.

이번 차수는 HTML/JS 파일을 외부 리소스로 쪼개는 단계가 아니라, 현재 구조 안에서 interop/bootstrap, selected-item panel data, UI command routing 경계를 읽기 쉽게 만드는 것이 목표다.

## Non-Goals

- Prisma panel UX 변경
- runeword/action command 의미 변경
- CSS 시각 리디자인
- 외부 JS/CSS 파일 추가

## Affected Files

- Modify: `Data/PrismaUI/views/CalamityAffixes/index.html`
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.cpp`
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.PanelData.inl`
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.HandleUiCommand.inl`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_common.h`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_main.cpp`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_runeword_policy.cpp`

## Milestones

### 1. Interop parsing 정리

- array/object payload parsing helper 추출
- control panel open-state parsing/apply helper 추출

### 2. Bootstrap 정리

- Prisma interop registration helper 추출
- window fallback registration helper 추출
- panel event wiring/bootstrap helper 추출

### 3. Panel data 정리

- selected-item menu scan helper 추출
- tooltip clear helper 추출

### 4. Command routing 정리

- structured command dispatcher helper 추출
- event-backed command feedback helper 추출

### 5. Regression guard

- 패널 bootstrap extraction policy 테스트 추가
- panel data extraction policy 테스트 추가
- command routing extraction policy 테스트 추가

## Validation

1. Lint command
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- Risk: HTML 내 함수 재배치 과정에서 이벤트 wiring 순서가 미묘하게 바뀌면 패널 초기 상태가 달라질 수 있다.
- Rollback: 새 helper를 제거하고 기존 inline bootstrap 블록으로 되돌리면 된다.
