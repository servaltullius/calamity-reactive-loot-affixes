# EventBridge State Ownership Extraction Design

Date: 2026-03-06
Status: proposed

## Goal

`EventBridge`를 한 번에 해체하지 않고, 현재 동작과 세이브 포맷을 유지한 채 상태 소유권을 단계적으로 분리한다.

최종 목표는 아래 두 가지다.

1. `EventBridge`를 "게임 이벤트를 받아 각 서브시스템에 위임하는 facade"로 축소한다.
2. 실제 상태는 도메인별 state/service 객체가 소유하게 만들어 변경 파급 범위를 줄인다.

## Why Now

이미 아래와 같은 작은 추출은 진행되었다.

- `AffixRegistryState`
- `AffixSpecialActionState`
- config load pipeline wrapper
- runeword UI contract extraction

하지만 아직도 [`EventBridge.h`](../../skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h) 는 다음을 직접 소유한다.

- combat dedup / cooldown / low-health / recent evidence 상태
- loot preview / loot evaluated / currency ledger / shuffle bag 상태
- runeword recipe 진행 / selected base / rune fragment 상태
- instance affix slots / instance runtime state / passive spell 적용 상태
- dropped ref delete queue / reroll guard / trap runtime

즉, "config-derived state 일부를 분리한 상태"이지, "runtime ownership을 분리한 상태"는 아니다.

## Non-Goals

이번 설계의 비목표는 아래와 같다.

- 세이브 포맷 변경
- trigger/action 규칙 재설계
- runeword UX 변경
- Prisma UI 구조 전면 개편
- Hooks 동작 의미 변경

이번 설계는 책임과 소유권을 재배치하는 것이지, 게임 디자인을 바꾸는 것이 아니다.

## Problems In Current Shape

### 1. 상태 경계가 도메인 경계와 다르다

파일은 `Config/Loot/Triggers/Actions/Serialization`로 나뉘어 있지만, 실제 멤버 필드는 여전히 `EventBridge`가 다 갖고 있다.

결과:

- "어느 상태가 누구 것인가?"를 header만 봐서는 알기 어렵다.
- 작은 변경도 `EventBridge.h`를 넓게 흔든다.

### 2. 락 경계가 너무 넓다

현재는 `_stateMutex` 하나가 큰 범위를 보호한다.

이 구조는 초기 안정화에는 유리하지만, 아래 비용이 있다.

- UI polling과 gameplay state가 같은 락 영역에서 만날 수 있다.
- serialization / loot / runeword / combat가 논리적으로는 다른데도 같은 경쟁 구간에 묶인다.

### 3. 테스트 가능한 단위가 작아지지 않는다

지금도 helper 수준의 pure logic 테스트는 좋다. 하지만 도메인 상태가 `EventBridge` 안에 엉켜 있어 "상태 객체 단위" 테스트를 늘리기 어렵다.

## Options Considered

### Option 1. EventBridge를 유지하고 멤버만 조금씩 더 정리

장점:

- 가장 안전하다
- 현재 작업 방식과 잘 맞는다

단점:

- 구조 개선 속도가 너무 느리다
- ownership 문제는 거의 남는다

### Option 2. 상태 객체를 먼저 분리하고 EventBridge는 facade로 축소

장점:

- 동작 변경 없이도 구조가 눈에 띄게 좋아진다
- 테스트 단위가 자연스럽게 생긴다
- 이후 Hooks/Loot/UI 분리에 좋은 기반이 된다

단점:

- 중간 단계에서 어댑터 코드가 잠시 늘어난다
- 멤버/메서드 이동 순서를 잘못 잡으면 diff가 커질 수 있다

### Option 3. EventBridge를 여러 서비스로 한 번에 대수술

장점:

- 장기 구조는 가장 깔끔하다

단점:

- save/runtime/hook/UI 경계가 한 번에 흔들린다
- 현재 저장소의 회귀 방지 방식으로는 리스크가 크다

## Decision

Option 2를 선택한다.

즉:

- 먼저 "상태 소유권"을 분리한다.
- 그 다음에 "행동 오케스트레이션"을 분리한다.
- `EventBridge`는 마지막에 facade로 얇게 남긴다.

## Target Shape

### Layer 1. Entry / facade

- `main.cpp`
- `EventBridge`

역할:

- SKSE 이벤트 수신
- lifecycle 진입점
- 각 도메인 서비스 호출

### Layer 2. Domain state owners

- `CombatRuntimeState`
- `LootRuntimeState`
- `RunewordRuntimeState`
- `SerializationRuntimeState`
- `UiRuntimeState`

역할:

- 자기 도메인의 mutable state만 소유
- serialization/reset/sanitize helper 제공

### Layer 3. Domain services

- `ConfigLoader`
- `TriggerProcessor`
- `LootService`
- `RunewordService`
- `SerializationService`
- `PrismaUiBridge`

역할:

- 상태 객체를 입력으로 받아 규칙을 실행
- `EventBridge` 대신 도메인 API를 노출

## Proposed Ownership Split

### 1. CombatRuntimeState

소유 대상:

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

이유:

- 모두 trigger/combat evidence와 강하게 연결된다.
- save-state와 config-derived state가 아니라 "전투 중 mutable runtime state"다.

### 2. LootRuntimeState

소유 대상:

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

이유:

- loot evaluation, reroll guard, drop ledger, pity 모두 "획득/드랍/재평가" 도메인에 속한다.

### 3. RunewordRuntimeState

소유 대상:

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

이유:

- runeword는 이미 UI/recipe/crafting/transmute 흐름이 독립 도메인처럼 움직인다.
- 이후 `RunewordService` 추출의 기반이 된다.

### 4. SerializationRuntimeState

소유 대상:

- `_instanceAffixes`
- `_instanceStates`
- `_equippedInstanceKeysByToken`
- `_equippedTokenCacheReady`
- `_appliedPassiveSpells`
- `_miscCurrencyMigrated`
- `_miscCurrencyRecovered`

이유:

- instance-affix 영속성과 복원 일관성의 핵심 상태다.
- revert/load/save/reset 경계가 명확하다.

주의:

- `instanceAffixes`는 loot/runeword/combat/UI 모두에서 읽는다.
- 그래서 첫 단계에서는 "소유만 이동"하고 접근은 accessor로 유지하는 편이 안전하다.

### 5. UiRuntimeState

후보 범위:

- 현재 C++ 쪽의 runeword panel snapshot 관련 캐시
- Prisma와 직접 결합된 selected item snapshot 캐시

주의:

- 이 단계는 마지막으로 미룬다.
- 이유는 현재 UI 상태 상당수가 `PrismaTooltip.cpp`의 네임스페이스 static으로 존재해, `EventBridge`만 분리해도 구조 이득이 제한적이기 때문이다.

## Rollout Order

### Phase 1. 상태 객체만 먼저 만든다

순서:

1. `CombatRuntimeState`
2. `LootRuntimeState`
3. `RunewordRuntimeState`
4. `SerializationRuntimeState`

규칙:

- public data bag로 시작해도 된다
- 동작은 바꾸지 않는다
- `EventBridge`는 멤버를 하위 state 객체로 감싸기만 한다

이 단계의 성공 기준:

- `EventBridge.h` 멤버 수가 눈에 띄게 줄어든다
- reset/revert/load/save가 state 객체 단위로 읽힌다

### Phase 2. reset / sanitize / serialize 책임을 객체별 메서드로 이동

예시:

- `combat.ResetTransientState()`
- `loot.ResetEphemeralState()`
- `runeword.ResetSelectionState()`
- `serialization.ClearLoadedState()`

이 단계의 성공 기준:

- `EventBridge.Serialization.*`
- `EventBridge.Config.Reset.cpp`
- `EventBridge.Serialization.Lifecycle.cpp`

에서 "필드 나열 초기화"가 줄고 의도 단위 호출이 늘어난다.

### Phase 3. 읽기 API를 서비스 경계로 바꾼다

예시:

- `GetInstanceAffixesForKey()`
- `GetRunewordPanelState()`
- `TryBeginLootCurrencyLedgerRoll()`
- `ResolveActiveTriggerIndices()`

이 단계의 성공 기준:

- 외부 코드가 raw field를 직접 만지지 않는다.
- 도메인 규칙이 accessor/service로 이동한다.

### Phase 4. EventBridge를 facade로 줄인다

마지막 단계에서 `EventBridge`는 아래 역할만 남긴다.

- event sink 구현
- service 호출
- cross-domain transaction orchestration

## Locking Strategy

초기 단계에서는 락 분리까지 욕심내지 않는다.

먼저:

- `_stateMutex` 유지
- ownership만 분리

그 다음:

- 실제 contention 근거가 생긴 도메인만 락 분리

이 순서를 택하는 이유:

- 지금은 lock granularity보다 ownership clarity가 더 큰 문제다.
- 락을 먼저 쪼개면 deadlock surface만 넓어질 수 있다.

## API Compatibility Rule

기존 `EventBridge` 메서드 시그니처는 가능한 한 유지한다.

이유:

- `Hooks`
- `PrismaTooltip`
- runeword UI
- runtime gate tests

가 모두 `EventBridge` API에 기대고 있기 때문이다.

즉, 외부에서 볼 때는 그대로 두고, 내부 구현만 새 state owner를 향하게 한다.

## Validation Plan

각 phase에서 아래 순서를 유지한다.

1. runtime gate에 새 구조 요구사항 추가
2. 필요한 경우 static/pure helper 테스트 추가
3. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
4. `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
5. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`

이 설계는 C++ 런타임 리팩터지만, generator/runtime contract 회귀도 같이 본다.

## Initial Task Breakdown

### Task A. CombatRuntimeState extraction

가장 먼저 하는 이유:

- 전투 상태가 가장 응집도가 높다.
- `Hooks`/`Triggers` 회귀 범위를 분리해서 읽기 쉬워진다.

### Task B. LootRuntimeState extraction

두 번째인 이유:

- `Loot.Assign`와 `Serialization.Lifecycle`의 공용 상태를 하나로 묶을 수 있다.
- dropped ref delete queue도 함께 ownership이 정리된다.

### Task C. RunewordRuntimeState extraction

세 번째인 이유:

- UI/recipe/crafting/transmute가 이미 독립된 흐름이다.
- runeword는 개념적으로 분리하기 쉽다.

### Task D. SerializationRuntimeState extraction

마지막인 이유:

- 읽는 곳이 가장 넓다.
- `instanceAffixes` 접근 경계를 먼저 정리한 뒤 옮기는 편이 안전하다.

## Risks

### Risk 1. 상태는 옮겼는데 규칙은 그대로 섞여 있는 상태로 남을 수 있다

대응:

- phase 1 완료 후 바로 phase 2로 이어간다.
- 단순 "data bag 이동"으로 멈추지 않는다.

### Risk 2. serialization/load 경로에서 ownership 이전 중 누락이 날 수 있다

대응:

- `Revert`, `Load`, `Save`, `OnPostLoadGame`, `OnFormDelete`를 한 묶음으로 검토한다.
- 필드 단위 체크리스트를 먼저 만든 뒤 이동한다.

### Risk 3. UI와 runeword가 예상보다 더 깊게 얽혀 있을 수 있다

대응:

- runeword runtime state와 Prisma UI state를 같은 단계에 묶지 않는다.
- 먼저 runeword state만 분리하고 UI state는 뒤로 미룬다.

## Success Criteria

- `EventBridge.h`가 "모든 상태 보관소"처럼 보이지 않는다.
- reset/load/save/revert 경로가 도메인 이름으로 읽힌다.
- `Hooks`, `Triggers`, `Loot`, `Runeword` 변경 시 건드리는 필드 범위가 줄어든다.
- 기존 검증 명령이 모두 그대로 통과한다.

## Recommended Next Document

이 문서 다음에는 바로 아래 순서로 내려가는 것이 좋다.

1. `CombatRuntimeState extraction design`
2. `LootRuntimeState extraction design`
3. `RunewordRuntimeState extraction design`
4. `SerializationRuntimeState extraction design`
