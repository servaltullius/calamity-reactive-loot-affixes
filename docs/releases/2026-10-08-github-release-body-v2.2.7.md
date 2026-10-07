# Calamity - Reactive Loot & Affixes v2.2.7

v2.2.7은 **새 룬워드 쉐도우 복서(Shadow Boxer)**를 추가한 업데이트입니다. ESP에는 기존 레코드 뒤에 7개가 붙고, 코세이브 형식은 2.2.6과 같습니다.

*English summary: adds the Shadow Boxer runeword (Shael-Ko-Um, recommended base: any melee weapon), inspired by the Dungeon Fighter Online striker. Any melee hit starts an 8-second shadow stance (12-second cooldown); during it, every melee hit is repeated 0.25 s later by a shadow strike for 40% of that hit's physical damage. Shadow strikes are not new hits, so they never trigger other on-hit affixes or further shadow strikes. Each strike shows a dark purple wisp burst and a heavy punch sound. Difficulty applies to shadow strikes exactly as it does to your weapon. Includes the 2.2.6 trap marker crash fix. Existing saves work; install over 2.2.x with Replace.*

## 새 룬워드: 쉐도우 복서

- **룬:** 샤엘(Shael) · 코(Ko) · 움(Um)
- **추천 베이스:** 근접 무기. 한손·양손 무기와 맨손을 가리지 않고 모든 근접 공격에 적용됩니다.
- **발동:** 일반 공격이든 강공격이든 근접 공격이 적중하면 8초 동안 그림자 권투 상태가 됩니다. 재사용 대기시간은 12초라, 계속 싸우면 8초 켜지고 4초 쉬는 흐름이 됩니다.
- **그림자 타격:** 그림자 권투 상태에서 근접 공격이 적중할 때마다 0.25초 뒤 그림자가 같은 적을 한 번 더 때립니다. 피해는 그 적중 물리 피해의 40%입니다.
- **대상 아님:** 활·석궁, 주문, 방패 치기는 그림자가 따라 하지 않습니다.
- **연쇄 없음:** 그림자 타격의 피해는 새 적중으로 치지 않습니다. 그래서 다른 적중 발동 효과(전격 등)를 다시 일으키지 않고, 그림자 타격이 또 그림자 타격을 부르지도 않습니다.
- **난이도:** 그림자 타격에도 칼질과 똑같은 난이도 배율이 적용됩니다. 어느 난이도에서든 실제 칼질 피해의 40%가 됩니다.

### 연출

- 발동하면 캐릭터 주위에 보라빛 오라가 잠깐 터지고, 8초 동안 흑단 갑옷과 같은 검은 연기가 몸을 감쌉니다.
- 그림자 타격마다 적의 몸에 보라빛 섬광과 **짙은 보라색 실타래 연기(1초)**가 터지고 묵직한 펀치 소리가 납니다.
- 이 연기는 이 모드 전용 이펙트입니다. 흡수 마법의 적중 이펙트를 바탕으로 색을 짙은 보라로 바꿨습니다. 원래 색은 얼음빛 파랑이라 전격 효과 사이에서 구분되지 않았습니다.

## 2.2.6에서 이어지는 수정

- 2.2.6의 덫 표시물 튕김 수정(충돌 없는 전용 메시)이 그대로 들어 있습니다. 2.2.5 이하에서 올라오는 경우 함께 받습니다.

## 호환성

- 기존 세이브를 그대로 쓸 수 있습니다. 코세이브 형식은 2.2.0과 같습니다.
- ESP에 레코드 7개(키워드 1, 마법 효과 3, 주문 2, 아트 오브젝트 1)가 추가됩니다. 모두 기존 레코드 뒤에 붙으므로 기존 레코드의 FormID는 바뀌지 않습니다.
- 그림자 타격 이펙트 메시(`Meshes\CalamityAffixes\ShadowEchoHit.nif`)가 함께 들어갑니다. 베데스다 메시를 고친 사본이라 게임 본편이 있어야만 쓸 수 있습니다.
- SE 1.5.97, AE 1.6.x, AE 1.7.x 모두 2.2.6과 같은 방식으로 동작합니다.

## 확인한 범위

- AE 1.6.1170 인게임에서 여러 차례 시험했습니다.
  - 그림자 권투 창이 13번 열렸습니다. 그림자 타격 47번이 모두 전용 보라 이펙트와 소리로 나왔고, 전격·화염 효과 사이에서 보라색으로 구분됐습니다.
  - 그림자 타격이 다른 적중 효과나 다른 그림자 타격을 다시 일으키지 않는 것을 로그로 확인했습니다.
- 난이도는 전문가와 전설에서 측정했습니다. 엔진이 그림자 타격에도 칼질과 같은 배율(이 모드팩 기준 전문가 0.8, 전설 0.4)을 이미 적용하므로, 모드에서는 따로 배율을 걸지 않습니다. 측정용 로그는 이번 릴리스에서 뺐습니다.
- 새 레코드가 기존 레코드 번호를 밀지 않는 것을 생성기 테스트로 고정했습니다.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.2.7_2026-10-08.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
