# Calamity - Reactive Loot & Affixes v2.1.2

v2.1.2는 룬워드 이름 표시를 바로잡은 업데이트입니다. 게임 동작과 코세이브는 2.1.1과 같고, ESP는 마법 효과 이름 19개만 바뀌었습니다. 확인 스크롤 교환은 [2.1.0 릴리스 노트](https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/releases/2026-09-27-github-release-body-v2.1.0.md), 2.0의 제작 흐름 전체는 [2.0.0 릴리스 노트](https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/releases/2026-09-26-github-release-body-v2.0.0.md)를 확인하세요.

## 고친 점

- **잘린 이름 7개:** 여러 단어로 된 룬워드가 툴팁에서 첫 단어만 보였습니다. "Runeword Holy"는 "Runeword Holy Thunder"로, "룬워드 킹스"는 "룬워드 왕의 은총"으로 표시합니다. 대상: Ancient's Pledge, Holy Thunder, King's Grace, Unbending Will, Voice of Reason, Crescent Moon, Flickering Flame.
- **한국어 이름 통일:** 37개 룬워드가 툴팁과 패널 레시피 목록에서 서로 다른 이름을 썼습니다(툴팁 "혼돈", 목록 "카오스"). 이제 94개 모두 패널 목록과 같은 이름을 씁니다.
- **Plague 표기:** 패널 목록의 "플래그"(flag처럼 읽힘)를 "플레이그"로 바로잡았습니다.
- **마법 효과 창의 영어 줄임말:** CTA, CoH, HOTO, BotD, Hustle(W)/(A)를 Call to Arms, Chains of Honor, Heart of the Oak, Breath of the Dying, Hustle-W/-A로 풀었습니다.

효과와 수치는 바뀌지 않습니다.

## 확인한 범위

- 새 ESP를 2.1.1 ESP와 기록 단위로 대조했습니다. 기록 순서, FormID, EditorID, 플래그, 그룹 구조와 이름을 뺀 모든 필드가 같고, 달라진 것은 효과 이름 19개뿐입니다. 세이브 호환에 영향이 없습니다.
- 룬워드 94개의 툴팁 이름과 기록 이름 235개를 레시피 목록·패널 이름표와 대조하는 테스트를 추가했습니다(수정 전 데이터에서 실패 확인). Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.
- 이번 변경은 표시 문자열만 바꾸므로 인게임 확인 없이 데이터 검증으로 릴리스했습니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.1.2_2026-09-27.zip`** 전체를 설치합니다. ESP와 DLL이 함께 바뀌었으므로 MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다. 1.x에서 올라오는 경우의 안내는 2.0.0 노트와 같습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
