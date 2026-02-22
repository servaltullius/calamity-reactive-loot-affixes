# Corpse Loot Compatibility Policy (2026-02-22)

## 배경

- `DeathItem` 직접 분배 방식은 모드 조합에 따라 타 모드 시체 드랍 체감을 저해할 수 있음.
- 실제 유저 피드백 기준 목표는 **“시체 루팅 UX 유지 + 타 모드 드랍 공존”**.

## 확정 정책

- 하이브리드 드랍(`loot.currencyDropMode=hybrid`)에서:
  - 시체: **SPID가 Perk를 배포**
  - 드랍 실행: **Perk EntryPoint `AddLeveledListOnDeath`**
  - 상자/월드: SKSE 런타임 롤
- `DeathItem` 직접 분배 라인은 사용하지 않음.

## 현재 구현 포인트

- SPID 규칙:
  - `affixes/modules/keywords.spidRules.json`
  - 생성물: `Data/CalamityAffixes_DISTR.ini`
- ESP 생성기(사망 드랍 Perk 생성):
  - `tools/CalamityAffixes.Generator/Writers/KeywordPluginBuilder.cs`
  - 생성 Perk:
    - `CAFF_Perk_DeathDropRunewordFragment`
    - `CAFF_Perk_DeathDropReforgeOrb`

## 유지보수 가드레일

- 시체 드랍 경로를 건드릴 때 아래를 반드시 확인:
  1. `_DISTR.ini`에 `DeathItem = ...`가 다시 들어오지 않았는지
  2. ESP에 위 2개 Perk + `AddLeveledListOnDeath`가 유지되는지
  3. 타 모드 드랍(예: HDA/Adventurer Guild) 루팅 회귀가 없는지

