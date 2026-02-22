## Calamity Reactive Loot & Affixes v1.2.18-rc27 (Pre-release)

이번 RC는 요청하신 대로 **시체 드랍 경로를 SPID Item 방식으로 전환**하고,  
**상자/월드 드랍은 기존 SKSE 런타임 롤을 유지**한 빌드입니다.

---

### 핵심 변경

1) **시체 드랍: SPID Perk -> SPID Item 전환**
- `affixes/modules/keywords.spidRules.json`에서 시체 통화 분배 라인을 `Perk = ...`에서 `Item = ...`으로 변경
  - `Item = CAFF_LItem_RunewordFragmentDrops|...`
  - `Item = CAFF_LItem_ReforgeOrbDrops|...`
- 생성 산출물(`Data/CalamityAffixes_DISTR.ini`, `affixes.json`, runtime `affixes.json`) 동기화

2) **상자/월드 드랍 경로 유지**
- 하이브리드 정책 유지:
  - 시체: SPID Item 분배
  - 상자/월드: SKSE activation 롤
- activation 이벤트의 시체 분기는 계속 스킵(중복 롤 방지)하도록 유지
  - `skse/CalamityAffixes/src/EventBridge.Triggers.Events.cpp`

3) **정책 가드 강화 (회귀 방지)**
- Generator 로더에서 아래 라인을 정책 위반으로 즉시 실패 처리:
  - `DeathItem = ...` (기존 유지)
  - `Perk = CAFF_Perk_DeathDrop...` (레거시 시체 드랍 Perk 라인)
  - `tools/CalamityAffixes.Generator/Spec/AffixSpecLoader.cs`
- lint에서도 동일 정책 위반을 에러로 처리
  - `tools/lint_affixes.py`
- 관련 테스트 추가/갱신:
  - `tools/CalamityAffixes.Generator.Tests/AffixSpecLoaderTests.cs`
  - `tools/CalamityAffixes.Generator.Tests/SpidIniWriterTests.cs`

4) **문서 정책 동기화**
- README 드랍 정책을 SPID Item 기준으로 갱신
  - 시체는 SPID Item, 상자/월드는 런타임
  - MCM 확률 슬라이더의 즉시 반영 범위(상자/월드 런타임 중심) 명시
- 참조 정책 문서 갱신
  - `docs/references/2026-02-22-corpse-drop-compat-policy.md`

---

### 사용자 영향

- 시체 루팅 UX는 그대로 유지됩니다.
- 시체 통화 드랍은 SPID Item 분배에 의해 더 단순한 파이프라인으로 동작합니다.
- 상자/컨테이너/월드 통화 드랍은 기존 런타임 롤 정책을 그대로 유지합니다.
- SPID 분배는 로드 시점/분배 규칙 기반이므로, MCM 확률 변경이 이미 분배된 시체 인벤토리에 즉시 재분배되지는 않습니다.

---

### 검증

- `python3 tools/compose_affixes.py --check` PASS
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` PASS
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` PASS (60/60)
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` PASS
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (2/2)
- `python3 -m unittest tools/tests/test_compose_affixes.py` PASS (2/2)

