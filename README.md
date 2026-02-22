# Calamity - Reactive Loot & Affixes

[![Latest Release](https://img.shields.io/github/v/release/servaltullius/calamity-reactive-loot-affixes?label=Latest%20Release)](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/latest)
[![Download (MO2 ZIP)](https://img.shields.io/badge/Download-MO2%20ZIP-2ea44f)](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/latest)

## 다운로드 (플레이어)

- GitHub Releases (최신): https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/latest
- 다운로드 파일: `CalamityAffixes_MO2_vX.Y.Z_YYYY-MM-DD.zip`

## 필수/권장 모드

- 필수: SKSE64, Address Library for SKSE Plugins, Prisma UI
- 권장: SkyUI
- 선택: KID, SPID, MCM Helper, I4

## 5분 설치 (MO2 기준)

1. 위 “필수” 모드를 설치합니다.
2. GitHub Releases에서 `CalamityAffixes_MO2_vX.Y.Z_YYYY-MM-DD.zip`을 다운로드합니다.
3. MO2에서 “Install a new mod from an archive”로 ZIP을 설치하고 Enable 합니다.
4. SKSE로 실행합니다.
5. 정상 동작 확인: `Documents/My Games/Skyrim Special Edition/SKSE/CalamityAffixes.log`가 생성됩니다.

## 사용법 (한 줄 요약)

- 아이템 획득 시에는 **룬워드 조각/재련 오브**만 확률 롤이 수행됩니다. (어픽스 자동 부여 없음)
- 인벤/루팅/상점에서 아이템을 “선택”하면 Prisma UI 툴팁에 어픽스 설명이 표시됩니다.
- Prisma 조작 패널 토글: 기본 `F11` (MCM에서 변경 가능)
- 룬워드 패널에서 **재련 오브(Reforge Orb)** 를 사용해 선택 장비를 재련할 수 있습니다.
- 룬워드 패널에서 레시피를 선택하면 **효과/권장 베이스/룬 순서/상세**가 상태 영역에 표시되며, 레시피 항목 hover로도 효과 요약을 확인할 수 있습니다.

## 문제 해결 (가장 흔한 것)

- 툴팁이 안 뜸: Prisma UI 설치 확인, `Data/PrismaUI/views/CalamityAffixes/index.html`가 있는지 확인, 로그 확인
- 실행/크래시: 런타임과 맞는 SKSE/Address Library인지 확인, `Data/SKSE/Plugins/*.dll` 충돌 여부 확인
- 설정 변경:
  - MCM Helper가 있으면 MCM에서 조정
  - 없으면 `Data/MCM/Config/CalamityAffixes/settings.ini`를 `Data/MCM/Settings/CalamityAffixes.ini`로 복사해서 오버라이드
  - 런타임/MCM UI 값은 `Data/SKSE/Plugins/CalamityAffixes/user_settings.json`에도 저장되어, 새 게임/재시작 후에도 마지막 설정을 복원합니다.
  - 런타임(MCM) 값 저장은 짧은 디바운스(약 250ms)로 묶어 디스크 쓰기 빈도를 줄입니다.
  - `user_settings.json` 파싱/IO 실패 시 기존 파일을 빈 값으로 덮어쓰지 않고 보존합니다.

## 이 레포는 무엇인가요?

- 플레이어용 배포 ZIP과 함께, 소스 코드(SKSE 플러그인), 생성기(dotnet), 문서를 같이 관리하는 개발 레포입니다.
- 개발/설계/배포 자동화 상세는 아래 “펼치기” 섹션을 참고하세요.

<details>
<summary>개발/설계 상세 (펼치기)</summary>

### Maintainer 빠른 링크

- GitHub Releases(전체): https://github.com/servaltullius/calamity-reactive-loot-affixes/releases
- 넥서스 업로드 실무 가이드: `docs/releases/2026-02-09-nexus-upload-playbook.md`
- 넥서스 붙여넣기 완성본(v0.1.0-beta.1): `docs/releases/2026-02-09-nexus-publish-copy-v0.1.0-beta.1.md`

스카이림(SE/AE)용 “Diablo/PoE 스타일 어픽스 + 프로크(확률 발동) + ICD” 시스템을 만들기 위한 개발 레포입니다.

## 목표(요약)

- 아이템(무기/아머)에 **어픽스(확률 발동 효과)**를 붙이고
- 중앙 Papyrus 매니저가 **프로크/ICD/가드**를 처리하며
- (권장) SKSE 플러그인이 **모든 무기/마법/소환수/DoT 적용(=틱 X)** 트리거를 안정적으로 공급합니다.

## 적용 범위(현재)

- 기본 런타임 대상은 **플레이어 장비/플레이어 인벤토리**입니다.
- 전투 트리거는 **플레이어 본인 + 플레이어가 지휘하는 소환체(player-owned summon/proxy)** 까지 지원합니다.
- **일반 팔로워/NPC가 착용한 장비를 독립 주체로 추적/발동**하는 모드는 현재 지원하지 않습니다.

## 어픽스 적용 방식(인스턴스 / Reforge 중심)

- SKSE 플러그인은 아이템 **인스턴스(ExtraUniqueID)** 상태를 코세이브로 추적/저장합니다.
- 현재 기본 정책은 **획득/제작 시 자동 어픽스 롤링 비활성화**이며, 어픽스는 재련(Reforge)으로 부여/재롤합니다.
- 장기 세이브 안정성을 위해 인스턴스 어픽스는 SKSE 코세이브에 **문자열이 아닌 64-bit 토큰(FNV-1a)**로 저장합니다(v5). *(v1~v4 세이브는 로드 시 자동 마이그레이션)*
  - 따라서 `affix.id`는 **리네임 금지**(기존 세이브 매칭이 깨짐). 변경이 필요하면 “새 id 추가”로 버전업합니다.
- (안전한 프루닝) 플레이어가 월드에 **드랍한 아이템 레퍼런스가 삭제될 때만** 해당 인스턴스 엔트리를 정리해 코세이브 누적을 완화합니다(재획득 시엔 프루닝 안 함).
- (기본 UX) Prisma UI 오버레이로 **선택 중인 아이템의 어픽스 효과 설명을 표시**합니다. (**Prisma UI 필수**, Norden UI 등 UI 테마와 무관)
  - 아이템 메뉴(인벤/루팅/상점)에서 “선택된 아이템”이 어픽스 인스턴스면 표시되고, 아니면 자동으로 숨겨집니다.
  - 합성 프리뷰(획득 전 가상 롤) 경로는 기본 비활성화되어 “프리뷰-획득 불일치”를 줄입니다.
- (기본값) `loot.renameItem=true` : 아이템 이름에 **짧은 어픽스 라벨**을 붙여(좌측 리스트) 빠르게 식별합니다.
  - `loot.nameMarkerPosition=trailing`이면 이름 마커를 뒤에 붙입니다. 예: `철검*` (정렬 안정화)

기본값: `loot.runewordFragmentChancePercent=8`, `loot.reforgeOrbChancePercent=5`, `loot.currencyDropMode=hybrid(고정)`, `loot.renameItem=true`, `loot.nameMarkerPosition=trailing`, `loot.sharedPool=true`
참고: `loot.chancePercent`는 현재 기본 정책에서 실질적으로 사용되지 않는 레거시 호환 필드입니다.
추가 안전장치(권장): `loot.trapGlobalMaxActive=64` (전역 트랩 하드캡, 0=무제한)

## 제작 아이템(대장간/제작)에서는 어픽스가 어떻게 붙나요?

- 제작 완료 아이템도 기본적으로 **자동 어픽스 롤링이 적용되지 않습니다**.
- 룬워드/재련 플로우를 통해 인스턴스 효과를 부여하는 설계입니다.

## 팔로워/NPC에게 아이템을 주면 적용되나요?

- 현재 설계는 플레이어 중심이라, **팔로워/NPC 장비를 별도 인스턴스로 굴리는 방식은 미지원**입니다.
- 따라서 “NPC가 착용한 같은 아이템도 동일한 어픽스 시스템으로 발동”을 기대하면 현재 빌드에서는 동작이 제한적입니다.
- 안정성을 우선해 범위를 좁힌 상태이며, 필요 시 추후 `팔로워 한정` 단계부터 확장하는 것을 권장합니다.

## 레포 구조

- `doc/` : 개발 명세서(컨셉/제약/설계)
- `doc/4.현재_구현_어픽스_목록.md` : 현재 구현된 효과 목록(한국어 요약)
- `doc/5.룬워드_94개_효과_정리.md` : 룬워드 94개 레시피별 효과 설명(직접 정의형 + 자동 합성형)
- `Data/` : 게임 `Data/`에 그대로 설치 가능한 “스테이징” 폴더 (MO2 테스트용)
  - `Data/CalamityAffixes.esp` : 키워드/스펠/MGEF + MCM Helper Quest(자동 생성, 단일 ESP)
  - `Data/Scripts/Source/` : Papyrus 소스(.psc)
    - 활성 컴파일 대상: `CalamityAffixes_ModeControl.psc`, `CalamityAffixes_ModEventEmitter.psc`, `CalamityAffixes_MCMConfig.psc`
    - `CalamityAffixes_MCMConfig.psc` : MCM Helper용 config 스크립트(Quest에 바인딩)
    - `CalamityAffixes_ModeControl.psc` : Prisma/MCM 공용 글로벌 브리지(수동 모드 + 룬워드 + Prisma 패널 토글)
    - `CalamityAffixes_AffixManager.psc` / `CalamityAffixes_PlayerAlias.psc` / `CalamityAffixes_SkseBridge.psc`는 레거시 참고용 소스(현재 배포 빌드에서 .pex 미생성)
  - `Data/Scripts/*.pex` : MCM/런타임에 필요한 컴파일 결과물(빌드 시 자동 생성)
  - `Data/MCM/Config/CalamityAffixes/` : MCM Helper 설정
    - `General / 일반` 페이지: 런타임 설정 + DotApply 안전 자동비활성 토글 + Prisma 패널 열기 토글 + 패널 언어(EN/KO/Both)
    - `keybinds.json`: `prisma_panel_toggle` 포함(Prisma 패널 빠른 토글)
    - MCM `Panel Language / 패널 언어`에서 Prisma 패널 텍스트를 영어/한국어/병행 표기로 전환
  - `Data/CalamityAffixes_KID.ini` : KID 템플릿(태그/옵션 분배)
  - `Data/CalamityAffixes_DISTR.ini` : SPID 템플릿(분배)
- `skse/CalamityAffixes/` : CommonLibSSE-NG 기반 SKSE 플러그인(CMake/vcpkg)

## 설치 전 준비물(플레이어)

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
  - 패널 단축키/언어 값은 `Data/SKSE/Plugins/CalamityAffixes/user_settings.json`에 동기화되며, MCM 설정 파일이 없는 상태에서도 해당 값으로 복원됩니다.
  - 패널이 열리면 입력/커서는 패널이 잡아 “클릭-스루”로 인벤이 함께 눌리는 현상을 방지합니다(ESC 또는 Close로 닫기).
  - 패널이 열리면 툴팁은 패널 안으로 이동합니다(중복 표시 방지).

#### (권장) SkyUI

- SkyUI (인벤/루팅 UI 품질 향상; 본 모드 핵심 로직에는 필수 아님) ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/12604))

#### (권장) KID

- KID (태그 분배; DoT 태그 `CAFF_TAG_DOT` 포함) ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/55728))
- 주의: `CAFF_TAG_DOT`을 **필터 없이(Magic Effect + `NONE`) 전체 분배**하면, 키워드 기반 디스펠/정화 효과와 상호작용해 “적용중인 효과”가 사라지는 등 **심각한 부작용**이 날 수 있습니다. 그래서 기본 KID 규칙은 `*Alch*|H`(바닐라 독/알케미 해로운 MGEF)로 **좁게 제한**하고, 위험한 무필터 규칙은 생성 단계에서 출력 자체를 건너뜁니다.

#### (선택) SPID

- SPID (NPC/아이템/퍼크 분배) ([Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/36869))
- 참고: SPID는 분배 도구이며, 본 모드의 어픽스 런타임 대상 범위를 자동으로 NPC 전체로 확장하지는 않습니다.

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
- `CAFF_TAG_DOT`은 `DotApply` 트리거용 DoT 태그입니다. 기본 배포본에서는 **비활성(chance=0)** 이며, 필요하면 `Data/CalamityAffixes_KID.ini`에서 바닐라 독 규칙(`*Alch*|H`)의 chance를 `100`으로 바꿔 활성화하세요. 다른 DoT(모드/커스텀 MGEF)에 반응시키려면 `affixes/modules/*.json`(조합 결과는 `affixes/affixes.json`)의 `keywords.kidRules`에 **좁은 필터 규칙**을 추가해 생성기로 다시 빌드하세요.

관련 명세는 `doc/1.개발명세서.md`의 `5.4` 참고.

## 개발/테스트 준비물

- .NET SDK 8.0+ (`tools/CalamityAffixes.Generator`, `tools/CalamityAffixes.Generator.Tests`)
- Python 3 (lint/JSON 스모크 체크)
- CMake + Ninja + C++ toolchain (SKSE DLL 빌드)
- Papyrus Compiler 환경 (`tools/compile_papyrus.sh`, `tools/build_mo2_zip.sh`에서 사용)

상세 절차는 아래 `SKSE 플러그인 빌드(개발자용)` / `MO2 배포 ZIP 생성` 섹션을 따릅니다.

## SKSE 플러그인 빌드(개발자용)

> 이 레포는 Linux에서도 편집 가능하지만, SKSE 플러그인 빌드는 보통 Windows + Visual Studio 2022 환경이 필요합니다.

참고(입문/환경 구성): [skyrim.dev - Your first SKSE plugin in C++](https://skyrim.dev/skse/first-plugin)

### 개요

- 프로젝트 위치: `skse/CalamityAffixes/`
- CMake + vcpkg로 `commonlibsse-ng-flatrim` 의존성을 사용합니다.
- 빌드 산출물: `CalamityAffixes.dll`
- 설치 위치(스테이징): `Data/SKSE/Plugins/CalamityAffixes.dll`

### 빌드 경로 선택 (실수 방지)

- `linux-release-commonlibsse` 프리셋: 테스트/정적체크 용도(g++).
- `build.linux-clangcl-rel`: 실제 Windows 타깃 DLL 산출용(WSL cross, `clang-cl + lld-link + xwin`).
- Windows 환경에서는 Visual Studio Developer Command Prompt에서 빌드하는 경로를 권장합니다.

`fatal error: Windows.h: No such file or directory` 발생 시:
- 대부분 `g++` 경로(테스트 프리셋)로 DLL 빌드를 시도한 경우입니다.
- DLL 빌드 경로를 `build.linux-clangcl-rel` 또는 Windows VS 경로로 전환하세요.

### WSL/Linux cross로 DLL 빌드

```bash
cd skse/CalamityAffixes
cmake --build build.linux-clangcl-rel --target CalamityAffixes
ctest --test-dir build.linux-clangcl-rel --output-on-failure
cp -f build.linux-clangcl-rel/CalamityAffixes.dll ../../Data/SKSE/Plugins/CalamityAffixes.dll
```

최신 DLL 반영 확인:

```bash
sha256sum build.linux-clangcl-rel/CalamityAffixes.dll ../../Data/SKSE/Plugins/CalamityAffixes.dll
```

두 해시가 동일하면 `Data/SKSE/Plugins/CalamityAffixes.dll`이 최신 산출물입니다.

### 최소 스모크 체크(레포 내에서 가능한 것)

```bash
python3 tools/compose_affixes.py --check
python3 -m pip install --user jsonschema
python3 tools/lint_affixes.py --spec affixes/affixes.json --schema affixes/affixes.schema.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json
python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null
python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null
```

## Papyrus ↔ SKSE 브리지

  - 이벤트 시그니처 근거 요약: `docs/references/2026-02-09-event-signatures.md`
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
  - 활성 Papyrus 브리지(컴파일 대상):
    - `Data/Scripts/Source/CalamityAffixes_ModeControl.psc`
    - `Data/Scripts/Source/CalamityAffixes_ModEventEmitter.psc`
    - `Data/Scripts/Source/CalamityAffixes_MCMConfig.psc`
  - `CalamityAffixes_SkseBridge.psc` / `AffixManager` 기반 경로는 레거시 참고용이며, 현재 기본 빌드 파이프라인에서는 컴파일하지 않습니다.

## 룬워드 제작(실험)

- 현재는 **유저 자유 베이스 지정**을 지원합니다. (장착 중 무기/방어구를 순환 선택)
- 레시피는 D2/D2R 기준 **94개 룬워드 카탈로그**(2차 확장)를 제공합니다. 레시피별 **권장 베이스 타입(무기/방어구)**은 안내만 하고 강제하지 않습니다.
- `affixes/affixes.json`(모듈 조합 결과)에 정의되지 않은 룬워드 최종 어픽스는 SKSE 런타임에서 **ID 기반 스타일 매핑 + 개별 튜닝 + 베이스 타입 분산 + 레거시 폴백**으로 자동 합성됩니다. (D2 오라 감성은 Flame/Frost/Shock Cloak, Oak/Stone/Iron/Ebonyflesh, Fear/Frenzy 등 바닐라 주문 치환)
- Prisma 룬워드 패널은 레시피가 많아진 것을 반영해 **검색 + 총/검색결과 카운트**를 표시합니다.
- 조각 수급:
  - 기본 하이브리드:
    - 시체: SPID `DeathItem` 분배(ActorType 직접 필터) 경로
    - 상자/월드: SKSE 런타임 롤 경로
  - SPID ActorType 필터에 걸리지 않는 시체(일부 모드 추가 적 등)는 현재 정책상 시체 통화 드랍이 발생하지 않습니다. (하이브리드: 시체 SPID-only)
  - 확률은 `loot.runewordFragmentChancePercent` (MCM 슬라이더 포함)로 제어합니다.
  - Prisma 디버그 버튼: `+1 next fragment`, `+1 recipe set`
- 재련 오브(리포지):
  - 룬 조각과 동일한 하이브리드 경로(시체 SPID + 상자/월드 런타임)로 획득합니다.
  - 확률은 `loot.reforgeOrbChancePercent` (MCM 슬라이더 포함)로 제어합니다.
  - 룬워드 패널 `Reforge / 재련` 버튼으로 **선택된 장착 장비**에 1개를 소모해 재련합니다.
  - 일반 장비 재련: 일반 어픽스를 재굴림합니다.
  - 완성 룬워드 장비 재련: **전체 유효 룬워드 풀에서 1개 랜덤 + 일반 어픽스 최대 3개**를 함께 재롤합니다.
- 완성 시 룬워드 결과 토큰은 인스턴스 슬롯의 최우선 토큰으로 반영됩니다. (일반 어픽스와 공존 가능)

## 다음 단계(구현 시작 체크리스트)

1) (권장) `affixes/modules/*.json` 편집 후 `python3 tools/compose_affixes.py`로 `affixes/affixes.json` 조합 → 생성기 실행  
2) 두 가지 시작 방식 중 선택
   - **CK 0 (SKSE 런타임만 사용)**: 생성기 + KID + SKSE DLL만으로 동작(Quest/Alias 없음)
   - **CK 최소 (Papyrus 매니저 사용)**: Quest + PlayerAlias 세팅 후 스크립트 부착/프로퍼티 연결
3) DoT 태그(`CAFF_TAG_DOT`) 및 어픽스 키워드 구성  
4) 테스트 아이템/스펠로 “장비 스캔 → 적중/DoT 적용 → 프로크/ICD” 흐름 확인

### 생성기(키워드/ini 자동 생성)

```bash
python3 tools/compose_affixes.py
dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data
```

생성기는 아래도 함께 갱신합니다:
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json` (SKSE 런타임 설정)
- `Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json` (Generator↔SKSE 트리거/액션 + 런워드 카탈로그/룬 가중치 계약 스냅샷)
- `Data/SKSE/Plugins/InventoryInjector/CalamityAffixes.json` (InventoryInjector placeholder: `rules: []`)
- `Data/CalamityAffixes.esp` (키워드 + **옵션: records 기반 Spell/MGEF 자동 생성** + MCM Quest + `CalamityAffixes_MCMConfig` 바인딩)

런워드 계약 데이터 SSOT:
- `affixes/runeword.contract.json` (런워드 카탈로그 + 룬 가중치)
- Generator는 이 파일을 우선 로드하고, 결과를 `runtime_contract.json`으로 스냅샷 배포합니다.

런타임에서 생성/유지되는 사용자 설정 파일:
- `Data/SKSE/Plugins/CalamityAffixes/user_settings.json` (MCM 런타임 값 + Prisma 패널 단축키/언어)
  - 저장 방식: 임시 파일 기록 + flush + 원자적 교체(중간 손상 위험 완화)

### 드랍 경로 정책

- 드랍 정책은 `hybrid` 단일 경로를 기본으로 사용합니다.
  - 시체: SPID `DeathItem` 분배(ActorType 직접 필터)
  - 상자/월드: SKSE 런타임 롤
  - SPID ActorType 필터 미매칭 시체: 드랍 없음 (시체 SPID-only)
- 레벨리스트(LVLI) 주입/오버라이드는 더 이상 사용하지 않습니다.

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

</details>
