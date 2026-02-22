## Calamity Reactive Loot & Affixes v1.2.18-rc25 (Pre-release)

이번 RC는 **시체 루팅 호환성 안정화**에 초점을 맞췄습니다.  
핵심은 시체 드랍 경로를 `DeathItem` 직접 분배에서, **SPID Perk 분배 + Perk EntryPoint(`AddLeveledListOnDeath`)**로 전환한 것입니다.

---

### 왜 바꿨나

- 사용자 피드백에서 “Calamity 활성화 시 일부 타 모드 드랍 루팅이 사라진다”는 증상이 확인되었습니다.
- 특히 Adventurer’s Guild(HDA 계열)처럼 자체 시체 드랍/DeathItem 체계를 사용하는 모드와의 공존성이 중요했습니다.
- 목표는 단순히 우리 통화(룬조각/재련오브)를 떨구는 것이 아니라, **타 모드 루팅을 방해하지 않는 방식**으로 유지하는 것이었습니다.

---

### 핵심 변경

1) **시체 통화 드랍 메커니즘 전환 (중요)**
- 이전:
  - SPID `DeathItem = CAFF_LItem_*` 라인으로 직접 분배
- 현재:
  - SPID `Perk = CAFF_Perk_DeathDrop*` 분배
  - 해당 Perk의 EntryPoint가 `AddLeveledListOnDeath`를 호출해 시체 드랍 리스트를 추가 롤

2) **CalamityAffixes.esp 생성기 확장**
- 생성기가 아래 사망 드랍 Perk 2개를 자동 생성합니다.
  - `CAFF_Perk_DeathDropRunewordFragment`
  - `CAFF_Perk_DeathDropReforgeOrb`
- 각 Perk는 각각의 LVLI를 사망 시점에 추가 롤합니다.
  - `CAFF_LItem_RunewordFragmentDrops`
  - `CAFF_LItem_ReforgeOrbDrops`

3) **SPID 규칙 단순화**
- `affixes/modules/keywords.spidRules.json`에서 `DeathItem` 라인을 제거하고 `Perk` 라인으로 통일
- 생성물 `Data/CalamityAffixes_DISTR.ini`도 동일 정책 반영

4) **문서화 강화**
- README 드랍 경로 정책을 새 구조로 갱신
- 호환성 정책 문서 신설:
  - `docs/references/2026-02-22-corpse-drop-compat-policy.md`

---

### 사용자 영향

- 시체에서 룬조각/재련오브를 루팅하는 UX는 유지됩니다.
- 타 모드 시체 드랍과의 충돌 가능성을 낮추는 방향으로 동작합니다.
- 상자/월드 드랍은 기존처럼 SKSE 런타임 롤 경로를 사용합니다.

---

### 알려진 제한

- 하이브리드 정책상 시체 경로는 SPID 필터 기반이므로,
  - `ActorType` 필터에 매칭되지 않는 특수 몹은 시체 통화 드랍이 없을 수 있습니다.
- 본 RC는 “시체 루팅 호환성” 안정화가 목적이며, 룬워드/재련 확률 기본값(8%/5%)은 유지됩니다.

---

### 검증

- `python3 tools/compose_affixes.py --check` PASS
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` PASS
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` PASS (55/55)
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (2/2)
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` PASS
- `python3 tools/wb_compat_check.py Data/CalamityAffixes.esp` PASS (No issues)

---

### 산출물

- `CalamityAffixes_MO2_v1.2.18-rc25_2026-02-22.zip`

