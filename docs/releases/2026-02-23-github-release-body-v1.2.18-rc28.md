## Calamity Reactive Loot & Affixes v1.2.18-rc28 (Pre-release)

이번 RC는 보고해주신 **CoC 번개 계열 히트 아크 잔상** 이슈를 우선적으로 수정한 안정화 빌드입니다.

---

### 핵심 변경

1) **CoC 피드백 VFX에서 Shock 계열 아트 차단**
- `skse/CalamityAffixes/src/Hooks.cpp`
  - `ResolveCastOnCritProcFeedbackArt(...)` 경로에서 `ResistShock` 매직이펙트는 피드백 HitArt 선택 대상에서 제외
- 목적:
  - 화면에 번개 아크가 잔류하는 케이스를 원천 차단
  - 잔류 이펙트 이후 CoC가 "안 터지는 것처럼 보이는" 체감 문제 완화

2) **회귀 방지 테스트 추가**
- `tools/CalamityAffixes.Generator.Tests/RepoSpecRegressionTests.cs`
  - `RuntimeHook_CastOnCrit_SkipsFeedbackVfxForShockSpells` 추가
  - 런타임 훅 소스에 Shock 제외 가드가 존재하는지 고정 검증

---

### 사용자 영향

- CoC 번개 계열(예: Lightning Bolt / Thunderbolt / Chain Lightning)은
  **추가 피드백 VFX를 강제로 띄우지 않으며**, 사운드 피드백은 유지됩니다.
- Fire/Frost 계열의 짧은 안전 피드백 VFX 동작은 기존과 동일합니다.
- 드랍/룬워드/런타임 통화 드랍 경로 정책은 이번 RC에서 변경되지 않습니다.

---

### 검증

- `python3 scripts/vibe.py doctor --full` PASS
- `tools/release_verify.sh --fast` PASS
  - compose/lint/MCM JSON sanity PASS
  - `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` PASS (61/61)
  - `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (2/2)
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` PASS

---

### 참고

- 본 RC는 **시각 잔상 안정화 핫픽스** 성격이며, 기존 데이터/드랍 밸런스에는 영향을 주지 않습니다.
