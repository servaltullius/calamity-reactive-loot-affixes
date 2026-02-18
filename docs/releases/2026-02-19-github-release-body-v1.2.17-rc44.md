## CalamityAffixes v1.2.17-rc44

`v1.2.17-rc43` 후속 프리릴리즈입니다. MCM에서 룬워드 조각/재련 오브 확률을 직접 조정할 수 있도록 옵션을 추가하고, 런타임 반영 안정성을 보강했습니다.

### 핵심 변경
- MCM 확률 슬라이더 추가
  - `Runeword Fragment Chance / 룬워드 조각 확률`
  - `Reforge Orb Chance / 재련 오브 확률`
- 런타임 연동 강화
  - MCM 변경값을 ModEvent로 SKSE 런타임에 즉시 반영
  - 값 범위는 런타임에서 `0~100`으로 clamp
- 리셋/설정 누락 방어
  - 기본값 리셋 경로에서도 즉시 재동기화
  - 설정 누락(-1 sentinel) 시 안전 기본값(`8.0`, `3.0`) 폴백
- 회귀 방어 추가
  - `runtime_gate_store_checks`에 MCM 드랍 확률 브릿지 정책 테스트 추가

### 검증
- `CAFF_PACKAGE_VERSION=1.2.17-rc44 tools/release_verify.sh --fast --with-mo2-zip` 통과
  - Generator tests: PASS (38/38)
  - SKSE ctest: PASS (2/2)
  - Papyrus compile: PASS
  - lint_affixes: PASS
  - MO2 ZIP 생성: PASS

### 배포 파일
- `dist/CalamityAffixes_MO2_v1.2.17-rc44_2026-02-19.zip`

### 참고
- 본 버전은 프리릴리즈(Release Candidate)입니다.
