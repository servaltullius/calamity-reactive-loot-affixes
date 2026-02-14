# Calamity - Reactive Loot & Affixes

Player-centric ARPG-style instance affix mod for Skyrim SE/AE

---

## 한국어

### 모드 소개
Calamity - Reactive Loot & Affixes는 Skyrim SE/AE용 플레이어 중심 ARPG 스타일 어픽스 모드입니다.
아이템을 획득하거나 제작해 플레이어 인벤토리로 들어오는 순간, 아이템 인스턴스(ExtraUniqueID) 기준으로 **1~3개의 어픽스**를 롤링합니다.

### 핵심 기능

#### 다중 어픽스 시스템
- 아이템당 **1~3개** 어픽스 부여 (70% / 25% / 5%)
- 아이템 이름에 **★ / ★★ / ★★★** 표시로 어픽스 수 구분
- **Prefix (57개)**: 전투 중 확률 발동(Proc) 효과 — Firestorm, Thunderbolt, Soul Trap 등
- **Suffix (60개)**: 장비 착용 시 상시 적용되는 패시브 스탯 보너스 — 체력 +50, 화염 저항 +10% 등
  - 3단계 티어: Minor (60%) / Major (30%) / Grand (10%)
  - 20개 패밀리 (Health, Magicka, Stamina, 각종 저항, 크리티컬 데미지 등)

#### 전투 시스템
- Proc 발동 + ICD(내부 쿨다운) + 중복 히트 방지
- 다중 어픽스 Proc 밸런스 ("Best Slot Wins"): 1어픽스=100%, 2어픽스=80%, 3어픽스=65%
- 크리티컬 데미지 보너스: 크리티컬 히트 시 SKSE 훅을 통해 직접 데미지 증폭 (+10/20/30%)

#### 룬워드
- **94개 레시피**, Diablo 2 스타일 룬 조합 시스템
- 룬 프래그먼트 수집 → 레시피 완성 → 장비에 적용
- 적용 시 기존 어픽스를 교체 (ReplaceAll)
- Prisma UI에서 진행 상태 확인 가능

#### UI & 설정
- **Prisma UI** 기반 툴팁 및 조작 패널
- **MCM** 설정 패널 (드롭 확률, 핫키, UI 언어 등)
- **한국어 / 영어 이중언어** 지원 (MCM에서 전환)
- SKSE 코세이브 기반 인스턴스 상태 유지

### 적용 범위 (중요)

현재 런타임은 **플레이어 중심**입니다.
- 플레이어 장비/인벤토리 기준으로 동작합니다.
- 전투 트리거는 플레이어 본인 + 플레이어 지휘 소환체(player-owned summon/proxy)까지 지원합니다.
- 일반 팔로워/NPC 장비를 독립 어픽스 소유자로 추적/운영하는 기능은 현재 지원하지 않습니다.

### 필수 의존성

- SKSE64
- Address Library for SKSE Plugins
- Prisma UI

### 권장/선택 의존성

- SkyUI (권장)
- KID (권장)
- SPID (권장)
- MCM Helper (권장 — MCM 설정 패널 사용)
- I4 (선택)

### 설치 방법 (MO2 권장)

1. Main File 다운로드
2. MO2로 설치
3. CalamityAffixes.esp 활성화
4. SKSE로 실행

### 주의사항

- Prisma UI가 없으면 툴팁/패널 UI가 표시되지 않습니다.
- KID DoT 태그를 과도하게 넓게 분배하면 부작용이 발생할 수 있습니다.

---

## English

### Overview
Calamity - Reactive Loot & Affixes is a player-centric ARPG-style affix mod for Skyrim SE/AE.
When an item is looted or crafted and enters the player inventory, **1 to 3 affixes** are rolled per item instance (ExtraUniqueID).

### Core Features

#### Multi-Affix System
- **1 to 3 affixes** per item (70% / 25% / 5%)
- Items display **★ / ★★ / ★★★** prefix to indicate affix count
- **Prefix (57)**: Combat proc effects — Firestorm, Thunderbolt, Soul Trap, etc.
- **Suffix (60)**: Passive stat bonuses while equipped — Health +50, Fire Resist +10%, etc.
  - 3 tiers: Minor (60%) / Major (30%) / Grand (10%)
  - 20 families (Health, Magicka, Stamina, Resistances, Critical Damage, etc.)

#### Combat System
- Proc chance + ICD (internal cooldown) + duplicate-hit protection
- Multi-affix Proc balancing ("Best Slot Wins"): 1 affix=100%, 2 affixes=80%, 3 affixes=65%
- Critical damage bonus: Direct damage amplification on critical hits via SKSE hook (+10/20/30%)

#### Runewords
- **94 recipes**, Diablo 2-style rune combination system
- Collect rune fragments → complete recipe → apply to equipment
- Replaces all existing affixes on application (ReplaceAll)
- Track progress via Prisma UI panel

#### UI & Settings
- **Prisma UI** tooltip and control panel
- **MCM** settings panel (drop rate, hotkeys, UI language, etc.)
- **Korean / English bilingual** support (switchable in MCM)
- SKSE co-save based instance state persistence

### Scope (Important)

Current runtime scope is **player-centric**.
- Affix runtime is based on player inventory/equipment.
- Combat triggers support the player and player-owned summons/proxies.
- Independent affix ownership/runtime for regular followers/NPC equipment is not supported yet.

### Required Dependencies

- SKSE64
- Address Library for SKSE Plugins
- Prisma UI

### Recommended / Optional

- SkyUI (recommended)
- KID (recommended)
- SPID (recommended)
- MCM Helper (recommended — enables MCM settings panel)
- I4 (optional)

### Installation (MO2 recommended)

1. Download the Main File
2. Install with MO2
3. Enable CalamityAffixes.esp
4. Launch via SKSE

### Notes

- Without Prisma UI, tooltip/control panel UI will not be shown.
- Overly broad KID DoT tagging may cause side effects.
