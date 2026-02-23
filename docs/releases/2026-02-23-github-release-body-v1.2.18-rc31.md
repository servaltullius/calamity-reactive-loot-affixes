## Calamity Reactive Loot & Affixes v1.2.18-rc31 (Pre-release)

이번 RC는 **EventBridge Config 1단계 모듈 분리(리팩터링)**에 집중한 유지보수성 개선 빌드입니다.

---

### 핵심 변경

1) **Config 리셋 로직 분리**
- `skse/CalamityAffixes/src/EventBridge.Config.Reset.cpp`
  - `ResetRuntimeStateForConfigReload()`를 전용 파일로 이동
  - 런타임 상태 초기화 책임을 분리해 `LoadConfig()` 가독성 개선

2) **런타임 설정/유저설정 로직 분리**
- `skse/CalamityAffixes/src/EventBridge.Config.RuntimeSettings.cpp`
  - `LoadRuntimeConfigJson(...)`
  - `ApplyLootConfigFromJson(...)`
  - `ApplyRuntimeUserSettingsOverrides(...)`
  - `SyncCurrencyDropModeState(...)`
  - `PersistRuntimeUserSettings(...)` 등 설정/동기화 함수군을 전용 파일로 이동

3) **원본 Config 파일 경량화**
- `skse/CalamityAffixes/src/EventBridge.Config.cpp`
  - `LoadConfig()`의 핵심 흐름(초기화 → 로드 → 인덱싱/합성)에 집중
  - 파일 크기 대폭 축소로 후속 분리 단계(Phase 2+) 준비

4) **빌드/정적검증 결합도 보정**
- `skse/CalamityAffixes/CMakeLists.txt`
  - 신규 분리 파일 빌드 등록
- `skse/CalamityAffixes/tests/runtime_gate_store_checks_settings_policy.cpp`
  - 런타임 유저설정 검증 포인트를 분리 파일 기준으로 갱신
  - 구조 리팩터링 시 정적 정책 테스트가 계속 동작하도록 보강

---

### 사용자 영향

- **게임플레이 동작/밸런스 수치 변경 없음**
- 내부 구조 정리로 향후 버그 수정/기능 확장 시 안정성과 작업 속도 개선

---

### 검증

- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` PASS
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release && ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (61/61, 2/2)
- `python3 tools/compose_affixes.py --check` PASS
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null` PASS
- `python3 scripts/vibe.py doctor --full` PASS

---

### 리뷰 요약

- High-signal 기준으로 점검했고, 이번 RC에서 확인된 고위험 이슈는 없습니다.
