# CalamityAffixes 릴리즈 노트 (v0.1.0-beta.1 / 2026-02-09)

## 선행 의존성

- 필수
  - SKSE64 (런타임 버전 일치): https://skse.silverlock.org/
  - Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444
  - Prisma UI (툴팁/조작 패널 프레임워크): https://www.nexusmods.com/skyrimspecialedition/mods/148718
- 권장
  - SkyUI: https://www.nexusmods.com/skyrimspecialedition/mods/12604
  - Keyword Item Distributor (KID): https://www.nexusmods.com/skyrimspecialedition/mods/55728
  - Spell Perk Item Distributor (SPID): https://www.nexusmods.com/skyrimspecialedition/mods/36869
- 선택
  - MCM Helper: https://www.nexusmods.com/skyrimspecialedition/mods/53000
  - Inventory Interface Information Injector (I4): https://www.nexusmods.com/skyrimspecialedition/mods/85702

## 변경 요약

- 런타임 상태 키를 인스턴스 단일 키에서 복합 키 `(instanceKey, affixToken)`로 리팩터링해, 어픽스 간 상태 섞임을 방지했습니다.
- 직렬화 v5를 추가하고 per-affix 런타임 상태 레코드(`IRST`)를 명시적으로 저장하도록 변경했습니다.
- 기존 세이브(v1~v4) 로드 호환(마이그레이션) 경로를 유지했습니다.
- 드랍 아이템 인스턴스 삭제 시 해당 인스턴스의 모든 런타임 상태가 함께 정리되도록 보강했습니다.

## 적용 범위(중요)

- 현재 런타임 대상은 **플레이어 장비/플레이어 인벤토리**입니다.
- 전투 트리거는 **플레이어 본인 + 플레이어가 지휘하는 소환체(player-owned summon/proxy)** 까지 지원합니다.
- **일반 팔로워/NPC가 착용한 장비를 독립 주체로 추적/발동**하는 모드는 현재 지원하지 않습니다.

## 문서/배포 업데이트

- README에 적용 범위(플레이어 중심)와 팔로워/NPC 제한 사항을 명시했습니다.
- MCM 설정 페이지에 런타임 범위 안내 문구를 추가했습니다.
- Nexus 업로드 가이드를 추가했습니다:
  - `docs/releases/2026-02-09-nexus-upload-playbook.md`
  - `docs/releases/2026-02-09-nexus-publish-copy-v0.1.0-beta.1.md`
- 배포 산출물(`CalamityAffixes.esp`, `*_DISTR.ini`, `*_KID.ini`, Papyrus `.pex`, `CalamityAffixes.dll`)을 재생성하고 MO2 ZIP을 재빌드했습니다.

## 사후 유지보수 업데이트 (2026-02-09)

- `EventBridge.Loot.cpp`의 UI 가드 분기(툴팁 후보 모호성, 룬워드 베이스 커서 계산)를 pure helper로 분리했습니다.
  - `skse/CalamityAffixes/include/CalamityAffixes/LootUiGuards.h`
  - `skse/CalamityAffixes/tests/test_loot_ui_guards.cpp`
- Trigger suppression / per-target ICD 판정 규칙을 pure helper로 분리해 테스트 가능 영역을 넓혔습니다.
  - `skse/CalamityAffixes/include/CalamityAffixes/TriggerGuards.h`
  - `skse/CalamityAffixes/tests/test_trigger_guards.cpp`
- 이벤트 시그니처 내부 레퍼런스를 문서화했습니다.
  - `docs/references/2026-02-09-event-signatures.md`

## 테스트 범위 확장

- SKSE static-check 모듈을 기존 7개에서 9개로 확대했습니다.
  - 추가: `test_loot_ui_guards`, `test_trigger_guards`

## 검증

- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`
- `tools/build_mo2_zip.sh`
- 생성 산출물: `dist/CalamityAffixes_MO2_2026-02-09.zip`
