## Calamity Reactive Loot & Affixes v1.2.17-rc64 (Pre-release)

이번 RC는 **룬워드 조각/재련 오브 드랍의 SPID + 런타임 하이브리드 안정화**가 핵심입니다.

### 핵심 변경 (중점)
- **시체 드랍: SPID DeathItem 경로 고정**
  - `CAFF_TAG_CURRENCY_DEATH_DIST` 태그를 기준으로
  - `CAFF_LItem_RunewordFragmentDrops`, `CAFF_LItem_ReforgeOrbDrops`를 시체 드랍에 분배
- **상자/월드 드랍: SKSE 런타임 롤 유지**
  - 시체 외 루팅 경로(컨테이너/월드)는 기존 런타임 롤로 처리
- **폴백 보강**
  - SPID 태그가 없는 시체(일부 모드 추가 적 포함)는 런타임 활성화 시점 롤이 폴백으로 동작
- **중복 롤 방지**
  - SPID 태그 대상 시체에서 런타임 시체 롤을 스킵해 이중 드랍 가능성 제거
- **확률 동기화 안정화**
  - 런타임 모드에서도 `CAFF_LItem_*` 확률 동기화를 유지해 MCM/설정 반영 일관성 개선
- **기본 확률 상향**
  - 룬워드 조각 `16%`
  - 재련 오브 `10%`

### 보조 변경
- `spidRules` 로딩/렌더링 안정화 (JSON 문자열 처리 보강)
- 스펙 유지보수성 개선을 위한 모듈화 기반 도입 (`affixes/modules/*.json` → `affixes/affixes.json` 조합)

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc64_2026-02-20.zip`

### 검증
- `tools/release_verify.sh --fast` 통과
- `CAFF_PACKAGE_VERSION=1.2.17-rc64 tools/build_mo2_zip.sh` 통과
