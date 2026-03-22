## Calamity Reactive Loot & Affixes v1.2.20

`v1.2.20`은 `rc2`부터 `rc23`까지 검증한 변경을 정리한 정식 릴리즈입니다.
이번 정식판은 룬워드/어픽스 콘텐츠 재설계, EventBridge·PrismaTooltip 구조 정리, 전투 프리징 수정, Bloom 트랩 피드백 보강, SE/AE 공용 빌드 안정화, Linux/WSL 릴리즈 검증 보강까지 한 사이클로 묶습니다.

---

### 주요 변경 사항

- 룬워드 효과군을 스카이림 마법 학파 기반으로 재설계하고, 중복 효과를 고유 효과로 치환했으며, 재변환/재련 UX와 워크벤치 패널 레이아웃을 대폭 손봤습니다.
- 프리픽스/서픽스 데이터와 한국어 설명을 대규모로 정리해 utility prefix 3종 복귀, suffix 패밀리 재편, 공개 카탈로그 정리를 반영했습니다.
- `EventBridge`/`PrismaTooltip`/serialization/runtime ownership을 helper와 controller 중심으로 분해해 유지보수성과 테스트 가능성을 높였습니다.
- Bloom 계열 트랩에 `planted / burst` 피드백과 생성 실패 진단을 추가해, 심김/대기/발동을 인게임에서 확인할 수 있게 했습니다.
- 플레이어 피격 시 `TESHitEvent` fallback 재진입으로 이어지던 전투 프리징을 수정하고, `HandleHealthDamage` hook 기반 incoming hit routing으로 안정화했습니다.
- SE/AE 공용 CommonLibSSE 타깃과 stale build cache 문제를 정리해 Address Library 초기화 실패를 막고, 릴리즈 패키징 경로를 안정화했습니다.
- 특수 액션 계약을 강화해 잘못된 `runtime.trigger`나 암묵적인 `0 = always-on` 보정을 제거하고, `debugHudNotifications`와 `debugVerboseLogging`을 분리했습니다.
- Linux/WSL 환경에서도 generator·runtime contract sync·MO2 ZIP 검증을 계속 수행할 수 있도록 `CAFF_DOTNET` 주입과 패키징 fallback을 보강했습니다.

### 검증

- `python3 -m unittest discover -s tools/tests -p 'test_*.py'`
- `CAFF_DOTNET=/home/kdw73/.dotnet/dotnet CAFF_PACKAGE_VERSION=1.2.20 tools/release_verify.sh --fast --strict --with-mo2-zip`
- 인게임 스모크 테스트
  - 실제 설치 ZIP 로드 확인
  - 전투/피격 프리징 없음
  - Bloom/Trap 발동 확인
  - 인게임에서 별도 문제 없음

### 배포 파일

- `CalamityAffixes_MO2_v1.2.20_2026-03-22.zip`
- SHA256: `f04489288b1bb5f7bda448237e3a7ea5fb2d7a0f3666274025e307a814fa96a4`

### 비교

- https://github.com/servaltullius/calamity-reactive-loot-affixes/compare/v1.2.19.2...v1.2.20
