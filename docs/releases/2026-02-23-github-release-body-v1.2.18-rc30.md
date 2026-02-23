## Calamity Reactive Loot & Affixes v1.2.18-rc30 (Pre-release)

이번 RC는 **코드 심층 리뷰 결과 반영(런타임 상태 리셋 안정화)**에 집중한 안정화 빌드입니다.

---

### 핵심 변경

1) **Load/Revert transient 상태 리셋 대칭화**
- `skse/CalamityAffixes/src/EventBridge.Serialization.cpp`
  - `Load(...)`와 `Revert(...)` 모두에서 전투/트리거 관련 일시 상태를 동일하게 초기화하도록 보강
  - 포함 항목(핵심):
    - 활성 어픽스 카운트/활성 트리거 캐시
    - DoT cooldown 캐시
    - per-target cooldown store / non-hostile first-hit gate
    - recent combat event 캐시
    - low-health consumed/snapshot 캐시
    - trigger proc budget 윈도우 상태

2) **회귀 방지 테스트 추가**
- `skse/CalamityAffixes/tests/runtime_gate_store_checks_loot_policy.cpp`
  - `CheckSerializationTransientRuntimeResetPolicy()` 추가
  - `Load/Revert` 양쪽에 필수 리셋 패턴이 존재하는지 검증
- `skse/CalamityAffixes/tests/runtime_gate_store_checks_common.h`
- `skse/CalamityAffixes/tests/runtime_gate_store_checks_main.cpp`
  - 신규 검증 함수를 런타임 게이트 체크 스위트에 연결

---

### 사용자 영향

- 세이브 로드/리버트 이후 남아 있던 내부 전투 상태로 인해 발생할 수 있는
  **유령 프로크/쿨다운 누수/상태 잔류 리스크를 완화**합니다.
- 전투 체감 밸런스 수치 자체는 변경하지 않았습니다.

---

### 검증

- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` PASS (61/61)
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` PASS
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (2/2)
- `python3 tools/compose_affixes.py --check` PASS
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null` PASS

---

### 리뷰 요약

- High-signal 기준(bug/state consistency)으로 검토했고,
  이번 RC에서 확인된 고위험 이슈는 위 1건을 패치로 해소했습니다.
