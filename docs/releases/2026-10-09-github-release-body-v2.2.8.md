# Calamity - Reactive Loot & Affixes v2.2.8

v2.2.8은 **powerofthree's Tweaks 없이도 모드가 제대로 동작하게** 고친 업데이트입니다. ESP와 코세이브 형식은 2.2.7과 같고, DLL만 바뀝니다.

*English summary: the mod no longer needs powerofthree's Tweaks. It finds its own spells, magic effects, currency items and hit art by editor ID, and Skyrim discards those editor IDs at load, so up to 2.2.7 everything relied on po3 Tweaks' "Load EditorIDs" (on by default, but never listed as a requirement). The DLL now reads CalamityAffixes.esp at startup and indexes its 793 records itself. Checks that ask "is this one of our spells?" now actually work: Archmage no longer procs off the mod's own spells, the mod's spell hits are kept out of weapon procs, and a corpse that already carries a currency is not rolled for it again. With po3 Tweaks installed, the armor deny list and boss-chest name checks also apply to other mods' forms. ESP and co-save are unchanged; install over 2.2.x with Replace.*

## 수정: powerofthree's Tweaks 의존 제거

- 이 모드는 자기 주문·마법 효과·재화 아이템·아트를 이름(에디터 ID)으로 찾습니다. 그런데 스카이림은 이 종류의 이름을 게임을 불러올 때 버립니다.
- 그래서 2.2.7까지는 powerofthree's Tweaks의 "Load EditorIDs"(기본 켜짐)가 이름을 되살려 줘야만 어픽스 효과, 룬 조각·오브 같은 재화, 적중 이펙트가 제대로 동작했습니다. 필수 모드 목록에는 이 사실이 빠져 있었습니다.
- 이제 DLL이 시작할 때 `CalamityAffixes.esp`를 직접 읽어 레코드 793개의 이름표를 만들어 두고, 그 표로 찾습니다. 별도 파일을 추가하지 않고 설치된 ESP를 그대로 읽으므로, 이름표와 ESP가 어긋날 일이 없습니다.
- powerofthree's Tweaks가 있는 환경에서는 찾는 결과가 이전과 같습니다. 권장 모드로는 계속 남겨 둡니다.

## 이제 실제로 동작하는 검사

주문의 이름은 powerofthree's Tweaks가 있어도 기존 방식으로는 읽히지 않아, "이 모드의 주문인지" 확인하는 검사들이 늘 "아니다"로 끝났습니다. 이번에 이름표로 고치면서 원래 의도대로 동작합니다.

- **대마법사:** 이 모드가 시전한 주문(전격, 그림자 타격 등)의 적중에서는 발동하지 않습니다. 원래 의도한 재귀 방지입니다. 플레이어가 직접 쓴 주문에서는 그대로 발동합니다.
- **무기 적중 판정:** 이 모드의 주문으로 생긴 적중은 무기 적중 발동 판정에서 빠집니다.
- **시체 재화:** 시체가 이미 룬 조각이나 오브를 들고 있으면 같은 종류를 다시 굴리지 않습니다(중복 지급 방지).
- **방어구 제외 목록·보스 상자:** 이름에 `rewardbox`·`lootbox` 등이 들어간 다른 모드의 보상용 방어구는 어픽스 대상에서 빠지고, 이름에 `boss`가 들어간 상자는 보스 상자로 취급됩니다. 다른 모드·바닐라 폼의 이름은 powerofthree's Tweaks가 있을 때만 읽을 수 있으므로, 이 두 가지는 powerofthree's Tweaks가 있을 때만 적용됩니다.

## 호환성

- 기존 세이브를 그대로 쓸 수 있습니다. ESP와 코세이브 형식은 2.2.7과 같습니다.
- powerofthree's Tweaks는 필수에서 권장으로 바뀝니다.
- SE 1.5.97, AE 1.6.x, AE 1.7.x 모두 2.2.7과 같은 방식으로 동작합니다.

## 확인한 범위

- AE 1.6.1170 인게임에서 두 번 시험했습니다.
  - **powerofthree's Tweaks 켜짐:** 이름표 793개를 모두 찾았고 경고는 0건이었습니다. 어픽스 발동 100회, 적중 이펙트 130회, 그림자 타격 38번이 모두 이펙트·소리와 함께 나왔습니다.
  - **powerofthree's Tweaks의 "Load EditorIDs" 끔:** 이름표 793개를 모두 찾았고 경고는 0건이었습니다. 어픽스 244개가 로드됐고 착용 어픽스·지속 효과도 정상 적용됐습니다. 발동 55회, 적중 이펙트 65회, 그림자 타격 17번이 모두 정상이었고, 시체 재화 레코드도 모두 찾아 보스가 룬 조각과 재련 오브를 확정으로 받았습니다.
- 새 호스트 검사가 실제 배포 ESP를 읽어 레코드 793개와 고정 FormID를 확인합니다. 소스에서 엔진의 이름표 함수를 직접 쓰면 실패하도록 막았습니다.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.2.8_2026-10-09.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
