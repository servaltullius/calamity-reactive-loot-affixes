# Calamity - Reactive Loot & Affixes v2.2.4

v2.2.4는 **주문·범위 피해가 무기 적중으로 처리되던 문제**를 고친 핫픽스입니다. ESP, 코세이브, 게임 데이터는 2.2.3과 같습니다. 2.2.2에서 올라오는 경우 [2.2.3](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.2.3)의 AE 1.7.x 로드 수정도 함께 받습니다.

*English summary: spell and hazard damage without a weapon (e.g. a crit-cast Ice Storm ticking every 0.1s) was filled in with the weapon the player held, so "on hit" affixes kept firing without any swing. On-hit affixes now react only to weapon and unarmed attacks, explosions, and bow/crossbow arrows. Also includes the 2.2.3 fix for Skyrim AE 1.7.x. No ESP or save changes; install over 2.2.2 or 2.2.3 with Replace, no new game needed.*

## 고친 점

- **칼질 없이도 "적중 시" 효과가 계속 발동하던 문제를 고쳤습니다.**

  무기 정보가 없는 피해(예: 치명 시전 얼음 폭풍이 0.1초마다 주는 피해)를 플레이어가 들고 있는 무기로 채워 넣는 바람에, 주문·범위 피해의 매 틱이 칼질처럼 처리됐습니다. 실측에서 뇌격 숙련이 48번 중 46번을 이렇게 발동해(초당 약 5회) 칼질보다 많은 피해를 냈고, 성장 단계도 비정상적으로 빨리 올랐습니다. 생명력 흡수 같은 다른 적중 시 효과도 같은 영향을 받았습니다.

  이제 적중 시 효과는 **무기·맨손 공격, 폭발, 활·석궁 화살**에만 반응합니다. 화살은 들고 있는 무기가 아니라 발사한 투사체에서 무기를 확인하므로 계속 적중으로 인정됩니다. 이 피해들이 같은 적중의 중복 판정 칸을 차지해 실제 칼질이 무시되던 일도 함께 사라집니다.

- **이미 처리한 적중이 나중에 들어온 주문 피해마다 다시 적중으로 처리되던 문제를 고쳤습니다.**

  적의 "마지막으로 맞은 기록"은 다음 적중이 올 때까지 그대로 남습니다. 오래된 기록을 걸러 내는 방어가 마지막 처리 후 5초 안에서만 작동해서, 화살로 맞힌 지 13초 지난 매머드에 얼음 폭풍이 번지자 틱마다 그 화살이 새 활 적중으로 처리됐습니다. 0.15초마다 뇌격 숙련이 터지고 치명 시전이 다시 시전되며 연쇄가 이어졌습니다. 이제 같은 적중 기록은 시간과 상관없이 한 번만 적중으로 처리합니다.

## 2.2.3에서 고친 점 (그대로 포함)

- 스카이림 AE 1.7.x(예: 1.7.104)에서 `failed to open address library file`, 이어서 `Unsupported address library format: 5` 오류로 모드가 로드되지 않던 문제. 신고해 주신 분이 1.7.104에서 확인했습니다.

## 호환성

- 적중 시 효과가 주문·범위 피해로 발동하던 만큼 전투 중 체감 피해와 성장 속도가 줄어들 수 있습니다. 이것이 원래 의도한 수치입니다.
- 기존 세이브를 그대로 쓸 수 있습니다. 코세이브 형식은 2.2.0과 같습니다.

## 확인한 범위

- 판정 규칙은 컴파일 시점 테스트로 고정했습니다: 무기 기록, 공격 주문, 맨손(근접 플래그), 폭발, 투사체의 활·석궁은 적중으로 인정하고, 그 밖의 주문·해저드 틱은 들고 있는 무기와 상관없이 인정하지 않습니다.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.
- AE 1.6.1170 인게임(전문가 난이도)에서 확인했습니다. 근접 공격만 할 때 치명 시전 얼음 폭풍 틱으로 뇌격 숙련이 연발하지 않습니다. 매머드를 활로 맞힌 뒤 얼음 폭풍이 계속 닿는 동안에도 화살이 다시 적중으로 처리되지 않았고, 이어서 특대검으로 친 적중에서는 적중 효과가 정상 발동했습니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.2.4_2026-10-04.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
