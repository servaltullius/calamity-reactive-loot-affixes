# Loot Sanitize Structure Follow-up Plan

Date: 2026-03-07
Status: completed

## Goal

`EventBridge.Loot.Assign.cpp`의 tracked loot sanitize 경로를 한 단계 더 분해해서, per-instance slot sanitize 함수가 토큰 허용 판정, 동일성 비교, 상태 반영 책임을 한 번에 들고 있지 않도록 정리한다.

같은 차수에서 최근 리팩터로 흐트러진 테스트/선언 파일의 형식 일관성도 함께 복구한다.

## Non-Goals

- loot rule 의미 변경
- affix slot sanitize 알고리즘 변경
- 새로운 loot/runtime policy 추가

## Affected Files

- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp`
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/detail/EventBridge.PrivateApi.inl`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_common.h`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_main.cpp`
- Modify: `skse/CalamityAffixes/tests/runtime_gate_store_checks_loot_policy.cpp`

## Milestones

### 1. Loot sanitize helper extraction

- 토큰 허용 판정 helper 추출
- slot equality helper 추출
- sanitized slot 적용 helper 추출

### 2. Regression guard

- helper extraction이 유지되는지 검사하는 loot policy 테스트 추가

### 3. Formatting cleanup

- 최근 touched 파일들의 들여쓰기/정렬 일관성 복구

## Validation

1. Lint command
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`

## Risks / Rollback

- Risk: string-based policy tests가 들여쓰기/문구에 과민할 수 있다.
- Rollback: helper 선언/정의를 원래 함수로 되돌리고 새 policy 검사만 제거하면 된다.
