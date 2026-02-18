# In-Game QA Smoke Checklist (2026-02-09)

## 목적

- 릴리즈 전 최소 인게임 회귀 확인 항목을 고정합니다.
- static-check/단위 테스트가 잡기 어려운 이벤트 타이밍/입력 경합을 수동으로 검증합니다.

## 테스트 환경

1. MO2 별도 테스트 프로필(권장: 새 세이브)
2. 필수 의존성 로드 확인
   - SKSE64
   - Address Library
   - Prisma UI
3. 디버그 확인용 옵션
   - MCM `Debug Notifications` ON (권장)
   - 필요 시 SKSE 로그 tail

## (옵션) 로그 시그니처 자동 체크

인게임 테스트 후, SKSE 로그에 “플러그인 로드/설정 로드/훅 설치/Prisma 초기화” 시그니처가 찍혔는지 자동으로 확인합니다.

```bash
python3 tools/qa_skse_logcheck.py --log "<CalamityAffixes.log 경로>"
```

WSL에서 흔한 경로 예시:

```bash
python3 tools/qa_skse_logcheck.py --log "/mnt/c/Users/<You>/Documents/My Games/Skyrim Special Edition/SKSE/CalamityAffixes.log"
```

## 공통 성공 기준

- CTD/무한 스크립트 지연 없음
- HUD/인벤토리 입력 잠김 없음
- 동일 액션 반복 시 상태가 누적 오염되지 않음

## 시나리오 A: 픽업 보조 리소스 롤

1. 무기/방어구를 루팅으로 10개 이상 획득
2. 룬 조각/재련 오브 획득 로그/알림 확인
3. 같은 드랍 아이템을 버렸다가 다시 주워도 자동 어픽스가 붙지 않는지 확인

Pass:
- 적격 소스(시체/상자/월드)에서만 조각/오브 롤 수행
- 자동 어픽스 부여가 발생하지 않음

## 시나리오 B: 툴팁/선택 일치성

1. 동일 이름/유사 이름 아이템 2개 이상 준비
2. 인벤토리에서 행 선택을 반복 전환
3. 툴팁이 선택 행과 불일치하지 않는지 확인

Pass:
- 모호할 때는 잘못된 툴팁이 뜨지 않음(무응답 허용)
- 선택된 행과 일치할 때만 설명 노출

## 시나리오 C: 룬워드 패널 플로우

1. 베이스 순환/레시피 순환/선택 상태 확인
2. 부족 룬 상태 메시지 확인
3. 룬 조각 지급 후 완성 시 결과 어픽스 적용 확인

Pass:
- 베이스 커서/선택 상태 일관성 유지
- 완료 후 `Runeword Complete`/효과 설명이 정상 노출

## 시나리오 D: Trigger suppression + ICD

1. 빠른 연타(혹은 다단 히트) 상황에서 프로크 로그/알림 확인
2. 동일 타겟에 per-target ICD가 기대대로 차단하는지 확인
3. ICD 경과 후 재발동되는지 확인

Pass:
- 중복 hit 억제창에서 과발동 없음
- per-target ICD 중복 발동 없음
- ICD 만료 후 재발동 가능

## 시나리오 E: DotApply 저발동 정책

1. `CAFF_TAG_DOT` 활성 규칙에서 DoT 적용/갱신 테스트
2. tick마다 난사되지 않고 적용/갱신 시점만 반응하는지 확인
3. 과도한 효과 소거/정화 부작용 여부 확인

Pass:
- DotApply 이벤트가 강한 ICD(기본 1.5s)로 제한됨
- 원치 않는 전역 디스펠/버프 소거 현상 없음

## 시나리오 F: 소환체/플레이어 범위

1. 플레이어 및 player-owned summon으로 각각 전투 수행
2. 일반 팔로워/NPC 장비 발동 여부 관찰

Pass:
- 플레이어/소환체 경로 정상
- 팔로워/NPC 미지원 범위가 문서 설명과 일치

## 릴리즈 체크 기록 템플릿

```text
빌드:
런타임:
의존성:
세이브:

A Loot-time: PASS/FAIL (메모)
B Tooltip: PASS/FAIL (메모)
C Runeword: PASS/FAIL (메모)
D Suppression/ICD: PASS/FAIL (메모)
E DotApply: PASS/FAIL (메모)
F Scope: PASS/FAIL (메모)

최종 판정:
```
