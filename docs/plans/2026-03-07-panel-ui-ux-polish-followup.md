# Panel UI UX Polish Follow-up Plan

Date: 2026-03-07
Status: completed

## Goal

직전 UX 구조 개선 위에 실제 사용감 다듬기를 더한다.

이번 후속 차수의 목표는 다음 세 가지다.

1. Runeword 단계 카드와 진행 표시가 상태 변화 시 더 분명하게 강조되도록 한다.
2. 검색/선택 관련 empty state를 설명형 문구로 정교화한다.
3. Affix 탭을 컨텍스트 카드 + 읽기 쉬운 tooltip 영역으로 정리해 정보 밀도를 낮춘다.

## Non-Goals

- 새 Prisma interop API 추가
- 룬워드/어픽스 기능 의미 변경
- UI 언어 기본값 변경
- 외부 JS/CSS 파일 분리

## Affected Files

- Modify: `Data/PrismaUI/views/CalamityAffixes/index.html`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_common.h`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_main.cpp`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_runeword_policy.cpp`

## Milestones

### 1. Step emphasis polish

- 단계 전환 시 animation class 부여
- active/complete step 시각 강조 보강
- reduced-motion 환경에서 과한 애니메이션 방지

### 2. Empty state copy

- inventory base list empty state 정교화
- recipe search empty state 정교화
- affix tab empty state를 안내형 카드로 전환

### 3. Affix tab density

- 선택 아이템 컨텍스트 카드 추가
- affix 상세 영역을 compact typography로 정리
- 읽기 전 안내 문구를 상단 lead 텍스트로 이동

### 4. Regression guard

- 새 UX polish marker를 회귀 테스트에 추가

## Validation

1. Lint command
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- Risk: 새 empty state helper가 기존 list render와 충돌하면 일부 안내 영역이 중복될 수 있다.
- Risk: step state animation이 과하면 오버레이에서 시각 노이즈가 생길 수 있다.
- Rollback: helper와 affix overview block을 제거하고 기존 단순 text layout으로 되돌리면 된다.
