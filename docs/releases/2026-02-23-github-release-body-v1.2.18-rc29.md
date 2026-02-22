## Calamity Reactive Loot & Affixes v1.2.18-rc29 (Pre-release)

이번 RC는 **레거시 시체 드랍 경로 정리 + 문서 정책 일치화**에 집중한 정리 빌드입니다.

---

### 핵심 변경

1) **Generator 레거시 시체 드랍 Perk 생성 제거**
- `tools/CalamityAffixes.Generator/Writers/KeywordPluginBuilder.cs`
  - 더 이상 `CAFF_Perk_DeathDropRunewordFragment` / `CAFF_Perk_DeathDropReforgeOrb`를 생성하지 않도록 정리
- 현재 정책은 시체 드랍을 `SPID Item` 분배로 고정하므로, 미사용 레거시 생성 경로를 제거해 유지보수 부담을 낮춤

2) **회귀 테스트 정책 갱신**
- `tools/CalamityAffixes.Generator.Tests/KeywordPluginBuilderTests.cs`
  - 하이브리드 설정에서 레거시 DeathDrop Perk가 생성되지 않는지 검증하도록 테스트 의도를 업데이트

3) **Nexus 설명 문서 정책 정합화**
- `docs/NEXUS_DESCRIPTION.md`
- `docs/NEXUS_DESCRIPTION_BBCODE.txt`
  - 시체 드랍 설명을 `SPID Perk + AddLeveledListOnDeath`에서
    `SPID Item distribution (actor inventory -> corpse loot on death)`로 수정

---

### 사용자 영향

- 실제 인게임 드랍 동작은 기존 `rc28`과 동일합니다.
- 이번 변경은 **정책/코드 정합성 강화 및 레거시 제거**가 목적이며, 체감 밸런스 변경은 없습니다.

---

### 검증

- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` PASS (61/61)
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (2/2)
- `python3 tools/compose_affixes.py --check` PASS
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null` PASS
- `python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null` PASS
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` PASS

---

### 참고

- `currencyDropMode`의 hybrid 강제와 SPID Item 시체 권한 정책은 유지됩니다.
- 레거시 호환 로딩(세이브/설정 해석)은 안전하게 유지됩니다.
