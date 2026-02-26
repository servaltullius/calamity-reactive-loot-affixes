# Calamity — Reactive Loot & Affixes 심층 종합 분석

기준: 2026-02-26 · 4개 병렬 분석 에이전트 결과 통합

---

## 1. Executive Summary

Skyrim SE/AE SKSE64 플러그인 **Calamity — Reactive Loot & Affixes**는 Diablo 2/3 스타일 어픽스 시스템을 스카이림에 이식한 프로젝트다. 드롭 아이템에 1~4개의 Prefix(proc 발동형)/Suffix(패시브 스탯) 효과를 부여하며, D2식 룬워드 합성 시스템까지 포함한다.

### 규모

| 영역 | 규모 |
|------|------|
| C++ 소스 | 56개 파일, ~16,900 LOC |
| C++ 헤더 | 36개 파일, ~3,300 LOC |
| C++ 합계 | 92개 파일, **~20,200 LOC** |
| C# Generator | ~15개 파일 |
| C# 테스트 | 9개 파일 (xUnit) |
| Data 모듈 | 7개 JSON 모듈 → 1개 composed spec (6,057 LOC) |
| 어픽스 | 143개 (83 prefix + 60 suffix) |
| 룬워드 레시피 | 94개 |

### 핵심 아키텍처 결정

- `EventBridge` 중심 event-driven SKSE 런타임 (8개 이벤트 sink)
- `affixes.json` 단일 spec 기반 C# Generator(Mutagen v0.52)로 ESP/INI/Runtime JSON 코드 생성
- 인스턴스 상태는 `InstanceAffixSlots`(4-token) + co-save v7 마이그레이션 체계로 영속화
- UI는 PrismaUI overlay + bilingual i18n(`English` / `한국어` / `English + 한국어`)

### 코드 품질 평가

- **강점**: 도메인 분할, serialization migration 성숙도, 방어 로직(ICD/중복 hit guard/proc depth guard), Generator 중심 재현성, shuffle bag 공정 분배, pity system
- **약점**: `EventBridge` God class 경향(734 LOC 헤더, 50+ 중첩 타입), 글로벌 recursive_mutex 병목 가능성, C++ 런타임 테스트 부재(static checks 중심)

---

## 2. Architecture Overview

### 시스템 구성도

```text
┌─────────────────────────────────────────────────────────────────┐
│                       Data Pipeline                              │
│                                                                  │
│  affixes/modules/*.json                                          │
│       │                                                          │
│       │ compose_affixes.py                                       │
│       v                                                          │
│  affixes/affixes.json ──────┐                                    │
│       │                     │ lint_affixes.py                     │
│       │                     v                                    │
│       │          affix_validation_contract.json                   │
│       │          runeword.contract.json                           │
└───────┼──────────────────────────────────────────────────────────┘
        │
        v
┌───────────────────────────────────────────────────────────────────┐
│                    C# Generator (Mutagen v0.52)                    │
│                                                                    │
│  AffixSpecLoader ─→ GeneratorRunner ─→ Writers                     │
│                                          ├─ KeywordPluginBuilder   │
│                                          │    → CalamityAffixes.esp│
│                                          ├─ KidIniWriter           │
│                                          │    → CalamityAffixes_KID.ini│
│                                          ├─ SpidIniWriter          │
│                                          │    → CalamityAffixes_DISTR.ini│
│                                          ├─ Runtime JSON copy      │
│                                          │    → affixes.json       │
│                                          │    → runtime_contract.json│
│                                          └─ InventoryInjectorJsonWriter│
│                                               → CalamityAffixes.json│
└──────────────────────────┬────────────────────────────────────────┘
                           │
                           v
┌──────────────────────────────────────────────────────────────────┐
│              C++ SKSE64 Plugin (EventBridge Runtime)              │
│                                                                   │
│  ┌─────────┐  ┌─────────┐  ┌──────────┐  ┌──────────────┐       │
│  │ Config   │  │  Loot   │  │ Triggers │  │   Actions    │       │
│  │ ~3,100   │  │ ~4,400  │  │ ~1,570   │  │   ~1,600     │       │
│  │  LOC     │  │  LOC    │  │  LOC     │  │    LOC       │       │
│  └────┬─────┘  └────┬────┘  └────┬─────┘  └──────┬───────┘       │
│       │              │            │                │               │
│       └──────────────┴────────────┴────────────────┘               │
│                           │                                        │
│                    ┌──────┴──────┐                                  │
│                    │Serialization│ ~1,073 LOC                      │
│                    │  (co-save)  │ 6 record types                  │
│                    └─────────────┘                                  │
│                           │                                        │
│    ┌──────────────────────┼──────────────────────┐                 │
│    │                      │                      │                 │
│    v                      v                      v                 │
│ ┌──────────┐    ┌──────────────┐    ┌────────────────┐            │
│ │ Hooks.cpp│    │PrismaTooltip │    │  Papyrus MCM   │            │
│ │ (576 LOC)│    │  (891 LOC)   │    │  (46 LOC C++)  │            │
│ └──────────┘    └──────────────┘    └────────────────┘            │
│      │                 │                     │                     │
│      v                 v                     v                     │
│  HandleHealth     PrismaUI Panel      Papyrus Scripts              │
│  DamageHook       + Tooltip           + MCM Helper                 │
└──────────────────────────────────────────────────────────────────┘
```

### 3개 서브시스템

| 서브시스템 | 역할 | 주요 기술 |
|-----------|------|----------|
| **C++ Plugin** | 게임 이벤트 수신, affix 런타임 실행, serialization, tooltip 공급 | C++23, clang-cl, CommonLibSSE-NG |
| **C# Generator** | spec 검증 → ESP/KID/SPID/runtime_contract 생성 | .NET, Mutagen v0.52, xUnit |
| **Data Pipeline** | 모듈 조합/검증/lint → 배포 가능한 단일 spec 생산 | Python, JSON Schema |

### 모듈 간 의존성 매트릭스

| From ╲ To | C++ Plugin | C# Generator | Data Pipeline |
|-----------|-----------|-------------|--------------|
| **C++ Plugin** | 내부 도메인 의존 (Config/Loot/Triggers/Actions/Serialization) | 없음 (빌드타임 역의존 없음) | `affixes.json` 런타임 read 강의존 |
| **C# Generator** | 출력물(ESP/INI/JSON)로 Plugin 입력 생성 | Mutagen + Spec Loader 내부 의존 | `affixes/affixes.json` 직접 의존 |
| **Data Pipeline** | Plugin 런타임 contract의 사실상 원천 | Generator 입력 원천 | compose/lint/manifest 상호 검증 |

---

## 3. C++ Plugin Deep Dive

### 3.1 EventBridge 클래스 구조

**파일**: `include/CalamityAffixes/EventBridge.h` (734 LOC)

```cpp
class EventBridge final :
    public RE::BSTEventSink<RE::TESHitEvent>,
    public RE::BSTEventSink<RE::TESDeathEvent>,
    public RE::BSTEventSink<RE::TESEquipEvent>,
    public RE::BSTEventSink<RE::TESActivateEvent>,
    public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>,
    public RE::BSTEventSink<RE::TESContainerChangedEvent>,
    public RE::BSTEventSink<RE::TESUniqueIDChangeEvent>,
    public RE::BSTEventSink<SKSE::ModCallbackEvent>
```

**싱글톤**: Private default constructor + static `GetSingleton()` (thread-safe static)

#### 8개 SKSE 이벤트 싱크

| 이벤트 | 용도 |
|--------|------|
| `TESHitEvent` | Papyrus 경유 히트 감지 → prefix proc 트리거 |
| `TESDeathEvent` | Kill-type 어픽스 발동 |
| `TESEquipEvent` | 장비/해제 → passive suffix spell 관리 |
| `TESActivateEvent` | 시체/컨테이너 활성화 → currency drop |
| `TESMagicEffectApplyEvent` | DoT 키워드 태깅 + ICD |
| `TESContainerChangedEvent` | 아이템 이동 → 어픽스 롤링 |
| `TESUniqueIDChangeEvent` | ExtraUniqueID 변경 추적 |
| `SKSE::ModCallbackEvent` | MCM 설정 변경, 룬워드 UI 이벤트 |

#### 내부 Enum (Lines 133-190)

| Enum | 값 |
|------|----|
| `Trigger` | kHit, kIncomingHit, kDotApply, kKill, kLowHealth |
| `ActionType` | kDebugNotify, kCastSpell, kCastSpellAdaptiveElement, kCastOnCrit, kConvertDamage, kMindOverMatter, kArchmage, kCorpseExplosion, kSummonCorpseExplosion, kSpawnTrap |
| `AffixSlot` | kNone, kPrefix, kSuffix |
| `Element` | kNone, kFire, kFrost, kShock |
| `SyntheticRunewordStyle` | 37종 (kAdaptiveStrike, kSignatureGrief, ...) |
| `LootNameMarkerPosition` | kLeading, kTrailing |
| `CurrencyDropMode` | kHybrid |

#### 핵심 중첩 구조체

| 구조체 | Lines | 역할 |
|--------|-------|------|
| `Action` | 192-259 | 어픽스 효과 페이로드 (spell, conversion, trap, corpse explosion 등 68개 필드) |
| `TrapInstance` | 261-282 | 지면 트랩 인스턴스 (위치, 반경, TTL, arm/trigger 상태) |
| `InstanceRuntimeState` | 284-289 | 아이템별 어픽스 진화 상태 (XP, mode cycle) |
| `RunewordRecipe` | 291-302 | D2식 레시피 (rune sequence, result affix token) |
| `AffixRuntime` | 352-390 | JSON에서 로드된 어픽스 정의 (identity, loot, trigger, action, suffix props) |
| `LootConfig` | 481-510 | 런타임 루트 설정 (확률, 소스별 배율, 네이밍, 안전 제한) |
| `LootShuffleBagState` | 512-516 | 셔플백 상태 (D2식 공정 분배) |

#### AffixRuntime 구조체 상세 (Lines 352-390)

```
AffixRuntime:
  Identity:     id, token(FNV-1a), keywordEditorId, keyword*, label, displayName(En/Ko)
  Eligibility:  lootType, trigger, procChancePct, lootWeight, icd, perTargetIcd
  Conditions:   requireRecentlyHit, requireRecentlyKill, requireNotHitRecently
  Low Health:   lowHealthThresholdPct, lowHealthRearmPct
  Lucky Hit:    luckyHitChancePct, luckyHitProcCoefficient
  Suffix Props: slot, family, passiveSpell*, critDamageBonusPct, scrollNoConsumeChancePct
  Action:       Action struct (type, spell, effectiveness, magnitudeScaling, ...)
  Cooldown:     nextAllowed (steady_clock)
```

#### 멤버 변수 카테고리 (Lines 550-728)

**Affix Registry & Index** (Lines 562-595):
```
_affixes: vector<AffixRuntime>                           — 마스터 레지스트리
_activeCounts: vector<uint32_t>                          — 장착 중인 어픽스별 활성 카운트
_affixIndexById: unordered_map<string, size_t>           — ID → index O(1) 룩업
_affixIndexByToken: unordered_map<uint64_t, size_t>      — token → index O(1) 룩업

Trigger-partitioned indices (캐시된 벡터):
  _hitTriggerAffixIndices, _incomingHitTriggerAffixIndices
  _dotApplyTriggerAffixIndices, _killTriggerAffixIndices, _lowHealthTriggerAffixIndices
  _activeHitTriggerAffixIndices, _activeIncomingHitTriggerAffixIndices, ...

Action-type indices:
  _castOnCritAffixIndices, _convertAffixIndices, _mindOverMatterAffixIndices
  _archmageAffixIndices, _corpseExplosionAffixIndices, _summonCorpseExplosionAffixIndices

Loot eligibility indices:
  _lootWeaponAffixes, _lootArmorAffixes, _lootWeaponSuffixes, _lootArmorSuffixes
  _lootSharedAffixes, _lootSharedSuffixes

Shuffle bags (6개):
  _lootPrefixSharedBag, _lootPrefixWeaponBag, _lootPrefixArmorBag
  _lootSuffixSharedBag, _lootSuffixWeaponBag, _lootSuffixArmorBag

_appliedPassiveSpells: unordered_set<RE::SpellItem*>     — suffix 중복 방지
```

**Instance Storage** (Lines 596-607):
```
_instanceAffixes: unordered_map<uint64_t, InstanceAffixSlots>  — ★ 유일한 진실의 원천 ★
_lootPreviewAffixes: unordered_map (UI 미리보기 캐시)
_lootEvaluatedInstances: unordered_set (재롤링 방지 가드)
_lootCurrencyRollLedger: unordered_map (통화 드롭 추적)
_activeSlotPenalty: vector<float> (proc penalty by count)
_instanceStates: unordered_map (진화/모드 상태)
_equippedInstanceKeysByToken: unordered_map (장착 토큰 캐시)
```

**Runeword State** (Lines 611-622):
```
_runewordRecipes: vector<RunewordRecipe>                    — 94개 레시피
_runewordRecipeIndexByToken: unordered_map                  — token → index
_runewordRecipeIndexByResultAffixToken: unordered_map       — result → index
_runewordRuneFragments: unordered_map<uint64_t, uint32_t>   — 인벤토리 룬 수량
_runewordInstanceStates: unordered_map                      — 진행 중인 합성 상태
_runewordSelectedBaseKey: optional<uint64_t>                — 선택된 베이스 아이템
```

**Combat Context** (Lines 649-658):
```
_lastHitAt, _lastHit, _lastPapyrusHitEventAt
_recentOwnerHitAt, _recentOwnerKillAt, _recentOwnerIncomingHitAt
_playerCombatEvidenceExpiresAt
_lowHealthTriggerConsumed, _lowHealthLastObservedPct
```

### 3.2 도메인별 소스 파일 맵

#### Configuration & Parsing (~3,100 LOC)

| 파일 | LOC | 역할 |
|------|-----|------|
| `EventBridge.Config.cpp` | 190 | 메인 config 로더 오케스트레이터 |
| `EventBridge.Config.Reset.cpp` | 81 | 상태 초기화 (new game/load 실패) |
| `EventBridge.Config.LootRuntime.cpp` | 319 | 루트 설정 로딩, 검증 |
| `EventBridge.Config.RuntimeSettings.cpp` | 197 | MCM 사용자 설정 영속화 |
| `EventBridge.Config.AffixParsing.cpp` | 280 | 어픽스 엔트리 JSON 파싱 |
| `EventBridge.Config.AffixTriggerParsing.cpp` | 128 | trigger 조건 파싱 (ICD, cooldown, health window) |
| `EventBridge.Config.AffixActionParsing.cpp` | 486 | Action 페이로드 파싱 (cast, convert, trap, corpse 등) |
| `EventBridge.Config.Indexing.cpp` | 13 | trigger/action 인덱스 빌드 (wrapper) |
| `EventBridge.Config.IndexingShared.cpp` | 121 | 공유 인덱싱 로직 |
| `EventBridge.Config.AffixRegistry.cpp` | 31 | ID 기반 레지스트리 룩업 |
| `EventBridge.Config.LootPools.cpp` | 27 | 루트 풀 초기화 |
| `EventBridge.Config.RunewordSynthesis.cpp` | 128 | 룬워드 카탈로그 로딩 |
| `EventBridge.Config.RunewordSynthesis.Style.cpp` | 198 | 합성 룬워드 스타일 enum 검증 |
| `EventBridge.Config.RunewordSynthesis.StyleSelection.cpp` | 280 | 랜덤 스타일 선택 |
| `EventBridge.Config.RunewordSynthesis.StyleExecution.cpp` | 757 | 합성 룬워드 효과 실행 |
| `EventBridge.Config.RunewordSynthesis.SpellSet.cpp` | 93 | 룬워드 스펠 룩업/조합 |

#### Loot System (~4,400 LOC)

| 파일 | LOC | 역할 |
|------|-----|------|
| `EventBridge.Loot.cpp` | 613 | 루트 이벤트 오케스트레이션 (TESContainerChangedEvent) |
| `EventBridge.Loot.Assign.cpp` | 889 | 어픽스 롤링, P/S 분배, 아이템명 |
| `EventBridge.Loot.AffixSlotRoll.cpp` | 196 | 슬롯별 가중 어픽스 선택 |
| `EventBridge.Loot.PreviewClaimStore.cpp` | 51 | 미리보기 캐시 |
| `EventBridge.Loot.Runtime.cpp` | 166 | 런타임 아이템 뮤테이션 |
| `EventBridge.Loot.TooltipResolution.cpp` | 572 | 어픽스 텍스트 & 툴팁 렌더링 |
| `EventBridge.Loot.Runeword.BaseSelection.cpp` | 210 | 룬워드 베이스 선택 |
| `EventBridge.Loot.Runeword.Selection.cpp` | 5 | 베이스/레시피 선택 디스패치 |
| `EventBridge.Loot.Runeword.RecipeUi.cpp` | 72 | 합성 패널 상태 |
| `EventBridge.Loot.Runeword.RecipeEntries.cpp` | 638 | 사용 가능 레시피 목록 (룬 요구사항 포함) |
| `EventBridge.Loot.Runeword.PanelState.cpp` | 162 | UI 패널 상태 (삽입 가능 여부, 요약 텍스트) |
| `EventBridge.Loot.Runeword.Catalog.cpp` | 562 | 룬워드 카탈로그 JSON 로딩 |
| `EventBridge.Loot.Runeword.Crafting.cpp` | 314 | 룬 삽입, 진행 추적 |
| `EventBridge.Loot.Runeword.Transmute.cpp` | 439 | 레시피 완성, 어픽스 교체 |
| `EventBridge.Loot.Runeword.Reforge.cpp` | 266 | 오브로 재롤 (토큰 프로모션) |
| `EventBridge.Loot.Runeword.Policy.cpp` | 45 | 룬워드 자격 검사 |
| `EventBridge.Loot.Runeword.Detail.cpp` | 205 | 룬워드 spec/detail 룩업 |

#### Triggers & Events (~1,570 LOC)

| 파일 | LOC | 역할 |
|------|-----|------|
| `EventBridge.Triggers.cpp` | 601 | 이벤트 싱크 디스패처 (Hit, Death, Equip, Activate, MagicEffect) |
| `EventBridge.Triggers.HealthDamage.cpp` | 364 | HandleHealthDamage 훅 평가기 |
| `EventBridge.Triggers.Events.cpp` | 1,203 | RebuildActiveCounts, 패시브 스펠 관리, currency 드롭 |

#### Actions & Spell Casting (~1,600 LOC)

| 파일 | LOC | 역할 |
|------|-----|------|
| `EventBridge.Actions.cpp` | 14 | 메인 디스패처 |
| `EventBridge.Actions.Dispatch.cpp` | 42 | 타입별 라우팅 |
| `EventBridge.Actions.Cast.cpp` | 337 | CastSpell & CastSpellAdaptiveElement |
| `EventBridge.Actions.Special.cpp` | 703 | CastOnCrit, ConvertDamage, MindOverMatter, Archmage |
| `EventBridge.Actions.Corpse.cpp` | 552 | CorpseExplosion & SummonCorpseExplosion |
| `EventBridge.Actions.Trap.cpp` | 230 | SpawnTrap 배치 & 추적 |

#### Serialization (~1,073 LOC)

**단일 파일**: `EventBridge.Serialization.cpp`

6개 co-save 레코드:

| 레코드 | FourCC | 버전 | 내용 |
|--------|--------|------|------|
| InstanceAffixes | `IAXF` | v7 | `[baseID(u32) uniqueID(u16) count(u8) tokens[4](u64)]` per entry |
| InstanceRuntimeStates | `IRST` | v1 | 진화 XP, 모드 사이클 카운터 |
| RunewordState | `RWRD` | v1 | 선택된 베이스, 레시피 진행, 룬 파편 |
| LootEvaluated | `LRLD` | v1 | 평가 완료 인스턴스 (재롤 방지) |
| LootCurrencyLedger | `LCLD` | v2 | 통화 드롭 추적 (30일 TTL) |
| LootShuffleBags | `LSBG` | v2 | 6개 셔플백 상태 (cursor + order) |

#### UI & Persistence (~1,200+ LOC)

| 파일 | LOC | 역할 |
|------|-----|------|
| `PrismaTooltip.cpp` | 891 | 툴팁 후킹, 언어 모드, 스탯 포맷팅 |
| `PrismaLayoutPersistence.cpp` | 291 | PrismaUI 패널 레이아웃 저장/복원 |
| `UserSettingsPersistence.cpp` | 247 | MCM 설정 디바운스(250ms) & I/O |
| `RunewordContractSnapshot.cpp` | 225 | 런타임 contract 검증 |
| `Papyrus.cpp` | 46 | 스크립트 함수 등록 |

#### Hooks & Entry (~700+ LOC)

| 파일 | LOC | 역할 |
|------|-----|------|
| `Hooks.cpp` | 576 | Actor::HandleHealthDamage vfunc 후킹 (SE: 0x104, VR: 0x106) |
| `TrapSystem.cpp` | 61 | 프레임 틱 후크 (main loop 디스패치) |
| `main.cpp` | 131 | 플러그인 진입점 (SKSEPluginLoad, 메시지 핸들러) |

### 3.3 InstanceAffixSlots — 핵심 데이터 구조

**파일**: `include/CalamityAffixes/InstanceAffixSlots.h` (109 LOC)

```cpp
struct InstanceAffixSlots {
    std::array<std::uint64_t, 4> tokens{};  // [0..2]=regular, [3]=runeword
    std::uint8_t count = 0;                  // 실제 점유 (0-4)

    constexpr bool HasToken(uint64_t) const;
    constexpr uint64_t GetPrimary() const;       // tokens[0] 또는 0
    constexpr bool AddToken(uint64_t);           // append
    constexpr bool PromoteTokenToPrimary(uint64_t);  // [0]으로 이동
    constexpr void Clear();
    constexpr void ReplaceAll(uint64_t);         // D2-style 전체 교체
};

constexpr size_t kMaxRegularAffixesPerItem = 3;
constexpr size_t kMaxRunewordAffixesPerItem = 1;
constexpr size_t kMaxAffixesPerItem = 4;  // 3 regular + 1 runeword
```

**인스턴스 키**: `(FormID << 16) | UniqueID` → `uint64_t`
**어픽스 토큰**: FNV-1a 64-bit hash of affix ID string

### 3.4 헤더 유틸리티 파일 (34개, ~2,400 LOC)

| 헤더 | LOC | 역할 |
|------|-----|------|
| `AffixToken.h` | 19 | FNV-1a 64-bit 해시 |
| `InstanceStateKey.h` | 63 | (instanceKey + affixToken) 해시 키 |
| `MagnitudeScaling.h` | 63 | Actor value 스케일링 (Health, Magicka, Stamina) |
| `RuntimePolicy.h` | 58 | 컴파일타임 정책 플래그 & 이벤트 이름 |
| `RuntimeContract.h` | 80 | JSON contract 스키마 |
| `LootUiGuards.h` | 335 | 별표 감지, 이름 포맷팅 헬퍼 |
| `TriggerGuards.h` | 242 | 윈도우 계산 (recently-hit, ICD, duplicate) |
| `LootRollSelection.h` | 237 | 가중 랜덤 선택 + 셔플백 |
| `LootEligibility.h` | 114 | 루트 타입 체크, 제외 목록 |
| `LootRerollGuard.h` | 111 | 동일 아이템 재롤 방지 |
| `NonHostileFirstHitGate.h` | 151 | 비적대 NPC 첫 히트 허용 제어 |
| `PerTargetCooldownStore.h` | 126 | 어픽스별 대상 ICD 추적 |
| `LootCurrencyLedger.h` | 54 | 통화 드롭 원장 (30일 TTL) |
| `LootCurrencySourceHelpers.h` | 152 | 소스별 통화 롤 로직 |
| `LootSlotSanitizer.h` | 47 | 로드 시 무효 suffix 슬롯 제거 |
| `AdaptiveElement.h` | 48 | Element enum + 저항 평가 |
| `CombatContext.h` | 89 | Hit/Attacker 컨텍스트 헬퍼 |
| `HitDataUtil.h` | 42 | HitData 파싱 (power attack, crit 등) |
| `PlayerOwnership.h` | 38 | 플레이어 소유 판정 |
| `PointerSafety.h` | 54 | Actor/Item 유효성 검사 |
| `ResyncScheduler.h` | 23 | 프레임 기반 인터벌 스케줄링 |
| `RuntimeUserSettingsDebounce.h` | 64 | 250ms MCM 설정 디바운스 |

### 3.5 동시성 모델

| 컴포넌트 | 잠금 방식 | 타입 | 비고 |
|---------|----------|------|------|
| `_affixes`, 인덱스들 | `_stateMutex` | recursive_mutex | Config 로드 시에만 write; 게임플레이 중 대부분 read |
| `_instanceAffixes` | `_stateMutex` | recursive_mutex | Serialization, loot, equip 이벤트 |
| `_rng` | `_rngMutex` | mutex | 셔플백, 롤링 |
| `_allowNonHostilePlayerOwnedOutgoingProcs` | atomic\<bool\> | lock-free | MCM 구동, 전투 중 폴링 |
| `_hasActiveTraps` | atomic\<bool\> | lock-free | 프레임 틱 최적화 |
| `_procDepth` | atomic\<uint32\> | lock-free | 재진입 가드 |

SKSE 이벤트 핸들러는 메인 스레드에서 실행되며, TES 스크립트 VM은 이벤트 처리 중 블로킹된다. Serialization은 save/load 중 quiescent 상태에서 발생한다.

### 3.6 상수 & 튜닝 파라미터

**어픽스 & 루트 튜닝:**

| 상수 | 값 | 용도 |
|------|----|------|
| `kAffixCountWeights` | `[70.0, 22.0, 8.0]` | 1/2/3 어픽스 분배 확률 |
| `kMultiAffixProcPenalty` | `[1.0, 0.8, 0.65, 0.5]` | Best Slot Wins 패널티 |
| `kLootChancePityFailThreshold` | 3 | 루트 연속 실패 보정 |
| `kRunewordFragmentPityFailThreshold` | 99 | 룬 파편 연속 실패 보정 |
| `kReforgeOrbPityFailThreshold` | 39 | 리포지 오브 연속 실패 보정 |

**쿨다운 윈도우:**

| 상수 | 값 | 용도 |
|------|----|------|
| `kDotApplyICD` | 1,500ms | DoT 별도 ICD |
| `kCastOnCritICD` | 150ms | 크리티컬 히트 proc ICD |
| `kDuplicateHitWindow` | 100ms | 중복 히트 억제 |
| `kPapyrusHitEventWindow` | 80ms | Papyrus 히트 이벤트 stale |
| `kHealthDamageHookStaleWindow` | 5,000ms | 훅 실효 윈도우 |

**캐시 & 원장:**

| 상수 | 값 | 용도 |
|------|----|------|
| `kLootEvaluatedRecentKeep` | 2,048 | 평가 완료 인스턴스 캐시 |
| `kLootEvaluatedPruneEveryInserts` | 128 | 정리 주기 |
| `kLootPreviewCacheMax` | 1,024 | 미리보기 캐시 |
| `kLootCurrencyLedgerMaxEntries` | 8,192 | 통화 원장 최대 |
| `kLootCurrencyLedgerTtlDays` | 30 | 통화 원장 TTL |

**트랩 설정:**

| 상수 | 값 | 용도 |
|------|----|------|
| `trapGlobalMaxActive` | 48 | 전역 최대 활성 트랩 |
| `trapCastBudgetPerTick` | 6 | 프레임당 캐스트 예산 |
| `triggerProcBudgetPerWindow` | 12 | 윈도우당 proc 예산 |
| Proc budget window | 100ms | 예산 윈도우 크기 |

### 3.7 빌드 시스템

**CMakeLists.txt** (225 LOC):
- CMake 3.21+, C++23, CommonLibSSE-NG + nlohmann_json
- 출력: `CalamityAffixes.dll` (SHARED library)
- 크로스 컴파일: clang-cl on Linux/WSL → Windows x86_64
- SKSE 트램폴린: 1 KB 훅 메모리
- 테스트: `BUILD_TESTING` 옵션 → static checks (20+ 테스트 파일)

---

## 4. Critical Data Flows

### Flow 1: Item Drop → Affix Rolling → Assignment → Naming

```
TESContainerChangedEvent (newContainer == playerFormID)
    │
    ▼
ProcessEvent() [EventBridge.Loot.cpp:326-495]
    ├─ reroll guard 검사 (플레이어 drop/re-pick, stash)
    ├─ 아이템 유효성 (weapon/armor, exclusion list)
    │
    ▼
ProcessLootAcquired() [EventBridge.Loot.Assign.cpp:730-805]
    ├─ config 로드 확인, runtime enabled 확인
    ├─ currency roll gate 확인 (source tier)
    │
    ▼
RollAffixCount() [EventBridge.Loot.Assign.cpp:438-453]
    ├─ std::discrete_distribution<unsigned int> weights: [70.0, 22.0, 8.0]
    ├─ 결과: 1, 2, 또는 3
    │
    ▼
For each affix slot:
    RollAffixByType() [EventBridge.Loot.AffixSlotRoll.cpp]
    ├─ Prefix 롤: 무기/방어구별 또는 공유 풀에서 선택
    │   └─ SelectWeightedEligibleLootIndexWithShuffleBag (D2식 셔플백)
    │   └─ 필터: slot == kPrefix, EffectiveLootWeight() > 0
    ├─ Suffix 롤 (2-3 어픽스 아이템만):
    │   └─ 필터: slot == kSuffix, 동일 family 제외 (중복 방지)
    │   └─ Suffix 티어: Minor(60%), Major(30%), Grand(10%)
    │
    ▼
P/S 분배 규칙:
    1 어픽스 (70%): P 또는 S 50/50
    2 어픽스 (25%): 1P + 1S 고정
    3 어픽스 (5%): 2P+1S 또는 1P+2S 랜덤
    │
    ▼
_instanceAffixes[MakeInstanceKey(formID, uniqueID)] = slots
    │
    ▼
EnsureMultiAffixDisplayName() [EventBridge.Loot.Assign.cpp:593-728]
    ├─ 기존 어픽스 마커/temper 접미사 제거
    ├─ count==1 → "*", count==2 → "**", count>=3 → "***"
    ├─ kTrailing: "철검*" / kLeading: "* 철검"
    └─ ExtraTextDisplayData 설정
```

### Flow 2: Serialization (Save/Load/Revert)

**Save 경로:**
```
Save(SKSE::SerializationInterface*) [EventBridge.Serialization.cpp:54-270]
    │
    ├─ IAXF v7: for each (key, slots) in _instanceAffixes
    │   └─ Write: baseID(u32) | uniqueID(u16) | count(u8) | tokens[0..3](u64×4)
    │
    ├─ IRST v1: for each (stateKey, state) in _instanceStates
    │   └─ Write: baseID | uniqueID | affixToken(u64) | evolutionXp(u32) | modeCycleCounter(u32) | modeIndex(u32)
    │
    ├─ RWRD v1: selected base key, recipe/base cycle cursors, fragments map, instance states
    │
    ├─ LRLD v1: evaluated instance keys set
    │
    ├─ LCLD v2: (ledgerKey → dayStamp) map for 30-day tracking
    │
    └─ LSBG v2: 6 shuffle bags (cursor + shuffled indices each)
```

**Load 경로:**
```
Load(SKSE::SerializationInterface*) [EventBridge.Serialization.cpp:272+]
    │
    ├─ 1. 전체 상태 초기화
    │   └─ _instanceAffixes.clear(), _instanceStates.clear(), shuffle bags reset
    │
    ├─ 2. 레코드별 역직렬화
    │   ├─ IAXF: v1~v6 → v7 자동 마이그레이션
    │   │   └─ v6 이전: 단일 토큰 → 4-토큰 슬롯 변환
    │   ├─ IRST: evolution XP, mode cycle 복원
    │   ├─ RWRD: selected base, recipe cursors, fragments 복원
    │   ├─ LRLD: 추적 세트 복원
    │   ├─ LCLD: 통화 원장 복원
    │   └─ LSBG: 셔플백 상태 복원
    │
    └─ 3. RebuildActiveCounts() → 장착 패시브 스펠 재동기화
```

**Revert**: 모든 런타임 maps/cache/trap/ledger 완전 초기화

### Flow 3: Tooltip → PrismaUI (bilingual i18n)

```
GetInstanceAffixTooltip(uint64_t instanceKey, int uiLanguageMode)
  [EventBridge.Loot.TooltipResolution.cpp]
    │
    ├─ _instanceAffixes에서 key 룩업
    ├─ 각 token → _affixIndexByToken에서 AffixRuntime 조회
    ├─ uiLanguageMode에 따라 displayName 선택:
    │   ├─ 0 (English): displayNameEn
    │   ├─ 1 (한국어): displayNameKo
    │   └─ 2 (Both): "displayNameEn / displayNameKo"
    ├─ 스킬명, magnitude, 조건 포맷팅
    └─ HTML/tooltip 문자열 반환
         │
         ▼
PrismaTooltip.cpp (891 LOC)
    ├─ g_uiLanguageMode: atomic<int> (line 89), 기본값 2 (bilingual)
    ├─ RefreshUiLanguageFromMcm(): MCM 설정 파일 폴링 → g_uiLanguageMode 갱신
    └─ PrismaUI overlay로 렌더링
```

### Flow 4: Equip → Passive Suffix Spell Application

```
RebuildActiveCounts() [EventBridge.Triggers.Events.cpp:1031-1203]
    │
    ├─ 1. 상태 초기화
    │   └─ _activeCounts.assign(0), _equippedInstanceKeysByToken.clear()
    │
    ├─ 2. 플레이어 인벤토리 순회
    │   for each InventoryEntryData:
    │     for each ExtraDataList:
    │       ├─ Get UniqueID → _instanceAffixes 룩업
    │       ├─ WORN 체크 (ExtraWorn / ExtraWornLeft)
    │       └─ if WORN:
    │           for each affix token in slots:
    │             ├─ _activeCounts[affixIdx]++ (어픽스별 장착 수)
    │             ├─ if kSuffix && passiveSpell → desiredPassives에 추가
    │             ├─ _activeCritDamageBonusPct 누적
    │             └─ _equippedInstanceKeysByToken[token] 추적
    │
    ├─ 3. 패시브 스펠 diff 적용
    │   ├─ RemovePreviousPassiveSpells(): 이전 적용 중 desiredPassives에 없는 것 제거
    │   ├─ ApplyDesiredPassiveSpells(): 새로 필요한 것 추가
    │   └─ _appliedPassiveSpells = desiredPassives (캐시 갱신)
    │
    └─ 4. proc penalty 재계산
        └─ _activeSlotPenalty 벡터 갱신
```

**Suffix 동일 스펠 비중첩**: 같은 Suffix가 여러 장비에 있어도 `RE::SpellItem*` 중복 방지로 1회만 적용. 다른 티어는 다른 스펠이므로 중첩 가능.

### Flow 5: Runeword Crafting (D2-style)

```
SelectRunewordBase() → _runewordSelectedBaseKey 설정
    │
    ▼
SelectRunewordRecipe() → _runewordSelectedRecipeToken 설정
    │
    ▼
InsertRunewordRuneIntoSelectedBase()
    ├─ 룬 파편 소모 검증
    ├─ _runewordInstanceStates[instanceKey] 진행 업데이트
    ├─ insertedRunes++
    └─ 완전 매칭 시:
         │
         ▼
ApplyRunewordResult(instanceKey, recipe)
  [EventBridge.Loot.Runeword.Transmute.cpp:19-98]
    ├─ 1. ResolveRunewordApplyBlockReason() 검증
    │   └─ kMissingResultAffix, kAffixSlotsFull 체크
    ├─ 2. 결과 affix token 조회
    ├─ 3. PromoteTokenToPrimary(resultToken)
    │   └─ D2-style: 기존 slots shift → 새 토큰 [0]에 삽입
    │   └─ count capped at kMaxAffixesPerItem (4)
    ├─ 4. EnsureInstanceRuntimeState (XP=0, mode=0)
    ├─ 5. _runewordInstanceStates.erase(instanceKey)
    ├─ 6. EnsureMultiAffixDisplayName() + RebuildActiveCounts()
    └─ 7. "Runeword Complete: {name}" 알림
```

### Flow 6: Config Loading (affixes.json → AffixRuntime)

```
LoadConfig() [EventBridge.Config.cpp:110-165]
    │
    ├─ ResetRuntimeStateForConfigReload()
    │   └─ _affixes, _activeCounts, 모든 인덱스 초기화
    │
    ├─ InitializeRunewordCatalog()
    │   └─ "runewordRecipes" 배열에서 카탈로그 빌드
    │
    ├─ LoadRuntimeConfigJson(j)
    │   └─ Data/SKSE/Plugins/CalamityAffixes/affixes.json 읽기
    │
    ├─ ValidateRuntimeContractSnapshot()
    │   └─ 런타임 contract 버전 호환성 확인
    │
    ├─ ApplyLootConfigFromJson(j) → _loot (LootConfig) 채우기
    │
    ├─ ParseConfiguredAffixesFromJson()
    │   for each affix in j["keywords"]["affixes"]:
    │     InitializeAffixFromJson()
    │       ├─ id, editorId, name/nameEn/nameKo 파싱
    │       ├─ RE::TESForm::LookupByEditorID<RE::BGSKeyword>(editorId)
    │       ├─ ApplyAffixSlotAndFamilyFromJson() → slot, family
    │       ├─ ApplyAffixTriggerFromJson() → trigger, ICD, conditions
    │       ├─ ApplyAffixLootWeightsFromJson() → lootWeight, procChancePct
    │       └─ For suffix: ApplyPassiveSpellFromJson() → passiveSpell* (Ability type)
    │
    ├─ IndexConfiguredAffixes()
    │   ├─ _affixIndexById, _affixIndexByToken 구축
    │   ├─ Trigger-partitioned: _hitTriggerAffixIndices, _killTriggerAffixIndices, ...
    │   ├─ Action-partitioned: _castOnCritAffixIndices, _convertAffixIndices, ...
    │   └─ Loot pools: _lootWeaponAffixes, _lootArmorAffixes, _lootWeaponSuffixes, ...
    │
    ├─ SynthesizeRunewordRuntimeAffixes()
    │   └─ 합성 룬워드 어픽스 생성 (lootWeight < 0, 롤링 불가)
    │
    ├─ RebuildSharedLootPools()
    │
    ├─ SanitizeAllTrackedLootInstancesForCurrentLootRules()
    │   └─ 세이브에서 현재 config에 없는 어픽스 제거
    │
    └─ _configLoaded = true → RebuildActiveCounts()
```

---

## 5. C# Generator Pipeline

### 5.1 프로젝트 구조

```
tools/CalamityAffixes.Generator/
├── Program.cs                         — CLI 진입점
├── GeneratorRunner.cs                 — 오케스트레이터 (~85 LOC)
├── Spec/
│   ├── AffixSpec.cs                   — 오브젝트 모델 (~357 LOC)
│   ├── AffixSpecLoader.cs             — JSON 로더 + 검증 (~500 LOC)
│   └── RunewordContractCatalog.cs     — D2 룬워드 카탈로그
└── Writers/
    ├── KeywordPluginBuilder.cs        — ESP 생성 (Mutagen) (~512 LOC)
    ├── KidIniWriter.cs                — KID v3 INI (~102 LOC)
    ├── SpidIniWriter.cs               — SPID INI (~67 LOC)
    ├── McmPluginBuilder.cs            — MCM Helper Quest
    └── InventoryInjectorJsonWriter.cs — InventoryInjector JSON (~58 LOC)
```

### 5.2 Spec 클래스 계층

```
AffixSpec (루트 컨테이너)
├── version: int
├── modKey: string ("CalamityAffixes.esp")
├── eslFlag: bool (ESL 플래그)
├── loot: LootSpec? (드롭율, 안전 제한, 네이밍)
└── keywords: KeywordSpec
    ├── tags: KeywordDefinition[] (CAFF_TAG_DOT 등)
    ├── affixes: AffixDefinition[] (143개)
    │   ├── id, editorId, name, nameEn, nameKo
    │   ├── slot ("prefix"/"suffix"), family
    │   ├── kid: KidRule (KID v3 배포 규칙)
    │   ├── records: AffixRecordSpec?
    │   │   ├── magicEffect(s): MagicEffectRecordSpec
    │   │   │   ├── editorId, name, actorValue, resistValue
    │   │   │   ├── hostile, recover, archetype ("ValueModifier" 기본)
    │   │   │   └── magicSkill (Destruction, Restoration...)
    │   │   └── spell(s): SpellRecordSpec
    │   │       ├── editorId, name, delivery ("Self"/"TargetActor")
    │   │       ├── spellType ("Spell"/"Ability"), castType
    │   │       └── effect(s): SpellEffectRecordSpec
    │   └── runtime: Dict<string, object> (C++ trigger/action config)
    ├── kidRules: KidRuleEntry[]
    └── spidRules: Dict<string, object?>[]
```

### 5.3 LootSpec (런타임 설정)

```
chancePercent: 50%                          — 전역 드롭율
runewordFragmentChancePercent: 8%           — 룬 파편 드롭
reforgeOrbChancePercent: 5%                 — 리포지 오브 드롭
currencyDropMode: "hybrid"                  — corpse=SPID, container/world=runtime

소스별 배율:
  lootSourceChanceMultCorpse: 0.80           — 시체 80%
  lootSourceChanceMultContainer: 1.00        — 컨테이너 100%
  lootSourceChanceMultBossContainer: 1.15    — 보스 컨테이너 115%
  lootSourceChanceMultWorld: 1.00            — 월드 100%

안전 제한:
  trapGlobalMaxActive, trapCastBudgetPerTick, triggerProcBudgetPerWindow

필터:
  armorEditorIdDenyContains, bossContainerEditorIdAllowContains

네이밍:
  renameItem, nameMarkerPosition, nameFormat="{base}[{affix}]"
```

### 5.4 KeywordPluginBuilder (ESP 생성 — Mutagen)

**생성 순서:**
1. **MCM Helper Quest** — `CalamityAffixes_MCM` (FormID 안정성을 위해 항상 먼저)
2. **Keywords** — tags + affixes (143개 어픽스 키워드)
3. **MagicEffects** — ActorValue, ResistValue, Archetype, 적대/비적대 플래그
4. **Spells** — SpellType(Spell/Ability), CastType(FireAndForget/ConstantEffect), 효과 목록
5. **MiscItems** — 룬 파편 (El~Zod, 33개) + 리포지 오브
6. **LeveledItems** — `CAFF_LItem_RunewordFragmentDrops` (가중 티켓), `CAFF_LItem_ReforgeOrbDrops`

**Mutagen 사용 패턴:**
```csharp
var mod = new SkyrimMod(ModKey.FromFileName(spec.ModKey), SkyrimRelease.SkyrimSE);
if (spec.EslFlag) mod.ModHeader.Flags |= SkyrimModHeader.HeaderFlag.Small;

// 레코드 추가
var spell = mod.Spells.AddNew();
spell.Flags = (SpellDataFlag)0;  // ★ 중요: 모든 기본 플래그 명시적 초기화
spell.Type = SpellType.Ability;
spell.CastType = CastType.ConstantEffect;

((IModGetter)mod).WriteToBinary(new FilePath(pluginPath));
```

**이름 정제** (`ToPluginSafeName`): Korean/non-ASCII 제거 → "/" 왼쪽 우선 → EditorID 폴백

### 5.5 AffixSpecLoader (검증)

**검증 항목:**
- ModKey 형식 (.esp/.esm/.esl, 경로 구분자 불가)
- EditorID 중복 (keyword/spell/MGEF 전체)
- ActorValue enum 유효성
- Spell delivery/castType/spellType 조합
- Spell effect MGEF 참조 존재 여부
- Runtime shape: trigger + action.type from validation contract

**검증 contract 로딩 우선순위:**
1. `tools/affix_validation_contract.json` (cwd에서 상위 탐색)
2. 하드코딩된 폴백 contract (CI/테스트용)

### 5.6 모듈 조합 시스템

```json
// affixes/affixes.modules.json
{
  "version": 1,
  "root": "modules/spec.root.json",
  "keywords": {
    "tags": ["modules/keywords.tags.json"],
    "affixes": [
      "modules/keywords.affixes.core.json",
      "modules/keywords.affixes.runewords.json",
      "modules/keywords.affixes.suffixes.json"
    ],
    "kidRules": ["modules/keywords.kidRules.json"],
    "spidRules": ["modules/keywords.spidRules.json"]
  }
}
```

`compose_affixes.py`가 모듈을 병합하여 `affixes/affixes.json` (6,057 LOC) 생성.

### 5.7 룬워드 Contract 카탈로그

**로딩 우선순위:**
1. `affixes/runeword.contract.json` (사전 생성 스냅샷)
2. C++ 소스 파싱 폴백: `RunewordCatalogRows.inl` + `EventBridge.Loot.Runeword.Detail.cpp`
3. 하드코딩 D2 룬 래더 (33개 룬, El=1200 ~ Zod=2)

**룬 가중치 예시**: El=1200, Eld=1000, ..., Jah=4, Cham=3, Zod=2
→ LeveledItem에서 ~48 티켓 max로 정규화

### 5.8 테스트 (xUnit, 9개 파일)

| 파일 | 역할 |
|------|------|
| `AffixSpecLoaderTests.cs` (39KB) | 중복 EditorID, malformed JSON, 검증 |
| `KeywordPluginBuilderTests.cs` (35KB) | Keywords, MagicEffects, Spells, currency lists, MCM quest |
| `GeneratorRunnerTests.cs` (15KB) | E2E 통합: 파일 출력 검증, runtime contract 구조 |
| `KidIniRendererTests.cs` (5.7KB) | Rule 병합, chance 포맷, safety checks |
| `SpidIniWriterTests.cs` (1.5KB) | Comment/line pass-through |
| `InventoryInjectorJsonWriterTests.cs` (1.7KB) | Schema URL 검증 |
| `McmPluginBuilderTests.cs` (2.7KB) | Quest FormID 안정성 |
| `RuntimeContractSyncTests.cs` (4KB) | Contract 자동 생성 검증 |
| `RepoSpecRegressionTests.cs` (19KB) | 실제 리포 spec 대비 회귀 |

---

## 6. Cross-Cutting Concerns

### 6.1 i18n (국제화)

**데이터 흐름:**
```
affixes.json (nameEn/nameKo)
    → C++ AffixRuntime (displayNameEn/displayNameKo)
    → GetInstanceAffixTooltip(uiLanguageMode)
    → PrismaTooltip.cpp (g_uiLanguageMode atomic)
    → PrismaUI overlay 렌더링
```

**MCM 설정:**
```json
{ "id": "iUiLanguage:General", "type": "enum",
  "options": ["English", "한국어", "English + 한국어"], "defaultValue": 2 }
```

- `g_uiLanguageMode`: `atomic<int>` (PrismaTooltip.cpp:89), 기본값 2
- `RefreshUiLanguageFromMcm()`: MCM 설정 파일 주기적 폴링 → atomic 갱신
- ESP 스펠 이름: 엔진 제약으로 bilingual 포맷 "Calamity: Arc Lightning / 아크 번개"

### 6.2 Proc Penalty (Best Slot Wins)

```cpp
// EventBridge.h:439
static constexpr std::array<float, kMaxAffixesPerItem> kMultiAffixProcPenalty =
    { 1.0f, 0.8f, 0.65f, 0.5f };

// EventBridge.Triggers.cpp:479-480
const float penalty = (i < _activeSlotPenalty.size() && _activeSlotPenalty[i] > 0.0f)
    ? _activeSlotPenalty[i] : 1.0f;
const float chance = std::clamp(
    affix.procChancePct * _runtimeProcChanceMult * penalty, 0.0f, 100.0f);
```

| 어픽스 수 | 패널티 | 실효 proc |
|----------|--------|----------|
| 1 | 1.0× | 100% |
| 2 | 0.8× | 80% |
| 3 | 0.65× | 65% |
| 4 | 0.5× | 50% |

`RebuildActiveCounts()`에서 장비 변경 시 `_activeSlotPenalty` 벡터 재계산.

### 6.3 Currency System (통화 드롭)

**드롭 확률 (기본값):**

| 소스 | 루트 배율 | 룬 파편 | 리포지 오브 |
|------|---------|---------|-----------|
| 시체 | 0.80× | 8% | 5% |
| 컨테이너 | 1.00× | 8% | 5% |
| 보스 컨테이너 | 1.15× | 8% | 5% |
| 월드 | 1.00× | 8% | 5% |

**Rate Limiting:**
- `_lootCurrencyRollLedger`: `unordered_map<uint64_t, uint32_t>` (dayStamp)
- `kLootCurrencyLedgerTtlDays = 30` — 30일 후 만료
- `kLootCurrencyLedgerMaxEntries = 8192` — 하드 캡

**Pity System:**
- Loot: 3연속 실패 후 보정
- 룬 파편: 99연속 실패 후 보장
- 리포지 오브: 39연속 실패 후 보장

### 6.4 MCM 시스템

**아키텍처:**
```
MCM Helper Schema (config.json) → Papyrus Quest (CalamityAffixes_MCMConfig.psc)
    → SKSE Mod Events → C++ EventBridge (런타임 설정)
```

**MCM 페이지 (3개):**
1. **Runtime**: 활성화, proc 배율 (0.1~3.0), 룬워드 파편 확률 (0~100%)
2. **Prisma UI**: 패널 설정
3. **Hotkey & Debug**: 디버그 알림, 핫키

**Papyrus Leader Election:**
- 랜덤 토큰으로 canonical quest 인스턴스만 활성화
- FormID 정규화로 `Game.GetFormFromFile()` 검증
- 다중 플러그인 로드 시 중복 MCM 등록 방지

**SKSE Mod Events:**
```
CalamityAffixes_SetEnabled
CalamityAffixes_SetProcChanceMult
CalamityAffixes_SetRunewordFragmentChance
...
```

### 6.5 Trap System

**생명주기:**
```
SpawnTrap() → TrapInstance 생성 (_traps 벡터)
    → Arm Delay (설정 ms 대기)
    → Active Period (반경 내 액터 체크)
    → Trigger (스펠 캐스트, 카운터++, rearm delay)
    → Expire (TTL 만료 또는 trigger 한도)
```

**틱 구현:** `_traps` 벡터 선형 탐색, 프레임당 `castBudgetPerTick` (기본 6) 예산 제한.

### 6.6 Inventory Injector

**현재 상태:** 빈 스키마 (rules 배열 비어 있음)
```json
{ "$schema": "...InventoryInjector.schema.json", "rules": [] }
```
어픽스 UX는 Prisma UI overlay에서 처리 → InventoryInjector 텍스트 주입 불필요 (SkyUI 이름 중복 방지).

### 6.7 CI/CD & 스크립트

**메인 스크립트:** `scripts/vibe.py`
- `.vibe/` 에코시스템 래퍼
- `boundaries.json`: 코드 구조 분석 (coupling, complexity)
- `reports/`: 설정, 핫스팟, typecheck 상태

**CI 워크플로우:**
- `.github/workflows/ci-verify.yml`: 빌드/테스트
- `.github/workflows/vibekit-guard.yml`: Compose freshness + SKSE static checks + 생성 콘텐츠 drift 감지

### 6.8 테스트 인프라

| 영역 | 프레임워크 | 커버리지 |
|------|----------|---------|
| C++ static checks | GTest | 컴파일타임 assertion (20+ 파일) |
| C# Generator | xUnit | 로더/렌더러/러너/contract regression (9개 파일) |
| Papyrus 스크립트 | N/A | 테스트 프레임워크 없음 |
| 런타임 proc/trigger | N/A | 라이브 게임에서만 검증 가능 |
| PrismaUI | N/A | 런타임 검증만 |
| Serialization round-trip | N/A | 미테스트 |

---

## 7. Key Metrics

| 항목 | 수치 |
|------|------|
| C++ 총 LOC | ~20,200 |
| C++ 소스 파일 | 56 |
| C++ 헤더 파일 | 36 |
| C# Generator 파일 | ~15 |
| C# 테스트 파일 | 9 |
| 어픽스 총 수 | 143 (83 prefix + 60 suffix) |
| 룬워드 레시피 | 94 |
| 룬 종류 | 33 (El ~ Zod) |
| 직렬화 버전 | v7 (v1-v6 마이그레이션) |
| co-save 레코드 | 6종 (IAXF, IRST, RWRD, LRLD, LCLD, LSBG) |
| SKSE 이벤트 싱크 | 8종 |
| Action 타입 | 10종 |
| Trigger 타입 | 5종 |
| EventBridge 중첩 타입 | 50+ |
| Synthetic Runeword Style | 37종 |
| MCM 페이지 | 3 |
| UI 언어 모드 | 3 (EN, KO, Both) |

---

## 8. Design Strengths (강점)

1. **도메인 분할 완성도**: EventBridge가 Config(16파일)/Loot(17파일)/Triggers(3파일)/Actions(6파일)/Serialization(1파일)로 기능별 명확히 분리
2. **직렬화 마이그레이션 성숙도**: v1→v7 자동 마이그레이션, 하위 호환 유지
3. **Shuffle Bag**: D2식 공정한 어픽스 분배 (6개 독립 셔플백)
4. **Pity System**: Loot(3)/Rune(99)/Orb(39) 연속 실패 보정으로 사용자 경험 보장
5. **Generator 파이프라인**: Mutagen으로 ESP 코드 생성 → CK/xEdit 없이 재현 가능한 빌드
6. **방어 코딩**:
   - Duplicate hit window (100ms)
   - Proc depth guard (atomic 재진입 방지)
   - NonHostile first-hit gate
   - Per-target ICD (120ms)
   - Papyrus hit event stale window (5s)
   - Currency ledger rate limiting (30일 TTL)
7. **모듈 spec 조합**: 7개 JSON 모듈 독립 편집 → compose 스크립트로 단일 spec
8. **Bilingual i18n**: atomic 변수 기반 실시간 언어 전환, MCM 연동
9. **Validation Contract**: C#/C++ 간 런타임 shape 자동 검증
10. **MCM Leader Election**: FormID churn 대응, 중복 quest 방지

---

## 9. Potential Risks / Technical Debt

### Critical

| 리스크 | 상세 | 영향 |
|--------|------|------|
| **God Class** | `EventBridge.h` 734 LOC, 50+ 중첩 타입, 모든 상태 단일 클래스 보유 | 유지보수 난이도 상승, 컴파일 시간 증가 |
| **전역 Lock** | `_stateMutex` (recursive_mutex)로 거의 모든 상태 접근 보호 | 잠재적 병목, 데드락 리스크 |
| **C++ 런타임 테스트 부재** | Static checks만 있고, 로직 단위 테스트 없음 | Proc/trigger/serialization 회귀 감지 불가 |

### Moderate

| 리스크 | 상세 | 영향 |
|--------|------|------|
| `procChancePct` 이중 의미 | Prefix=proc 확률, Suffix=드롭 가중치 | 코드 가독성 저하, 신규 개발자 혼란 |
| Trap linear scan | `_traps` vector 전체 순회 | 트랩 수 증가 시 프레임 드롭 가능 |
| 룬워드 파일 복잡도 | 17개 `.cpp` 파일로 분산 | 새 기능 추가 시 학습 곡선 |
| Papyrus 테스트 불가 | 게임 런타임에서만 검증 | MCM/모드 이벤트 관련 버그 조기 발견 불가 |
| Serialization round-trip 미테스트 | Save→Load 사이클 자동 검증 없음 | 마이그레이션 코드 변경 시 데이터 손실 리스크 |

### Low

| 리스크 | 상세 |
|--------|------|
| InventoryInjector 미사용 | 빈 rules 배열 — 향후 활용 또는 제거 결정 필요 |
| `_dotCooldowns` max 4096 | 극단적 DoT 스팸 시 캐시 포화 가능 (실제 발생 확률 매우 낮음) |

---

## 10. Recommendations (개선 권고)

### 단기 (Low Effort, High Impact)

1. **`procChancePct` 리네이밍**: `prefixProcChancePct` / `suffixLootWeight`로 분리하여 의미 명확화
2. **Serialization round-trip 테스트**: Save→Load 사이클 자동 검증 GTest 추가
3. **Runtime contract CI 강화**: CI에서 `runtime_contract.json` ↔ C++ ActionType enum 자동 비교

### 중기 (Moderate Effort)

4. **EventBridge 분할**: Config/Loot/Combat/Serialization을 별도 클래스/네임스페이스로 분리, EventBridge는 facade 역할
5. **C++ unit test 확대**: GTest + mock SKSE interface로 핵심 로직 테스트 (rolling, penalty, tooltip format)
6. **Trap spatial hash**: 선형 탐색 대신 공간 해시로 트랩 조회 성능 개선

### 장기 (High Effort)

7. **Lock 세분화**: `_stateMutex` 하나 대신 도메인별 mutex (config_mutex, loot_mutex, combat_mutex)
8. **InventoryInjector 결정**: 활용하거나 Generator에서 제거
9. **통합 테스트 하네스**: SKSE mock runtime으로 이벤트→proc→serialization E2E 테스트

---

## 부록: 주요 파일 경로

```
C++ Core:
  include/CalamityAffixes/EventBridge.h                    (734 LOC)
  include/CalamityAffixes/InstanceAffixSlots.h             (109 LOC)
  src/EventBridge.Config.*.cpp                             (16 files, ~3,100 LOC)
  src/EventBridge.Loot.*.cpp                               (17 files, ~4,400 LOC)
  src/EventBridge.Triggers.*.cpp                           (3 files, ~1,570 LOC)
  src/EventBridge.Actions.*.cpp                            (6 files, ~1,600 LOC)
  src/EventBridge.Serialization.cpp                        (1,073 LOC)
  src/Hooks.cpp                                            (576 LOC)
  src/PrismaTooltip.cpp                                    (891 LOC)
  src/main.cpp                                             (131 LOC)

C# Generator:
  tools/CalamityAffixes.Generator/Spec/AffixSpec.cs        (~357 LOC)
  tools/CalamityAffixes.Generator/Spec/AffixSpecLoader.cs  (~500 LOC)
  tools/CalamityAffixes.Generator/Writers/KeywordPluginBuilder.cs  (~512 LOC)
  tools/CalamityAffixes.Generator/GeneratorRunner.cs       (~85 LOC)

Data:
  affixes/affixes.json                                     (6,057 LOC)
  affixes/affixes.modules.json                             (모듈 매니페스트)
  affixes/runeword.contract.json                           (레시피 카탈로그)
  tools/affix_validation_contract.json                     (검증 contract)

UI & Config:
  Data/MCM/Config/CalamityAffixes/config.json              (256 LOC)
  Data/MCM/Config/CalamityAffixes/settings.ini
  Data/PrismaUI/views/CalamityAffixes/index.html

Papyrus:
  Data/Scripts/Source/CalamityAffixes_MCMConfig.psc
  Data/Scripts/Source/CalamityAffixes_ModeControl.psc
  Data/Scripts/Source/CalamityAffixes_AffixManager.psc
```
