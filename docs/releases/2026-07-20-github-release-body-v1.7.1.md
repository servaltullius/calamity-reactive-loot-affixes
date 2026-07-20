# Calamity - Reactive Loot & Affixes v1.7.1

이번 프리릴리스는 적격 네임드 고유 적과 보스에게 룬워드 조각·재련 오브 보상을 보장해, 강한 적을 처치했을 때 보상이 더 분명하게 느껴지도록 한 공개 테스트 빌드입니다.

자동 검증과 커밋된 소스의 별도 fresh checkout 재현은 게시 전에 완료합니다. 다만 실제 Skyrim에서 네임드·보스 분류와 획득 결과를 확인하는 인게임 검증은 아직 완료되지 않았으므로 중요한 세이브는 별도로 백업해 주세요.

## 네임드 고유·보스 확정 보상

- Actor Base가 `Unique`인 적:
  - 룬워드 조각 또는 재련 오브 중 정확히 1개를 확정 획득합니다.
  - 기본 선택 비율은 룬워드 조각 `70%`, 재련 오브 `30%`입니다.
- `LocRefTypeBoss`가 지정된 보스:
  - 룬워드 조각 1개와 재련 오브 1개를 각각 확정 획득합니다.
- 보스 분류는 네임드 고유 분류보다 우선합니다.
- 이미 해당 카테고리가 처리되었거나 시체에 같은 통화가 존재하면 기존 카테고리별 중복 방지 계약에 따라 그 카테고리는 다시 지급하지 않습니다.

## 일반 드랍과 피티

- 일반 적의 기본값은 룬워드 조각 `12%`, 재련 오브 `7%`로 유지합니다.
- 네임드 고유·보스 확정 보상 대상은 같은 시체에서 일반 `12% / 7%` 판정을 추가로 수행하지 않습니다.
- 확정 보상은 일반 룬 조각 무드롭 피티를 증가시키거나 초기화하지 않습니다.
- 기존 룬 조각 99회 무드롭 피티와 룬 가중치 `El-Amn=4 / Sol-Um=3 / Mal-Lo=2 / Sur-Zod=1`은 변경하지 않았습니다.
- 모든 통화 판정은 계속 SKSE death-event 기반 eligible hostile corpse-only 경로에서만 수행합니다. 일반 상자·컨테이너와 새 SPID 통화 분배는 사용하지 않습니다.

네임드 고유 보상의 기본 조각 선택 비율은 데이터 설정 `loot.uniqueActorGuaranteedRunewordChancePercent=70`입니다. 이번 버전에는 이 값을 위한 새 MCM 옵션을 추가하지 않았습니다. 기존 MCM의 룬 조각·재련 오브 확률은 일반 적의 `12% / 7%` 판정에만 적용됩니다.

## 호환성

- 기존 시체별 카테고리 ledger와 legacy SPID 중복 감지를 그대로 사용합니다.
- 코세이브 직렬화 토큰·버전, MCM 옵션, 룬워드 제작·재련 계약과 243개 공개 효과를 변경하지 않았습니다.
- `Data/CalamityAffixes.esp`의 레코드와 바이트는 v1.7.0과 동일합니다.
- 기존 세이브에서 바로 업데이트할 수 있으며 새 게임은 필요하지 않습니다.

## 자동 검증

- affix compose/check 및 데이터 lint: 통과, 경고 0개
- Python 도구 테스트: 75/75 통과
- Generator Release 테스트: 129/129 통과
- SKSE clang-cl Release 빌드: 성공
- SKSE CTest/runtime gate: 3/3 통과
- Papyrus 3종 컴파일: 성공, 오류·경고 0개
- MO2 패키지 구조, 정확한 PEX 집합, CRC, v1.7.1 버전과 DLL 해시: 통과
- 커밋된 소스의 fresh checkout 전체 릴리스 재현: 통과 (고정 SDK·의존성 캐시 재사용)

## 배포 파일

- `CalamityAffixes_MO2_v1.7.1_2026-07-20.zip`
- `CalamityAffixes_MO2_v1.7.1_2026-07-20.zip.sha256`
- ZIP SHA-256: `f85335d6e2b510005af7940a97c18ecbc247017fc9dae0b00f9cb7bf146ba3b0`
- 패키지 DLL SHA-256: `76700f47ec4531adfaf1e56b22e436245b71c74e6830fd5ecd00d2ad3d998f51`
- 패키지 ESP SHA-256: `a83325d352d519dbea9848b456eb3cfe0a9cbbc49216c9abf23bfabe63ead535`

## 아직 필요한 인게임 공개 테스트

- Actor Base `Unique`인 적이 조각 또는 오브 중 정확히 하나만 지급하는지
- `LocRefTypeBoss`가 지정된 보스가 조각과 오브를 각각 하나씩 지급하는지
- Unique이면서 Boss인 적이 보스 규칙을 우선 적용하는지
- 일반 적의 `12% / 7%` 드랍과 99회 조각 피티가 기존과 동일하게 작동하는지
- 이미 통화가 있거나 같은 시체를 다시 처리할 때 중복 지급되지 않는지
- v1.6.0 VFX와 v1.7.0 Prisma 패널의 미완료 인게임 시각 검증

위 항목이 확인될 때까지 이 릴리스는 **Public Test 프리릴리스**로 유지합니다.

## 설치

1. `CalamityAffixes_MO2_v1.7.1_2026-07-20.zip`을 MO2에서 설치합니다.
2. 기존 Calamity 설치를 교체하도록 활성화합니다.
3. DLL, ESP, JSON, MCM/Papyrus/PrismaUI 파일이 모두 같은 v1.7.1 패키지에서 왔는지 확인합니다.

Skyrim SE/AE 및 SKSE/CommonLibSSE-NG 기반 환경이 필요합니다.
