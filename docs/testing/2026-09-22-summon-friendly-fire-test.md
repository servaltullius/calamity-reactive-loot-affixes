# RC6 Calamity 자동 소환수 아군 피해 수정 테스트

## 보고와 코드에서 확인한 범위

- 사용자는 장비 효과로 자동 소환된 패밀리어·정령이 플레이어 공격에 적대화하거나 플레이어에게 피해를 준다고 보고했다.
- RC6에는 등록된 소환수의 비적대 대상 체력 피해 억제가 있었지만, 플레이어의 일반 공격으로부터 소환수를 보호하는 경로와 아군 피격 무시 설정이 없었다.
- 등록 목록은 PreLoad/Revert에서 지워지고 저장·복원되지 않았다. 2026-09-22 로그에는 체력 피해 훅 설치 성공과 세이브 재로드가 확인된다. 실제 문제가 난 개체의 등록 여부나 피해 호출 스택은 기록되어 있지 않아, 모든 증상의 단일 원인으로 단정하지 않는다.

## 변경

- Calamity 시전의 새 SummonCreatureEffect에서 확인한 개체 레퍼런스에만 IgnoreFriendlyHits 플래그를 설정한다. 공용 NPC 베이스나 전역 아군 피해 설정은 수정하지 않는다.
- 플레이어·팀메이트·플레이어 소유 개체·다른 Calamity 소환수의 공격이 등록된 소환수에게 주는 체력 피해를 억제한다. 적이 소환수에게 주는 피해는 유지한다.
- 등록된 소환수는 일시적으로 지휘자/적대 관계가 변해도 Calamity 공격 효과의 대상으로 선택하지 않는다.
- 기존 소환수 발신 피해 및 폭발 소유권 확인 경로는 유지한다.
- 별도 CSUM v1 코세이브 레코드에 최대 64개 실제 레퍼런스 FormID를 저장한다. Load에서는 ID만 해석하고 PostLoadGame에서 살아 있는 소환 개체의 보호를 복원한다. NPC 베이스나 공유 바닐라 소환 주문만으로 소유권을 추측하지 않는다.
- 보호 등록 성공, 등록 제한 시간 초과, 로드 복원 개수를 로그에 남긴다.

## 자동 검증

- DLL 교차 빌드 성공.
- Python 187개, .NET 생성기 166개, 플러그인 CTest 2개 통과.
- 아군/적/수동 소환수 구분, 적대화·지휘자 손실 조건을 포함한 보호 정책 512조합 검사.
- 추가 호스트 테스트에서 저장된 ID 읽기, 빈 레코드, 최대 크기, 미지원 버전, 초과 크기, 모든 잘림 길이 거부 확인.
- 저장소 지정 lint/문서·CommonLib·Papyrus·Prisma·버전 검사 통과. runtime_contract 동기화 및 git diff --check 통과.
- MO2 ZIP 구조·CRC·PEX 집합·버전과 빌드 DLL 해시 일치 확인.

위 결과는 호스트/정적 검증이다. 실제 소환, 피격 적대화 및 저장·로드 후 게임 동작은 아래 테스트가 필요하다.

## 준비된 설치

- 프로필: `TKL - MUNG ADDON - SummonFix Test`
- DLL 덮어쓰기 모드: `CalamityAffixes RC6 SummonFix1 - DLL Test`
- 원본 `TKL - MUNG ADDON`의 프로필 파일·세이브/코세이브 43개는 해시 변화 없이 보존했다. 세이브/코세이브 파일 24개를 테스트 프로필에 복사했다.
- 테스트 프로필은 정상 동작이 확인된 PrismaUI 1.5.1과 기존 RC6 전체 모드를 유지하고, 수정 DLL만 가장 높은 우선순위로 활성화한다.
- 원본 프로필 선택 상태는 변경하지 않는다. Overwrite와 일부 전역 설정은 MO2 프로필 간 공유되므로 완전한 설치 격리는 아니다.
- 기록: `G:/TAKEALOOK/TOOLS/Calamity-summon-fix-20260922/verification.json`

DLL SHA256: `0eca8006743c348505c3ba8523169ec48e522670f1237e3880b09de37e049f1d`

전체 ZIP: `CalamityAffixes_MO2_v1.7.5-rc6-summonfix1_2026-09-22.zip`

ZIP SHA256: `0b4e3e092afb659de8c49b9b8b4ed931b8708c08112bc7f9f9aab056dc97d070`

## 인게임 확인 순서

1. MO2를 재시작하고 테스트 프로필을 선택해 SKSE로 실행한다.
2. 이전 빌드에서 소환된 개체는 사라질 때까지 기다린다. 이전 세이브에는 CSUM 기록이 없어 소급 식별할 수 없다.
3. Calamity 장비로 패밀리어·아트로나크를 새로 소환한다. 로그의 `protected Calamity summon`으로 등록을 확인한다.
4. 해당 소환수를 일반 공격으로 여러 번 맞혀도 체력 피해나 적대화가 생기지 않는지, 적에게는 정상적으로 피해를 받는지 확인한다.
5. 소환수의 공격·광역 마법과 불꽃 패밀리어의 폭발에 플레이어가 피해를 받지 않는지 확인한다.
6. 새 소환수가 살아 있을 때 별도 슬롯에 저장하고 다시 로드한다. `restored summon protection` 로그와 양방향 보호 유지 여부를 확인한다.
7. 수동 소환수·일반 NPC·적 소환수의 기존 전투 규칙이 유지되는지 확인한다.

이미 적대화된 이전 소환수의 전투 상태를 강제로 초기화하지 않는다. 이상이 있으면 원본 프로필로 복귀하고 새 CalamityAffixes.log를 확인한다.

## 2차 수정: summonfix2

### 1차 인게임 결과와 판단 한계

- 사용자는 1차 테스트 후 소환수가 자신의 공격에 여전히 피해를 받는 것 같다고 보고했다.
- 테스트 로그의 `Sep 22 2026 10:15:24` 빌드와 폭풍 아트로나크 `FF0017AB`, `FF0031B2`의 보호 등록을 확인했다. 현재 원본 프로필에도 사용자가 SummonFix1 DLL 모드를 활성화한 상태다. 설치 누락으로 판단하지 않는다.
- 기존 차단 로그는 DEBUG여서 실제 차단 호출 여부·공격 종류·체력 변화는 확인되지 않았다. 피격 모션/피해 숫자와 실제 체력 손실도 아직 구분되지 않았다.
- 코드에는 `!inHook`일 때만 보호를 검사하는 누락이 있었다. 중첩 피해는 원래 값 그대로 전달됐다. 이것이 사용자가 본 모든 피해의 원인이라고 확정하지 않는다.
- 참고한 [CommonLib MagicTarget 정의](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/M/MagicTarget.h)에는 효과별 AddTarget 가상 함수와 caster/effect 데이터가 있다. [FloatingDamageNG의 엔진 훅](https://github.com/alandtse/FloatingDamageNG/blob/main/src/Hooks.cpp)은 마법의 actor-value 변경이 별도 경로를 가질 수 있음을 설명한다. 로컬 고정 버전의 선언으로 훅 시그니처와 테이블을 확인했다. 현재 실행 파일의 온디스크 코드 분석은 유효한 명령열을 얻지 못했으므로 엔진 동작 검증 근거로 사용하지 않았다.

### 2차 변경

- 보호 검사를 재진입·proc guard보다 먼저 실행한다. 보호 대상이면 0 데미지 콜백을 호출하는 대신 엔진 체력 피해 호출 자체를 취소한다.
- Actor/Character/PlayerCharacter의 MagicTarget AddTarget에서 hostile/detrimental 효과를 효과별로 검사한다. 기존 정확한 Calamity 소환수/시전 소유권 규칙에 해당하는 아군 피해만 거부한다. 다른 대상, 적 피해, 비유해 효과는 원래 함수로 전달한다.
- 이미 다른 모드가 해당 슬롯을 교체했다면 강제로 덮어쓰지 않고 경고한다. 후속 모드가 훅을 다시 교체하거나 엔진 적용 경로를 우회하는 경우까지 게임 밖에서 보장할 수 없다.
- `blocked friendly health damage`, `blocked friendly magic effect`, `friendly hit observed`를 각각 제한된 횟수로 INFO 기록한다. 피격 이벤트는 체력 변경 전일 수 있으므로 한 줄의 health 값만으로 최종 피해를 단정하지 않는다.
- 보호 등록 시 `healthHookDirect`도 기록한다. false는 최상위 vtable 슬롯이 이 DLL의 훅이 아니라는 뜻이며, 다른 모드의 정상 체이닝 여부까지 판정하지 않는다.
- 호스트 테스트에서 실제 체력 훅이 사용하는 경계 함수를 호출해 중첩 아군 피해 차단, 중첩 적 피해 전달, 추가 proc 미발동, proc guard 안의 보호, 예외 후 guard 복원을 검사한다.

### 2차 테스트 방법

1. MO2를 재시작하고 `TKL - MUNG ADDON - SummonFix2 Test` 프로필로 실행한다. 새 DLL 오버레이는 `CalamityAffixes RC6 SummonFix2 - DLL Test`다.
2. 기존 소환수가 사라진 뒤 장비로 새 소환수를 부른다. 이미 적용된 지속 마법은 새 AddTarget 차단을 소급 통과하지 않는다.
3. 적이 없는 곳에서 일반 공격, 강공격, 인챈트 공격, 마법을 각각 시험한다. 체력바 감소/사망과 피격 모션/숫자를 구분한다. 가능하면 콘솔에서 소환수를 선택해 `getav health`로 전후 값을 비교한다.
4. 적 공격으로는 체력이 줄고, 회복 효과는 정상 적용되는지 확인한다. 소환수의 플레이어 공격·폭발도 확인한다.
5. 문제 재현 시 이번 로그에는 차단 경로와 체력 관찰값이 남는다. 새 로그와 사용한 공격 종류를 함께 확인한다.

2차 빌드/패키지 검증은 인게임 해결 확정과 구분한다. 해시와 설치 보존 검증은 `G:/TAKEALOOK/TOOLS/Calamity-summon-fix2-20260922/verification.json`에 기록한다.

## 3차 수정: summonfix3 — 시작 CTD

- 2차 빌드는 폐기 대상이다. 2026-09-22 15:49:12 시작 CTD에서 `CalamityAffixes.dll+015CC1D`, `mov rdi, [rax+0x08]`, RAX=0이 확인됐다. 당시 설치 DLL SHA256은 `d833bb70c63f6a1bb7ae2b0ba0476952d820cfb1d653bfb7647c6826ec693415`로 2차 패키지와 일치한다.
- 보관한 동일 DLL/PDB를 llvm-symbolizer로 대조하면 `FriendlyMagicTargetHook<RE::Actor>::Install`, Hooks.cpp:59에 정확히 대응한다. 소환이나 저장 로드 전에 `kDataLoaded`의 훅 설치에서 발생했다.
- 고정된 CommonLib의 Actor에는 자체 VTABLE 멤버가 없다. `RE::Actor::VTABLE`은 4개 항목인 TESObjectREFR 테이블을 상속한다. 2차 코드의 `[4]`가 배열 경계를 벗어나 잘못된 주소를 읽었다. 이는 수정 코드의 결함이며 사용자 설치 오류로 판단하지 않는다.
- 템플릿에는 클래스 대신 명시적인 `RE::VTABLE_Actor`, `RE::VTABLE_Character`, `RE::VTABLE_PlayerCharacter` 배열을 전달한다. static_assert로 5개 이상인지 검사하고 constexpr `.at(4)`로 항목을 선택한다. 해석된 주소가 null이면 역참조하지 않는다.
- 실제 Hooks.cpp 복사본에 잘못된 `RE::Actor::VTABLE`을 다시 넣고 동일 교차 컴파일 명령으로 빌드했을 때, 새 static_assert의 메시지로 컴파일이 거부되는 것을 확인했다. 수정된 실제 코드는 빌드에 성공했다.
- 2차의 체력 피해·마법 보호 정책은 유지한다. 이번 수정 범위는 시작 CTD의 테이블 선택 오류다. 소환수 아군 피해의 실제 해결 여부는 여전히 별도 인게임 확인이 필요하다.
- 사용자 현재 프로필의 모드 활성화 상태를 바꾸지 않고, 이미 켜 둔 `CalamityAffixes RC6 SummonFix2 - DLL Test` 안의 DLL을 3차 수정본으로 교체한다. 이름은 유지하고 meta.ini에 실제 3차 버전을 기록한다. 원본 DLL/PDB/로그/메타데이터는 `G:/TAKEALOOK/TOOLS/Calamity-summon-fix3-20260922`에 보관한다.
- 다음 실행에서 `installed Actor friendly MagicTarget::AddTarget hook (summonfix3)` 등 3개 훅 설치 로그와 메뉴 진입을 우선 확인한다. 이후 새 소환수의 체력 감소, 적 피해, 플레이어 피격을 확인한다.

## 최종 사용자 확인과 정식 릴리스

2026-09-22 사용자가 3차 수정본을 실행하고 “테스트 해보니까 보호 잘 되는것같아”라고 보고했다. 시작 CTD를 수정한 뒤의 정상 실행 및 소환수 보호 개선 보고로 기록한다. 공격 종류·소환 종류·로드 복원 등 각 세부 항목의 별도 완료 보고로 확대하지 않는다. 사용자가 이 결과를 기준으로 커밋과 v1.7.5 정식 릴리스 게시를 요청했다. 위 단계별 “인게임 검증 대기” 문구는 각 테스트 당시 상태를 나타낸다.
