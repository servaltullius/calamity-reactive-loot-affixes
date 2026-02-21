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

### 추가 업데이트 (문서/가이드)
- **룬워드 효과 설명 문서 갱신**
  - `doc/5.룬워드_94개_효과_정리.md`를 최신 런타임 기준으로 교정했습니다.
  - `Call to Arms` 트리거 표기(피격 -> 적중), `Faith` 효과 설명(광신의 돌격) 등 최신 상태 반영.
- **Nexus 상세 설명 동기화**
  - `docs/NEXUS_DESCRIPTION.md`
  - `docs/NEXUS_DESCRIPTION_BBCODE.txt`
  - 현재 빌드 기준 정책(직접 정의 16 + 자동 합성 78, Adaptive 수동 오버라이드)을 반영했습니다.
- **94개 룬워드 변경 비교표 추가**
  - `docs/reviews/2026-02-21-runeword-94-diff-rc2-to-rc6.md`
  - 기준: `v1.2.18-rc2 -> v1.2.18-rc6`, 결과: 변경 29 / 유지 65.
- **룬워드 패널 UI 개선**
  - 선택된 레시피의 효과/권장 베이스/룬 순서/상세를 패널 상태 영역에 표시하도록 개선했습니다.
  - 레시피 리스트 항목 hover 시 효과/상세가 툴팁으로 표시되어, 제작 전 효과 파악이 쉬워졌습니다.

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
