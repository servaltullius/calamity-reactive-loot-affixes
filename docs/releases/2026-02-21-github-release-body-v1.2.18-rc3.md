## Calamity Reactive Loot & Affixes v1.2.18-rc3 (Pre-release)

이번 RC는 드랍/분배 경로를 **하이브리드(SPID 시체 + 월드/컨테이너 런타임)** 기준으로 고정하고,
`UserPatch` 및 LVLI(레벨드리스트) 레거시를 코드/도구/문서에서 정리하는 데 집중했습니다.

### 핵심 변경
- **UserPatch 체계 제거**
  - `tools/CalamityAffixes.UserPatch` 프로젝트 삭제
  - `build_user_patch.sh`, `build_user_patch_wizard.*` 삭제
  - Generator 내부 `CurrencyUserPatchBuilder` 제거
- **스펙 레거시 키 제거/차단**
  - `loot.currencyLeveledListTargets` 제거
  - `loot.currencyLeveledListAutoDiscoverDeathItems` 제거
  - 과거 키가 입력되면 로더가 명시적 예외를 발생시켜 조기 실패
- **Generator CLI 단순화**
  - `--masters` 인자 제거
  - 생성 기본 흐름을 `--spec + --data` 중심으로 고정
- **문서/설명 동기화**
  - README, Nexus 설명(마크다운/BBCode)에서 UserPatch/레벨드리스트 의존 문구 정리
  - 현재 운영 철학을 하이브리드 고정 모델로 명확화

### 검증
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release` 통과 (47/47)
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure` 통과 (2/2)
- `python3 tools/compose_affixes.py --check` 통과
- `python3 tools/lint_affixes.py --spec affixes/affixes.json --manifest affixes/affixes.modules.json --generated Data/SKSE/Plugins/CalamityAffixes/affixes.json` 통과

### 산출물
- `CalamityAffixes_MO2_v1.2.18-rc3_<YYYY-MM-DD>.zip`
