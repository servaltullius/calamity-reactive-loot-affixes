## Calamity Reactive Loot & Affixes v1.2.18-rc1 (Pre-release)

이번 RC는 게임플레이 중 프레임 저하 가능 구간을 집중 최적화한 성능 안정화 빌드입니다.

### 핵심 변경점
- 전투 트리거 핫패스 최적화
  - 활성 트리거 인덱스 캐시를 도입해 히트 이벤트당 불필요 순회를 줄였습니다.
  - `CalamityAffixes_Hit` ModEvent 중복 발행 억제 윈도우를 추가했습니다.
- 트랩 시스템 최적화
  - 트랩 틱에 라운드로빈 커서 + 방문 예산을 적용해 난전 시 스파이크를 완화했습니다.
  - 트랩이 모두 소멸하면 활성 플래그를 즉시 정리하도록 보강했습니다.
- 시체폭발 타깃 수집 최적화
  - 대상 수집 시 근접 상위 N 유지 방식으로 정렬/수집 비용을 줄였습니다.
- UI/로그 경량화
  - Prisma 폴링/설정 갱신 간격을 완화했습니다.
  - 드랍 알림(DebugNotification) 쿨다운을 추가해 우상단 메시지 스팸을 줄였습니다.
  - spdlog flush 정책을 `warn` 중심으로 조정해 런타임 로그 I/O 부담을 완화했습니다.
- 기본 성능 가드값 조정
  - `trapGlobalMaxActive`: 64 -> 48
  - `trapCastBudgetPerTick`: 8 -> 6
  - 장비 리빌드 기본 간격: 5000ms -> 8000ms

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.18-rc1_2026-02-21.zip`

### 검증
- `python3 tools/compose_affixes.py --check`
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json`
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`
- `CAFF_PACKAGE_VERSION=1.2.18-rc1 tools/build_mo2_zip.sh`
