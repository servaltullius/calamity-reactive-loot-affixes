## Calamity Reactive Loot & Affixes v1.2.20-rc21 (Pre-release)

이번 RC는 `v1.2.20-rc20` 후속 호환성 핫픽스입니다.
릴리즈 빌드가 `SE 전용` CommonLibSSE install tree를 참조하던 문제를 고쳐,
배포 DLL이 `SE/AE 공용` flatrim 기준으로 재구성되도록 바로잡았습니다.

---

### 변경 사항

- 릴리즈/패키징 빌드가 `extern/CommonLibSSE-NG/install.linux-clangcl-rel-se` 대신 `extern/CommonLibSSE-NG/install.linux-clangcl-rel`을 사용하도록 수정했습니다.
- `ensure_skse_build.py`가 `CMAKE_PREFIX_PATH`까지 stale 여부를 검사하게 바꿔, 잘못된 `SE` 캐시를 유지한 상태로 ZIP을 다시 찍는 문제를 막았습니다.
- 로컬 CMake preset도 flatrim(SE/AE shared) 기준으로 맞췄습니다.

### 검증

- `python3 tools/ensure_skse_build.py --lane plugin`
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --no-tests=error --output-on-failure`
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- `CAFF_PACKAGE_VERSION=1.2.20-rc21 tools/release_verify.sh --fast --strict --with-mo2-zip` 통과

### 배포 파일

- `CalamityAffixes_MO2_v1.2.20-rc21_2026-03-08.zip`
- SHA256: `ba3c9426c8cb9792189c5bf8686bd73cc294f3ee5360a65ec9584cb2faaf8c67`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20-rc20...v1.2.20-rc21
