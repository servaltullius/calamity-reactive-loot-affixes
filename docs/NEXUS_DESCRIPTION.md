# Calamity - Reactive Loot & Affixes

Player-centric ARPG-style instance affix mod for Skyrim SE/AE

---

## 한국어

### 모드 소개
Calamity - Reactive Loot & Affixes는 Skyrim SE/AE용 플레이어 중심 ARPG 스타일 어픽스 모드입니다.
아이템 인스턴스(ExtraUniqueID) 단위로 상태를 추적하며, 현재 버전(v1.2.17) 기본 정책은 **재련(Reforge) 중심**입니다.

### 현재 버전 핵심 정책 (v1.2.17)
- 아이템 획득/제작 시 **자동 어픽스 부여 없음**
- **재련 오브(Reforge Orb)** 사용 시 선택 장비에 어픽스를 부여/재롤
- 슬롯 모델: **룬워드 1 + 일반 어픽스 최대 3**
- 이름 마커(★ 계열) + Prisma 툴팁으로 인스턴스 상태 확인

### 룬워드
- Diablo 2 스타일 **94개 레시피**
- 룬워드 조각 수집 -> 레시피 완성 -> 장비 적용
- 완성 룬워드 장비도 재련 가능 (룬워드 + 일반 어픽스 동시 재롤)

### 드랍 정책 (현재)
- 드랍 모드: **`hybrid` 고정**
- 시체: **SPID DeathItem 분배 우선**
- 월드/컨테이너: **SKSE 런타임 롤**
- 기본 확률: 룬워드 조각 `5%`, 재련 오브 `3%`
- MCM에서 확률 조정 가능 (런타임 적용)

### 전투 시스템
- Proc 발동 + ICD(내부 쿨다운) + 중복 히트 방지
- 다중 어픽스 Proc 밸런스(Best Slot Wins): 1어픽스=100%, 2어픽스=80%, 3어픽스=65%
- 크리티컬 히트 추가 증폭(SKSE 훅 기반)

### UI & 설정
- Prisma UI 기반 툴팁/조작 패널
- MCM 설정 패널(확률, 핫키, 언어, 런타임 옵션)
- 한국어/영어 전환 지원
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
- SPID (권장)
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
- 기본 배포 ZIP에는 UserPatch 도구를 포함하지 않습니다.

---

## English

### Overview
Calamity - Reactive Loot & Affixes is a player-centric ARPG-style affix mod for Skyrim SE/AE.
It tracks item instances via ExtraUniqueID, and the current stable version (v1.2.17) is **Reforge-centric**.

### Current Core Policy (v1.2.17)
- **No automatic affix assignment** on loot/craft
- Use **Reforge Orb** to grant/reroll affixes on selected gear
- Slot model: **1 runeword + up to 3 regular affixes**
- Star markers (★ series) + Prisma tooltip for instance readability

### Runewords
- **94 Diablo 2-style recipes**
- Collect runeword fragments -> complete recipe -> apply to equipment
- Completed runeword gear can also be reforged (runeword + regular affixes reroll together)

### Drop Policy (Current)
- Drop mode: **`hybrid` only**
- Corpses: **SPID DeathItem distribution first**
- Containers/world pickups: **SKSE runtime roll**
- Default rates: runeword fragment `5%`, reforge orb `3%`
- Rates are adjustable via MCM (applies at runtime)

### Combat System
- Proc chance + ICD (internal cooldown) + duplicate-hit protection
- Multi-affix proc balancing (Best Slot Wins): 1 affix=100%, 2 affixes=80%, 3 affixes=65%
- Critical hit amplification via SKSE hook

### UI & Settings
- Prisma UI tooltip/control panel
- MCM settings (rates, hotkeys, language, runtime options)
- Korean / English language switch
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
- SPID (recommended)
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
- UserPatch tools are not bundled in the default release ZIP.
