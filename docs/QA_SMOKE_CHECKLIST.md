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

## 7. 안정성 시나리오

릴리스 후보 전에는 [QA_STABILITY_SCENARIOS.md](QA_STABILITY_SCENARIOS.md)의
시나리오 매트릭스를 별도로 수행합니다. 일반 전투 스모크는 그 대체가 되지
않습니다.
