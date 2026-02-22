## Calamity Reactive Loot & Affixes v1.2.18-rc26 (Pre-release)

이번 RC는 **시체 드랍 2안(Perk + AddLeveledListOnDeath) 고정 안정화**와  
**CoC(선더볼트 계열) 피드백 이펙트 안전화**를 함께 반영한 빌드입니다.

---

### 핵심 변경

1) **시체 드랍 2안 강제 가드 추가**
- Generator 로더에서 `keywords.spidRules[].line`에 `DeathItem = ...`가 들어오면 즉시 실패하도록 변경
  - `tools/CalamityAffixes.Generator/Spec/AffixSpecLoader.cs`
- lint 단계에서도 동일 정책 위반을 에러로 처리
  - `tools/lint_affixes.py`
- 효과: 실수로 DeathItem 라인이 재유입되어 시체 루팅 호환성이 흔들리는 회귀를 빌드 타임에 차단

2) **사망 이벤트 디버그 프로브 추가 (원인분리용)**
- TESDeathEvent 경로에 “분배/실행 상태” 점검 로그를 추가
  - Perk 존재/배포 여부
  - LVLI 존재/ChanceNone 값
  - death 시점 시체 인벤토리 통화 스냅샷(룬조각/재련오브)
  - `skse/CalamityAffixes/src/EventBridge.Triggers.Events.cpp`
- 효과: “분배는 됐는데 실행이 안 됨” vs “분배 자체가 안 됨”을 로그로 빠르게 분리 가능

3) **CoC 피드백 이펙트 안전화**
- 기존 hit shader 기반 피드백 대신, 짧은 hit art + sfx 경량 피드백으로 교체
  - 타겟별 짧은 ICD(120ms) 가드 추가
  - `skse/CalamityAffixes/src/Hooks.cpp`
- 회귀 방지 회귀테스트 추가
  - `tools/CalamityAffixes.Generator.Tests/RepoSpecRegressionTests.cs`

4) **문서/배포 문구 동기화**
- NEXUS 설명(한/영)에서 시체 드랍 경로를 2안 기준으로 통일
  - `docs/NEXUS_DESCRIPTION.md`
  - `docs/NEXUS_DESCRIPTION_BBCODE.txt`
- 호환성 정책 문서 갱신
  - `docs/references/2026-02-22-corpse-drop-compat-policy.md`

---

### 사용자 영향

- 시체 루팅 UX는 유지됩니다.
- 타 모드 드랍 공존성 방침(2안)이 빌드 단계에서 강제됩니다.
- CoC 피드백은 “지속 잔상 위험이 큰 shader” 대신 “짧은 안전 피드백”으로 동작합니다.

---

### 검증

- `python3 tools/compose_affixes.py --check` PASS
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` PASS
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` PASS (59/59)
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` PASS
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (2/2)
- `python3 -m unittest tools/tests/test_compose_affixes.py` PASS (2/2)
- `CAFF_PACKAGE_VERSION=1.2.18-rc26 tools/build_mo2_zip.sh` PASS

---

### 산출물

- `CalamityAffixes_MO2_v1.2.18-rc26_2026-02-23.zip`
