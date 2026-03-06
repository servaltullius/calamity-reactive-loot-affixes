# SerializationRuntimeState Extraction Design

Date: 2026-03-06
Status: proposed

## Context

canonical instance-affix 저장소는 현재 여러 도메인이 공용으로 읽는 핵심 상태다.

대표적으로 아래가 있다.

- `_instanceAffixes`
- `_instanceStates`
- `_equippedInstanceKeysByToken`
- `_equippedTokenCacheReady`
- `_appliedPassiveSpells`
- migration flag

이 상태는 loot/runeword/combat/UI가 모두 참조하지만, 본질적으로는 "저장/복원/인스턴스 영속성" 도메인에 더 가깝다.

## Decision

이 상태를 `SerializationRuntimeState`로 묶는다.

단, 이 단계는 가장 마지막에 수행한다.

이유:

- 읽는 경로가 가장 넓다.
- 잘못 움직이면 거의 모든 도메인에 diff가 난다.

따라서 앞선 `Combat/Loot/Runeword` ownership 정리가 끝난 뒤, 마지막에 canonical storage ownership을 옮긴다.

## Ownership Scope

### 포함

- `_instanceAffixes`
- `_instanceStates`
- `_equippedInstanceKeysByToken`
- `_equippedTokenCacheReady`
- `_appliedPassiveSpells`
- `_miscCurrencyMigrated`
- `_miscCurrencyRecovered`

### 제외

- loot preview/evaluated/cache
- runeword recipe/runtime selection 상태
- combat evidence/cooldown 상태

## Why Last

- `instanceAffixes`는 거의 모든 도메인이 읽는다.
- 이걸 먼저 옮기면 단순 ownership 추출이 아니라 전체 API 변경처럼 번질 수 있다.
- 반대로 앞의 세 문서를 먼저 끝내면, 마지막에는 "누가 canonical storage를 읽는가"가 훨씬 선명해진다.

## Proposed Shape

```cpp
struct SerializationRuntimeState
{
    std::unordered_map<std::uint64_t, InstanceAffixSlots> instanceAffixes;
    std::unordered_map<InstanceStateKey, InstanceRuntimeState, InstanceStateKeyHash> instanceStates;

    std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> equippedInstanceKeysByToken;
    bool equippedTokenCacheReady{ false };

    std::unordered_set<RE::SpellItem*> appliedPassiveSpells;

    bool miscCurrencyMigrated{ false };
    bool miscCurrencyRecovered{ false };

    void ClearLoadedState();
};
```

## File Plan

### 새 파일

- `skse/CalamityAffixes/include/CalamityAffixes/SerializationRuntimeState.h`

### 수정 파일

- `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- `skse/CalamityAffixes/src/EventBridge.Serialization.Save.cpp`
- `skse/CalamityAffixes/src/EventBridge.Serialization.Load.cpp`
- `skse/CalamityAffixes/src/EventBridge.Serialization.Lifecycle.cpp`
- `skse/CalamityAffixes/src/EventBridge.Triggers.ActiveCounts.cpp`
- `skse/CalamityAffixes/src/EventBridge.Loot*.cpp`
- `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.*.cpp`

## Migration Steps

### Step 1. canonical storage ownership 이동

`EventBridge` 직접 멤버를 `_serializationState`로 치환한다.

### Step 2. lifecycle helper 이동

우선 아래 의미를 state 메서드로 올린다.

- clear loaded state
- passive spell clear
- equipped token cache clear

### Step 3. accessor 우선 전략

직접 필드 접근을 당장 없애기보다, 먼저 accessor를 만든다.

예시:

- `FindInstanceAffixes()`
- `FindInstanceRuntimeState()`
- `EraseInstanceRuntimeStates()`
- `ClearEquippedTokenCache()`

이유:

- 호출자가 매우 많아서 한 번에 raw field 제거를 시도하면 diff가 커진다.

### Step 4. save/load 코드 정리

`Save/Load/Revert`는 여전히 `EventBridge` entry point로 남겨도 된다. 대신 내부에서는 `_serializationState`를 통해 읽고 쓰게 한다.

## Boundary Rule

이 state는 "canonical instance storage"의 owner다.

즉:

- loot preview는 owner가 아니다
- runeword selected base는 owner가 아니다
- combat active trigger cache도 owner가 아니다

이 문서의 핵심은 "읽는 곳이 많다"와 "주인이 누구냐"를 분리해 생각하는 것이다.

## Validation

1. runtime gate에 `SerializationRuntimeState.h` 요구 추가
2. save/load/revert 경로가 `_serializationState`를 통해 읽는지 구조 검사 추가
3. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
5. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`

## Risks

### Risk 1. read path가 넓어 치환 누락 가능성 가장 큼

대응:

- `rg "_instanceAffixes|_instanceStates|_appliedPassiveSpells|_equippedInstanceKeysByToken"`로 전체 호출 지도를 먼저 만든다.

### Risk 2. passive spell ownership이 serialization에 들어가는 것이 어색할 수 있음

대응:

- 현재는 save/load/revert와 함께 정리되는 경향이 더 강하므로 여기 둔다.
- 나중에 상태 분리가 더 진행되면 `PassiveEffectRuntimeState`로 분리할 여지는 남긴다.

### Risk 3. migration flag와 canonical storage가 같은 객체에 있는 것이 과할 수 있음

대응:

- 지금은 둘 다 persistence lifecycle에 종속되므로 같이 둔다.
- 별도 가치가 생기면 `MigrationState`로 분리한다.

## Success Criteria

- `instanceAffixes`와 관련 lifecycle이 한 ownership 아래 정리된다.
- `Save/Load/Revert`가 더 명확하게 읽힌다.
- 다른 도메인은 canonical storage의 "소유자"가 아니라 "사용자"로 보이게 된다.
