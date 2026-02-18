# v1.2.17-rc18

상호작용 오브젝트(침낭/작업대 등) 경로에서 보고된 CTD 대응과 릴리즈 절차 안정화 문서 보강을 포함한 프리릴리즈입니다.

## 변경 요약
- `HandleHealthDamage` 훅 경로에 포인터 안전 가드를 추가해 비정상 attacker 포인터(`0x2` 등) 역참조 크래시를 차단했습니다.
- CombatContext/PlayerOwnership 경로에도 동일한 포인터 정제 로직을 적용해 동일 계열 회귀를 줄였습니다.
- 포인터 안전 규칙과 훅 진입 정책을 정적 테스트로 추가해 회귀 방어를 강화했습니다.
- 빌드/배포 문서에 DLL 빌드 lane 분리(`g++` 테스트 lane vs `clang-cl` DLL lane)와 `Windows.h` 오류 대응을 명시했습니다.

## 주요 반영 사항
- 런타임 안정화
  - `skse/CalamityAffixes/src/Hooks.cpp`
  - `skse/CalamityAffixes/include/CalamityAffixes/CombatContext.h`
  - `skse/CalamityAffixes/include/CalamityAffixes/PlayerOwnership.h`
  - `skse/CalamityAffixes/include/CalamityAffixes/PointerSafety.h` (신규)
- 테스트
  - `skse/CalamityAffixes/tests/test_pointer_safety.cpp` (신규)
  - `skse/CalamityAffixes/CMakeLists.txt` 테스트 타겟 등록
- 문서
  - `AGENTS.md`
  - `README.md`
  - `docs/releases/release-note-template.md`
  - `docs/releases/2026-02-09-nexus-upload-playbook.md`

## 검증
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` 통과
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` 통과 (2/2)
- `tools/release_verify.sh --fast --with-mo2-zip` 통과
- 배포 ZIP 생성 완료:
  - `CalamityAffixes_MO2_v1.2.17-rc18_2026-02-18.zip`

## 참고
- 본 릴리즈는 프리릴리즈(Release Candidate)입니다.
- 동일 증상 재발 시 CrashLogger 로그와 함께 제보해 주세요.
