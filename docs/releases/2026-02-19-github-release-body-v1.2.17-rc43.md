## CalamityAffixes v1.2.17-rc43

`v1.2.17-rc42` 후속 프리릴리즈입니다. 드랍 경제 밸런스 조정을 위해 룬워드 조각/재련 오브 기본 확률을 하향했습니다.

### 핵심 변경
- 드랍 확률 조정
  - `runewordFragmentChancePercent`: `10.0 -> 8.0`
  - `reforgeOrbChancePercent`: `4.0 -> 3.0`
- 데이터 재생성
  - 생성기 기준으로 `Data/SKSE/Plugins/CalamityAffixes/affixes.json`, `Data/CalamityAffixes_KID.ini`, `Data/CalamityAffixes_DISTR.ini` 동기화

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc43 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc43_2026-02-19.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
