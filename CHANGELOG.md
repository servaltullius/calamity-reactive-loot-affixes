# Changelog

이 프로젝트의 주요 변경사항은 이 파일에 기록합니다.

형식은 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)를 따르고,
버전 표기는 [Semantic Versioning](https://semver.org/spec/v2.0.0.html) 기반으로 관리합니다.

## [Unreleased]

## [1.2.23] - 2026-07-11

`v1.2.23`은 4K에서 확인된 기존 어픽스 툴팁 외관을 유지하면서 QHD, FHD, 울트라와이드 해상도에서 화면 비율에 맞게 크기와 기본 위치를 조정하는 UI 호환성 업데이트입니다.

### Added

- 4K, QHD, FHD, UWQHD의 툴팁 배율·기본 위치와 모바일 분기, 수동 글자 크기 저장 경로를 고정하는 회귀 테스트를 추가했습니다.

### Changed

- 툴팁 자동 배율 기준을 `3840x2160`의 기존 `2.5x` 외관으로 명시하고, 뷰포트의 가로·세로 중 작은 비율을 사용하도록 변경했습니다.
- 저장된 사용자 배치가 없을 때 툴팁 기본 `right/top` 위치도 같은 4K 기준으로 비례 조정합니다.

### Fixed

- QHD와 FHD에서 어픽스 툴팁이 4K보다 화면을 더 크게 차지하던 문제를 수정했습니다.
- `3440x1440` 같은 울트라와이드에서 가로 폭만 따라 툴팁이 과도하게 커질 수 있는 문제를 높이 기준 비율로 제한했습니다.

### Compatibility

- 세이브 및 어픽스 런타임 저장 형식은 변경하지 않았습니다.
- 기존 `prisma_tooltip_layout.json`의 수동 `right/top` 위치와 글자 크기는 그대로 유지됩니다. 현재 해상도의 새 기본 위치를 적용하려면 해당 해상도에서 툴팁 UI를 초기화해야 합니다.
- 폭 `900px` 이하 분기와 기존 글자 크기 조절 범위 `70%~180%`는 유지됩니다.

## [1.2.22] - 2026-07-10

`v1.2.22`는 아이템 판매·보관 중 ExtraUniqueID 소유권이 바뀔 때 인스턴스 상태가 끊기거나 다른 아이템으로 이어질 수 있던 문제를 막는 안전성 핫픽스입니다.

### Added

- 룬워드 패널에 선택 장비의 Calamity 어픽스, 룬워드 진행도, 런타임 상태, 프리뷰와 표시 이름을 제거하는 `Reset State / 상태 초기화` 동작을 추가했습니다.
- 실수 방지를 위해 상태 초기화는 6초 안에 두 번 눌러야 하며, 소모한 재련 오브와 룬 조각은 환불되지 않습니다.
- 판매·상자 보관·회수 및 목적지 키 충돌을 owner+UID 기준으로 검증하는 회귀 테스트를 추가했습니다.

### Changed

- `ExtraUniqueID` 인스턴스 키 의미를 SKSE 원본 계약인 `(owner FormID, UID)`로 명확히 통일했습니다.
- 구버전이 `(item FormID, UID)`로 만든 플레이어 인벤토리 키는 UID가 유일할 때만 로드 후 변환합니다. 같은 owner+UID에 애매한 기존 상태가 있으면 먼저 fail-closed로 제거해 현재 아이템에 잘못 붙지 않게 합니다.
- 생성물 최신성 검사는 파일 수정시각 대신 실제 JSON 콘텐츠를 비교하도록 변경했습니다.
- GitHub Actions가 fresh checkout에서도 xwin과 CommonLibSSE-NG를 외부 캐시에 준비한 뒤 실제 DLL/runtime-gate를 빌드하도록 보강했습니다.

### Fixed

- `TESUniqueIDChangeEvent`의 아이템 FormID를 플레이어 소유자 ID로 오인해 판매·상자 이동 이벤트를 놓치던 문제를 수정했습니다.
- UID 변경 목적지에 기존 상태가 있을 때 두 아이템의 어픽스 토큰을 합치던 동작을 제거했습니다. 소유자를 증명할 수 없는 충돌 상태는 양쪽 모두 fail-closed로 제거해 다른 아이템으로 이어지지 않게 합니다.
- 플레이어가 직접 생성한 `ExtraUniqueID`에 아이템 FormID를 기록하던 문제를 수정해 실제 소유자 FormID를 기록합니다.

### Compatibility

- 이미 다른 아이템에 잘못 붙어 버린 legacy 상태는 원래 소유자를 안전하게 추론할 수 없습니다. 해당 장비를 선택하고 `Reset State / 상태 초기화`를 사용해야 합니다.
- 상태 초기화 뒤 자동 어픽스 재롤을 막기 위해 evaluated 표시는 유지됩니다.

## [1.2.21] - 2026-03-22

`v1.2.21`은 `v1.2.20` 이후 확인된 룬워드 워크벤치 스크롤 UX 문제를 정리한 유지보수 릴리즈입니다.

### Changed

- Prisma 룬워드 레시피 목록이 전역 inertia 스크롤을 타지 않고, 목록 전용 amplified wheel scroll 경로를 사용하도록 조정했습니다.
- CEF가 작은 휠 델타를 줄 때도 의미 있게 내려가도록 최소 스크롤 step을 추가해, 휠을 과도하게 많이 굴려야 하던 문제를 완화했습니다.

### Fixed

- 룬워드 레시피 목록이 무겁게 끊기거나, 반대로 너무 조금씩만 내려가서 긴 목록 탐색이 불편하던 문제를 수정했습니다.

## [1.2.20] - 2026-03-22

상세한 RC 흐름은 [v1.2.20 RC 통합 노트](docs/releases/2026-03-22-v1.2.20-rc02-rc23-consolidated-notes.md)를 참고하세요.

### Added

- 룬워드 효과군을 스카이림 마법 학파 기반으로 재설계하고, 겹치던 효과를 고유 효과로 바꿔 빌드 선택지가 더 분명해졌습니다.
- Utility prefix 3종을 전투용 효과로 복귀시키고, suffix 패밀리를 재편해 장비 파밍 결과가 더 뚜렷하게 갈리도록 정리했습니다.
- Bloom 계열 트랩에 `planted / burst` 피드백과 생성 실패 진단을 추가해, 심김과 실제 발동을 인게임에서 바로 확인할 수 있게 했습니다.

### Changed

- 룬워드 워크벤치 UI를 재설계해 레시피 프리뷰 기본 오픈, 스크롤 복구, 레이아웃 밀도 조정, 패널 드래그/배치 개선을 한 사이클에 묶었습니다.
- 아이템 설명과 공개 문서를 대규모로 손봐, 한국어 설명이 더 구체적으로 보이고 내부용 정의는 플레이어 문서에서 숨기도록 정리했습니다.
- 트리거, 룬워드, 직렬화, Prisma 패널 관련 내부 구조를 분해해 이후 기능 수정과 검증이 쉬운 방향으로 정리했습니다.
- 특수 액션 계약을 강화해 잘못된 `runtime.trigger`나 암묵적인 `0 = always-on` 보정을 제거했고, 디버그 HUD와 verbose 로그도 분리했습니다.

### Fixed

- 플레이어 피격 시 `TESHitEvent` fallback 재진입으로 이어지던 전투 프리징을 수정하고, `HandleHealthDamage` hook 기반의 안정적인 incoming hit routing으로 되돌렸습니다.
- SE/AE 공용 CommonLibSSE 타깃이 깨져 Address Library 초기화가 실패하던 릴리즈 빌드 문제를 수정했습니다.
- `ConvertDamage`의 원소별 동시 적용, `AttackDamageMult`/`WeaponSpeedMult` magnitude 단위, 어픽스 개수 reroll, 방어구 suffix 드롭 풀 누락 같은 데이터/런타임 버그를 정리했습니다.
- 프로젝트 경로 이동 뒤 build tree가 깨지던 문제, generator/runtime contract sync 불안정, MO2 ZIP 패키징과 Papyrus compile 검증 경로를 보강해 배포 안정성을 높였습니다.

## [1.2.20-rc23] - 2026-03-09

### Added

- Bloom 계열 트랩에 `planted / burst` 진단 HUD와 생성 실패 사유 알림을 추가해, 트랩이 심어졌는지와 실제 발동했는지를 인게임에서 바로 확인할 수 있게 했습니다.

### Changed

- 특수 액션(`CastOnCrit`, `ConvertDamage`, `MindOverMatter`, `Archmage`, `CorpseExplosion`, `SummonCorpseExplosion`)은 이제 유효한 `runtime.trigger`와 명시적 `runtime.procChancePercent > 0`이 없으면 보정 없이 실패하도록 계약을 강화했습니다.
- 기존 특수 액션의 `procChancePercent: 0.0` 데이터는 모두 의미 보존 형태의 `100.0`으로 마이그레이션했습니다.
- 공개 문서 생성 경로에서 INTERNAL 항목을 기본 숨김 처리해, 플레이어용 카탈로그와 prefix 문서가 내부 companion spell 정의를 직접 드러내지 않도록 정리했습니다.
- `debug notifications` 설정을 `debugHudNotifications`와 `debugVerboseLogging`으로 분리하고, legacy `debugNotifications`는 호환용 입력으로만 읽도록 바꿨습니다.

### Fixed

- debug HUD/log 분리 과정에서 룬워드 안내, 복구 알림, 설정 변경 피드백 같은 일반 HUD 메시지까지 숨겨지던 회귀를 수정했습니다.
- release/build 검증 스크립트가 `dotnet` 실행 경로 문제로 테스트 커버리지를 잃지 않도록 `CAFF_DOTNET` 주입 경로와 fake-`dotnet` 기반 테스트를 추가했습니다.

## [1.2.20-rc22] - 2026-03-08

### Fixed

- 플레이어 피격 시 `TESHitEvent` fallback incoming trigger가 proc 후속 이벤트를 다시 먹으며 프리징으로 이어질 수 있던 경로를 재진입 guard로 차단했습니다.
- `PlayerCharacter HandleHealthDamage` hook를 기본 경로로 사용하도록 바꿔, 플레이어 incoming hit 처리가 불안정한 fallback 경로에 과도하게 의존하던 문제를 줄였습니다.
- 예전 `allowPlayerHealthDamageHook` 런타임 override는 무시하고 항상 플레이어 피격 hook를 활성화하도록 정리했습니다.

## [1.2.20-rc21] - 2026-03-08

### Fixed

- 릴리즈 빌드가 `CommonLibSSE-NG`의 `install.linux-clangcl-rel-se`를 참조하던 문제를 `install.linux-clangcl-rel` flatrim(SE/AE 공용) 기준으로 수정했습니다.
- `ensure_skse_build.py`가 `CMAKE_PREFIX_PATH`까지 검사해, 잘못된 SE 캐시를 유지한 채 패키징하는 상황을 막도록 보강했습니다.

## [1.2.20-rc20] - 2026-03-08

### Changed

- Prisma control panel 드래그 중 위치 갱신을 `requestAnimationFrame + transform` 기반으로 정리해 레이아웃 흔들림과 잦은 style write를 줄였습니다.

### Fixed

- 대형 Prisma 패널을 끌 때 드래그가 무겁거나 미세하게 튀는 체감 문제를 완화했습니다.
- 패널 드래그/리사이즈/복원 경로가 서로 다른 `left/top/right/bottom` 갱신 방식을 섞던 부분을 공통 anchored-position helper로 맞췄습니다.

## [1.2.20-rc19] - 2026-03-08

### Added

- 프로젝트 경로 이동 뒤에도 `ensure_skse_build.py`와 no-space run-root helper가 stale build tree를 자동 복구하도록 정리했습니다.
- `HealthDamage` guard 조합을 fake 이벤트 시퀀스로 직접 검증하는 runtime-gate 테스트를 추가했습니다.
- `Transmute` fragment consume/rollback helper와 `Reforge` regular-slot reroll helper를 분리해 동작 기반 검증 지점을 늘렸습니다.

### Changed

- SKSE 플러그인 버전 메타데이터가 CMake 생성 버전을 단일 진실원으로 사용하도록 정리했습니다.
- `EventBridge`의 serialization load, trigger runtime, runeword selection/transmute/reforge 흐름을 helper 중심으로 더 잘게 분해했습니다.
- `PrismaTooltip` 명령/상태 처리를 controller와 view-state helper 기준으로 재배치했습니다.

### Fixed

- 프로젝트 경로 변경 뒤 `ctest`/release verification이 예전 절대경로에 묶여 깨지던 문제를 줄였습니다.
- `HealthDamage` stale-window, per-target repeat, low-health snapshot 검증이 문자열 검색에 과도하게 의존하던 상태를 보강했습니다.
- 룬워드 재련 시 보존해야 하는 runeword token과 regular affix reroll 비교 경로를 분리해 회귀 위험을 낮췄습니다.

[Unreleased]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.23...HEAD
[1.2.23]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.22...v1.2.23
[1.2.22]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.21...v1.2.22
[1.2.21]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20...v1.2.21
[1.2.20]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.19.2...v1.2.20
[1.2.20-rc23]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc22...v1.2.20-rc23
[1.2.20-rc22]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc21...v1.2.20-rc22
[1.2.20-rc21]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc20...v1.2.20-rc21
[1.2.20-rc20]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc19...v1.2.20-rc20
[1.2.20-rc19]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc18...v1.2.20-rc19
