## Calamity Reactive Loot & Affixes v1.2.18-rc32 (Pre-release)

이번 RC는 **장비 재착용 시 이름 suffix 중복 버그 수정**과 **Config/Runeword 합성 파이프라인 모듈 분리 리팩터링**을 함께 반영한 안정화 빌드입니다.

---

### 핵심 변경

1) **아이템 이름 중복 suffix 버그 수정 (유저 체감 이슈)**
- `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp`
  - 대장간 재련(temper) suffix가 붙은 장비에서, 착용/해제 반복 시 `(초급)(초급)...`처럼 suffix가 누적되던 문제를 수정했습니다.
  - `ExtraTextDisplayData.customNameLength` 기준으로 **커스텀 이름 본체**만 재작성하고, 동적 temper suffix는 정규화 후 중복 제거합니다.
  - 재명명 비교 기준을 표시명(`GetDisplayName`)이 아닌 저장된 커스텀명으로 바꿔 `SetName` 반복 호출을 줄였습니다.

2) **Affix Config 파싱 모듈 분리 (유지보수성 개선)**
- 신규 분리:
  - `skse/CalamityAffixes/src/EventBridge.Config.AffixParsing.cpp`
  - `skse/CalamityAffixes/src/EventBridge.Config.AffixTriggerParsing.cpp`
  - `skse/CalamityAffixes/src/EventBridge.Config.AffixActionParsing.cpp`
  - `skse/CalamityAffixes/src/EventBridge.Config.LootRuntime.cpp`
- `EventBridge.Config.cpp`는 오케스트레이션 중심으로 경량화했고, 파싱 책임을 목적별 파일로 분리했습니다.

3) **Runeword 합성 로직 모듈 분리 (가독성/검증성 개선)**
- 신규 분리:
  - `skse/CalamityAffixes/src/EventBridge.Config.RunewordSynthesis.SpellSet.h/.cpp`
  - `skse/CalamityAffixes/src/EventBridge.Config.RunewordSynthesis.StyleSelection.cpp`
  - `skse/CalamityAffixes/src/EventBridge.Config.RunewordSynthesis.Style.cpp`
  - `skse/CalamityAffixes/src/EventBridge.Config.RunewordSynthesis.StyleExecution.cpp`
- `EventBridge.h`에 합성 스타일 enum/헬퍼 선언을 정리해 스타일 선택/튜닝/실행 책임을 분리했습니다.

4) **회귀 방지 검증 보강**
- `skse/CalamityAffixes/tests/runtime_gate_store_checks_loot_policy.cpp`
  - temper suffix 정규화 로직 존재를 런타임 게이트 체크에 추가했습니다.
- `skse/CalamityAffixes/tests/runtime_gate_store_checks_settings_policy.cpp`
  - Config 분리 구조를 반영해 정적 정책 체크 포인트를 업데이트했습니다.

---

### 사용자 영향

- **버그 수정:** 재련된 장비의 이름 suffix가 착용/해제 때 무한 누적되던 현상이 완화됩니다.
- **게임플레이 밸런스 수치 변화 없음:** 발동 확률/수치 밸런스 자체는 조정하지 않았습니다.
- **내부 안정성 개선:** 향후 버그 수정/확장 시 수정 범위와 리스크를 줄일 수 있는 구조로 정리했습니다.

---

### 검증

- `python3 scripts/vibe.py doctor --full` PASS
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` PASS
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release && ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (61/61, 2/2)
- `python3 tools/compose_affixes.py --check` PASS
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null` PASS

---

### 리뷰 요약

- High-signal 기준(bug/state consistency)으로 점검했고, 이번 RC 반영 범위에서 추가 고위험 이슈는 확인되지 않았습니다.
