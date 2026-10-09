# Calamity - Reactive Loot & Affixes v2.3.0-rc2

2.3.0-rc2는 [2.3.0-rc1](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.3.0-rc1)의 제작 개편(룬워드가 접두 칸을 차지)에 **패널 색·배치 개편**을 더한 공개 테스트 빌드입니다. DLL·ESP·코세이브는 rc1과 같고, 바뀐 것은 패널 화면뿐입니다. 문제가 없으면 같은 내용으로 2.3.0 정식을 냅니다. 그때까지 정식 버전은 v2.2.8입니다.

*English summary: same gameplay, DLL, ESP and co-save as 2.3.0-rc1 (a runeword now takes the item's prefix slot). New in rc2 is the panel: the neon-green dashboard look is replaced by a palette taken from common Skyrim UI themes (smoky brown, bone text, muted gold), the Item Affixes tab now puts the item you are working on first (its three slots, the affix to replace and the actions) with resources and the equipped build as reference on the right, the Runeword tab drops a title-only row so the recipe list shows more entries, and redundant chips and always-on hints are gone. Install over 2.2.x or 2.3.0-rc1 with Replace, no new game needed.*

## rc1에서 이어지는 제작 개편

- 장비의 어픽스 칸 3개 중 첫 칸(머리 칸)에는 **룬워드나 접두 중 하나**만 들어가고, 나머지 두 칸은 접미입니다.
- 접두가 있는 장비에 룬워드를 입히면 그 접두가 사라집니다(확인창). 룬워드 장비는 정제·확인·확장 모두 접미만 다룹니다.
- **룬워드 제거** 버튼: 정제 오브 1개로 룬워드를 지우고 그 칸에 새 접두를 굴립니다.
- 2.3.0 전에 룬워드와 접두를 함께 가진 장비는 예전 규칙 그대로입니다.

자세한 규칙은 [rc1 릴리스 노트](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.3.0-rc1)를 보세요.

## rc2에서 바뀐 패널

### 색

- 형광초록 대시보드 색을 버리고, 인벤토리·MCM 테마와 어울리는 **갈색 연기 바탕, 뼈색 글자, 바랜 금빛 강조**로 바꿨습니다. 색은 테마 화면에서 직접 뽑았습니다.
- 위험(벽돌색)과 경고(호박색)도 채도를 낮춰 화면에서 튀지 않게 했고, 모서리는 덜 둥글게 했습니다.

### 아이템 어픽스 탭

- **왼쪽:** 작업하는 장비. 슬롯 띠(룬워드 / 접미 1 / 접미 2) 아래에 장비의 칸이 목록으로 나오고, 그중 재련할 어픽스를 골라 바로 아래 버튼으로 작업합니다. 룬워드 줄은 "재련 대상 아님"으로 표시됩니다.
- **오른쪽:** 제작 재료와 장착 빌드, 가리킨 아이템 확인. 참고용 정보를 한쪽으로 모았습니다.
- 장비 툴팁 전체 문장은 "장비 효과 전체 보기"에 접어 두었습니다. 같은 어픽스가 두 번 나오지 않습니다.

### 룬워드 탭

- 제목만 있던 "룬워드 작업대" 줄과 선택 레시피 칩을 없애고, 레시피 이름은 검토 칸 제목으로 옮겼습니다.
- 룬 조각과 천장을 한 줄로 줄였습니다. 레시피 목록에 줄이 더 보입니다.

### 군더더기 정리

- 장착 빌드의 "발동 100%" 칩, 준비된 레시피의 "N/N개 보유", 늘 떠 있던 천장 설명문(마우스를 올리면 보임)을 표시하지 않습니다.
- 효과가 없는 장착 빌드 그룹은 숨깁니다.
- 고급 탭에는 장비 카드가 나오지 않고, 단계 번호(1·2·3)는 룬워드 탭에서만 씁니다.
- 패널 뒤 HUD 글자가 덜 비치도록 바탕을 조금 더 불투명하게 했습니다.

## 확인한 범위

- AE 1.6.1170 인게임(4K)에서 새 색과 세 탭의 배치를 확인했습니다. rc1의 제작 동작(변환·정제·룬워드 제거·옛 장비·저장 후 유지)은 rc1에서 확인했습니다.
- 패널을 세 가지 크기 × 세 가지 언어 설정으로 측정해, rc1보다 새로 잘리거나 가려지는 곳이 없는 것을 확인했습니다(말줄임으로 잘리던 곳은 10곳 줄었습니다).
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다. 패널에 팔레트 밖의 색이 다시 들어오면 실패하는 검사를 추가했습니다.

## 설치

1. **`CalamityAffixes_MO2_v2.3.0-rc2_2026-10-09.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
