## Calamity Reactive Loot & Affixes v1.2.18-rc6 (Pre-release)

이번 RC는 룬워드 고유성 강화와 적응형(Adaptive) 런타임 정합성 보강에 집중했습니다.

### 핵심 변경
- **룬워드 전용 스펠/효과 확장**
  - 상위 룬워드(예: Infinity/Grief/HOTO/BotD/Last Wish/Doom/Dream)에 전용 `MGEF/SPEL` 레코드를 추가해 범용 스펠 재사용을 줄였습니다.
- **생성기 스펙 확장**
  - `records.magicEffects[]`, `records.spells[]`, `spell.effects[]`를 공식 지원하도록 확장했습니다.
  - 단일 필드(`magicEffect`, `spell`, `effect`)와의 하위호환은 유지합니다.
- **Adaptive 런타임 정책 정렬**
  - `CastSpellAdaptiveElement`는 기본적으로 저항 기반 적응형 선택을 유지합니다.
  - `modeCycle`은 수동 오버라이드(`manualOnly=true`)로만 동작하도록 강제/보정했습니다.
  - 수동 모드에서 `Auto` 슬롯(적응형 기본)을 유지해 전투 중 의도적으로만 고정 원소로 전환할 수 있습니다.
- **툴팁/린트 정합성 보강**
  - Adaptive 수동 모드의 `Auto` 상태를 툴팁에 명확히 표시합니다.
  - 린트가 `records.spells[]`와 Adaptive `modeCycle` 정책을 인식하도록 보강했습니다.

### 검증
- `python3 tools/compose_affixes.py --check` 통과
- `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data` 통과
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` 통과
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` 통과
- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --target CalamityAffixes` 통과
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` 통과
- `CAFF_PACKAGE_VERSION=1.2.18-rc6 tools/build_mo2_zip.sh` 통과

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc6_2026-02-21.zip`
