## Calamity Reactive Loot & Affixes v1.2.17

`v1.2.16` 이후 RC 구간(`v1.2.17-rc*`)에서 검증한 변경사항을 묶은 정식 릴리즈입니다.

### 핵심 변경 사항
- 드랍 파이프라인 정리:
  - `loot.currencyDropMode`는 `hybrid` 단일 정책으로 정리
  - 시체 드랍은 SPID 우선 + 런타임 폴백 구조로 안정화
- UserPatch 배포/사용성 개선:
  - 배포 ZIP에 UserPatch 위저드/EXE 포함
  - MO2 프로필 탐지/로드오더 기반 패치 생성 흐름 개선
  - UserPatch 결과 플러그인 ESL 플래그(ESPFE) 처리 지원
- MCM/런타임 설정 안정화:
  - 룬워드 조각/재련 오브 확률 슬라이더 및 런타임 반영 경로 개선
  - 외부 설정 파일 영속화 경로 보강
- 룬워드/툴팁 품질 개선:
  - 룬워드 효과/이름 표시 일관성 개선(한글/영문 혼재 완화)
  - 누락되던 설명 텍스트 보강 및 패널 반영 안정화
- 런타임 안정성/검증 체계 강화:
  - DoT 관측 set 하드캡 추가 등 메모리 안전성 보강
  - 루팅 핫패스 로그/조회 비용 정리
  - `lint_affixes.py`에 JSON Schema 검증(`--schema`) 추가 및 CI 반영

### 기본값/정책
- 기본 드랍 확률:
  - 룬워드 조각: `5%`
  - 재련 오브: `3%`
- 기본 드랍 모드: `hybrid`

### 검증
- `tools/release_verify.sh --fast`
- `CAFF_PACKAGE_VERSION=1.2.17 tools/build_mo2_zip.sh`

### 산출물
- `CalamityAffixes_MO2_v1.2.17_2026-02-21.zip`
