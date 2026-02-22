## Calamity Reactive Loot & Affixes v1.2.18-rc24 (Pre-release)

Elden Rim 쪽 SPID 분배 패턴(`-PlayerKeyword` 기반)을 참고해, **SPID 시체 드랍의 “ActorType 누락” 사각지대**를 줄였습니다.

### 핵심 변경
- 시체 드랍(SPID `DeathItem`) 보강
  - 기존 ActorType 직접 필터 라인은 유지
  - ActorType 키워드가 누락된 몹(일부 모드 추가 몹)을 위한 **fallback DeathItem 라인** 추가
  - 조건은 `-PlayerKeyword` + “일반 ActorType 키워드들 미보유”로 구성해 **중복 드랍을 방지**합니다.

### 검증
- `python3 tools/compose_affixes.py --check` PASS
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` PASS
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` PASS (54/54)
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` PASS (2/2)

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc24_2026-02-22.zip`
