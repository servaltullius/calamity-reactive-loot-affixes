## Calamity Reactive Loot & Affixes v1.2.20-rc19 (Pre-release)

이번 RC는 프로젝트 경로 이동 이후 흔들리던 빌드/패키징 경로를 안정화하고,
`EventBridge`와 `PrismaTooltip`의 거대한 책임을 더 잘게 나눠 다음 기능 작업 비용을 낮추는 데 초점을 맞춘 정리 릴리즈입니다.

---

### 변경 사항

- 경로 이동 뒤에도 `ensure_skse_build.py`가 stale `CMakeCache.txt`/`DartConfiguration.tcl`을 감지해 plugin/runtime-gate build tree를 자동 재구성하도록 정리했습니다.
- SKSE 플러그인 메타데이터 버전을 CMake 생성 버전 기준으로 통일해 DLL 메타, 패키징 버전, 릴리즈 버전 흐름을 맞췄습니다.
- `EventBridge`의 serialization load, trigger runtime, runeword selection/transmute/reforge 절차를 helper 단위로 추가 분리했습니다.
- `Transmute`는 fragment consume/rollback 절차를 별도 helper로 분리했고, `Reforge`는 preserved runeword token을 유지한 regular-slot reroll 판단을 pure helper로 분리했습니다.
- `PrismaTooltip` 명령 라우팅과 view-state/telemetry 책임을 controller/helper 쪽으로 정리했습니다.
- runtime-gate에 fake health-damage 이벤트 시퀀스 검증을 추가해 stale-window, duplicate signature, per-target repeat guard 조합을 동작 기준으로 확인하도록 보강했습니다.

### 검증

- `CAFF_PACKAGE_VERSION=1.2.20-rc19 tools/release_verify.sh --fast --strict --with-mo2-zip` 통과
  - compose/lint/json sanity
  - tools unittest
  - runtime contract sync
  - Generator 테스트 71/71
  - SKSE ctest 3/3
  - MO2 패키징 생성 완료

### 배포 파일

- `CalamityAffixes_MO2_v1.2.20-rc19_2026-03-08.zip`
- SHA256: `ea4c65ad2b141a3beb0f07d835cc9c5f30685ae1c2090b3a50086deeb51485c4`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc18...v1.2.20-rc19
