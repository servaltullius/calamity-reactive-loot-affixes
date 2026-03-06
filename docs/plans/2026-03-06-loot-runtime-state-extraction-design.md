# LootRuntimeState Extraction Design

Date: 2026-03-06
Status: proposed

## Context

loot 관련 mutable state는 현재 여러 파일로 흩어진 것처럼 보이지만 실제 소유는 모두 `EventBridge` 안에 있다.

특히 아래가 함께 움직인다.

- shuffle bag
- loot preview cache
- evaluated instance cache
- currency ledger
- pity streak
- reroll guard
- dropped ref delete queue

이 상태들은 공통적으로 "획득/드랍/재평가/중복 방지" 도메인에 속한다.

## Decision

loot 관련 runtime state를 `LootRuntimeState`로 분리한다.

이번 단계의 목표는 정책 코드를 옮기는 것이 아니라, 정책이 기대는 상태의 ownership을 한곳으로 모으는 것이다.

## Ownership Scope

### 포함

- `_lootPrefixSharedBag`
- `_lootPrefixWeaponBag`
- `_lootPrefixArmorBag`
- `_lootSuffixSharedBag`
- `_lootSuffixWeaponBag`
- `_lootSuffixArmorBag`
- `_lootPreviewAffixes`
- `_lootPreviewRecent`
- `_lootEvaluatedInstances`
- `_lootEvaluatedRecent`
- `_lootEvaluatedInsertionsSincePrune`
- `_lootCurrencyRollLedger`
- `_lootCurrencyRollLedgerRecent`
- `_lootChanceEligibleFailStreak`
- `_runewordFragmentFailStreak`
- `_reforgeOrbFailStreak`
- `_activeSlotPenalty`
- `_lootRerollGuard`
- `_pendingDroppedRefDeletes`
- `_dropDeleteDrainScheduled`
- `_playerContainerStash`

### 제외

- `_loot` config 값 자체
- `instanceAffixes`
- runeword recipe/selection/runtime state
- trap instance vector

제외 이유:

- config는 `LootConfig`로 별도 의미가 있다.
- `instanceAffixes`는 serialization ownership이 더 강하다.
- trap은 action/runtime 쪽 성격이 강하다.

## Why This Second

- combat state보다 넓지만 여전히 경계가 비교적 선명하다.
- dropped ref prune, reroll guard, loot ledger가 함께 묶여 ownership clarity가 크게 좋아진다.
- [`EventBridge.Loot.Assign.cpp`](../../skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp) 와 [`EventBridge.Serialization.Lifecycle.cpp`](../../skse/CalamityAffixes/src/EventBridge.Serialization.Lifecycle.cpp) 사이 공용 상태가 명확해진다.

## Proposed Shape

```cpp
struct LootRuntimeState
{
    LootShuffleBagState prefixSharedBag{};
    LootShuffleBagState prefixWeaponBag{};
    LootShuffleBagState prefixArmorBag{};
    LootShuffleBagState suffixSharedBag{};
    LootShuffleBagState suffixWeaponBag{};
    LootShuffleBagState suffixArmorBag{};

    std::unordered_map<std::uint64_t, InstanceAffixSlots> previewAffixes;
    std::deque<std::uint64_t> previewRecent;

    std::unordered_set<std::uint64_t> evaluatedInstances;
    std::deque<std::uint64_t> evaluatedRecent;
    std::size_t evaluatedInsertionsSincePrune{ 0 };

    std::unordered_map<std::uint64_t, std::uint32_t> currencyRollLedger;
    std::deque<std::uint64_t> currencyRollLedgerRecent;

    std::uint32_t lootChanceEligibleFailStreak{ 0 };
    std::uint32_t runewordFragmentFailStreak{ 0 };
    std::uint32_t reforgeOrbFailStreak{ 0 };

    std::vector<float> activeSlotPenalty;
    LootRerollGuard rerollGuard{};
    std::vector<LootRerollGuard::RefHandle> pendingDroppedRefDeletes;
    std::atomic_bool dropDeleteDrainScheduled{ false };
    std::map<std::pair<RE::FormID, RE::FormID>, std::int32_t> playerContainerStash;

    void ResetEphemeralState();
};
```

## File Plan

### 새 파일

- `skse/CalamityAffixes/include/CalamityAffixes/LootRuntimeState.h`

### 수정 파일

- `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- `skse/CalamityAffixes/src/EventBridge.Loot*.cpp`
- `skse/CalamityAffixes/src/EventBridge.Serialization.Save.cpp`
- `skse/CalamityAffixes/src/EventBridge.Serialization.Lifecycle.cpp`
- 관련 runtime gate 파일

## Migration Steps

### Step 1. 필드 이동

`EventBridge` 직접 멤버를 `_lootState` 하나로 교체한다.

### Step 2. reset 경로 정리

아래가 개별 필드 초기화 대신 `ResetEphemeralState()` 호출을 쓰게 만든다.

- `Revert()`
- `ResetRuntimeStateForConfigReload()`

### Step 3. lifecycle helper 정리

다음 helper는 `LootRuntimeState` 메서드로 점진 이동 후보가 된다.

- evaluated/prune helper
- ledger begin/finalize helper
- dropped ref delete queue helper

### Step 4. 후속 policy/service 분리 준비

이 단계가 끝나면 다음 분리가 쉬워진다.

- loot roll selection policy
- currency grant policy
- reroll guard lifecycle

## Special Note: Serialization Boundary

`LootRuntimeState`는 save 대상과 비-save 대상을 함께 갖는다.

예:

- `currencyRollLedger`, `shuffle bags`, `evaluatedInstances`는 save 대상
- `pendingDroppedRefDeletes`, `dropDeleteDrainScheduled`는 비-save 대상

그래서 reset helper를 둘로 나눌 여지가 있다.

- `ClearSavedLootState()`
- `ClearEphemeralLootState()`

하지만 첫 단계에서는 오버엔지니어링을 피하고 `ResetEphemeralState()` 하나만 두고, `Save/Load`는 기존 함수가 직접 필요한 필드만 다루게 두는 편이 안전하다.

## Validation

1. runtime gate에 `LootRuntimeState.h` 존재성 추가
2. dropped ref delete queue와 loot ledger가 `_lootState` 아래로 이동했는지 구조 체크 추가
3. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
5. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`

## Risks

### Risk 1. save 대상/비-save 대상 혼합으로 helper 의미가 흐려질 수 있다

대응:

- 구조체 주석으로 save 대상 필드와 ephemeral 필드를 구분한다.
- `Save/Load` 단계에서 무리하게 메서드화를 시도하지 않는다.

### Risk 2. `InstanceAffixSlots` dependency가 너무 일찍 끌려들 수 있다

대응:

- preview cache 정도만 `LootRuntimeState`에 유지하고, canonical instance storage는 `SerializationRuntimeState`로 남긴다.

### Risk 3. dropped ref prune 경로가 직렬화 ownership과 얽힐 수 있다

대응:

- prune 결정은 loot state가 하되, 실제 canonical instance erase는 serialization state accessor를 통해 하게 만드는 후속 단계를 염두에 둔다.

## Success Criteria

- loot 관련 mutable state가 `EventBridge` header에서 한 덩어리로 보인다.
- `Loot.Assign`, `Serialization.Lifecycle`, `Loot.Preview`가 공유하는 상태 범위가 명확해진다.
- 이후 `LootService` 분리 시 상태 ownership 때문에 막히지 않는다.
