# CombatRuntimeState Extraction Design

Date: 2026-03-06
Status: proposed

## Context

`EventBridge` 안에는 trigger 처리와 직접 연결된 전투용 mutable state가 많이 남아 있다.

대표적으로 아래가 해당된다.

- per-target cooldown
- non-hostile first-hit gate
- recent hit evidence
- low-health rearm/consumed state
- health-damage hook 관측 상태
- DotApply cooldown 및 안전 경고 상태

이 상태들은 save-state도 아니고 config-derived state도 아니다. 전부 "전투 중 생성되고 소멸하는 runtime state"다.

## Decision

이 상태들을 `CombatRuntimeState` 하나로 먼저 묶는다.

첫 단계에서는:

- 상태 소유권만 이동
- 기존 `EventBridge` public API 유지
- 기존 `_stateMutex` 유지
- 동작 변경 없음

즉, `EventBridge`는 직접 필드를 들고 있지 않고 `_combatState`를 통해 접근하게 만든다.

## Ownership Scope

### 포함

- `_perTargetCooldownStore`
- `_nonHostileFirstHitGate`
- `_combatRuntime`
- `_lastHitAt`
- `_lastHit`
- `_lastPapyrusHitEventAt`
- `_lastPapyrusHit`
- `_lowHealthTriggerConsumed`
- `_lowHealthLastObservedPct`
- `_procDepth`
- `_healthDamageHookSeen`
- `_healthDamageHookLastAt`
- `_dotCooldowns`
- `_dotCooldownsLastPruneAt`
- `_dotObservedMagicEffects`
- `_dotTagSafetyWarned`
- `_dotObservedMagicEffectsCapWarned`
- `_dotTagSafetySuppressed`

### 제외

- `_affixes`, `_activeCounts`, `_active*TriggerAffixIndices`
- loot shuffle bag / preview / ledger 상태
- runeword recipe/selection/runtime state
- instance affix serialization state

제외 이유는 간단하다. 이번 문서는 "전투 중 변화하는 evidence/cooldown/gate 상태"만 분리하는 단계이기 때문이다.

## Why This First

- 응집도가 가장 높다.
- `Hooks.cpp` 와 `EventBridge.Triggers*.cpp` 경계가 더 잘 보인다.
- save/load 형식에 직접 손대지 않아 리스크가 상대적으로 낮다.
- 추후 `TriggerProcessor` 분리의 기반이 된다.

## Proposed Shape

```cpp
struct CombatRuntimeState
{
    PerTargetCooldownStore perTargetCooldownStore{};
    NonHostileFirstHitGate nonHostileFirstHitGate{};
    CombatRuntimeStateData combatRuntime{};

    std::chrono::steady_clock::time_point lastHitAt{};
    LastHitKey lastHit{};
    std::chrono::steady_clock::time_point lastPapyrusHitEventAt{};
    LastHitKey lastPapyrusHit{};

    std::unordered_map<LowHealthTriggerKey, bool, LowHealthTriggerKeyHash> lowHealthTriggerConsumed;
    std::unordered_map<RE::FormID, float> lowHealthLastObservedPct;

    std::atomic<std::uint32_t> procDepth{ 0 };
    bool healthDamageHookSeen{ false };
    std::chrono::steady_clock::time_point healthDamageHookLastAt{};

    std::unordered_map<std::uint64_t, std::chrono::steady_clock::time_point> dotCooldowns;
    std::chrono::steady_clock::time_point dotCooldownsLastPruneAt{};
    std::unordered_set<RE::FormID> dotObservedMagicEffects;
    bool dotTagSafetyWarned{ false };
    bool dotObservedMagicEffectsCapWarned{ false };
    bool dotTagSafetySuppressed{ false };

    void ResetTransientState();
};
```

이 단계에서 중요한 점은 "메서드를 많이 넣지 않는다"는 것이다. 먼저 데이터 bag + reset helper 수준으로 시작하고, 다음 단계에서 메서드를 올린다.

## File Plan

### 새 파일

- `skse/CalamityAffixes/include/CalamityAffixes/CombatRuntimeState.h`

### 수정 파일

- `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- `skse/CalamityAffixes/src/EventBridge.Config.Reset.cpp`
- `skse/CalamityAffixes/src/EventBridge.Triggers.cpp`
- `skse/CalamityAffixes/src/EventBridge.Triggers.HealthDamage.cpp`
- `skse/CalamityAffixes/src/Hooks.cpp`
- 관련 static/runtime gate test 파일

## Migration Steps

### Step 1. 필드 이동

- `EventBridge.h` 에서 combat 관련 필드를 제거
- `_combatState` 멤버 하나로 교체

### Step 2. reset 경로 이동

- `Revert()`
- `ResetRuntimeStateForConfigReload()`

에서 combat 관련 필드 개별 초기화를 지우고 `ResetTransientState()` 호출로 바꾼다.

### Step 3. 접근 경로 치환

예시:

- `_perTargetCooldownStore` -> `_combatState.perTargetCooldownStore`
- `_lastPapyrusHit` -> `_combatState.lastPapyrusHit`
- `_dotCooldowns` -> `_combatState.dotCooldowns`

### Step 4. 후속 pure helper 승격

필드 이동이 끝나면 아래 메서드를 `CombatRuntimeState` 쪽으로 점진 이동할 수 있다.

- low-health gate 관련 helper
- DotApply pruning helper
- recent hit evidence helper

이 단계는 extraction 직후 필수는 아니다.

## Compatibility Rule

기존 외부 호출자는 `EventBridge`를 그대로 사용한다.

예:

- `Hooks`
- `PrismaTooltip`
- trigger dispatch

즉, 이번 작업은 내부 ownership 이동이지, 외부 API 재설계가 아니다.

## Validation

1. runtime gate에 `CombatRuntimeState.h` 존재성과 `EventBridge` 위임 구조를 요구하는 체크 추가
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
4. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`

Generator 테스트를 같이 돌리는 이유는 C++만 바꿔도 runtime contract/검증 루틴이 예상치 않게 깨지는지 함께 보려는 것이다.

## Risks

### Risk 1. atomic 포함 상태를 값 복사하려는 실수

대응:

- copy/move 의미를 명확히 하지 않으면 기본 생성만 허용한다.
- state 객체는 `EventBridge` 멤버로만 둔다.

### Risk 2. reset 경로 누락

대응:

- `Revert` 와 `ResetRuntimeStateForConfigReload` 를 같은 체크리스트로 본다.
- hook clear 경로까지 같이 읽는다.

### Risk 3. Hooks 쪽 코드 가독성 일시 악화

대응:

- 이번 단계에서는 치환만 하고, 후속 단계에서 `TriggerProcessor`/combat helper 분리로 다시 정리한다.

## Success Criteria

- `EventBridge.h` 에서 전투용 ephemeral 상태가 대폭 줄어든다.
- reset/revert 코드가 "combat state reset"이라는 의도 단위로 읽힌다.
- `Hooks` 와 `Triggers`가 어떤 상태를 공유하는지 한눈에 보인다.
