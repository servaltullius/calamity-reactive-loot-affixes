## Calamity Reactive Loot & Affixes v1.2.20-rc23 (Pre-release)

이번 RC는 Bloom 트랩 피드백 보강과 spec/runtime 계약 정리, 그리고 debug HUD 분리 회귀 수정까지 묶은 안정화 프리릴리즈입니다.
`rc22` 이후 들어간 트랩 진단, 특수 액션 계약 하드닝, 공개 문서 정리, 릴리즈 검증 주입 경로 보강을 한 번에 반영했습니다.

---

### 변경 사항

- Bloom 계열 트랩에 `planted / burst` HUD 진단과 생성 실패 사유 알림을 추가해, 심김/대기/발동 흐름을 인게임에서 바로 확인할 수 있게 했습니다.
- 특수 액션은 이제 잘못된 `runtime.trigger`를 조용히 `Hit`로 보정하지 않고, 유효한 trigger와 명시적 `runtime.procChancePercent > 0`이 없으면 로드 실패 처리합니다.
- 기존 `0 = always-on` special action 데이터는 모두 `100.0`으로 마이그레이션해, 생성기/린터/런타임 계약을 같은 의미로 맞췄습니다.
- 공개 문서 생성에서 INTERNAL 항목을 기본 숨김 처리해, 플레이어용 카탈로그와 prefix 문서에서 내부 companion spell 정의가 직접 노출되지 않게 정리했습니다.
- `debugHudNotifications`와 `debugVerboseLogging`을 분리하면서 발생했던 일반 HUD 알림 회귀를 수정해, 룬워드/복구/설정 알림은 계속 보이고 진단성 HUD만 디버그 토글을 따르도록 바로잡았습니다.
- release/build 검증 스크립트에 `CAFF_DOTNET` 주입 경로를 추가하고 fake `dotnet` 기반 테스트를 넣어, 로컬 `dotnet` 경로 문제가 있어도 릴리즈 검증 커버리지가 유지되도록 보강했습니다.

### 검증

- `python3 -m unittest discover -s tools/tests -p 'test_*.py'`
- `python3 tools/ensure_skse_build.py --lane plugin --lane runtime-gate`
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
- `CAFF_PACKAGE_VERSION=1.2.20-rc23 tools/release_verify.sh --fast --strict --with-mo2-zip`

### 배포 파일

- `CalamityAffixes_MO2_v1.2.20-rc23_2026-03-09.zip`
- SHA256: `d9225705fe44843540f780742ffcd24336bd052c8452f61b2c3a118b139c35e4`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc22...v1.2.20-rc23
