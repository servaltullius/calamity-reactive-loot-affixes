# Calamity - Reactive Loot & Affixes

Player-centric ARPG-style instance affix mod for Skyrim SE/AE

---

## 한국어

### 모드 소개
Calamity - Reactive Loot & Affixes는 Skyrim SE/AE용 플레이어 중심 ARPG 스타일 어픽스 모드입니다.
아이템 인스턴스(ExtraUniqueID) 단위로 상태를 추적하며, 기본 정책은 자동 어픽스 드랍이 아니라 **재련(Reforge) 중심**입니다.

### 핵심 기능

#### 다중 어픽스 시스템
- 기본 자동 어픽스 부여 없이, 재련 경로에서 일반 어픽스를 부여/재롤
- 슬롯 모델: **룬워드 1 + 일반 어픽스 최대 3**
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
- 완성 룬워드 장비도 재련 가능하며, 재련 시 룬워드 1개 + 일반 어픽스 최대 3개를 함께 재롤
- Prisma UI에서 진행 상태 확인 가능

#### UI & 설정
- **Prisma UI** 기반 툴팁 및 조작 패널
- **MCM** 설정 패널 (런타임 토글, 핫키, UI 언어, 디버그 등)
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
5. (선택, 권장) 모드 추가 적 드랍까지 반영하려면 `CalamityAffixes_UserPatch.esp`를 생성해 함께 활성화

#### 사용자 환경별 드랍 패치(UserPatch)
- 목적: 각 유저의 실제 로드오더에 맞춰 **모드 추가 적 드랍 리스트(DeathItem\* + NPC DeathItem 참조)**에 룬워드 조각/재련 오브 드랍을 주입
- 가장 쉬운 방법(Windows): `tools\build_user_patch_wizard.cmd` 실행 후 선택 창에서 경로만 지정
- 위저드는 기본 경로를 자동 탐지하며, MO2가 감지되면 `ModOrganizer.ini`의 `selected_profile` 기준 프로필의 `loadorder.txt`(또는 `plugins.txt`)를 우선 선택
- 배포 ZIP 위저드는 동봉된 `CalamityAffixes.UserPatch.exe` + `affixes/affixes.json`을 자동 사용 (소스 프로젝트 경로 불필요)
- 생성되는 `CalamityAffixes_UserPatch.esp`는 ESL 플래그(ESPFE)로 기록되어 풀 ESP 슬롯 소모를 줄임
- `CalamityAffixes.UserPatch.exe`는 CLI 인자 기반이므로, 일반 사용자는 `build_user_patch_wizard.cmd` 실행 권장
- 생성 명령(예시):

```bash
dotnet run --project tools/CalamityAffixes.UserPatch -- \
  --spec affixes/affixes.json \
  --masters "C:\Path\To\Skyrim Data" \
  --loadorder "C:\Users\<User>\AppData\Local\Skyrim Special Edition\plugins.txt" \
  --output Data/CalamityAffixes_UserPatch.esp
```

- Synthesis 호환 인자: `-d`(DataFolderPath), `-l`(LoadOrderFilePath), `-o`(OutputPath)

#### UserPatch 트러블슈팅
- `No arguments were provided`:
  - `CalamityAffixes.UserPatch.exe`를 직접 실행한 상태입니다. `build_user_patch_wizard.cmd`로 실행하세요.
- `MSB1009: Project file does not exist`:
  - 구버전 패키지 또는 소스 경로 전제 실행일 때 주로 발생합니다. `v1.2.17-rc59+` ZIP으로 업데이트하세요.
- 특정 모드만 반영되거나 대상이 적게 나옴:
  - 위저드에서 **현재 MO2 프로필**의 `loadorder.txt`/`plugins.txt`를 선택했는지 확인하세요.
  - 완료 로그의 `TargetsByMod`에서 모드별 적용 개수를 확인할 수 있습니다.

### 주의사항

- Prisma UI가 없으면 툴팁/패널 UI가 표시되지 않습니다.
- KID DoT 태그를 과도하게 넓게 분배하면 부작용이 발생할 수 있습니다.

---

## English

### Overview
Calamity - Reactive Loot & Affixes is a player-centric ARPG-style affix mod for Skyrim SE/AE.
The mod tracks item instances via ExtraUniqueID and currently uses a **Reforge-centric** model rather than auto-rolling affixes on pickup.

### Core Features

#### Multi-Affix System
- No default auto-affix assignment on pickup; affixes are granted/rerolled via Reforge
- Slot model: **1 runeword + up to 3 regular affixes**
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
- Completed runeword items can also be reforged; reforge rerolls 1 runeword + up to 3 regular affixes
- Track progress via Prisma UI panel

#### UI & Settings
- **Prisma UI** tooltip and control panel
- **MCM** settings panel (runtime toggles, hotkeys, UI language, debug, etc.)
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
5. (Optional, recommended) generate and enable `CalamityAffixes_UserPatch.esp` for mod-added enemy drops

#### Per-user drop patch (UserPatch)
- Goal: inject runeword fragment/reforge orb drops into **mod-added enemy drop lists (DeathItem\* + NPC DeathItem references)** based on each user's active load order.
- Easiest (Windows): run `tools\build_user_patch_wizard.cmd` and choose paths in the dialogs.
- The wizard auto-fills defaults and, when MO2 is detected, prioritizes the selected profile from `ModOrganizer.ini` (`profiles/<selected_profile>/loadorder.txt` or `plugins.txt`).
- In the release ZIP, the wizard automatically uses bundled `CalamityAffixes.UserPatch.exe` + `affixes/affixes.json` (no source project path needed).
- The generated `CalamityAffixes_UserPatch.esp` is written as ESL-flagged (ESPFE) to reduce full ESP slot usage.
- `CalamityAffixes.UserPatch.exe` is a CLI-argument tool; most users should start from `build_user_patch_wizard.cmd`.
- Example:

```bash
dotnet run --project tools/CalamityAffixes.UserPatch -- \
  --spec affixes/affixes.json \
  --masters "C:\Path\To\Skyrim Data" \
  --loadorder "C:\Users\<User>\AppData\Local\Skyrim Special Edition\plugins.txt" \
  --output Data/CalamityAffixes_UserPatch.esp
```

- Synthesis-compatible aliases: `-d` (DataFolderPath), `-l` (LoadOrderFilePath), `-o` (OutputPath)

#### UserPatch Troubleshooting
- `No arguments were provided`:
  - You launched `CalamityAffixes.UserPatch.exe` directly. Start from `build_user_patch_wizard.cmd`.
- `MSB1009: Project file does not exist`:
  - Usually an old package or source-path-based launch. Update to `v1.2.17-rc59+` ZIP.
- Patch seems to include only one mod / too few targets:
  - Ensure the wizard selected the **active MO2 profile** `loadorder.txt`/`plugins.txt`.
  - Check the completion log line `TargetsByMod` for per-mod counts.

### Notes

- Without Prisma UI, tooltip/control panel UI will not be shown.
- Overly broad KID DoT tagging may cause side effects.
