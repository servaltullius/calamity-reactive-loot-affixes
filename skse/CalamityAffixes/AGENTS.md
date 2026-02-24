# SKSE Runtime Guide

## Scope

- 대상: `skse/CalamityAffixes/**` (런타임 C++ 플러그인 + 런타임 테스트)
- 엔트리포인트: `skse/CalamityAffixes/src/main.cpp`
- 목표: SKSE DLL 안정성, 런타임 상태 일관성, 저장/로드 안전성 유지

## Structure

- `src/EventBridge.*.cpp`: 도메인 분할된 런타임 핵심 로직
- `src/Hooks.cpp`: 전투 훅 진입점
- `src/EventBridge.Serialization.cpp`: co-save save/load/revert/form-delete 처리
- `src/TrapSystem.cpp`: trap tick 스케줄링 브리지
- `include/CalamityAffixes/*.h`: 공용 헤더/도메인 타입
- `tests/*.cpp`: static/runtime gate checks

## Build And Test

```bash
cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes
ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure
```

- 런타임 게이트 테스트 프로젝트: `skse/CalamityAffixes/tests/CMakeLists.txt`
- 정적 체크 타깃: `CalamityAffixesStaticChecks`

## Conventions

- 런타임 로직은 `EventBridge.<domain>.<subdomain>.cpp` 네이밍을 유지합니다.
- 새 상태 필드 추가 시 save/load/revert 경로와 prune 경로를 함께 갱신합니다.
- `src/` 구현은 `include/` 공개 API 경계를 넘지 않도록 유지합니다.
- `CMAKE_CXX_STANDARD 23` 기준으로 작성합니다.

## Anti-Patterns

- `linux-release-commonlibsse` 결과물을 DLL 산출 경로로 사용하지 마세요(테스트/정적체크 전용).
- `extern/**` 또는 `.xwin/**`를 수정하지 마세요.
- 훅 경로에서 공유 상태를 락/메인스레드 보장 없이 갱신하지 마세요.
- 저장 포맷 변경 시 버전 마이그레이션/호환 처리 없이 기존 레코드 의미를 바꾸지 마세요.

## Where To Look

- 전투 트리거/프로크: `src/EventBridge.Triggers.HealthDamage.cpp`, `src/EventBridge.Triggers.Events.cpp`, `src/EventBridge.Triggers.cpp`
- 아이템 드랍/인스턴스키/리롤 가드: `src/EventBridge.Loot.cpp`, `src/EventBridge.Loot.*.cpp`
- 직렬화/정리: `src/EventBridge.Serialization.cpp`
- 훅 설치/라우팅: `src/Hooks.cpp`
