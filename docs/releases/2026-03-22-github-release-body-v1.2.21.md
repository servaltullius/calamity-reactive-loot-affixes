## Calamity Reactive Loot & Affixes v1.2.21

`v1.2.21`은 `v1.2.20` 이후 확인된 Prisma 룬워드 레시피 목록 스크롤 UX 문제를 정리한 유지보수 릴리즈입니다.

---

### 유저가 체감할 변화

- 룬워드 워크벤치 중앙 레시피 목록이 더 부드럽고 크게 내려가도록 스크롤 경로를 조정했습니다.
- 긴 레시피 목록을 볼 때 휠을 과도하게 여러 번 굴려야 하던 문제를 완화했습니다.

### 기술적으로 바뀐 점

- 레시피 목록은 전역 inertia 스크롤 대신 목록 전용 wheel-scroll 경로를 사용합니다.
- CEF의 작은 픽셀 단위 휠 입력에도 최소 스크롤 step을 적용해, 실제 체감 이동량이 충분히 나오도록 보정했습니다.
- 관련 UI 정책은 runtime-gate 테스트로 고정했습니다.

### 검증

- `ctest --test-dir skse/CalamityAffixes/tests/build --no-tests=error --output-on-failure`
- `CAFF_DOTNET=/home/kdw73/.dotnet/dotnet CAFF_PACKAGE_VERSION=1.2.21 tools/release_verify.sh --fast --strict --with-mo2-zip`
- 인게임 확인
  - 룬워드 레시피 목록 스크롤 체감 개선 확인

### 배포 파일

- `CalamityAffixes_MO2_v1.2.21_2026-03-22.zip`
- SHA256: `ba45038de145c98529edbc53760e8b3a7c97d348e3d33573933b05c1f0db11a3`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.20...v1.2.21
