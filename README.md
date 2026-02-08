# Calamity - Reactive Loot & Affixes

[![Latest Release](https://img.shields.io/github/v/release/servaltullius/calamity-reactive-loot-affixes?include_prereleases&label=Latest%20Release)](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases)
[![Download MO2 Zip](https://img.shields.io/badge/Download-MO2%20ZIP-2ea44f)](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/download/v0.1.0-beta/CalamityAffixes_MO2_2026-02-08.zip)

최신 릴리즈/다운로드: https://github.com/servaltullius/calamity-reactive-loot-affixes/releases

스카이림(SE/AE)용 “Diablo/PoE 스타일 어픽스 + 프로크(확률 발동) + ICD” 시스템을 만들기 위한 개발 레포입니다.

## 목표(요약)

- 아이템(무기/아머)에 **어픽스(확률 발동 효과)**를 붙이고
- 중앙 Papyrus 매니저가 **프로크/ICD/가드**를 처리하며
- (권장) SKSE 플러그인이 **모든 무기/마법/소환수/DoT 적용(=틱 X)** 트리거를 안정적으로 공급합니다.

## 어픽스 적용 방식(인스턴스 / Loot-time)

- SKSE 플러그인이 **플레이어 획득/제작 시점**에 아이템 **인스턴스(ExtraUniqueID)** 별로 어픽스를 롤링/저장(=리롤)
- 장기 세이브 안정성을 위해 인스턴스 어픽스는 SKSE 코세이브에 **문자열이 아닌 64-bit 토큰(FNV-1a)**로 저장합니다(v3). *(v1/v2 세이브는 로드 시 자동 마이그레이션)*
  - 따라서 `affix.id`는 **리네임 금지**(기존 세이브 매칭이 깨짐). 변경이 필요하면 “새 id 추가”로 버전업합니다.
- (안전한 프루닝) 플레이어가 월드에 **드랍한 아이템 레퍼런스가 삭제될 때만** 해당 인스턴스 엔트리를 정리해 코세이브 누적을 완화합니다(재획득 시엔 프루닝 안 함).
- (기본 UX) Prisma UI 오버레이로 **선택 중인 아이템의 어픽스 효과 설명을 표시**합니다. (**Prisma UI 필수**, Norden UI 등 UI 테마와 무관)
  - 아이템 메뉴(인벤/루팅/상점)에서 “선택된 아이템”이 어픽스 인스턴스면 표시되고, 아니면 자동으로 숨겨집니다.
- (기본값) `loot.renameItem=true` : 아이템 이름에 **짧은 어픽스 라벨**을 붙여(좌측 리스트) 빠르게 식별합니다.

기본값: `loot.chancePercent=25`, `loot.renameItem=true`, `loot.sharedPool=true` (무기/방어구 공용 풀 롤링)
추가 안전장치(권장): `loot.trapGlobalMaxActive=64` (전역 트랩 하드캡, 0=무제한)

## 제작 아이템(대장간/제작)에서는 어픽스가 어떻게 붙나요?

- 제작 완료로 생성된 아이템이 **플레이어 인벤토리로 들어오는 순간**(ContainerChanged)에도 동일하게 “loot-time 롤링”이 적용됩니다.
- 즉 “파밍/제작마다 리롤되는” ARPG식 인스턴스 어픽스를 의도합니다.

## 레포 구조

- `doc/` : 개발 명세서(컨셉/제약/설계)
- `doc/4.현재_구현_어픽스_목록.md` : 현재 구현된 효과 목록(한국어 요약)
- `doc/5.룬워드_94개_효과_정리.md` : 룬워드 94개 레시피별 효과 설명(직접 정의형 + 자동 합성형)
- `Data/` : 게임 `Data/`에 그대로 설치 가능한 “스테이징” 폴더 (MO2 테스트용)
  - `Data/CalamityAffixes.esp` : 키워드/스펠/MGEF + MCM Helper Quest(자동 생성, 단일 ESP)
  - `Data/Scripts/Source/` : Papyrus 소스(.psc)
    - `CalamityAffixes_MCMConfig.psc` : MCM Helper용 config 스크립트(Quest에 바인딩)
    - `CalamityAffixes_ModeControl.psc` : Prisma/MCM 공용 글로벌 브리지(수동 모드 + 룬워드 + Prisma 패널 토글)
  - `Data/Scripts/*.pex` : MCM/런타임에 필요한 컴파일 결과물(빌드 시 자동 생성)
  - `Data/MCM/Config/CalamityAffixes/` : MCM Helper 설정
    - `General / 일반` 페이지: 런타임 설정 + DotApply 안전 자동비활성 토글 + Prisma 패널 열기 토글 + 패널 언어(EN/KO/Both)
    - `keybinds.json`: `prisma_panel_toggle` 포함(Prisma 패널 빠른 토글)
    - MCM `Panel Language / 패널 언어`에서 Prisma 패널 텍스트를 영어/한국어/병행 표기로 전환
  - `Data/CalamityAffixes_KID.ini` : KID 템플릿(태그/옵션 분배)
  - `Data/CalamityAffixes_DISTR.ini` : SPID 템플릿(분배)
- `skse/CalamityAffixes/` : CommonLibSSE-NG 기반 SKSE 플러그인(CMake/vcpkg)

## 준비물(개발/테스트)

### 게임/런타임

- Skyrim SE 1.5.97 포함 지원(그리고 AE 1.6.x까지 확장 가능)
- SKSE64: 런타임에 맞는 버전 설치 (SE/AE 빌드가 분리됨) ([skse.silverlock.org](https://skse.silverlock.org/))
- Address Library for SKSE Plugins: 런타임(1.5.x vs 1.6.x)에 맞는 파일 설치 ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/32444))

### 의존성(기본 UX 기준)

- Prisma UI (**필수**, 툴팁 오버레이) ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/148718))
  - 뷰 파일 위치: `Data/PrismaUI/views/CalamityAffixes/index.html`
  - 메인 조작 UI(수동 모드/룬워드/디버그)도 Prisma 패널에서 처리
  - 인벤토리에서는 기본적으로 “툴팁은 클릭-스루(비상호작용)”입니다. 패널은 **단일 단축키**(`prisma_panel_toggle`, MCM에서 설정)로 열고/닫습니다. 키가 미설정이면 기본 `F11`로 토글됩니다(인벤 밖에서도 가능).
  - 패널 표시 언어는 MCM `Panel Language / 패널 언어` 옵션에서 즉시 변경할 수 있습니다.
  - 패널이 열리면 입력/커서는 패널이 잡아 “클릭-스루”로 인벤이 함께 눌리는 현상을 방지합니다(ESC 또는 Close로 닫기).
  - 패널이 열리면 툴팁은 패널 안으로 이동합니다(중복 표시 방지).

#### (권장) SkyUI

- SkyUI (인벤/루팅 UI 품질 향상; 본 모드 핵심 로직에는 필수 아님) ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/12604))

#### (권장) KID

- KID (태그 분배; DoT 태그 `CAFF_TAG_DOT` 포함) ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/55728))
- 주의: `CAFF_TAG_DOT`을 **필터 없이(Magic Effect + `NONE`) 전체 분배**하면, 키워드 기반 디스펠/정화 효과와 상호작용해 “적용중인 효과”가 사라지는 등 **심각한 부작용**이 날 수 있습니다. 그래서 기본 KID 규칙은 `*Alch*|H`(바닐라 독/알케미 해로운 MGEF)로 **좁게 제한**하고, 위험한 무필터 규칙은 생성 단계에서 출력 자체를 건너뜁니다.

#### (선택) SPID

- SPID (NPC/아이템/퍼크 분배) ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/36869))

### 의존성(선택)

- MCM Helper ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/53000))
  - 본 모드 설정(향후)을 위해 사용합니다.
- Inventory Interface Information Injector (I4) ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/85702))
  - 본 모드는 `Data/SKSE/Plugins/InventoryInjector/CalamityAffixes.json`를 **빈 placeholder**로 동봉합니다(`rules: []`).
  - 툴팁/효과 표시 자체에 필수는 아닙니다.

## DoT 정책 (PoE식 “저발동”)

- DoT는 **hit로 취급하지 않음**
- “틱마다 발동” 금지
- 대신 **DoT 적용/갱신 시 1회**만 트리거를 흘림
  - C++ 플러그인에서 `TESMagicEffectApplyEvent`를 받고
  - `CAFF_TAG_DOT` 키워드가 붙은 MGEF만 선별하여
  - 대상+이펙트 단위로 강한 ICD(기본 1.5초)로 레이트리밋 후 Papyrus로 전달
- `CAFF_TAG_DOT`은 `DotApply` 트리거용 DoT 태그입니다. 기본 배포본에서는 **비활성(chance=0)** 이며, 필요하면 `Data/CalamityAffixes_KID.ini`에서 바닐라 독 규칙(`*Alch*|H`)의 chance를 `100`으로 바꿔 활성화하세요. 다른 DoT(모드/커스텀 MGEF)에 반응시키려면 `affixes/affixes.json`의 `keywords.kidRules`에 **좁은 필터 규칙**을 추가해 생성기로 다시 빌드하세요.

관련 명세는 `doc/1.개발명세서.md`의 `5.4` 참고.

## SKSE 플러그인 빌드(개발자용)

> 이 레포는 Linux에서도 편집 가능하지만, SKSE 플러그인 빌드는 보통 Windows + Visual Studio 2022 환경이 필요합니다.

참고(입문/환경 구성): [skyrim.dev - Your first SKSE plugin in C++](https://skyrim.dev/skse/first-plugin)

### 개요

- 프로젝트 위치: `skse/CalamityAffixes/`
- CMake + vcpkg로 `commonlibsse-ng-flatrim` 의존성을 사용합니다.
- 빌드 산출물: `CalamityAffixes.dll`
- 설치 위치(스테이징): `Data/SKSE/Plugins/CalamityAffixes.dll`

### 최소 스모크 체크(레포 내에서 가능한 것)

```bash
python3 tools/lint_affixes.py --spec affixes/affixes.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json
python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null
python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null
```

## Papyrus ↔ SKSE 브리지

  - C++ 이벤트 싱크: `skse/CalamityAffixes/src/EventBridge.cpp`
  - `TESHitEvent` → `CalamityAffixes_Hit` (sender = 대상 Actor)
  - `TESDeathEvent` → `CalamityAffixes_Kill` (sender = 사망한 Actor) *(현재 Papyrus로는 미연결 — 추후 확장용)*
  - `TESMagicEffectApplyEvent`(+ `CAFF_TAG_DOT`) → `CalamityAffixes_DotApply` (sender = 대상 Actor)
  - `ModCallbackEvent` 수신:
    - `CalamityAffixes_MCM_SetEnabled` / `CalamityAffixes_MCM_SetDebugNotifications` / `CalamityAffixes_MCM_SetValidationInterval` / `CalamityAffixes_MCM_SetProcChanceMult` / `CalamityAffixes_MCM_SpawnTestItem`
      → MCM General 설정을 런타임에 즉시 반영
    - `CalamityAffixes_UI_SetPanel` / `CalamityAffixes_UI_TogglePanel`
      → Prisma 조작 패널 열기/닫기
    - `CalamityAffixes_ModeCycle_Next` / `CalamityAffixes_ModeCycle_Prev` → 장착 중 `modeCycle.manualOnly=true` affix 모드 수동 전환
    - `CalamityAffixes_Runeword_*` → 룬워드 베이스 순환/레시피 순환/룬 삽입/상태 조회/테스트용 조각 지급
  - Papyrus 수신: `Data/Scripts/Source/CalamityAffixes_SkseBridge.psc`
  - `RegisterForModEvent`로 수신 후 `AffixManager.ProcessOutgoingHit(target)`로 전달

## 룬워드 제작(실험)

- 현재는 **유저 자유 베이스 지정**을 지원합니다. (장착 중 무기/방어구를 순환 선택)
- 레시피는 D2/D2R 기준 **94개 룬워드 카탈로그**(2차 확장)를 제공합니다. 레시피별 **권장 베이스 타입(무기/방어구)**은 안내만 하고 강제하지 않습니다.
- `affixes/affixes.json`에 정의되지 않은 룬워드 최종 어픽스는 SKSE 런타임에서 **ID 기반 스타일 매핑 + 개별 튜닝 + 베이스 타입 분산 + 레거시 폴백**으로 자동 합성됩니다. (D2 오라 감성은 Flame/Frost/Shock Cloak, Oak/Stone/Iron/Ebonyflesh, Fear/Frenzy 등 바닐라 주문 치환)
- Prisma 룬워드 패널은 레시피가 많아진 것을 반영해 **검색 + 총/검색결과 카운트**를 표시합니다.
- 조각 수급:
  - 기본: 어픽스 신규 롤링 시 확률적으로 룬 조각을 얻음
  - Prisma 디버그 버튼: `+1 next fragment`, `+1 recipe set`
- 완성 시 선택 베이스 인스턴스의 어픽스가 룬워드 최종 어픽스로 교체됩니다.

## 다음 단계(구현 시작 체크리스트)

1) (권장) `affixes/affixes.json` → 생성기 실행으로 플러그인/ini/런타임 설정 생성  
2) 두 가지 시작 방식 중 선택
   - **CK 0 (SKSE 런타임만 사용)**: 생성기 + KID + SKSE DLL만으로 동작(Quest/Alias 없음)
   - **CK 최소 (Papyrus 매니저 사용)**: Quest + PlayerAlias 세팅 후 스크립트 부착/프로퍼티 연결
3) DoT 태그(`CAFF_TAG_DOT`) 및 어픽스 키워드 구성  
4) 테스트 아이템/스펠로 “장비 스캔 → 적중/DoT 적용 → 프로크/ICD” 흐름 확인

### 생성기(키워드/ini 자동 생성)

```bash
dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data
```

생성기는 아래도 함께 갱신합니다:
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json` (SKSE 런타임 설정)
- `Data/SKSE/Plugins/InventoryInjector/CalamityAffixes.json` (InventoryInjector placeholder: `rules: []`)
- `Data/CalamityAffixes.esp` (키워드 + **옵션: records 기반 Spell/MGEF 자동 생성** + MCM Quest + `CalamityAffixes_MCMConfig` 바인딩)

### MO2 배포 ZIP 생성

```bash
tools/build_mo2_zip.sh
```

위 스크립트는 ZIP 생성 전에 자동으로:
1) 생성기 실행(esp/json/ini 갱신)
2) Papyrus 컴파일(`Data/Scripts/*.pex`)
를 수행합니다.

상세 CK 체크리스트: `doc/2.CK_MVP_셋업_체크리스트.md`

자동화(B 방식) 워크플로우: `doc/3.B_데이터주도_생성기_워크플로우.md`
