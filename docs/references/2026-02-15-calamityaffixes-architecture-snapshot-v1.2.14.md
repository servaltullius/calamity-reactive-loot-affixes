# CalamityAffixes 코드 설계 분석 (v1.2.14 스냅샷)

기준 시점: 2026-02-15  
기준 커밋: `2dd7546` (`release: v1.2.14 review-followup hotfixes`)

---

## 1) 전체 구조 개요

CalamityAffixes는 4개 레이어로 분리된 구조입니다.

| 레이어 | 기술 | 역할 | 규모(LOC) |
|---|---|---|---|
| C++ SKSE 플러그인 | C++23, CommonLibSSE-NG | 핵심 런타임(롤링/트리거/액션/직렬화/UI 브리지) | 11,168 (`skse/CalamityAffixes/src/*.cpp`) |
| C# Generator | .NET 8 + Mutagen | ESP/INI/런타임 JSON 생성 | 1,021 (`tools/CalamityAffixes.Generator/*.cs`) |
| PrismaUI | HTML/CSS/JS (SPA) | 인게임 툴팁/컨트롤 패널 | 2,720 (`Data/PrismaUI/views/CalamityAffixes/index.html`) |
| Papyrus | Skyrim Papyrus | MCM/모드이벤트 브리지 | 7개 소스(665 LOC), 활성 컴파일 타깃 3개 |

> 참고: Papyrus는 소스 7개가 존재하지만, 현재 배포 빌드에서 `.pex` 생성 대상은 3개(`ModeControl`, `ModEventEmitter`, `MCMConfig`)입니다.  
> 근거: `tools/compile_papyrus.sh:95`

---

## 2) C++ 플러그인 아키텍처

### 2.1 핵심 컨트롤러

`EventBridge` 싱글톤이 7개 게임 이벤트를 구독하는 메인 오케스트레이터입니다.

```cpp
class EventBridge final :
  public RE::BSTEventSink<RE::TESHitEvent>,
  public RE::BSTEventSink<RE::TESDeathEvent>,
  public RE::BSTEventSink<RE::TESEquipEvent>,
  public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>,
  public RE::BSTEventSink<RE::TESContainerChangedEvent>,
  public RE::BSTEventSink<RE::TESUniqueIDChangeEvent>,
  public RE::BSTEventSink<SKSE::ModCallbackEvent>
```

근거: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h:32`

### 2.2 패턴 관점

- Singleton: 전역 상태/수명 관리 (`GetSingleton`)
- Observer: Skyrim 이벤트 sink 7종
- Strategy/Dispatcher: `ActionType` 기반 액션 분기
- Flyweight 성격: 어픽스 정의는 공유, 인스턴스는 토큰 슬롯만 저장
- Tokenization(Hashing): 문자열 ID를 64-bit 토큰으로 압축
- Bounded history/ring-like guard: 재롤/중복 히트/최근 평가 이력 방어

### 2.3 주요 상태 구조

- 인스턴스 키: `(FormID << 16) | UniqueID`
- 인스턴스 어픽스 맵:
  - `_instanceAffixes: unordered_map<uint64_t, InstanceAffixSlots>`
  - 슬롯 구조는 토큰 최대 3개 + count

### 2.4 서브시스템 분할 (src 기준 11,168 LOC)

| 도메인 | LOC | 비중 |
|---|---:|---:|
| Config | 2,082 | 18.6% |
| Loot | 3,142 | 28.1% |
| UI/Tooltip (`PrismaTooltip` + layout persistence) | 1,830 | 16.4% |
| Actions | 1,563 | 14.0% |
| Triggers | 1,295 | 11.6% |
| Serialization | 699 | 6.3% |
| Core/Entry (`main`, `Hooks`, `EventBridge`, `Papyrus`) | 358 | 3.2% |
| Traps (`EventBridge.Traps` + `TrapSystem`) | 199 | 1.8% |

---

## 3) 런타임 호출 흐름 (핵심)

### 3.1 아이템 획득 → 어픽스 부여

`TESContainerChangedEvent`  
→ `ProcessLootAcquired()`  
→ `ApplyMultipleAffixes()`  
→ `_instanceAffixes[key] = slots`  
→ `EnsureMultiAffixDisplayName()`

`v1.2.14` 기준 이름 마커는 UTF-8 별 대신 ASCII `*`, `**`, `***`를 사용합니다.  
근거: `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp:445`

### 3.2 장비 상태 갱신 → 패시브 활성

`TESEquipEvent`  
→ `RebuildActiveCounts()`  
→ 장착 인스턴스 어픽스 기준으로 활성 카운트/패시브/보너스 재구성

### 3.3 전투 → 트리거 프로세싱

- `HandleHealthDamage` 훅 경로 + `TESHitEvent` 폴백 경로 병행
- 중복 히트 억제, ICD/per-target ICD, proc depth guard 적용 후 액션 디스패치

`v1.2.14` 보정:
- `CalamityAffixes_Hit` ModEvent 발행도 적대 조건을 통과한 경우에만 수행
  - `skse/CalamityAffixes/src/EventBridge.Triggers.Events.cpp:160`
  - `skse/CalamityAffixes/include/CalamityAffixes/TriggerGuards.h:78`
- Papyrus 레거시 브리지 경로에도 적대성 가드 추가
  - `Data/Scripts/Source/CalamityAffixes_AffixManager.psc:251`
  - `Data/Scripts/Source/CalamityAffixes_AffixManager.psc:268`

---

## 4) 안전 장치 상수 (코드 기준)

| 상수 | 값 | 용도 |
|---|---:|---|
| `kDuplicateHitWindow` | 100ms | 동일 히트 중복 억제 |
| `kDotApplyICD` | 1500ms | DotApply 레이트리밋 |
| `kCastOnCritICD` | 150ms | CastOnCrit 최소 간격 |
| `kEquipResyncInterval` | 5000ms | 장비 상태 재동기화 주기 |
| `kPerTargetCooldownMaxEntries` | 8192 | per-target ICD 맵 상한 |
| `kLootEvaluatedRecentKeep` | 2048 | 최근 평가 인스턴스 유지량 |
| `kMaxTrackedRefs` | 512 | 드롭/재획득 재롤 방지 추적 버퍼 |
| `kDotCooldownMaxEntries` | 4096 | DotApply 관측 캐시 상한 |

근거: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h:386`, `skse/CalamityAffixes/include/CalamityAffixes/LootRerollGuard.h:101`, `skse/CalamityAffixes/src/EventBridge.Triggers.Events.cpp:22`

---

## 5) Generator 아키텍처

`affixes/affixes.json`(현재 143개: prefix 83, suffix 60) 기반으로 생성:

1. `KeywordPluginBuilder` → `Data/CalamityAffixes.esp`
2. `KidIniWriter` → `Data/CalamityAffixes_KID.ini`
3. `SpidIniWriter` → `Data/CalamityAffixes_DISTR.ini`
4. 기타 런타임 JSON 출력

핵심은 “데이터 편집 → 자동 생성 → 패키징” 반복이며, CK 수작업 의존도를 낮춘 구성입니다.

---

## 6) KID/DISTR 위치 정리 (오해 방지)

- KID/SPID INI는 자동 생성되며 완전한 “죽은 산출물”은 아닙니다.
- 다만 현재 코어 어픽스 부여의 주 경로는 C++ 인스턴스 롤링이며, KID/SPID는 보조/옵션 성격이 강합니다.
- README는 KID를 권장 의존성으로 문서화하고, I4는 placeholder(`rules: []`)를 명시합니다.

근거:
- `README.md:99` (활성 Papyrus 컴파일 타깃)
- `README.md:108` (KID/DISTR 산출물)
- `README.md:149` (I4 placeholder)

---

## 7) 설계 강점

- 도메인 분할이 비교적 명확하고, 이벤트-주도 흐름이 일관됨
- 인스턴스 키 기반 상태 관리로 “아이템 단위 영속성” 확보
- 다층 guard(ICD/중복 억제/재롤 방지)로 런타임 폭주 억제
- Generator 중심 파이프라인으로 배포 재현성 확보
- i18n(EN/KO) 경로가 데이터-런타임-UI까지 연결됨

---

## 8) 개선 후보 (우선순위)

1. `Config.cpp` 분할 (낮음)
- 파싱 책임이 큰 단일 파일에 집중되어 있어 유지보수 비용 증가 가능

2. `PrismaTooltip.cpp` 분할 (낮음)
- 폴링/상태/커맨드/렌더 경로가 한 파일에 밀집

3. Generator ↔ C++ 계약 타입 안정성 (중간)
- 런타임 JSON 계약을 더 강하게 타입화하면 회귀 여지 축소 가능

4. 필드 의미 명확화 (낮음)
- `procChancePct`가 컨텍스트별 의미가 달라 문서/검증 규칙 보강 여지

---

## 참고 링크 (외부 근거)

- CK Wiki `OnHit - ObjectReference`: https://ck.uesp.net/wiki/OnHit_-_ObjectReference
- CK Wiki `IsHostileToActor - Actor`: https://ck.uesp.net/wiki/IsHostileToActor_-_Actor
- CK Wiki `RegisterForModEvent - Form`: https://ck.uesp.net/wiki/RegisterForModEvent_-_Form

