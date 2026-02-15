## CalamityAffixes v1.2.11

프레임 드랍 완화 중심 안정화 릴리즈입니다.

### 핵심 변경
- 트리거 프로크에 0ms ICD + 100% 확률 조합이 있을 때 발생하던 연쇄 발동 스파이크를 완화했습니다.
- 장비 인스턴스 키 조회 경로에 캐시를 도입해 모드 효과 전환/진화 시 반복 인벤토리 스캔 비용을 줄였습니다.
- TESHitEvent fallback 경로에서 내부 프로크 스펠 소스를 가드해 재귀성 과발동 가능성을 줄였습니다.
- 함정/시체 폭발 대상 스캔을 거리 제곱 비교 기반으로 최적화하고, 대량 타깃 정렬 비용을 줄였습니다.
- 핫패스의 디버그 로그 평가를 조건부로 바꿔 런타임 오버헤드를 줄였습니다.

### 안정성/도구
- `ResolveTriggerProcCooldownMs` 유틸과 정적 체크 테스트를 추가했습니다.
- Generator 키워드 빌더 테스트를 보강했습니다.

### 검증
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` 통과 (25/25)
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixesStaticChecks` 통과
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` 통과
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` 통과

### 배포 파일
- `CalamityAffixes_MO2_v1.2.11_2026-02-15.zip`
