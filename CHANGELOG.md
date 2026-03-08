# Changelog

이 프로젝트의 주요 변경사항은 이 파일에 기록합니다.

형식은 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)를 따르고,
버전 표기는 [Semantic Versioning](https://semver.org/spec/v2.0.0.html) 기반으로 관리합니다.

## [Unreleased]

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

[Unreleased]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc19...HEAD
[1.2.20-rc19]: https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc18...v1.2.20-rc19
