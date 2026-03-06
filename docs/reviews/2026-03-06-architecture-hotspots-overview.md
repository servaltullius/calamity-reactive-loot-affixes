# Calamity 아키텍처 개요와 Hotspot 정리

기준 시점: 2026-03-06

이 문서는 "새 기여자가 지금 구조를 빠르게 이해하는 것"과 "다음 리팩터 우선순위를 정하는 것"에 초점을 둔 축약판이다.

## 1. 한 줄 요약

Calamity는 단순 SKSE DLL 저장소가 아니라 `모듈형 affix 데이터 -> compose/lint -> .NET Generator -> ESP/INI/runtime JSON -> SKSE 런타임 -> Prisma UI`까지 한 번에 관리하는 운영형 모노레포다.

## 2. 현재 구조

### 2.1 데이터 작성 계층

- 작성 원본은 [`affixes/affixes.modules.json`](../../affixes/affixes.modules.json) 과 [`affixes/modules/`](../../affixes/modules/) 아래 모듈들이다.
- [`tools/compose_affixes.py`](../../tools/compose_affixes.py) 가 모듈을 합쳐 [`affixes/affixes.json`](../../affixes/affixes.json) 을 만든다.
- 룬워드 레시피와 룬 가중치는 별도 SSOT인 [`affixes/runeword.contract.json`](../../affixes/runeword.contract.json) 이 담당한다.

### 2.2 생성기 계층

- [`tools/CalamityAffixes.Generator/Program.cs`](../../tools/CalamityAffixes.Generator/Program.cs) 가 `affixes/affixes.json` 을 입력으로 받는다.
- [`tools/CalamityAffixes.Generator/Writers/GeneratorRunner.cs`](../../tools/CalamityAffixes.Generator/Writers/GeneratorRunner.cs) 가 아래 산출물을 생성한다.
- `Data/CalamityAffixes.esp`
- `Data/CalamityAffixes_KID.ini`
- `Data/CalamityAffixes_DISTR.ini`
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json`
- `Data/SKSE/Plugins/InventoryInjector/CalamityAffixes.json`

### 2.3 런타임 계층

- [`skse/CalamityAffixes/src/main.cpp`](../../skse/CalamityAffixes/src/main.cpp) 는 매우 얇다.
- 실제 초기화는 `LoadConfig -> Register -> Hooks 설치 -> TrapSystem 설치 -> PrismaTooltip 설치` 순서로 연결된다.
- 게임 로직의 중심은 [`skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`](../../skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h) 이다.

### 2.4 UI/브리지 계층

- [`skse/CalamityAffixes/src/PrismaTooltip.cpp`](../../skse/CalamityAffixes/src/PrismaTooltip.cpp) 가 Prisma overlay, 패널 상태, 선택 아이템 툴팁, 패널 언어/핫키 복원을 담당한다.
- [`skse/CalamityAffixes/src/Papyrus.cpp`](../../skse/CalamityAffixes/src/Papyrus.cpp) 는 Papyrus -> SKSE ModCallbackEvent 브리지의 최소 진입점만 유지한다.

## 3. 핵심 데이터 흐름

```text
affixes/modules/*.json
  -> compose_affixes.py
  -> affixes/affixes.json
  -> CalamityAffixes.Generator
  -> ESP + KID/SPID + runtime JSON + runtime_contract
  -> SKSE EventBridge::LoadConfig()
  -> trigger/loot/runeword/action/serialization state 구성
  -> PrismaTooltip / Papyrus / Hooks 경로에서 소비
```

## 4. 현재 아키텍처의 장점

- 작성 데이터와 배포 산출물이 분리돼 있어 생성 재현성이 높다.
- `validation_contract`, `runtime_contract`, `runeword contract`를 분리해 Python/.NET/C++ 드리프트를 강하게 감시한다.
- 전투, 룻, 룬워드, UI, 직렬화가 파일 단위로는 꽤 잘 나뉘어 있다.
- 세이브 안정성, dropped ref prune, user settings debounce 같은 운영 이슈를 설계의 일부로 다룬다.
- 테스트가 "순수 로직"과 "구조 계약"을 각각 다른 방식으로 잡는다.

## 5. 현재 아키텍처의 약점

- 상태 소유권이 `EventBridge` 한 클래스에 과도하게 집중돼 있다.
- [`skse/CalamityAffixes/src/Hooks.cpp`](../../skse/CalamityAffixes/src/Hooks.cpp) 가 단순 훅 설치를 넘어서 damage 보정, deferred cast, stale-hit 방어까지 들고 있어 interception layer 치고 무겁다.
- [`skse/CalamityAffixes/src/PrismaTooltip.cpp`](../../skse/CalamityAffixes/src/PrismaTooltip.cpp) 와 `PrismaTooltip.*.inl` 묶음이 UI worker, 상태 캐시, 명령 디스패치까지 동시에 다룬다.
- 런타임 테스트는 여전히 "실게임 통합 회귀"보다 "소스/계약 회귀"에 더 강하다.

## 6. Hotspot 5개

### Hotspot 1. EventBridge 상태 집중

- 파일: [`skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`](../../skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h)
- 문제: affix registry, loot pools, runeword state, combat runtime, cooldown store, serialization state, UI 연계 상태가 한 클래스에 몰려 있다.
- 영향: 변경 파급 범위가 넓고, 락 경계와 수명 경계를 이해하기 어렵다.

### Hotspot 2. HealthDamage 훅 경로

- 파일: [`skse/CalamityAffixes/src/Hooks.cpp`](../../skse/CalamityAffixes/src/Hooks.cpp)
- 문제: vfunc 훅 설치, pointer sanitize, stale-hit 억제, damage 조정, deferred task까지 한 흐름에 몰려 있다.
- 영향: 모드 충돌, 디버깅 난도, 실제 전투 회귀 분석 비용이 커진다.

### Hotspot 3. Trigger 처리 파이프라인

- 파일: [`skse/CalamityAffixes/src/EventBridge.Triggers.cpp`](../../skse/CalamityAffixes/src/EventBridge.Triggers.cpp)
- 문제: `ProcessTrigger()` 가 lucky hit, low-health, per-target ICD, proc budget, active trigger cache, runtime state 업데이트를 함께 처리한다.
- 영향: 새 trigger 조건 추가 시 회귀 범위가 넓다.

### Hotspot 4. Loot assign 경로

- 파일: [`skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp`](../../skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp)
- 문제: 드랍 통화, pity, 이름 정리, affix roll, eligible 판정, 인스턴스 기록이 한 파일에 모여 있다.
- 영향: loot 정책 수정이 UX, 밸런스, 저장 상태까지 동시에 흔든다.

### Hotspot 5. Prisma UI 상태기계

- 파일: [`skse/CalamityAffixes/src/PrismaTooltip.cpp`](../../skse/CalamityAffixes/src/PrismaTooltip.cpp)
- 관련: `PrismaTooltip.PanelData.inl`, `PrismaTooltip.HandleUiCommand.inl`, `PrismaTooltip.Tick.inl`
- 문제: polling worker, selected item snapshot, panel command, layout persistence가 강결합돼 있다.
- 영향: UI 지연과 런타임 락 경합을 코드만 보고 추적하기 어렵다.

## 7. 우선 개선 순서

### 1순위. EventBridge를 상태 소유 단위로 먼저 분리

- 추천 분리 단위:
- `AffixRegistryState`
- `CombatRuntimeState`
- `LootRuntimeState`
- `RunewordRuntimeState`
- `SerializationState`
- 목표: EventBridge는 "오케스트레이터 + facade"만 남기고, 실상태는 하위 상태 객체가 소유하게 만든다.

### 2순위. Hooks를 "엔진 훅"과 "게임플레이 후처리"로 나누기

- `hook install / original call / pointer safety`
- `damage adjustment / stale-hit heuristics / deferred cast`
- 목표: 훅 충돌 문제와 게임플레이 회귀 문제를 별도 층으로 디버깅 가능하게 만든다.

### 3순위. Trigger 정책 계산을 순수 함수 묶음으로 추출

- `ResolveTriggerProcCooldownMs`
- lucky hit gate
- low-health rearm
- duplicate/stale suppression
- 목표: 실게임 없이도 검증 가능한 범위를 넓힌다.

### 4순위. Loot assign을 정책 계산과 적용 단계로 나누기

- `eligible / roll / pity / currency`
- `name mutation / instance mutation / ledger commit`
- 목표: 밸런스 변경과 게임 객체 mutation을 분리한다.

### 5순위. Prisma polling 경로를 계측 가능한 구조로 정리

- 추천 방향:
- worker tick reason 로깅
- panel open 시 task enqueue 빈도 계측
- selected item snapshot cache hit/miss 계측
- 목표: "UI가 무겁다"를 감으로 보지 않고 근거로 보게 만든다.

## 8. 새 기여자용 읽기 순서

1. [`README.md`](../../README.md)
2. [`docs/design/개발명세서.md`](../design/개발명세서.md)
3. [`docs/adr/0003-trigger-pipeline-healthdamage-hook-hit-event-dot-apply.md`](../adr/0003-trigger-pipeline-healthdamage-hook-hit-event-dot-apply.md)
4. [`tools/compose_affixes.py`](../../tools/compose_affixes.py)
5. [`tools/CalamityAffixes.Generator/Writers/GeneratorRunner.cs`](../../tools/CalamityAffixes.Generator/Writers/GeneratorRunner.cs)
6. [`skse/CalamityAffixes/src/main.cpp`](../../skse/CalamityAffixes/src/main.cpp)
7. [`skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`](../../skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h)
8. [`skse/CalamityAffixes/src/Hooks.cpp`](../../skse/CalamityAffixes/src/Hooks.cpp)
9. [`skse/CalamityAffixes/src/EventBridge.Triggers.cpp`](../../skse/CalamityAffixes/src/EventBridge.Triggers.cpp)
10. [`skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp`](../../skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp)

## 9. 이번 기준선 검증

아래 명령 기준으로 현재 저장소 상태를 확인했다.

```bash
python3 tools/compose_affixes.py --check
python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json
dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release
cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes
ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure
python3 tools/verify_runtime_contract_sync.py
```

결과 요약:

- compose check 통과
- lint 통과
- Generator.Tests 71/71 통과
- SKSE target 최신 상태 확인
- ctest 3/3 통과
- runtime_contract sync OK

## 10. 다음 문서 후보

- `EventBridge` 상태 분리 설계 초안
- `Hooks` 책임 분리 설계 초안
- `PrismaTooltip` worker 계측 계획
