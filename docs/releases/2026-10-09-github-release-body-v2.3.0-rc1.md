# Calamity - Reactive Loot & Affixes v2.3.0-rc1

2.3.0-rc1은 **룬워드가 접두 칸을 차지하도록** 바꾼 제작 개편의 공개 테스트 빌드입니다. 룬워드는 이제 접두 위에 덤으로 얹히지 않고, 접두와 맞바꾸는 선택입니다. ESP와 코세이브 형식은 [2.2.8](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.2.8)과 같고, 기존 세이브의 장비는 그대로 유지됩니다. 문제가 없으면 같은 내용으로 2.3.0 정식을 냅니다. 그때까지 정식 버전은 v2.2.8입니다.

*English summary: a runeword now takes the item's prefix slot. Every item has three affix slots: a head slot that holds a runeword or a prefix, then up to two suffixes. Transmuting onto an item with a prefix removes that prefix (the panel names it and asks for a second click within 6 seconds); suffixes stay. Scouring a runeword item rerolls only its suffixes; identifying a runeword-only item adds 1 suffix (75%) or 2 (25%); expansion counts the runeword as a slot. A new Remove Runeword button spends one Scouring Orb to take the runeword off and roll a prefix into its slot. Items that already had a runeword on top of a prefix keep working under the old rules. Same ESP and co-save format as 2.2.8; install over 2.2.x with Replace, no new game needed.*

## 무엇이 바뀌나

장비에는 어픽스 칸이 3개 있습니다. 첫 칸(머리 칸)에는 **룬워드나 접두 중 하나**가 들어가고, 나머지 두 칸은 접미입니다.

지금까지는 룬워드가 접두 위에 하나 더 붙어서, 룬워드를 입히는 것이 언제나 이득이었습니다. 이제는 고르는 문제가 됩니다.

- **접두:** 공격 효과가 많습니다(치명 시전, 덫, 소환, 피해 변환 등). 원하는 접두는 재련으로 노려야 해서 운이 따라야 합니다.
- **룬워드:** 버프·약화 효과가 많고, 17개는 상시 보너스(공격속도, 저항 등)까지 붙습니다. 룬 조각을 모으면 확정으로 만들 수 있습니다.

## 바뀐 동작

| 작업 | 이번 버전 |
|---|---|
| 접두가 있는 장비에 룬워드 변환 | 사라질 접두 이름을 보여 주고, 6초 안에 변환을 한 번 더 눌러야 진행합니다. 접두는 사라지고 접미는 남습니다. |
| 룬워드 장비 정제 | 룬워드는 그대로, 접미만 다시 굴립니다. 선택 재련 6회는 지금처럼 되돌려 받습니다. |
| 룬워드만 있는 장비에 확인 스크롤 | 접미 1개(75%) 또는 2개(25%)가 붙습니다. |
| 룬워드 장비 슬롯 확장 | 룬워드를 한 칸으로 세어 `룬워드 + 접미 2`까지, 지금과 같은 비용으로 늘어납니다. |
| **룬워드 제거 (새 버튼)** | 정제 오브 1개로 룬워드를 지우고 그 칸에 새 접두를 굴립니다. 접미와 남은 선택 재련 횟수는 그대로입니다. 사용한 룬 조각은 돌려받지 못하며, 6초 안에 한 번 더 눌러 확정합니다. |

- 룬워드와 접두가 함께 있을 때 둘 다 발동 확률이 80%로 줄던 규칙은 새 장비에서 더 나오지 않습니다.
- 툴팁의 룬워드 줄은 `[P]` 대신 `[R]`로 표시합니다. 패널의 슬롯 표시도 "룬워드 / 접미 1 / 접미 2"로 나옵니다.
- 룬워드 탭에서 베이스 불일치 문장이 네 번 반복되던 것을 검토 칸 한 곳으로 줄였습니다.

## 기존 세이브

- **이미 룬워드와 접두를 함께 가진 장비는 그대로입니다.** 전투·정제·재련·확장 모두 예전 규칙대로 동작합니다.
- 그 장비를 다른 룬워드로 바꾸거나 룬워드를 제거할 때만 새 규칙으로 넘어가고, 이때 함께 있던 접두가 사라진다고 확인창에 표시합니다.
- 코세이브 형식은 바뀌지 않았습니다. 다만 이 버전에서 만든 룬워드 장비(접두 없음)를 2.2.x로 되돌려 불러오면 비정상 구성으로 보여 정제만 할 수 있습니다.

## 확인한 범위

- AE 1.6.1170 인게임에서 확인했습니다.
  - 접두가 있는 장비에 변환: 경고 후 접두만 사라짐
  - 룬워드 장비 정제: 접미만 바뀜
  - 룬워드 제거: 새 접두가 생김
  - 기존 룬워드+접두 장비: 예전 규칙대로 동작
  - 저장 후 다시 불러오기: 그대로 유지
- 새 배치 규칙(제거·확인·정제·확장·옛 장비)을 정적 테스트로 고정했고, 패널 동작 테스트와 레이아웃 측정(2.2.8 대비 새 잘림 0)을 통과했습니다.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.

## 설치

1. **`CalamityAffixes_MO2_v2.3.0-rc1_2026-10-09.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
