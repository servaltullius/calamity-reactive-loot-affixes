# 전투 이벤트 락 교착 조사 — 2026-08-26

## 범위와 상태

- 분석 기준: `d8603e2` (`fix: prevent affix friendly fire`)에 해당하는 설치 DLL.
- 증상: 사용자가 SkyrimHunter 태도 간파베기 중 반복 프리징을 보고했다.
- 이 문서의 덤프·바이너리 조사 결과는 읽기 전용으로 독립 재확인했다.
- 아래 락 경계 수정과 빌드·회귀 테스트·별도 테스트 ZIP 검증을 완료했다.
  2026-09-05 사용자가 “해결된것같아”라고 보고했다. 에이전트의 직접 인게임
  재현 검증은 수행하지 않았다. 상세 결과와 artifact 해시는
  [test1 검증 기록](../testing/2026-08-26-combat-lock-test1.md)에 있다.
- 에이전트는 모드팩 설치 파일·설정·세이브를 변경하지 않았다. 2026-09-05
  사용자가 수정 커밋을 승인했으며, 푸시·배포·실제 설치는 요청하지 않았다.

## 결론과 증거 수준

확인된 두 덤프의 실제 unwind 경로, 복원된 nonvolatile 레지스터, scoped-lock
저장 포인터, 일치하는 설치 DLL/PDB 및 캡처된 엔진 코드는 다음 순환을 매우
강하게 뒷받침한다.

```text
체력 피해 처리 스레드
  _stateMutex 획득 → OnHealthDamage → ProcessTrigger → 주문 즉시 시전
                                                ↓ 엔진 이벤트 락 대기
마법효과 이벤트 스레드
  엔진 이벤트 락 획득 → TESMagicEffectApplyEvent 콜백
                                                ↓ _stateMutex 대기
```

여기서 “이미 획득한 락”은 호출 경로·기계어의 획득/해제 구간 및 guard 저장값을
근거로 한 강한 추론이다. 두 mutex 객체 본체의 owner 필드는 덤프에 포함되지
않아 직접 읽지 못했다. 따라서 owner 필드까지 읽어 완전히 증명했다고 표현하지
않는다.

모션 파일의 결함은 이번 조사에서 확인하지 않았다. 자동 리포트의 FSMP/Steam
overlay 지목은 사용자 제공 설명상 low-confidence 포인터 스캔 결과이며, 이
조사에서는 이를 독립적인 원인 확정 근거로 사용하지 않았다. 특정 모드나 모션의
다른 결함이 없다는 배제 진단도 아니다.

## 분석 입력과 바이너리 식별

덤프:

- `G:\TAKEALOOK\overwrite\SKSE\Plugins\Tullius Ctd Logs\SkyrimDiag_Hang_20260826_115308_644.dmp`
- `G:\TAKEALOOK\overwrite\SKSE\Plugins\Tullius Ctd Logs\SkyrimDiag_Hang_20260825_191952_918.dmp`

설치 DLL:

`G:\TAKEALOOK\mods\CalamityAffixes_MO2_v1.7.5_2026-08-24\SKSE\Plugins\CalamityAffixes.dll`

분석에 사용한 빌드 DLL/PDB:

- `/home/kdw73/Calamity - Reactive Loot & Affixes/skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll`
- `/home/kdw73/Calamity - Reactive Loot & Affixes/skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.pdb`

| 식별 항목 | 독립 확인 결과 |
|---|---|
| 설치 DLL SHA-256 | `3913f20e7ea8738a995df67b759df1bcca69d300a4921d108eec29af86b935e5` |
| 분석 당시 빌드 DLL SHA-256 | 설치 DLL과 동일 |
| PE CodeView RSDS GUID / PDB GUID | `8e4b2114-fb4c-0a7b-4c4c-44205044422e` |
| PE CodeView / PDB Age | `1` / `1` |
| PE 및 두 덤프 module timestamp | `1787535686` (`0x6a8ba146`) |
| PE 및 두 덤프 image size | `0x281000` |
| 두 덤프의 포착 코드와 설치 PE 비교 | Calamity RVA `0x1010d3` 40바이트, `0xf8cb9` 97바이트, `0x150f20` 64바이트 모두 byte-exact |

위 빌드 디렉터리는 후속 리빌드로 바뀔 수 있다. 재현 시 반드시 해시 및 GUID/Age를
다시 확인하고, 다른 PDB로 아래 소스 라인을 해석하지 않는다. 위 비교는 지정한
포착 코드 구간의 일치 확인이지 덤프 내 전체 module image의 해시 계산은 아니다.

## 8월 26일: TID 37460 / 28708

Module base:

- SkyrimSE: `0x7ff7b27e0000`
- CalamityAffixes: `0x7ffd427b0000`
- 엔진 이벤트 락: `0x7ff7b48d7e10` = `SkyrimSE+0x20f7e10`
- Calamity `_stateMutex`: `0x7ffd42a146e8`

| 스레드·프레임 | 복원 레지스터 / 메모리 관측 |
|---|---|
| T37460, `msvcp140+0x17da2` | `RBX=0x7ffd42a146e8`, mutex 획득 대기 경로 |
| T37460, `Calamity+0x1010e3` | `RBP=0x964bafe8a0`, `RSI=0x7ffd42a10418`; `[RBP+0x70]=[0x964bafe910]=0x7ffd42a146e8` |
| T37460, `SkyrimSE+0x5c6416` | `RSP=0x964bafe9a0`, `RDI=0x7ff7b48d7dc8`; guard `[RSP+0x70]=[0x964bafea10]=0x7ff7b48d7e10` |
| T28708, `SkyrimSE+0x194a2a` | `RDI=0x7ff7b48d7e10`, `RBP=0x7024` (= TID 28708); guard `[RSI]=[0x964bbfe8f0]=0x7ff7b48d7e10` |
| T28708, `SkyrimSE+0x5c6317` | 동일 엔진 guard `[RSP+0x70]=[0x964bbfe8f0]=0x7ff7b48d7e10` |
| T28708, `Calamity+0xf8cd4` | `RBP=0x964bbff9f0`, `RDI=0x7ffd42a103f8`; `[RBP+0x88]=[0x964bbffa78]=0x7ffd42a146e8` |

DLL 기계어에서 MagicEffectApply 콜백은 `RSI+0x42d0`를 mutex 인자로 넘기며,
OnHealthDamage는 `RDI+0x42f0`의 동일 mutex를 사용한다. 다중 event-sink 상속에
따른 `this` 위치 차이가 있어도 계산 결과는 같은 주소이다.

## 8월 25일: TID 15160 / 28016

Module base:

- SkyrimSE: `0x7ff742a40000`
- CalamityAffixes: `0x7ffd7b770000`
- 엔진 이벤트 락: `0x7ff744b37e10` = `SkyrimSE+0x20f7e10`
- Calamity `_stateMutex`: `0x7ffd7b9d46e8`

| 스레드·프레임 | 복원 레지스터 / 메모리 관측 |
|---|---|
| T15160, `msvcp140+0x17da2` | `RBX=0x7ffd7b9d46e8` |
| T15160, `Calamity+0x1010e3` | `RBP=0x12645fec10`, `RSI=0x7ffd7b9d0418`; `[RBP+0x70]=[0x12645fec80]=0x7ffd7b9d46e8` |
| T15160, `SkyrimSE+0x5c6416` | `RSP=0x12645fed10`, `RDI=0x7ff744b37dc8`; guard `[RSP+0x70]=[0x12645fed80]=0x7ff744b37e10` |
| T28016, `SkyrimSE+0x194a2a` | `RDI=0x7ff744b37e10`, `RBP=0x6d70` (= TID 28016); guard `[RSI]=[0x12648fe710]=0x7ff744b37e10` |
| T28016, `Calamity+0xf8cd4` | `RBP=0x12648ff810`, `RDI=0x7ffd7b9d03f8`; `[RBP+0x88]=[0x12648ff898]=0x7ffd7b9d46e8` |

ASLR로 실제 주소는 다르지만, 대기 함수·Calamity 호출 경로의 RVA와 락 상대
위치는 두 날짜 모두 동일하다.

## 일치 PDB의 공통 호출 경로

아래 RVA는 unwind로 복원한 **반환 주소**다. PDB 조회에는 각 값에서 1을 뺀
주소를 사용해 직전 call 명령에 대응시켰다. 소스 라인은 분석 기준 DLL의 PDB에
기록된 위치이므로 후속 수정 파일의 현재 줄 번호와 다를 수 있다.

| 반환 RVA | 함수 / 소스 위치 |
|---|---|
| `0x1010e3` | `ProcessEvent(TESMagicEffectApplyEvent)` → `std::scoped_lock<std::recursive_mutex>` → lock; `EventBridge.Triggers.MagicEffectApply.cpp:18` |
| `0x150f4b` | `CastHostileOnlySpellImmediate`; `HostileEffectGuard.cpp:621` |
| `0x10e15f` | `ExecuteCastSpellAction`, `ObserveImmediateHealthChange`와 시전 lambda; `EventBridge.Actions.Cast.cpp:321/335` |
| `0x10ca69` | `DispatchActionByType`; `EventBridge.Actions.Dispatch.cpp:92` |
| `0x10cc64` | `ExecuteActionWithProcDepthGuard` / `ExecuteAction`; `EventBridge.Actions.Dispatch.cpp:110/119` |
| `0xf4c48` | `TryProcessTriggerAffix`; `EventBridge.Triggers.cpp:152` |
| `0xf5365` | `ProcessTrigger`; `EventBridge.Triggers.cpp:204` |
| `0xfa9d2` | `ProcessOutgoingHealthDamageHit`; `EventBridge.Triggers.HealthDamage.Routing.cpp:154` |
| `0xf8cd4` | `OnHealthDamage`; `EventBridge.Triggers.HealthDamage.cpp:106` |
| `0x158212` | `ExecutePostHealthDamageActions`; `Hooks.Dispatch.cpp:274` |
| `0x1592df` | `SchedulePostHealthDamageActions`의 deferred task lambda; `Hooks.Dispatch.cpp:466/475` |

체력 피해 처리 쪽 unwind는 두 덤프 모두 `skse64_1_6_1170.dll+0x189df`까지
도달했다. 따라서 이 교착은 이미 SKSE deferred task 안에서 발생했다. 단순히
주문 시전을 `AddTask`로 한 번 더 감싸는 것만으로 해결을 입증할 수 없다.

## 엔진 코드 대조와 조사 한계

- `SkyrimSE+0x5c62e0` 이벤트 dispatcher는 event source의 `+0x48` 락을 획득한
  뒤 sink의 가상 콜백을 호출한다. 캡처된 호출 위치는 `+0x5c6413`, 반환 주소는
  `+0x5c6416`이다. 락 해제는 콜백이 반환한 이후 함수 말미에 있다.
- `SkyrimSE+0x1949b0` 락 획득 함수는 owner/count를 검사하고 대기한다. 대기 중
  반환 주소 `+0x194a2a`에서 복원한 `RDI`가 위 dispatcher guard와 동일하다.
- 디스크 SkyrimSE.exe의 해당 `.text`는 패킹된 바이트여서 그 디스어셈블을
  근거로 사용하지 않았다. PE의 unwind 정보와 **덤프에 포착된 실행 코드**를
  사용하고, 코드 바이트를 `llvm-mc-19`로 읽기 전용 디스어셈블했다.
- 두 날짜 모두 엔진 락 및 Calamity mutex 객체 본체를 읽으면 `uncaptured
  memory`가 나온다. owner 필드 직접 확인은 불가능하다.
- 제공 unwind 스크립트는 일반 PE unwind opcode를 처리하되 machine-frame 등
  미지원 항목에는 중단한다. 이번 핵심 프레임에서는 해당 중단이 발생하지 않았다.
- 체력 피해 스레드는 기록된 `G:\TAKEALOOK\Stock Game\skse64_1_6_1170.dll`
  디스크 파일이 없어 그 지점에서 unwind가 중단됐다. 그보다 위의 순환 경로와
  deferred-task 프레임은 이미 복원됐다.

## 읽기 전용 재현 방법

분석에 사용한 임시 스크립트:

`C:\Users\kdw73\AppData\Local\Temp\skyrim_hang_unwind_readonly.py`

실행 전에 전체 소스를 읽어 덤프/PE를 파싱하고 콘솔에 출력할 뿐, 파일 쓰기·설치
변경이 없음을 확인했다. 이 임시 파일은 저장소 관리 대상이 아니므로 추후 존재나
내용이 같다고 가정하지 않는다. Python에는 `pefile`, `minidump`가 필요하다.

PowerShell에서 기본 unwind와 설치 DLL 해시 확인:

```powershell
Get-Content -LiteralPath 'C:\Users\kdw73\AppData\Local\Temp\skyrim_hang_unwind_readonly.py'
Get-FileHash -Algorithm SHA256 -LiteralPath 'G:\TAKEALOOK\mods\CalamityAffixes_MO2_v1.7.5_2026-08-24\SKSE\Plugins\CalamityAffixes.dll'
python 'C:\Users\kdw73\AppData\Local\Temp\skyrim_hang_unwind_readonly.py' 'G:\TAKEALOOK\overwrite\SKSE\Plugins\Tullius Ctd Logs\SkyrimDiag_Hang_20260826_115308_644.dmp' 37460 28708
python 'C:\Users\kdw73\AppData\Local\Temp\skyrim_hang_unwind_readonly.py' 'G:\TAKEALOOK\overwrite\SKSE\Plugins\Tullius Ctd Logs\SkyrimDiag_Hang_20260825_191952_918.dmp' 15160 28016
```

일치하는 분석 DLL/PDB를 보존한 상태에서 WSL로 심볼 확인:

```bash
llvm-pdbutil-19 dump -summary '/home/kdw73/Calamity - Reactive Loot & Affixes/skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.pdb'
llvm-symbolizer-19 --obj='/home/kdw73/Calamity - Reactive Loot & Affixes/skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll' --relative-address --functions --inlines 0x1010e2 0x150f4a 0x10e15e 0x10ca68 0x10cc63 0xf4c47 0xf5364 0xfa9d1 0xf8cd3 0x158211 0x1592de
```

위 기본 unwind 출력 외의 guard 값은 동일 스크립트를 `runpy.run_path`로 읽어
`read`/`qword`/`apply_unwind` 함수를 재사용해 확인했다. 해당 날짜의 표에 적힌
stack 주소에서 little-endian 64-bit 값을 읽고, 같은 프레임의 복원 `RBP`/`RSP`와
offset을 대조하면 된다. owner 확인에는 추측값이나 0을 채우지 않고 미포착으로
기록한다. 엔진 코드도 dump memory segment에서 실제 바이트를 읽어 확인한다.

## 최소 수정 방향과 유지할 계약

현재 수정 방향은 state mutex를 제거하거나 모든 이벤트를 버리는 것이 아니라,
**엔진 이벤트 콜백 진입점이 다른 스레드의 `_stateMutex` 해제를 기다리지 않게
하는 것**이다.

대상은 EventBridge의 8개 `BSTEventSink` 진입점이다:

- `TESHitEvent`, `TESDeathEvent`, `TESEquipEvent`, `TESActivateEvent`
- `TESMagicEffectApplyEvent`, `TESContainerChangedEvent`, `TESUniqueIDChangeEvent`
- `SKSE::ModCallbackEvent`

설계 계약:

1. 진입점은 `_stateMutex`에 nonblocking `try_lock`을 사용한다. 무경합 경로는
   기존 동기 처리를 유지한다. 경합할 때만 소유권 있는 이벤트 snapshot을 FIFO
   inbox에 넣고 엔진 콜백을 반환한다.
2. 지연 작업은 엔진 이벤트 콜백 밖에서 다시 상태 보호를 얻어 원래 이벤트
   처리를 실행한다. 효과는 “한 번 더 AddTask”가 아니라, 엔진 락을 보유한
   콜백에서 state mutex로 향하는 **blocking wait edge 제거**이다.
3. inbox의 자체 동기화 구간에서는 state mutex 획득이나 엔진 호출을 하지
   않는다. queue를 비운 뒤 콜백을 실행해 별도 AB–BA 순환을 만들지 않는다.
4. 지연 작업이 원래 이벤트의 스택 포인터를 잡아두지 않게 한다. 참조 객체와
   문자열 등의 소유권을 유지하거나 안전한 handle/값 snapshot을 사용하며,
   drain 시 유효성을 다시 확인한다. FIFO의 범위와 경합 이벤트 순서도 테스트한다.
5. 세이브 로드·Revert 후 이전 세션의 queued event가 실행되지 않도록 generation
   무효화를 적용하고 테스트한다. 오래된 예약 task가 새 세션 queue 상태를
   잘못 변경하지 않는지도 확인한다.
6. `_stateMutex`와 HostileEffectGuard는 유지한다. 일반 타격 발동, 접두사 중첩,
   ICD, proc 재진입 방지, 시전자·아군·중립 보호는 기존 계약을 유지해야 한다.
   특히 지연 시점에서 재진입 문맥이 사라져 proc이 새 공격으로 오인되지 않도록
   원래 문맥 보존과 회귀 검증이 필요하다.

이 방향은 관측된 engine-event-lock → state-mutex 순환을 좁게 끊는 것이다.
모든 엔진 API가 임의의 다른 락에 대해 안전하다는 포괄적인 증명은 아니다.

## 검증 기록 및 실제 게임 확인 게이트

| 구분 | 최종 상태 | 확인 범위 |
|---|---|---|
| 두 덤프·DLL/PDB·락 주소 재확인 | 완료 | 위 관측 및 한계 |
| 동시성 회귀 테스트 | 통과 | 제품 dispatcher 13개 검사, 100회 반복; blocking-lock mutant는 AB–BA 검사에서 실패 |
| 수명·상태 전환 회귀 | 통과 (host) | 소유 snapshot, state 획득 후 epoch 재검사, stale task/새 세대 분리, 재진입 문맥 |
| 기존 게임 로직 회귀 | 통과 (정책/자동 검사) | 평타·중첩·ICD·아군/중립 보호의 기존 테스트; 게임 엔진 동작은 미확인 |
| DLL 빌드·전체 필수 검사 | 통과 | Python 184개, Generator 166개, native CTest 및 SKSE CTest 각각 2/2, 전체 린트 |
| 별도 테스트 패키지 | 완료 | `1.7.5-combat-lock-test1`; CRC·필수 파일·DLL byte identity·PDB GUID 확인 |
| Skyrim 인게임 해결 확인 | 사용자 개선 보고 | 2026-09-05 “해결된것같아”; 반복 횟수·개별 회귀 항목 결과 미제공, 에이전트 직접 확인 미수행 |

빌드·회귀 테스트가 통과해도 실제 Skyrim에서 프리징이 해결됐다는 뜻은 아니다.
인게임 확인 시 기존 설치와 세이브를 보존한 승인된 테스트 환경에서 같은 간파베기
전투를 반복하고, 일반 타격·중첩·ICD·아군 보호·세이브 로드도 확인해야 한다.
프리징이 다시 발생하면 테스트 DLL의 정확한 해시와 일치 PDB를 보존하고 새 덤프의
실제 unwind로 동일 순환인지 다른 대기인지 분리한다.
