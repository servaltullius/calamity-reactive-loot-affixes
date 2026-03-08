## Calamity Reactive Loot & Affixes v1.2.20-rc22 (Pre-release)

이번 RC는 플레이어 피격 중 발생하던 전투 프리징 핫픽스입니다.
`TESHitEvent` fallback incoming trigger가 proc 후속 이벤트를 다시 먹는 경로를 정리하고,
플레이어 incoming hit은 `PlayerCharacter HandleHealthDamage` hook를 기본 경로로 사용하도록 바로잡았습니다.

---

### 변경 사항

- `TESHitEvent` incoming fallback에 proc-depth 재진입 guard를 추가해, 플레이어 피격 후속 이벤트가 다시 trigger dispatch를 밟으며 얼어붙는 경로를 차단했습니다.
- `PlayerCharacter HandleHealthDamage` hook를 기본 활성으로 변경해, 플레이어 incoming hit 처리가 안정적인 hook 경로를 우선 사용하도록 수정했습니다.
- 예전 `allowPlayerHealthDamageHook` 런타임 override는 더 이상 사용하지 않고, legacy 설정이 있어도 hook 기본 동작을 유지하도록 정리했습니다.
- runtime-gate 테스트에 player hit hook 기본 정책과 incoming fallback guard 정책을 함께 고정했습니다.

### 검증

- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- `CAFF_PACKAGE_VERSION=1.2.20-rc22 tools/release_verify.sh --fast --strict --with-mo2-zip`

### 배포 파일

- `CalamityAffixes_MO2_v1.2.20-rc22_2026-03-08.zip`
- SHA256: `54724a4a143fa513914a1944d06bdb16e71f440117691e729220d4044e52c156`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc21...v1.2.20-rc22
