# Combat-lock test1 — 검증 및 인게임 확인 계획

## 범위

이 작업은 기존 `d8603e2` 위의 전투 이벤트 교착 수정이다. 공식 버전 선언은
`1.7.5` 그대로이고 패키지 구분자만 `1.7.5-combat-lock-test1`이다.
에이전트는 설치된 모드팩 파일·설정·세이브를 변경하지 않았다. 2026-09-05 사용자가
“해결된것같아”라고 보고하고 커밋을 승인했다. 푸시·공개 배포는 요청하지 않았다.

[덤프 조사와 락 근거](../reports/2026-08-26-combat-event-lock-investigation.md)를
먼저 참조한다. 두 덤프가 같은 AB–BA 순환을 매우 강하게 지지하지만 mutex owner
메모리는 미포착이며, 모션/FSMP/overlay의 모든 다른 결함을 배제한 것은 아니다.

## 코드의 안전 경계

- 8개 BSTEvent sink가 하나의 `DeferredEventDispatcher`를 경유한다.
- 상태 mutex에 `try_lock`으로 즉시 진입할 수 있고 backlog가 없으면 기존 동기
  처리를 유지한다. 경합 또는 이미 대기 중인 이벤트가 있으면 원래 순서를 보존해
  지연한다. `_stateMutex` 자체와 그 아래 발동/ICD 상태 보호는 유지한다.
- 큐 락은 데이터 이동에만 사용한다. state 대기, 핸들/참조 해제, 엔진 호출,
  SKSE task 예약은 큐 락을 놓은 후 실행한다. 한 batch는 최대 128개다.
- 이벤트의 NiPointer/BSFixedString/handle을 값으로 보관하고, HitData도 수신
  시점에 복사한다. 사용하지 않는 transient VATSCommand와 ModCallback sender는
  저장하지 않는다. Raw 이벤트 포인터나 나중 타격의 lastHitData를 재사용하지 않는다.
- 수신 시점의 **같은 스레드** proc 문맥을 저장한다. Hit 재진입은 기존처럼 억제하고,
  DoT 관측/ICD 및 Death 화폐·시폭은 유지하되 일반 proc만 기존 gate로 억제한다.
  Death의 복원 guard는 terminal unlock 전에 해제한다.
- PreLoad, serialization Load, Revert, PostLoad, config reset에서 epoch를
  무효화한다. batch를 꺼낸 뒤에도 state 획득 직후 다시 검사한다. 이전 task는
  새 세대의 예약 상태를 변경하지 못한다. 단순 active-count rebuild에는 epoch를
  바꾸지 않으므로 정상 장비 갱신이 대기 중 이벤트를 버리지 않는다.
- SKSE task interface가 없으면 blocking fallback으로 돌아가지 않는다.
  이벤트를 보관하고 다음 입력에서 예약을 재시도하며 오류 로그를 남긴다.

## 자동 검사

| 검사 | 결과 |
|---|---|
| Python workflow tests | 184개 통과 |
| Generator .NET tests | 166개 통과 |
| Host native CTest | 2/2 통과 |
| SKSE CTest (static + 실행되는 host runtime gate) | 2/2 통과 |
| 새 dispatcher 동작/동시성 검사 | 13개 통과, 별도로 100회 반복 통과 |
| Dispatcher AddressSanitizer + UndefinedBehaviorSanitizer | 통과 (엔진 독립 host 검사) |
| Dispatcher ThreadSanitizer | WSL에서 `unexpected memory mapping`으로 실행 시작 실패; 미검증 |
| 격리된 mutant: try-lock을 blocking lock으로 교체 | AB–BA 검사가 5초 제한에서 실패함을 확인 |
| affix lint / 문서 / JSON / pins / Prisma / version | 통과; brittle source pins 316 유지 |
| 최종 DLL 및 패키지 | 리빌드·MO2 ZIP verifier·DLL byte identity·archive CRC 모두 통과 |
| Skyrim 실제 프리징 해결 | 2026-09-05 사용자 “해결된것같아” 보고; 에이전트 직접 재현 검증은 미수행 |

13개 새 검사는 실제 제품 dispatcher를 실행한다. 무경합 재진입, 보고된 AB–BA
순서, 큐 락의 역순 방지, in-flight FIFO, 128개 batch/중복 drain, 8 producer의
정확히 한 번 처리, snapshot 소유권과 해제 중 재진입, state 락 대기 중 epoch
무효화, 이전 task/새 세대 분리, scheduler 실패 복구, 예외 후 미실행 순서 보존,
proc 원점의 스레드 독립성, terminal unlock 전 guard 해제를 포함한다.

기존 평타 판정·중첩 확률·ICD·friendly-fire 정책 테스트도 그대로 통과한다.
Skyrim 엔진 자체를 host 테스트로 실행한 것은 아니므로 이를 인게임 검증으로
해석하면 안 된다.

## 패키지 식별 기록

- ZIP: `dist/CalamityAffixes_MO2_v1.7.5-combat-lock-test1_2026-08-26.zip`
  (1,121,220 bytes)
- ZIP SHA-256: `e0a3940d20d38edf0282c95ee92123db21bb93ba2d3c7b955924a055171241be`
- ZIP 옆 `.zip.sha256` 파일로 재검증할 수 있다.
- 패키지 DLL SHA-256: `08df932363f4bc430a1754a72c2427ca5ffaa8474c21df58b4fda371eb61eab3`
- 일치 PDB: `dist/CalamityAffixes_MO2_v1.7.5-combat-lock-test1_2026-08-26.pdb`
  (22,474,752 bytes, ZIP 바깥 별도 보존)
- PDB SHA-256: `facbac49f842c02b0dc404c4faf7602faa3bd36f347782f98fa54fc23afd5389`
- DLL RSDS / PDB GUID: `a21a99fc-633d-7132-4c4c-44205044422e`, Age `1`.

`verify_mo2_zip.py --expected-dll ... --expected-version 1.7.5-combat-lock-test1`
검사를 통과했다. 패키지 DLL은 최종 빌드 DLL과 byte-exact이며 ESP·MCM 설정·기타
데이터는 저장소 기준과 동일하다. JSON 3개는 줄바꿈 등 출력 형식만 다르고 파싱한
값이 같다. PEX 3개는 표준 패키저가 수정하지 않은 원본 PSC를 재컴파일한 결과라
컴파일 시각/문자열 테이블 차이로 바이트가 달라진다(3개 모두 컴파일 오류/경고 0).

저장소의 `Data/SKSE/Plugins/CalamityAffixes.dll`과 현재 G: 설치 DLL은 기존
`3913f20e7ea8738a995df67b759df1bcca69d300a4921d108eec29af86b935e5` 그대로 유지했다.
테스트 DLL은 빌드 디렉터리와 이 테스트 ZIP에만 있다. 기존 덤프용 원본 PDB도
`dist/CalamityAffixes_MO2_v1.7.5_2026-08-24.pdb`에 보존되어 있으며 GUID/Age를
다시 확인했다. 새 덤프에는 새 PDB를 사용해야 한다.

## 승인 후 실제 게임에서 확인할 항목

사용자가 2026-09-05 해결된 것으로 보인다고 보고했다. 구체적인 반복 횟수와
아래 개별 항목의 결과는 제공되지 않았다. 아래는 추가 확인용 계획이며,
에이전트가 테스트 환경을 변경하거나 설치하려면 별도 승인이 필요하다.

1. 기존 설치와 세이브를 보존한 분리 프로필에서 테스트 DLL을 사용한다. 게임을
   완전히 종료한 뒤 교체해야 하며, 배포본과 테스트본 DLL을 혼동하지 않도록
   실제 MO2 승자 파일의 해시를 확인한다. 로그의 `combat-lock test` ingress
   표식도 확인한다. `disableHealthDamageRouting` 등 기존 설정은 변경하지 않는다.
2. 기존에 반복 프리징을 보였던 간파베기 전투를 동일 조건에서 여러 차례 수행한다.
   검증 횟수/시간/장비/적/재현 여부를 기록한다. 한 차례 성공만으로 해결을 확정하지 않는다.
3. 평타 proc, 강공/치명타 proc, 동일 접두사 중첩, 복수 proc 및 ICD가 기존과
   같은지 비교한다. 필요하면 이미 허용된 디버그 설정의 로그로 비교한다.
4. DoT apply/refresh, 소환체 사망 폭발, 처치 화폐, 시체 폭발 연쇄를 확인한다.
   시전자·아군·중립은 피해 없이, 적에게는 기존 효과가 작동해야 한다.
5. 전투/효과 직후 다른 세이브를 불러와 이전 월드의 효과가 재생되지 않는지
   확인한다. 이후 새 전투에서 proc이 정상 재개되는지도 확인한다.
6. 장비 변경, 아이템 획득/이동, UID 추적, MCM 토글 및 원래 프로필로의 복귀를
   확인한다. 테스트 전용 저장만 사용하며 원본 세이브를 덮어쓰지 않는다.
7. 다시 멈추면 새 덤프, 실제 로드된 DLL 해시, 해당 패키지 PDB, Calamity 로그를
   함께 보존한다. 동일 이벤트-lock 순환인지 다른 락/모션 문제인지 다시 unwind한다.

이 패치는 EventBridge가 관리하는 BSTEvent 락 역순을 끊는 수정이며, 임의의 다른
엔진/서드파티 락에 대한 프리징까지 모두 해결했다는 보장은 아니다.
