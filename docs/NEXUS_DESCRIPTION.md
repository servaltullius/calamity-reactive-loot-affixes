# Calamity - Reactive Loot & Affixes

Player-centric ARPG-style instance affix mod for Skyrim SE/AE

---

## 한국어

### 모드 소개
Calamity - Reactive Loot & Affixes는 Skyrim SE/AE용 플레이어 중심 ARPG 스타일 어픽스 모드입니다.
아이템 인스턴스(ExtraUniqueID) 단위로 상태를 추적하며, 현재 빌드 기본 정책은 **재련(Reforge) 중심**입니다.

### 현재 빌드 핵심 정책
- 아이템 획득/제작 시 **자동 어픽스 부여 없음**
- **재련 오브(Reforge Orb)** 사용 시 선택 장비에 어픽스를 부여/재롤
- 슬롯 모델: **룬워드 1 + 일반 어픽스 최대 3**
- 이름 마커(★ 계열) + Prisma 툴팁으로 인스턴스 상태 확인

### 룬워드
- Diablo 2 스타일 **94개 레시피**
- 룬워드 조각 수집 -> 레시피 완성 -> 장비 적용
- 완성 룬워드 장비 재련 시 룬워드 보존, 일반 어픽스만 재롤
- 룬워드 재변환: 기존 룬워드를 새 룬워드로 교체 가능
- 효과 구성: **94개 전부 JSON 개별 정의** (스카이림 마법 아키타입 + 다양한 actorValue 활용)
- Adaptive 계열은 기본 자동 선택을 유지하고, `modeCycle`이 있는 효과는 **수동 오버라이드 모드**를 지원
- 룬워드 패널에서 선택 레시피의 **효과/권장 베이스/룬 순서/상세**를 한국어/영어 설정에 맞춰 선택 상태·검색·hover에서 확인 가능

### 드랍 정책 (현재)
- 설정의 **`hybrid`는 구버전 호환 토큰**이며, 실제 판정 권한은 SKSE death event의 eligible hostile corpse-only 경로입니다.
- 플레이어 또는 player-owned summon/proxy가 처치한 적대 대상의 시체 인벤토리에 성공한 통화를 조용히 직접 추가합니다.
- 피해자가 팔로워/동료, 소환·지휘 액터, 아동, player-owned/비적대 대상이면 제외합니다. 환경 오브젝트와 player-owned가 아닌 독립 NPC/팔로워의 처치도 제외합니다.
- **일반 상자/컨테이너 활성화, 픽업, 월드 생성, 새 SPID 통화 분배는 모두 없습니다.**
- 기본 확률: 룬워드 조각 `8%`, 재련 오브 `5%` (MCM 변경은 다음 적격 사망부터 반영)
- 룬 가중치: `El-Amn=4`, `Sol-Um=3`, `Mal-Lo=2`, `Sur-Zod=1` (최대 `4:1`)
- 룬 조각 99회 연속 실패 피티를 유지하며, 피티와 시체별 중복 방지 ledger를 `CCRT` 코세이브 레코드에 저장합니다.
- 업데이트 전 SPID 통화가 남은 전환 시체는 룬 조각/재련 오브 카테고리별로 중복 판정을 건너뜁니다.
- SPID는 필요하지 않으며 동봉 `CalamityAffixes_DISTR.ini`는 빈 호환 산출물입니다.

### 전투 시스템
- Proc 발동 + ICD(내부 쿨다운) + 중복 히트 방지
- 다중 어픽스 Proc 밸런스(Best Slot Wins): 1어픽스=100%, 2어픽스=80%, 3어픽스=65%
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
- KID (권장)
- MCM Helper (권장)
- I4 (선택)

### 설치 방법 (MO2 권장)
1. Main File 다운로드
2. MO2로 설치
3. `CalamityAffixes.esp` 활성화
4. SKSE로 실행

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
It tracks item instances via ExtraUniqueID, and the current build is **Reforge-centric**.

### Current Core Policy
- **No automatic affix assignment** on loot/craft
- Use **Reforge Orb** to grant/reroll affixes on selected gear
- Slot model: **1 runeword + up to 3 regular affixes**
- Star markers (★ series) + Prisma tooltip for instance readability

### Runewords
- **94 Diablo 2-style recipes**
- Collect runeword fragments -> complete recipe -> apply to equipment
- Reforging completed runeword gear preserves the runeword; only regular affixes are rerolled
- Runeword re-transmutation: an existing runeword can be replaced with a new one
- Effect composition: **all 94 runewords individually defined in JSON** (Skyrim magic archetypes + diverse actorValues)
- Adaptive effects keep auto element selection by default, and `modeCycle` entries support **manual override mode**
- The runeword panel shows bilingual **effect/base/rune-order/detail** consistently in selection, search, and hover states

### Drop Policy (Current)
- **`hybrid` is now only a legacy config token**; the actual authority is the SKSE death-event eligible-hostile-corpse-only path.
- Successful currency is inserted silently and directly into a hostile corpse killed by the player or a player-owned summon/proxy.
- Followers/teammates, summoned or commanded victims, children, player-owned/non-hostile victims, environmental-object kills, and kills by independent non-player-owned NPCs/followers are excluded.
- **No generic container activation, pickup roll, world spawn, or new SPID currency distribution.**
- Default rates: runeword fragment `8%`, reforge orb `5%` (MCM changes apply to the next eligible death)
- Rune weights: `El-Amn=4`, `Sol-Um=3`, `Mal-Lo=2`, `Sur-Zod=1` (maximum `4:1`)
- The 99-failure rune-fragment pity remains; pity and the per-corpse duplicate ledger are saved in the `CCRT` co-save record.
- Transition corpses carrying old SPID currency skip duplicate rolls per fragment/orb category.
- SPID is not required; the bundled `CalamityAffixes_DISTR.ini` is an empty compatibility artifact.

### Combat System
- Proc chance + ICD (internal cooldown) + duplicate-hit protection
- Multi-affix proc balancing (Best Slot Wins): 1 affix=100%, 2 affixes=80%, 3 affixes=65%
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
- KID (recommended)
- MCM Helper (recommended)
- I4 (optional)

### Installation (MO2 recommended)
1. Download the Main File
2. Install with MO2
3. Enable `CalamityAffixes.esp`
4. Launch via SKSE

### Notes
- Without Prisma UI, tooltip/control panel UI will not be shown.
- Overly broad KID DoT tagging may cause side effects.
- Leveled-list (LVLI) injection/override is not used.
- If a new version is enabled as a separate MO2 mod, conflict priority may let the older `CalamityAffixes_DISTR.ini` override the new empty compatibility file. Replace/overwrite the old mod or disable the old DISTR file.
- Existing gear, currencies, completed runewords, and prior co-save records remain compatible; no new game is required.
