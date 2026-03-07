# Panel UI UX Improvements Plan

Date: 2026-03-07
Status: completed

## Goal

칼라미티 패널 UI/UX에서 현재 체감이 큰 구조 문제를 우선순위대로 정리한다.

이번 차수의 목표는 다음 네 가지다.

1. 키보드/패드 조작 시 포커스 위치가 분명히 보이도록 한다.
2. Runeword 메인 탭을 "베이스 선택 -> 레시피 선택 -> 변환" 흐름으로 더 쉽게 읽히게 만든다.
3. Advanced 탭에서 일반 설정과 Debug 도구를 시각적으로 분리한다.
4. 패널 전체의 시각 계층과 CTA 강조를 보강해 작업 창처럼 느껴지게 만든다.

## Non-Goals

- 룬워드 기능 의미 변경
- 새 Prisma interop API 추가
- Figma 기반 재디자인
- UI 언어 기본값(`both`) 변경

## Affected Files

- Modify: `Data/PrismaUI/views/CalamityAffixes/index.html`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_common.h`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_main.cpp`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_runeword_policy.cpp`

## Milestones

### 1. Focus visibility

- 버튼, 탭, 리스트, 검색 입력에 `:focus-visible` 스타일 추가
- 패널 open 시 현재 탭 포커스 보강

### 2. Runeword flow hierarchy

- 진행 단계(progress strip) 추가
- Step card 스타일 추가
- 베이스/레시피/변환 순서가 보이도록 마크업 재배치
- 상태 렌더에서 현재 단계 및 다음 행동 힌트 반영

### 3. Advanced / Debug separation

- Debug 도구를 접힌 경고 영역으로 이동
- 일반 설정 영역과 시각적으로 분리

### 4. Regression guard

- UX 구조 회귀 테스트 추가
- focus-visible, progress strip, debug details, primary CTA marker 검증

## Validation

1. Lint command
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- Risk: Runeword 탭 마크업 재배치로 인해 렌더 함수가 기대한 DOM 구조를 놓칠 수 있다.
- Risk: focus-visible 스타일이 과하면 오버레이 미관이 어수선해질 수 있다.
- Rollback: 기존 Runeword 배치와 Advanced flat section 구조로 되돌리면 된다.
