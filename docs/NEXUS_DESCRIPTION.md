# Calamity - Reactive Loot & Affixes

Player-centric ARPG-style instance affix mod for Skyrim SE/AE

---

## 한국어

### 모드 소개
Calamity - Reactive Loot & Affixes는 Skyrim SE/AE용 플레이어 중심 ARPG 스타일 어픽스 모드입니다.
아이템 인스턴스(ExtraUniqueID) 단위로 상태를 추적하며, 장비는 **확인 스크롤 → 선택 재련 → 정제**의 제작 흐름으로 성장시킵니다.

### 시작하기: 패널 열기
- 기본 키는 **F11**입니다. 월드와 인벤토리 어디서든 열리고, 다시 누르거나 ESC·닫기로 닫습니다.
- 키 바꾸기: ESC > 시스템 > 모드 설정 > Calamity Affixes > 단축키·디버그 > 칼라미티 패널 토글. 메뉴를 닫는 순간 적용되며 SkyUI와 MCM Helper가 필요합니다.
- Calamity 아이템 툴팁 아래에 현재 키가 표시됩니다. 게임패드 버튼으로는 열 수 없습니다.
- 반응이 없으면 다른 모드가 F11을 쓰는지 확인하고, MCM > 프리즈마 UI > 패널 토글(버튼)과 Prisma UI 상태 확인을 사용하세요.

### 현재 빌드 핵심 정책
- 아이템 획득/제작 시 **자동 어픽스 부여 없음**
- **확인 스크롤**: 일반 어픽스가 없는 장비에 1~3개 부여(60%/30%/10%)
- **재련 오브**: 고른 어픽스 하나만 교체(2개, 장비마다 6회), 슬롯 확장(1→2: 2개, 2→3: 4개)
- **정제 오브**: 슬롯 수를 유지한 채 일반 어픽스 전부를 한 번에 다시 굴리고 선택 재련 6회를 되돌림
- 남는 확인 스크롤은 재련 오브(3:1)나 정제 오브(10:1)로 교환(한 방향)
- 모든 제작 작업은 완성 룬워드와 그 성장 상태를 보존
- 슬롯 모델: **3칸 — 머리 칸(룬워드 또는 접두) 1 + 접미 최대 2**
- 이름 마커(★ 계열) + Prisma 툴팁으로 인스턴스 상태 확인

### 룬워드
- Diablo 2 스타일 **94개 레시피**
- 룬워드 조각 수집 -> 레시피 완성 -> 장비 적용
- 확인·선택 재련·정제 모두 완성 룬워드를 보존하고 일반 어픽스만 바꿈
- 룬워드 재변환: 기존 룬워드를 새 룬워드로 교체 가능
- 효과 구성: **94개 전부 JSON 개별 정의** (스카이림 마법 아키타입 + 다양한 actorValue 활용)
- Adaptive 계열은 기본 자동 선택을 유지하고, `modeCycle`이 있는 효과는 **수동 오버라이드 모드**를 지원
- 룬워드 패널에서 선택 레시피의 **효과/권장 베이스/룬 순서/상세**를 한국어/영어 설정에 맞춰 선택 상태·검색·hover에서 확인 가능

### 드랍 정책 (현재)
- 설정의 **`hybrid`는 구버전 호환 토큰**이며, 실제 판정 권한은 SKSE death event의 eligible hostile corpse-only 경로입니다.
- 플레이어 또는 player-owned summon/proxy가 처치한 적대 대상의 시체 인벤토리에 성공한 통화를 조용히 직접 추가합니다.
- 피해자가 팔로워/동료, 소환·지휘 액터, 아동, player-owned/비적대 대상이면 제외합니다. 환경 오브젝트와 player-owned가 아닌 독립 NPC/팔로워의 처치도 제외합니다.
- **일반 상자/컨테이너 활성화, 픽업, 월드 생성, 새 SPID 통화 분배는 모두 없습니다.**
- 일반 적 기본 확률: 룬워드 조각 `8%`, 재련 오브 `12%` (MCM 변경은 다음 적격 일반 적 사망부터 반영)
- 확인 스크롤 `20%`, 정제 오브 `4%`는 별도 독립 판정(MCM 조정 가능, 보스 확정·피티 없음)
- `Unique` 고유·네임드 적: 룬워드 조각 `40%` / 재련 오브 `60%` 중 1개 확정
- `LocRefTypeBoss` 보스 또는 `ActorTypeDragon` 드래곤: 룬워드 조각 1개 + 재련 오브 1개 확정 (`Boss/Dragon`이 `Unique`보다 우선)
- 고유·보스 확정 보상은 일반 확률 판정과 피티를 소비하거나 초기화하지 않음
- 룬 가중치: `El-Amn=4`, `Sol-Um=3`, `Mal-Lo=2`, `Sur-Zod=1` (최대 `4:1`)
- 룬 조각 99회 연속 실패 피티를 유지하며, 피티와 시체별 중복 방지 ledger를 `CCRT` 코세이브 레코드에 저장합니다.
- 업데이트 전 SPID 통화가 남은 전환 시체는 룬 조각/재련 오브 카테고리별로 중복 판정을 건너뜁니다.
- SPID는 필요하지 않으며 동봉 `CalamityAffixes_DISTR.ini`는 빈 호환 산출물입니다.

### 전투 시스템
- Proc 발동 + ICD(내부 쿨다운) + 중복 히트 방지
- 다중 어픽스 Proc 밸런스(Best Slot Wins): 아이템의 발동형 어픽스 수 기준 — 1개=100%, 2개=80%, 3개=65%, 4개=50%. 패시브(접미사) 효과는 단계 계산에 포함되지 않습니다.
- 크리티컬 히트 추가 증폭(SKSE 훅 기반)

### UI & 설정
- Prisma UI 기반 툴팁/조작 패널
- MCM 설정 패널(확률, 핫키, 언어, 런타임 옵션)
- 한국어/영어 전환 지원
- 레시피 효과·권장 베이스·룬 순서·상세의 이중 언어 표시
- viewport 경계 제한 + wheel/scrollbar/키보드 스크롤 + ARIA 접근성
- `user_settings.json` 기반 설정 영속화

### 적용 범위 (중요)
현재 런타임은 플레이어 중심입니다.
- 플레이어 장비/인벤토리 기준 동작
- 전투 트리거는 플레이어 + 플레이어 지휘 소환체(player-owned summon/proxy) 지원
- 일반 팔로워/NPC 장비를 독립 어픽스 소유자로 추적/운영하는 기능은 미지원

### 필수 의존성
- SKSE64
- Address Library for SKSE Plugins
- Prisma UI

### 권장/선택 의존성
- SkyUI (권장)
- powerofthree's Tweaks (권장. 2.2.7 이하에서는 사실상 필수, 2.2.8부터는 없어도 동작하며 있으면 다른 모드 방어구 제외 목록·보스 상자 이름 판정까지 적용)
- KID (권장)
- MCM Helper (권장)
- I4 (선택)

### 설치 방법 (MO2 권장)
1. Main File 다운로드
2. MO2로 설치
3. `CalamityAffixes.esp` 활성화
4. SKSE로 실행

### 전체 효과 목록
- [접두](PREFIX_EFFECTS.md) · [접미](SUFFIX_EFFECTS.md) · [룬워드](RUNEWORD_EFFECTS.md)

### 주의사항
- Prisma UI가 없으면 툴팁/패널 UI가 표시되지 않습니다.
- KID DoT 태그를 과도하게 넓게 분배하면 부작용이 발생할 수 있습니다.
- 레벨리스트(LVLI) 주입/오버라이드는 사용하지 않습니다.
- 새 버전을 MO2에서 별도 모드로 겹쳐 켜면 충돌 우선순위에 따라 구 `CalamityAffixes_DISTR.ini`가 새 빈 파일보다 우선할 수 있습니다. 기존 모드를 교체/덮어쓰거나 구 DISTR를 비활성화하세요.
- 기존 장비, 보유 통화, 완성 룬워드와 기존 코세이브 레코드는 호환되며 새 게임이 필요하지 않습니다.

---

## English

### Overview
Calamity - Reactive Loot & Affixes is a player-centric ARPG-style affix mod for Skyrim SE/AE.
It tracks item instances via ExtraUniqueID, and gear grows through a **Scroll of Identification → selected reforge → scouring** crafting flow.

### Getting Started: Open the Panel
- The default key is **F11**. It works in the world and in your inventory; press it again, ESC, or Close to close the panel.
- Change the key: ESC > System > Mod Configuration > Calamity Affixes > Hotkey & Debug > Toggle Calamity Panel. It applies when you close the menu and needs SkyUI and MCM Helper.
- Your current key is shown at the bottom of the Calamity item tooltip. Gamepad buttons cannot open the panel.
- Nothing happens? Check whether another mod uses F11, then try MCM > Prisma UI > Toggle Panel (Button) and Check Prisma UI Status.

### Current Core Policy
- **No automatic affix assignment** on loot/craft
- **Scroll of Identification**: grants 1-3 regular affixes to gear without any (60%/30%/10%)
- **Reforge Orb**: replaces one chosen affix (2 orbs, 6 times per item) and expands slots (1→2: 2 orbs, 2→3: 4 orbs)
- **Scouring Orb**: rerolls every regular affix at once while keeping the slot count, and gives the item its 6 selected reforges back
- Surplus Identify Scrolls trade one way for Reforge Orbs (3:1) or Scouring Orbs (10:1)
- Every crafting action preserves a completed runeword and its growth state
- Slot model: **3 slots — a head slot (runeword or prefix) + up to 2 suffixes**
- Star markers (★ series) + Prisma tooltip for instance readability

### Runewords
- **94 Diablo 2-style recipes**
- Collect runeword fragments -> complete recipe -> apply to equipment
- Identify, selected reforge, and scouring all preserve a completed runeword; only regular affixes change
- Runeword re-transmutation: an existing runeword can be replaced with a new one
- Effect composition: **all 94 runewords individually defined in JSON** (Skyrim magic archetypes + diverse actorValues)
- Adaptive effects keep auto element selection by default, and `modeCycle` entries support **manual override mode**
- The runeword panel shows bilingual **effect/base/rune-order/detail** consistently in selection, search, and hover states

### Drop Policy (Current)
- **`hybrid` is now only a legacy config token**; the actual authority is the SKSE death-event eligible-hostile-corpse-only path.
- Successful currency is inserted silently and directly into a hostile corpse killed by the player or a player-owned summon/proxy.
- Followers/teammates, summoned or commanded victims, children, player-owned/non-hostile victims, environmental-object kills, and kills by independent non-player-owned NPCs/followers are excluded.
- **No generic container activation, pickup roll, world spawn, or new SPID currency distribution.**
- Normal-enemy rates: runeword fragment `8%`, reforge orb `12%` (MCM changes apply to the next eligible normal-enemy death)
- Scroll of Identification `20%` and Scouring Orb `4%` roll independently (MCM-adjustable; no boss guarantee or pity)
- Unique/named actors: one guaranteed currency reward, selected as `40%` fragment / `60%` reforge orb
- `LocRefTypeBoss` actors and `ActorTypeDragon` dragons: one guaranteed fragment plus one guaranteed reforge orb (`Boss/Dragon` overrides `Unique`)
- Unique/boss guarantees neither run additional normal rolls nor advance/reset normal pity
- Rune weights: `El-Amn=4`, `Sol-Um=3`, `Mal-Lo=2`, `Sur-Zod=1` (maximum `4:1`)
- The 99-failure rune-fragment pity remains; pity and the per-corpse duplicate ledger are saved in the `CCRT` co-save record.
- Transition corpses carrying old SPID currency skip duplicate rolls per fragment/orb category.
- SPID is not required; the bundled `CalamityAffixes_DISTR.ini` is an empty compatibility artifact.

### Combat System
- Proc chance + ICD (internal cooldown) + duplicate-hit protection
- Multi-affix proc balancing (Best Slot Wins): tiered by the item's proc affix count — 1=100%, 2=80%, 3=65%, 4=50%. Passive suffix effects never raise the tier.
- Critical hit amplification via SKSE hook

### UI & Settings
- Prisma UI tooltip/control panel
- MCM settings (rates, hotkeys, language, runtime options)
- Korean / English language switch
- Bilingual recipe effect/base/rune-order/detail in selection, search, and hover
- Viewport clamping + wheel/scrollbar/keyboard scrolling + ARIA accessibility
- Persistent settings via `user_settings.json`

### Scope (Important)
Current runtime scope is player-centric.
- Based on player inventory/equipment
- Combat triggers support player + player-owned summons/proxies
- Independent affix ownership/runtime for regular followers/NPC equipment is not supported

### Required Dependencies
- SKSE64
- Address Library for SKSE Plugins
- Prisma UI

### Recommended / Optional
- SkyUI (recommended)
- powerofthree's Tweaks (recommended. Effectively required up to 2.2.7; from 2.2.8 the mod works without it, and with it the armor deny list and boss-chest name checks also apply to other mods' forms)
- KID (recommended)
- MCM Helper (recommended)
- I4 (optional)

### Installation (MO2 recommended)
1. Download the Main File
2. Install with MO2
3. Enable `CalamityAffixes.esp`
4. Launch via SKSE

### Full Effect List
- [Every prefix, suffix, and runeword in its in-game English text](EFFECTS_EN.md)

### Notes
- Without Prisma UI, tooltip/control panel UI will not be shown.
- Overly broad KID DoT tagging may cause side effects.
- Leveled-list (LVLI) injection/override is not used.
- If a new version is enabled as a separate MO2 mod, conflict priority may let the older `CalamityAffixes_DISTR.ini` override the new empty compatibility file. Replace/overwrite the old mod or disable the old DISTR file.
- Existing gear, currencies, completed runewords, and prior co-save records remain compatible; no new game is required.
