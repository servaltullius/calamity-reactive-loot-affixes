# 인게임 스모크 체크리스트

테스트 빌드를 설치할 때마다 반복하는 최소 검증 절차입니다. 원칙 한 가지:
**로그는 "엔진 경로가 돌았다"까지만 증명합니다.** 화면에 보였는지, 소리가
들렸는지, 잔상이 남는지는 눈과 귀로만 판정합니다. `instantiated=true`,
`played=true`, `magic effect apply observed`를 시각/음향 성공의 근거로 쓰지
마세요.

## 1. 설치

- [ ] MO2에서 기존 모드에 **Replace**로 설치 (Merge 금지 — 옛 파일이 남습니다)
- [ ] 게임 실행 후 새 `CalamityAffixes.log` 생성 확인
  (`Documents/My Games/Skyrim Special Edition/SKSE/`)

## 2. 빌드 신원 확인

- [ ] 설치된 DLL SHA-256이 배포 ZIP 안내 값과 일치:

```bash
sha256sum "<MO2 mods 폴더>/CalamityAffixes/SKSE/Plugins/CalamityAffixes.dll"
```

- 로그 첫 줄의 `build <날짜> <시각>` 배너는 **증분 빌드에서 낡은 값이 남을 수
  있으므로** 단독 근거로 쓰지 않습니다. 해시 또는 신규 로그 라인의 존재로
  판정합니다.

## 3. 디버그 설정

- [ ] `Data/SKSE/Plugins/CalamityAffixes/user_settings.json`에서
  `debugVerboseLogging=true`, `debugHudNotifications=true`
- 발동 확률 육안 확인이 목적이면 `runtime.procChanceMultiplier`를 임시로 올릴
  수 있습니다(최대 3.0). **확인 후 반드시 1.0으로 원복.**

## 4. 발동 파이프라인 (로그 관찰)

- [ ] 발동형 어픽스/룬워드 장착 → 여러 대 버티는 적에게 20타 이상
- [ ] 발동 시 HUD에 `CAFF proc: <affixId>` 표시
- [ ] 세션 종료 후:

```bash
python3 tools/qa_skse_logcheck.py
```

- [ ] `RESULT: PASS` (부팅·에러 판정)
- [ ] `feedback pipeline observation` 섹션에서 필요한 그룹이 `OBSERVED`
  - 이 섹션은 **경로 관찰 전용**입니다. 전투가 없던 세션은 `NOT OBSERVED`가
    정상이고 실패가 아닙니다.
  - `feedback pipeline engine-path rejections`에 항목이 있으면 엔진이 요청을
    거부한 것이므로 해당 어픽스를 인게임에서 재확인합니다.

참고 — 확률 기대치: 발동률 30%·ICD 0.8초 기준 20 유효타에서 평균 6회,
2~10회면 정상 범위입니다. 1~2대에 죽는 잡몹은 표본이 되지 않습니다.

## 5. 시각·음향 판정 (육안 전용 — 로그로 대체 불가)

- [ ] 발동 순간 대상/자신의 몸에 아트가 **실제로 보임**
- [ ] 발동음이 **실제로 들리고** 위치감(3D)이 자연스러움
- [ ] 대상 사망·래그돌 전환 후 아트가 지속시간 내에 사라지고 **잔류 없음**
- [ ] 다중 proc 동시 발동 시 과다 중첩/스팸으로 느껴지지 않음
- [ ] (원소 proc) 아트 테마가 효과와 어울림 (화염=화염 등)

## 6. 회귀 확인 (각 1회)

- [ ] 다른 계열 proc 1종 (예: Owner 버프형)
- [ ] 시체 폭발 1회
- [ ] 함정 어픽스 1종: 설치 → 발동 → 만료
- [ ] 아이템 툴팁: 발동형 어픽스의 기본/유효 발동률 표기 확인
- [ ] 저장 → 로드 → 발동 정상 (proc 영구 멈춤 회귀 감시)

### 곰덫 월드 마커 파일럿

- [ ] 디버그 패널에서 첫 번째 함정 어픽스인 `bear_trap`을 무기에 부여
- [ ] 발동 직후 적 발밑에 룬이 아니라 **바닐라 금속 곰덫 턱**이 실제로 보임
- [ ] 설치 직후 닫힌 기본 자세에 머물지 않고 `StartOpen`으로 열리며, 열린 뒤 900ms 동안 첫 발동이 보류됨
- [ ] 적이 밟아 논리 덫이 발동하면 `Trigger01`로 닫히고, 재무장 시 `Reset01`로 다시 열린 뒤에도 900ms 동안 다음 발동이 보류됨
- [ ] 닫힘·재무장 사운드가 behavior graph에서 각각 한 번만 들리고 명시적 사운드와 겹치지 않음
- [ ] 플레이어가 이동하거나 카메라를 돌려도 마커가 플레이어를 따라오지 않고 설치 좌표에 고정
- [ ] 활성화 문구가 없고, 걷기·시체·투사체와 충돌하지 않으며, 바닐라 `TrapBear`의 추가 피해가 발생하지 않음
- [ ] 발동 소모·12초 만료·cap 축출·셀 이탈 뒤 모델이 남지 않음
- [ ] 모델이 보이는 동안 즉시 퀵세이브하면 모델이 사라지고 활성 덫이 취소됨
- [ ] 같은 저장을 로드하고 원래 셀에 재진입해도 유령 모델·충돌·활성화 문구가 없음
- [ ] 로그의 생성 행에 `base=CAFF_MSTT_TRAP_BEAR_VISUAL`, `handleAllocated=true`, `resolved=true`, `strongOwnerRetained=true`, `spawned=true`
- [ ] 퀵세이브 로그에 `pre-save trap cleanup complete (activeTraps=0, unresolvedDeferred=0)`이 먼저 기록되고, 뒤의 `serialization save trap state`도 두 값이 0
- [ ] `handleAllocated=true, resolved=false` 또는 `unresolvedDeferred`가 0보다 크면 정상 판정하지 말고 해당 로그와 저장 파일을 보관

정식 릴리스 전에는 활성 곰덫 상태의 저장·로드를 20회 반복해 저장 크기가 계속
증가하지 않는지와 generated REFR/change-form 누적이 없는지도 확인합니다. 이 검증은
자동 테스트나 `spawned=true` 로그로 대체할 수 없습니다.

### 나머지 5종 물리 마커 회귀와 동료 보호

5종 모델의 지면 가시성은 2026-08-11 인게임에서 확인했습니다. 아래 가시성 항목은 다음 빌드용 반복 체크이며, 충돌·정리·동료/중립 보호·밀도 항목은 아직 완료되지 않은 인게임 검증입니다.

- [ ] 디버그 패널의 `Production Trap World-Ref Probe`를 한 번 눌러 실사용 6종 마커가 플레이어 앞 두 줄에 나타나고 약 3초 뒤 모두 사라지는지 확인
- [ ] probe 직후 `qa_skse_logcheck.py`에서 `trap world marker probe contracts: OBSERVED 6/6`과 `trap_world_marker_spawn`, `trap_world_marker_cleanup`의 `OBSERVED`를 확인(화면 가시성 판정은 별도)
- [ ] probe를 다시 실행한 직후 퀵세이브해 6종이 즉시 정리되고 `trap_pre_save_state_clean`과 `trap_serialization_state_clean`이 `OBSERVED`인지 확인
- [ ] 디버그 패널로 `rune_trap`, `plague_spore`, `tar_blight`, `siphon_spore`, `chaos_rune`을 차례로 부여해 각 함정을 최소 한 번씩 설치·발동
- [ ] `rune_trap`: 적 발밑에 화염 룬 글리프가 아니라 바닐라 석재 압력판(`TrapStonePressurePlate01.nif`)이 보임
- [ ] `plague_spore`: 독 룬 글리프가 아니라 바닐라 독거미 주머니(`spidersackdead.nif`)가 보임
- [ ] `tar_blight`: 재 룬 글리프가 아니라 바닐라 기름 웅덩이(`OilTrapPuddle01.nif`)가 보임
- [ ] `siphon_spore`: 광란 룬 글리프가 아니라 바닐라 흰거미 알(`ExpSpiderEggsAlbino.nif`)이 보임
- [ ] `chaos_rune`: 전격 룬 글리프가 아니라 바닐라 금속 압력판(`TrapPressurePlateMetal01.nif`)이 보임
- [ ] 다섯 모델 모두 대상의 발밑 지면에 자연스럽게 놓이고 떠 있거나 묻히거나 기울지 않으며, 플레이어 이동·카메라 회전을 따라오지 않음
- [ ] 각 물리 마커에 활성화 문구가 없고, 플레이어·동료·시체·투사체와 충돌하지 않으며, 원본 바닐라 함정의 스크립트 피해나 작동음이 추가로 발생하지 않음
- [ ] 기존의 짧은 무장 큐와 발동 폭발/음향이 각각 한 번씩 재생되고, 물리 마커를 가리거나 발동 뒤 오래 남지 않음
- [ ] 1회 발동 소모, 자연 만료, per-affix/global cap 축출, 셀 이탈, 설정 초기화 뒤 해당 물리 마커가 남지 않음
- [ ] 활성 마커가 있는 상태로 퀵세이브하면 함정과 마커가 취소되고, 같은 저장을 로드하거나 셀에 재진입해도 유령 모델·충돌·활성화 문구가 없음
- [ ] 로그의 각 생성 행에 예상 `base=CAFF_MSTT_TRAP_*_VISUAL`, `handleAllocated=true`, `resolved=true`, `strongOwnerRetained=true`, `spawned=true`가 기록됨
- [ ] 적과 동료/중립 NPC를 같은 함정 반경 안에 둔 A/B에서 적대 대상은 최대 2명까지만 설정 효과를 받고, 동료·중립 NPC는 피해·감속·방어 저하·재생 저하·저주를 받지 않음
- [ ] `magic effect apply observed`의 함정 MGEF 대상이 직접 선택된 적대 액터로만 기록되고, 동료 FormID가 나타나지 않음

`spawned=true`와 `Effect.Area=0` 정적 검증만으로 충돌 차단·정리·동료 무영향을
통과 처리하지 않습니다. 동료 동반 전투 A/B와 저장/로드·셀 전환 검증이 모두 끝난
뒤에만 hostile-direct 및 월드 레퍼런스 수명 계약을 인게임 확인 완료로 기록합니다.

## 7. 안정성 시나리오

릴리스 후보 전에는 [QA_STABILITY_SCENARIOS.md](QA_STABILITY_SCENARIOS.md)의
시나리오 매트릭스를 별도로 수행합니다. 일반 전투 스모크는 그 대체가 되지
않습니다.
