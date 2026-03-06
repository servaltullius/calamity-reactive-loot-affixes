# RunewordRuntimeState Extraction Design

Date: 2026-03-06
Status: proposed

## Context

runeword는 이미 개념적으로 독립 도메인에 가깝다.

- recipe catalog
- rune fragment inventory
- selected base
- current recipe cursor
- transmute in-progress guard
- per-instance runeword state

하지만 ownership은 아직 `EventBridge`에 붙어 있어, UI/crafting/transmute/panel state가 모두 monolith field set을 같이 바라본다.

## Decision

runeword 관련 상태를 `RunewordRuntimeState`로 묶는다.

목표는 runeword를 당장 서비스로 빼는 것이 아니라, "상태는 runeword 것"이라는 사실을 먼저 코드에 반영하는 것이다.

## Ownership Scope

### 포함

- `_runewordRecipes`
- `_runewordRecipeIndexByToken`
- `_runewordRecipeIndexByResultAffixToken`
- `_runewordRuneNameByToken`
- `_runewordRuneTokenPool`
- `_runewordRuneTokenWeights`
- `_runewordRuneFragments`
- `_runewordInstanceStates`
- `_runewordSelectedBaseKey`
- `_runewordBaseCycleCursor`
- `_runewordRecipeCycleCursor`
- `_runewordTransmuteInProgress`

### 제외

- runeword UI DTO 계약
- canonical `instanceAffixes`
- Prisma static cache

이유:

- DTO는 이미 `RunewordUiContracts`로 분리됐다.
- canonical instance storage는 아직 serialization ownership이 더 강하다.
- Prisma 캐시는 UI ownership이 더 강하다.

## Why This Third

- recipe/catalog/crafting/transmute 흐름이 이미 파일 단위로 잘 나뉘어 있다.
- UI contract 추출이 먼저 끝난 상태라 다음 단계가 자연스럽다.
- combat/loot ownership이 정리된 뒤에 들어가면 `instanceAffixes`와의 경계도 더 읽기 쉬워진다.

## Proposed Shape

```cpp
struct RunewordRuntimeState
{
    std::vector<RunewordRecipe> recipes;
    std::unordered_map<std::uint64_t, std::size_t> recipeIndexByToken;
    std::unordered_map<std::uint64_t, std::size_t> recipeIndexByResultAffixToken;
    std::unordered_map<std::uint64_t, std::string> runeNameByToken;
    std::vector<std::uint64_t> runeTokenPool;
    std::vector<double> runeTokenWeights;

    std::unordered_map<std::uint64_t, std::uint32_t> runeFragments;
    std::unordered_map<std::uint64_t, RunewordInstanceState> instanceStates;

    std::optional<std::uint64_t> selectedBaseKey{};
    std::uint32_t baseCycleCursor{ 0 };
    std::uint32_t recipeCycleCursor{ 0 };
    bool transmuteInProgress{ false };

    void ResetSelectionAndProgress();
};
```

## File Plan

### 새 파일

- `skse/CalamityAffixes/include/CalamityAffixes/RunewordRuntimeState.h`

### 수정 파일

- `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- `skse/CalamityAffixes/src/EventBridge.Config.RunewordSynthesis*.cpp`
- `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.*.cpp`
- `skse/CalamityAffixes/src/EventBridge.Serialization.Save.cpp`
- `skse/CalamityAffixes/src/EventBridge.Serialization.Load.cpp`
- `skse/CalamityAffixes/src/EventBridge.Serialization.Lifecycle.cpp`

## Migration Steps

### Step 1. catalog/runtime 필드 이동

recipe catalog와 rune fragment/runtime progress 상태를 `_runewordState` 아래로 옮긴다.

### Step 2. selection/reset helper 이동

아래 helper를 state 메서드 후보로 본다.

- selection reset
- cursor reset
- transmute guard reset

### Step 3. access boundary 고정

`GetCurrentRunewordRecipe`, `ResolveCompletedRunewordRecipe`, `SanitizeRunewordState` 같은 메서드는 우선 `EventBridge`에 남겨도 된다. 대신 내부 데이터 접근은 `_runewordState`로 통일한다.

### Step 4. 후속 service 분리 준비

그 다음에야 아래를 별도 `RunewordService` 후보로 본다.

- catalog lookup
- crafting/transmute apply
- recipe entry building
- panel state assembly

## Special Note: UI Coupling

runeword는 UI와 강하게 연결돼 있다. 하지만 이번 단계에서 UI까지 같이 빼면 diff가 커진다.

그래서 원칙은 아래와 같다.

- state ownership만 먼저 분리
- UI DTO는 이미 분리된 계약을 계속 사용
- Prisma command path는 기존 `EventBridge` 메서드를 그대로 호출

즉, runeword state extraction은 "UI 분리"가 아니라 "UI가 읽는 state의 주인을 명확히 하는 작업"이다.

## Validation

1. runtime gate에 `RunewordRuntimeState.h` 존재성과 `_runewordState` 경계를 요구
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
4. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`

## Risks

### Risk 1. recipe lookup 경로가 여러 파일에 넓게 흩어져 있어 치환 누락 가능성

대응:

- `rg "_runeword"` 기반 체크리스트를 먼저 만들고 치환한다.

### Risk 2. selection state와 instance storage의 ownership이 혼동될 수 있음

대응:

- `selectedBaseKey`와 `instanceStates`는 runeword ownership
- `instanceAffixes`는 serialization ownership

이 경계를 문서/주석으로 명확히 남긴다.

### Risk 3. panel state builder가 runtime state와 UI formatting을 같이 쥐고 있음

대응:

- 이번 단계에서는 panel formatting 메서드는 유지
- 후속 `RunewordService` 또는 `RunewordPanelAssembler` 분리 때 다룬다

## Success Criteria

- runeword 관련 mutable state가 `EventBridge`에서 한 덩어리로 빠진다.
- recipe/crafting/transmute 관련 파일들이 같은 ownership을 공유한다.
- 이후 runeword service 분리 시 field migration이 아니라 behavior migration만 남는다.
