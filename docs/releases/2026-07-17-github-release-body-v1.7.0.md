# Calamity - Reactive Loot & Affixes v1.7.0

이번 프리릴리스는 적격 적 시체에서 얻는 룬워드 조각과 재련 오브의 기본 체감 빈도를 높이고, Prisma 룬워드 패널을 실제 패널 너비에 맞는 작업 흐름으로 정리한 공개 테스트 빌드입니다.

자동 검증과 별도 fresh checkout 재현은 게시 전에 완료합니다. 다만 실제 게임을 오래 플레이해야 하는 드랍 체감 검증, 패널의 인게임 시각 확인과 v1.6.0 VFX smoke test는 아직 완료되지 않았으므로 중요한 세이브는 별도로 백업해 주세요.

## 기본 통화 드랍률 조정

- 룬워드 조각: 적격 적 시체당 기본 `8% → 12%`
- 재련 오브: 적격 적 시체당 기본 `5% → 7%`
- 통화 판정은 계속 SKSE death-event 기반 eligible hostile corpse-only 경로에서만 수행합니다.
- 일반 상자·컨테이너, 아이템 픽업, 월드 생성과 새 SPID 통화 분배는 사용하지 않습니다.
- 룬 조각 99회 무드롭 피티, 시체별 카테고리 ledger와 룬 가중치 `El-Amn=4 / Sol-Um=3 / Mal-Lo=2 / Sur-Zod=1`은 변경하지 않았습니다.

`12% / 7%`는 새 설치 또는 해당 사용자 override가 없는 경우의 기본값입니다. 기존 설치에서 `Data/SKSE/Plugins/CalamityAffixes/user_settings.json`에 저장된 MCM 확률은 사용자의 선택을 보존하기 위해 계속 우선 적용됩니다. 기존 사용자도 새 권고값을 사용하려면 MCM에서 룬 조각 `12`, 재련 오브 `7`로 설정해 주세요.

## Prisma 룬워드 패널 P1 개선

- 패널 자체 너비를 기준으로 작업대를 3열·2열·1열로 전환합니다.
  - `1100px` 이상: 베이스·레시피·실행 3열
  - `900px` 이상 `1100px` 미만: 베이스와 레시피 2열, 실행 영역은 다음 행 전체 너비
  - `900px` 미만: 세 작업 영역과 실행 버튼을 1열로 배치
- 중복된 상단 진행 스트립을 제거하고 `Base Selection`, `Recipe Explorer`, `Review & Action` 카드 자체가 active·complete·muted 상태와 현재 단계를 표시합니다.
- 레시피 검색 아래에 `All / Weapon / Armor / Mixed` 필터를 추가했습니다. 검색어와 필터는 AND 조건으로 적용됩니다.
- 필터로 현재 선택 레시피가 목록에서 숨겨져도 선택과 오른쪽 검토 상태를 유지합니다.
- `Transmute`를 유일한 주 실행 버튼으로 유지하고, `Reforge / Check Status`와 기본 접힌 `Recovery & Reset`을 역할별로 분리했습니다.
- 영어·한국어·이중 표기와 기본값 `English + 한국어`를 유지합니다.

## 호환성

- `runeword.insert`, `runeword.reforge`, `runeword.status`, `runeword.reset` 명령과 C++ 처리 의미는 변경하지 않았습니다.
- C++↔JS interop payload, MCM 옵션, 패널 위치·크기 저장 형식과 코세이브 직렬화 토큰을 변경하지 않았습니다.
- 룬워드 선택·변환·재련·초기화 조건과 243개 공개 효과의 수치·발동 계약을 변경하지 않았습니다.
- 신규 이미지·아이콘·커스텀 자산은 추가하지 않았습니다.
- 기존 세이브에서 바로 업데이트할 수 있으며 새 게임은 필요하지 않습니다.

## 자동 검증

- affix compose/check 및 데이터 lint: 통과, 경고 0개
- Prisma Node 동작·스크롤 테스트: 2/2 통과
- Python 도구 테스트: 75/75 통과
- Generator Release 테스트: 129/129 통과
- SKSE clang-cl Release 빌드: 성공
- SKSE CTest/runtime gate: 3/3 통과
- Papyrus 3종 컴파일: 성공, 오류·경고 0개
- MO2 패키지 구조, 정확한 PEX 집합, CRC, v1.7.0 버전과 DLL 해시: 통과
- 커밋된 소스의 fresh checkout 전체 릴리스 재현: 통과 (고정 SDK·의존성 캐시 재사용)

## 배포 파일

- `CalamityAffixes_MO2_v1.7.0_2026-07-17.zip`
- `CalamityAffixes_MO2_v1.7.0_2026-07-17.zip.sha256`
- ZIP SHA-256: `2e98f50dc44f45bbad4627ee67a13cbc8bc3364a1e972769f45167d7c8e10843`
- 패키지 DLL SHA-256: `197147e6e6d6fe69b1e2512a58a484ad0f9b33118aae05b53aaaf2510ca9744a`
- 패키지 ESP SHA-256: `a83325d352d519dbea9848b456eb3cfe0a9cbbc49216c9abf23bfabe63ead535`

## 아직 필요한 인게임 공개 테스트

- 던전 단위 플레이에서 `12% / 7%` 기본값의 실제 획득 체감과 과잉 공급 여부
- 1920×1080 기본 크기에서 패널 3열이 겹치지 않는지
- 패널을 약 1050px와 850px로 줄였을 때 2열·1열로 즉시 전환되는지
- 영어·한국어·이중 표기에서 버튼·배지·필터가 잘리거나 겹치지 않는지
- 검색·필터·레시피 선택·변환·재련·상태 확인·초기화가 기존과 동일하게 동작하는지
- v1.6.0의 함정 고정 마커, 저장·로드 ghost, 위치 음향, 시체 폭발과 방어 VFX가 실제 게임에서 정상인지

위 항목이 확인될 때까지 이 릴리스는 **Public Test 프리릴리스**로 유지합니다.

## 설치

1. `CalamityAffixes_MO2_v1.7.0_2026-07-17.zip`을 MO2에서 설치합니다.
2. 기존 Calamity 설치를 교체하도록 활성화합니다.
3. DLL, ESP, JSON, MCM/Papyrus/PrismaUI 파일이 모두 같은 v1.7.0 패키지에서 왔는지 확인합니다.
4. 기존 `user_settings.json`을 유지한 사용자는 필요하면 MCM에서 룬 조각 `12`, 재련 오브 `7`을 직접 적용합니다.

Skyrim SE/AE 및 SKSE/CommonLibSSE-NG 기반 환경이 필요합니다.
