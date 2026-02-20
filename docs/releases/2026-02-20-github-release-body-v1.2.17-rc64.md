## Calamity Reactive Loot & Affixes v1.2.17-rc64 (Pre-release)

이번 RC는 **드랍 파이프라인 안정화 + 스펙 모듈화 기반 정비**에 초점을 맞췄습니다.

### 핵심 변경
- 드랍 경로 하이브리드 안정화
  - 시체: SPID `DeathItem` 분배(태그 `CAFF_TAG_CURRENCY_DEATH_DIST`)
  - 상자/월드: SKSE 런타임 롤
  - SPID 태그가 없는 시체는 런타임 롤 폴백
- SPID 규칙 렌더링 보강
  - `keywords.spidRules` 로드 시 JSON 문자열 타입 처리 누락을 수정해, 스펙의 규칙 라인이 `_DISTR.ini`에 정확히 반영되도록 개선
- 중복 드랍 가드
  - 시체가 SPID 태그 분배 대상일 때 런타임 시체 롤을 건너뛰어 중복 롤을 방지
- 확률 동기화 개선
  - 런타임 모드에서도 `CAFF_LItem_RunewordFragmentDrops`, `CAFF_LItem_ReforgeOrbDrops` 생성/동기화를 유지
- 기본 드랍 확률 상향
  - 룬워드 조각: `16%`
  - 재련 오브: `10%`

### 스펙 모듈화 (유지보수성)
- `affixes/affixes.json` 단일 대형 파일을 모듈 소스로 분리
  - `affixes/affixes.modules.json`
  - `affixes/modules/*.json`
- 신규 조합 도구 추가
  - `python3 tools/compose_affixes.py`
  - `python3 tools/compose_affixes.py --check`
- 빌드/검증 스크립트 연동
  - `build_mo2_zip.sh`, `release_verify.sh`, `build_user_patch.sh`에서 조합 단계를 자동 수행
  - `lint_affixes.py`에 모듈 매니페스트 stale 검사 추가

### 문서/운영 가이드
- `README.md`, `docs/NEXUS_DESCRIPTION.md`, `AGENTS.md`를 모듈화 워크플로우 기준으로 갱신

### 포함 산출물
- `CalamityAffixes_MO2_v1.2.17-rc64_2026-02-20.zip`
  - `Tools/UserPatch/CalamityAffixes.UserPatch.exe`
  - `Tools/UserPatch/build_user_patch_wizard.cmd`
  - `Tools/UserPatch/build_user_patch_wizard.ps1`
  - `Tools/UserPatch/affixes/affixes.json`

### 검증
- `tools/release_verify.sh --fast` 통과
  - lint: PASS
  - Generator tests: PASS (48/48)
  - UserPatch build: PASS
  - SKSE ctest: PASS (2/2)
- `CAFF_PACKAGE_VERSION=1.2.17-rc64 tools/build_mo2_zip.sh` 통과
