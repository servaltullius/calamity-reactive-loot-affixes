# Changelog

이 프로젝트의 주요 변경사항은 이 파일에 기록합니다.

형식은 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)를 따르고,
버전 표기는 [Semantic Versioning](https://semver.org/spec/v2.0.0.html) 기반으로 관리합니다.

## [Unreleased]

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

[Unreleased]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc23...HEAD
[1.2.20-rc23]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc22...v1.2.20-rc23
[1.2.20-rc22]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc21...v1.2.20-rc22
[1.2.20-rc21]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc20...v1.2.20-rc21
[1.2.20-rc20]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc19...v1.2.20-rc20
[1.2.20-rc19]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc18...v1.2.20-rc19
