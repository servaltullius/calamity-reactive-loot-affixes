# Calamity - Reactive Loot & Affixes v1.5.0

이번 프리릴리스는 역할이 약하거나 서로 겹치던 일부 효과를 재설계하고, 발동을 즉시 알아볼 수 있는 경량 시각·청각 피드백을 추가한 공개 테스트 빌드입니다.

자동 빌드와 계약 테스트는 게시 전에 완료하지만, 실제 게임을 오래 플레이해야 하는 인게임 검증은 아직 완료되지 않았습니다. 기존 세이브에서 바로 시험할 수 있으나 중요한 세이브는 별도로 백업해 주세요.

## 효과 개편

### Voice of Power

- 피격 시 25% 확률로 발동합니다.
- 내부 재사용 대기시간은 8초입니다.
- Stamina를 즉시 30 회복합니다.
- 5초 동안 `AttackDamageMult`를 15% 올립니다.
- 발동 시 짧은 용언 계열 시각·청각 피드백을 표시합니다.

### Spirit

- 상시 최대 Magicka +30을 제공합니다.
- 적중 시 28% 확률로 발동합니다.
- 내부 재사용 대기시간은 10초입니다.
- 5초 동안 주문 흡수 확률(`AbsorbChance`)을 10포인트 올립니다.
- 발동 시 짧은 주문 흡수 계열 피드백을 표시합니다.

### Smoke

- 피격 시 22% 확률로 발동합니다.
- 내부 재사용 대기시간은 12초입니다.
- 플레이어가 아니라 실제 공격자에게 5초 동안 `SpeedMult` -30%를 적용합니다.
- 감속된 공격자에게 짧은 연기 계열 피드백을 표시합니다.

### Fury

- 적중 시 24% 확률로 발동합니다.
- 내부 재사용 대기시간은 12초입니다.
- 6초 동안 `WeaponSpeedMult`를 25% 올리고 Stamina를 즉시 30 회복합니다.
- 기존 Pattern/Faith와 역할이 겹치던 치명타 확률 보너스는 사용하지 않습니다.
- 발동 시 짧은 격노 계열 피드백을 표시합니다.

### Wealth

- 전투 중 확률 발동을 제거했습니다.
- 상시 Carry Weight +75와 Speechcraft +15를 제공합니다.
- 패시브가 실제로 새로 추가될 때만 짧은 금빛 피드백을 표시하며, 로드·재조정 때 반복 재생하지 않습니다.

### Wisdom

- 이번 버전에서는 정의, 레코드, 런타임 동작을 변경하지 않았습니다.
- 직접 원거리 무기 적중을 정확히 판별하는 별도 gate가 준비된 뒤 후속 개편합니다.

## VFX 및 피드백

- Voice of Power, Spirit, Smoke, Fury, Wealth에 각 효과의 정체성을 구분할 수 있는 짧은 피드백을 추가했습니다.
- 피드백은 기존 Skyrim 아트 오브젝트를 참조하며, 참조를 찾지 못해도 효과 본체는 정상 작동하도록 fail-soft 처리했습니다.
- 지속적인 화면 점유나 저장 상태 추가 없이 순간 피드백만 생성합니다.

## 세이브 및 레코드 호환성

- v1.4.0까지의 첫 733개 레코드 할당을 frozen fixture로 고정했습니다.
- append-only tail에 효과 레코드 9개와 피드백 ARTO 레코드 5개를 추가했습니다.
- 총 major record 수는 747개이며 Next Object ID는 `0xAEB`입니다.
- Spirit의 이전 active pair와 Wealth의 이전 proc pair는 삭제하거나 다른 의미로 재사용하지 않고 tombstone으로 보존했습니다.
- Spirit 패시브는 설정 기반 PostLoad refresh를 사용해 기존 장착 세이브에서도 새 최대 Magicka 의미를 재적용합니다.
- SKSE 직렬화 토큰 형식은 변경하지 않았습니다.

## 변경하지 않은 항목

- 룬 가중치와 드롭 분포
- 기본 파편 드롭률 8%
- 99회 무드롭 피티
- 기존 장비와 완성 룬워드의 소유권 구조
- 런타임 currency/drop 정책
- UI 프레임워크

## 자동 검증

게시 후보는 다음 검증을 통과했습니다.

- affix compose/check 및 데이터 lint: 통과, 경고 0개
- Python 도구 테스트: 75/75 통과
- Generator Release 테스트: 120/120 통과
- SKSE Linux clang-cl Release 빌드: 성공
- SKSE CTest/runtime gate: 3/3 통과
- ESP append-only allocation, export/reimport, ESL Small flag, FormID/EditorID 계약
- MO2 패키지 구조, CRC, 버전, DLL 해시 검증
- 커밋된 소스의 fresh checkout 전체 릴리스 재현 검증

## 배포 파일

- `CalamityAffixes_MO2_v1.5.0_2026-07-16.zip`
- `CalamityAffixes_MO2_v1.5.0_2026-07-16.zip.sha256`
- ZIP SHA-256: `3818239a2ba9c56ea1a23c0b3dc41a05b1fcdfb35ed78944dbf5df71db457924`
- DLL SHA-256: `7c926076c2ec639e0a5cc0ef6730369785cccc4bac9a0c4d3c8f54380fe16874`
- ESP SHA-256: `52ef3378ec62fe79593cb65ea99229e90ba33e16d0c515ded0a4c4111f3725b1`

## 아직 필요한 인게임 공개 테스트

사용자 환경에서 장기 인게임 검증은 아직 완료되지 않았습니다. 특히 다음 항목의 피드백을 부탁드립니다.

- Voice of Power: Stamina 30, 공격력 +15%/5초, 8초 ICD
- Spirit: 기존 v1.4.0 장착 세이브의 최대 Magicka +30 무재장착 갱신, 주문 흡수 +10/5초, 효과 종료 후 원복과 중복 여부
- Smoke: 새 발동이 플레이어가 아닌 실제 공격자를 감속하는지
- Fury: 한손·양손·쌍수·활과 animation/behavior mod 환경에서 공속 +25%가 자연스럽게 동작하는지
- Wealth: 기존 +100 임시 효과가 15초 이내 만료되고 새 +75 패시브가 중복되지 않는지
- 각 효과의 VFX가 과도하거나 누락되지 않는지

인게임 검증이 끝나기 전까지 이 릴리스는 **Public Test 프리릴리스**로 유지합니다.

## 설치

1. `CalamityAffixes_MO2_v1.5.0_2026-07-16.zip`을 MO2에서 설치합니다.
2. 기존 Calamity 설치를 교체하도록 활성화합니다.
3. DLL, ESP, JSON, MCM/PrismaUI 파일이 서로 같은 v1.5.0 패키지에서 왔는지 확인합니다.

Skyrim SE/AE 및 SKSE/CommonLibSSE-NG 기반 환경이 필요합니다.
