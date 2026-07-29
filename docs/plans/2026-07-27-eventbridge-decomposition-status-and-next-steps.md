# EventBridge 분해: 현황 감사와 다음 단계

Date: 2026-07-27
Updated: 2026-07-29
Status: in-progress
Supersedes-sequencing-of: [2026-03-06-eventbridge-state-ownership-extraction-design.md](2026-03-06-eventbridge-state-ownership-extraction-design.md)

## 2026-07-29 실행 결과

- Step 1 완료: `EventBridge.h`의 상태 참조 별칭 14개를 제거하고 모든 호출부가
  `AffixRuntimeCacheState` 또는 `InstanceTrackingState` 소유 객체를 직접 사용한다.
- Step 2 완료: 활성 트리거 캐시 재구축과 읽기 선택을 `AffixRuntimeCacheState` 내부로 옮겼다.
  `EventBridge`는 const 결과만 전달한다.
- Phase 2의 serialize/deserialize 책임 이동과 이후 facade 축소는 아직 남아 있다.

## 이 문서의 위치

2026-03-06 설계는 `EventBridge`를 facade 로 축소하는 4단계 로드맵을 제시했고 `Status: proposed`
상태로 남아 있다. 그 사이 Phase 1 은 실제로 실행되었지만 문서에는 반영되지 않았고, 그 결과
"어디까지 됐는지"를 코드를 직접 세어보지 않으면 알 수 없는 상태다.

이 문서는 **새 로드맵이 아니다.** 기존 로드맵에 대해 두 가지를 한다.

1. 2026-07-27 기준 실측 달성도를 기록한다.
2. P0/P1 결함 수정에서 얻은 근거로 **Phase 2~4 의 순서를 바꾼다.**

목표(facade 축소)와 비목표(세이브 포맷 변경, 트리거 규칙 재설계, 락 분리 선행 금지)는 기존
설계를 그대로 승계한다.

## 실측 현황 (2026-07-27)

측정 대상: `skse/CalamityAffixes/src/EventBridge.*.cpp` 64개 파일, 16,245줄.

### 규모

| 항목 | 값 |
| --- | --- |
| `EventBridge.h` | 188줄 |
| `EventBridge.PrivateApi.inl` | 650줄 / 235 선언 |
| `EventBridge.PublicApi.inl` | 135줄 / 40 선언 |
| `EventBridge::` 멤버 함수 정의 | 258개 |
| `BSTEventSink` 다중 상속 | 8개 |
| `_stateMutex` 획득 지점 | 35곳 / 19파일 |

### 계열별 분포

| 계열 | 파일 | 함수 정의 |
| --- | ---: | ---: |
| Loot (Runeword 제외) | 7 | 56 |
| Triggers | 13 | 54 |
| Actions | 9 | 46 |
| Config | 17 | 45 |
| Loot.Runeword | 12 | 41 |
| Serialization | 3 | 15 |
| Traps | 1 | 1 |

## 기존 계획 대비 달성도

### Phase 1 (상태 객체 생성) — 완료

9개 상태 구조체가 존재한다.

| 구조체 | 정의 위치 |
| --- | --- |
| `CombatRuntimeState` | `CombatRuntimeState.h:43` |
| `LootRuntimeState` | `LootRuntimeState.h:19` |
| `AffixRegistryState` | `AffixRegistryState.h:12` |
| `AffixSpecialActionState` | `AffixSpecialActionState.h:8` |
| `TrapRuntimeState` | `detail/EventBridge.Types.inl:250` |
| `RuntimeSettingsState` | `detail/EventBridge.Types.inl:284` |
| `RunewordRuntimeState` | `detail/EventBridge.Types.inl:384` |
| `AffixRuntimeCacheState` | `detail/EventBridge.StateGroups.inl:5` |
| `InstanceTrackingState` | `detail/EventBridge.StateGroups.inl:18` |

### Phase 1 이 실제로는 절반만 끝난 지점

구조체를 만드는 것과 호출부가 그 구조체를 통해 접근하는 것은 다르다. 후자를 측정하면
같은 Phase 1 안에서도 결과가 둘로 갈린다.

(아래 표의 기준: `src/EventBridge.*.cpp` 64개 파일. Step 1 의 338곳은 `include/` 를 포함한
전체 기준이므로 수치가 다르다.)

| 상태 그룹 | 소유 객체 직접 접근 | 별칭(alias) 경유 접근 |
| --- | ---: | ---: |
| `_lootState` | 110회 | — |
| `_combatState` | 95회 | — |
| `_affixRuntimeState` | **0회** | `_affixes` 101 + `_activeCounts` 37 + 트리거 인덱스 캐시 |
| `_instanceTrackingState` | **0회** | `_instanceAffixes` 31 + `_instanceStates` 17 |

`EventBridge.h:129-151` 에 남아 있는 참조 멤버 14개가 원인이다.

```cpp
std::vector<AffixRuntime>& _affixes{ _affixRuntimeState.affixes };
std::unordered_map<std::uint64_t, InstanceAffixSlots>& _instanceAffixes{ _instanceTrackingState.instanceAffixes };
```

이 별칭은 추출 당시 diff 를 작게 유지하려는 호환 shim 이었고, 그래서 **호출부가 한 줄도 바뀌지
않았다.** `AffixRuntimeCacheState` 와 `InstanceTrackingState` 는 헤더에 이름만 존재하고 소유권은
이전되지 않은 상태다. 반면 `CombatRuntimeState`/`LootRuntimeState` 는 별칭 없이 호출부까지
이전을 마쳤다.

**따라서 Phase 1 의 잔여 작업은 "구조체를 더 만드는 것"이 아니라 "별칭 14개를 제거하는 것"이다.**

### Phase 2 (reset/sanitize/serialize 책임 이동) — 부분 완료

- 완료: `CombatRuntimeState::ResetTransientState()` / `::Reset()`,
  `LootRuntimeState::ResetForConfigReload()` / `::ResetForLoadOrRevert()`.
  `EventBridge.Serialization.Lifecycle.cpp` 의 필드 나열 초기화는 **0건**으로 목표를 달성했다.
- 미완료: serialize/deserialize 책임. `Serialization.Lifecycle.cpp` 는 여전히 457줄이고
  `_stateMutex` 획득 지점 35곳 중 9곳이 이 파일 하나에 몰려 있다.

### Phase 3 (읽기 API 를 서비스 경계로) — 미착수

### Phase 4 (facade 축소) — 미착수

## 기존 계획이 예측하지 못한 것

2026-03-06 설계는 분해의 근거를 "ownership clarity"로 잡았다. 그 진단은 맞지만, 그 뒤에 실제로
발생한 결함들은 그 축으로 정렬되지 않는다.

P1-1 에서 고친 use-after-free 가 대표적이다
([TriggerDispatchSnapshot.h](../../skse/CalamityAffixes/include/CalamityAffixes/TriggerDispatchSnapshot.h)).

- `ProcessTrigger` 가 `_activeHitTriggerAffixIndices` 를 **가리키는 포인터**로 range-for 를 돌았다.
- 루프 본문이 `CastSpellImmediate` 로 엔진에 재진입한다.
- 엔진이 동기적으로 `TESEquipEvent` 를 올리고, 그 sink 가 `RebuildActiveCounts()` →
  `RebuildActiveTriggerIndexCaches()` 로 **순회 중인 바로 그 벡터**를 `clear()/reserve()/push_back()` 한다.
- `reserve()` 가 재할당하면 순회는 해제된 메모리를 읽는다.

여기서 중요한 것은 이 결함이 나온 벡터가 하필 `AffixRuntimeCacheState` — 즉 **별칭으로만 접근되어
소유권 이전이 일어나지 않은 그룹**이라는 점이다. 소유자가 명확했다면 "누가 이 벡터를 재구축할
수 있는가"가 타입 수준에서 드러났을 것이다.

두 번째 교정 사항은 락에 대한 것이다. 기존 설계는 "락 분리를 나중으로 미룬다"고 했고 그 결론은
유지되지만, 근거는 바뀌어야 한다.

- `_stateMutex` 는 `recursive_mutex` 다 (`EventBridge.h:175`).
- TrapSystem/PrismaTooltip 의 `std::jthread` 워커는 `SKSE::GetTaskInterface()->AddTask` 로만
  넘기므로 모든 획득이 **동일 스레드**에서 일어난다.
- 즉 이 락은 재진입 변경을 **막지 않는다.** 교착만 막는다.

**따라서 이 코드베이스에서 "락 범위를 줄인다"는 그 자체로는 안전성을 개선하지 않는다.** 실제
위험은 락 경합이 아니라 재진입 무효화이고, 그 대응은 락 조정이 아니라 소유권 분리와 스냅샷이다.

## 수정된 순서

우선순위 기준을 "코드량 감축"에서 **"재진입 무효화 표면 제거"**로 바꾼다.

### Step 1. 별칭 14개 제거 — `AffixRuntimeCacheState` / `InstanceTrackingState`

가장 먼저 하는 이유: 방금 터진 결함 클래스가 정확히 이 두 그룹에서 나왔고, 작업이 기계적이라
위험이 낮다.

- `EventBridge.h:129-151` 의 참조 멤버 14개를 삭제한다.
- 호출부 338곳(`src/` + `include/`, 선언 제외)을 `_affixRuntimeState.affixes` /
  `_instanceTrackingState.instanceAffixes` 형태로 바꾼다.
- 순수 치환이므로 동작 변화가 없어야 한다. 빌드가 검증 수단이다.
- 규모 때문에 한 커밋에 몰지 말고 상태 그룹 단위로 둘로 나눈다
  (`AffixRuntimeCacheState` 9개 / `InstanceTrackingState` 5개).

성공 기준: `_affixRuntimeState`/`_instanceTrackingState` 직접 접근이 0회에서 338회로 바뀌고,
참조 별칭 멤버가 0개가 된다.

### Step 2. 재구축 진입점을 타입으로 좁힌다

Step 1 로 소유자가 드러나면, "누가 캐시를 재구축하는가"를 명시할 수 있다.

- `RebuildActiveTriggerIndexCaches()` 를 `AffixRuntimeCacheState` 의 메서드로 옮긴다.
- 순회용 접근은 const 참조 또는 스냅샷만 노출한다.
- 목표는 **재진입 중 재구축이 컴파일 단계에서 눈에 띄게 만드는 것**이지, 금지하는 것이 아니다.

성공 기준: 캐시를 mutate 하는 함수가 `AffixRuntimeCacheState` 안에만 존재한다.
runtime gate 에 "순회 중 재구축이 스냅샷을 무효화하지 않는다"는 동작 테스트를 추가한다
(P1-1 에서 만든 `SnapshotTriggerIndices` 게이트를 이 축으로 확장).

### Step 3. Serialization 책임 이전 (기존 Phase 2 잔여분)

`Serialization.Lifecycle.cpp` 457줄, 락 획득 9곳이 남은 최대 단일 집중 지점이다.

- 각 state 객체에 `Serialize(SKSE::SerializationInterface*)` / `Deserialize(...)` 를 둔다.
- `EventBridge` 는 호출 순서와 버전 협상만 담당한다.
- **세이브 포맷은 바꾸지 않는다.** 바이트 단위 동일성이 이 단계의 유일한 합격 조건이다.

성공 기준: 기존 세이브 로드 후 상태가 동일. 포맷 회귀를 잡을 게이트가 선행 조건이다 —
없다면 이 단계는 착수하지 않는다.

### Step 4. Runeword 서브시스템 분리 (기존 Phase 3/4 의 첫 실물)

12파일 / 41함수 / `_runewordState` 참조 181회로 결합이 가장 자기완결적이다. 다른 계열에서
`_runewordState` 를 만지는 곳은 Triggers 3회, Config 3회, Serialization 18회뿐이다
(Serialization 은 Step 3 에서 이미 정리된다).

- `RunewordService` 로 분리하고 `EventBridge` 는 위임만 한다.
- 외부 API 시그니처는 유지한다 (기존 설계의 API Compatibility Rule 승계 — Hooks/PrismaTooltip/
  runeword UI/runtime gate 가 모두 여기 기대고 있다).

### 하지 않는 것

- **락 분리.** 위에서 밝힌 대로 동일 스레드 직렬화 구조에서는 이득이 없고 교착 표면만 넓힌다.
- **8개 sink 다중 상속 해체.** 결함 근거가 아직 없다.
- **한 번에 하는 대수술.** 기존 설계의 Option 3 기각 근거가 그대로 유효하다.

## 검증

각 단계에서 기존 설계의 검증 순서를 그대로 따른다. 다만 실행되지 않는 게이트가 있었던 전례
(`a527744` 에서 복구) 때문에 **게이트가 실제로 실행되는지 먼저 확인**하고 넘어간다.

1. runtime gate 에 해당 단계의 구조 요구사항 추가 — 추가 후 **일부러 깨뜨려 실패를 확인**한다.
2. `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
3. `ctest --test-dir skse/CalamityAffixes/tests/build --no-tests=error --output-on-failure`
4. `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
5. `python3 -m unittest discover -s tools/tests -p 'test_*.py'`

### 검증할 수 없는 것

Step 2~4 의 실제 효과(재진입 시 크래시 부재)는 스카이림 실행 없이 확인할 수 없다. 정적
검증으로 증명되는 것은 "순회 대상이 재구축 대상과 분리되었다"까지이며, 인게임 확인이 남는다.
이 한계는 P0-2(함정 셀 detach), P1-1(트리거 인덱스 스냅샷), P1-2(함정 틱 스냅샷)와 동일하다.
