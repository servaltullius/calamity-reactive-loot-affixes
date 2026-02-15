## CalamityAffixes v1.2.12

룬워드 파밍 템포를 올린 밸런스 조정 릴리즈입니다.

### 핵심 변경
- 룬워드 조각 획득 확률을 상향했습니다.
  - `loot.runewordFragmentChancePercent`: `0.3` -> `5.0`
- 적용 파일:
  - `affixes/affixes.json`
  - `Data/SKSE/Plugins/CalamityAffixes/affixes.json`

### 검증
- `tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: 통과 (25/25)
  - SKSE static checks (`ctest`): 통과
  - Papyrus compile: 통과
  - lint_affixes: 통과

### 배포 파일
- `CalamityAffixes_MO2_v1.2.12_2026-02-15.zip`
