# Calamity - Reactive Loot & Affixes v2.2.7

v2.2.7은 **새 룬워드 쉐도우 복서(Shadow Boxer)**를 추가한 업데이트입니다. ESP에는 기존 레코드 뒤에 8개가 붙고, 코세이브 형식은 2.2.6과 같습니다.

*English summary: adds the Shadow Boxer runeword (Shael-Ko-Um, recommended base: any melee weapon), inspired by the Dungeon Fighter Online striker. Any melee hit starts an 8-second shadow stance (12-second cooldown); during it, every melee hit is repeated 0.4 s later by a shadow strike for 40% of that hit's physical damage. Shadow strikes are not new hits, so they never trigger other on-hit affixes or further shadow strikes. Each strike shows a translucent fist of shadow smoke that forms on the target and disperses, with dark purple wisps and a deep shadow-punch sound (a whoosh into a low, heavy impact, mixed from vanilla sounds). Difficulty applies to shadow strikes exactly as it does to your weapon. Includes the 2.2.6 trap marker crash fix. Existing saves work; install over 2.2.x with Replace.*

## 새 룬워드: 쉐도우 복서

- **룬:** 샤엘(Shael) · 코(Ko) · 움(Um)
- **추천 베이스:** 근접 무기. 한손·양손 무기와 맨손을 가리지 않고 모든 근접 공격에 적용됩니다.
- **발동:** 일반 공격이든 강공격이든 근접 공격이 적중하면 8초 동안 그림자 권투 상태가 됩니다. 재사용 대기시간은 12초라, 계속 싸우면 8초 켜지고 4초 쉬는 흐름이 됩니다.
- **그림자 타격:** 그림자 권투 상태에서 근접 공격이 적중할 때마다 0.4초 뒤 그림자가 같은 적을 한 번 더 때립니다. 피해는 그 적중 물리 피해의 40%입니다.
- **대상 아님:** 활·석궁, 주문, 방패 치기는 그림자가 따라 하지 않습니다.
- **연쇄 없음:** 그림자 타격의 피해는 새 적중으로 치지 않습니다. 그래서 다른 적중 발동 효과(전격 등)를 다시 일으키지 않고, 그림자 타격이 또 그림자 타격을 부르지도 않습니다.
- **난이도:** 그림자 타격에도 칼질과 똑같은 난이도 배율이 적용됩니다. 어느 난이도에서든 실제 칼질 피해의 40%가 됩니다.

### 연출

- 발동하면 캐릭터 주위에 보라빛 오라가 잠깐 터지고, 8초 동안 흑단 갑옷과 같은 검은 연기가 몸을 감쌉니다.
- 그림자 타격마다 적의 가슴 앞에 **연기로 된 반투명한 그림자 주먹**이 맺혔다가 흩어집니다(약 0.45초). 짙은 보라색 실타래 연기가 함께 피어오르고, 짧은 바람 소리에 이어 **낮고 묵직한 그림자 펀치 소리**가 울립니다.
- 주먹은 이 모드 전용 이펙트입니다. 스카이림의 연기 텍스처를 재료로 16프레임 애니메이션을 만들어서, 유령이나 그림자 마법처럼 반투명한 몸에 부드러운 보라빛 테두리가 비칩니다. 적의 몸에 가려지지 않도록 몸 위에 겹쳐 그립니다.
- 소리는 바닐라 큰 주먹 소리를 낮게 내리고 어두운 잔향을 더한 뒤, 거꾸로 감은 맨손 휘두르기 소리와 불굴의 외침 저음을 섞은 전용 사운드입니다(3종 중 무작위). 적 위치가 아니라 귀 앞에서 거리와 상관없이 들리게 재생합니다. 바닐라 주먹 소리는 칼 타격음과 음색이 비슷한 데다 적 발밑에서 재생돼 전투 중에 묻혔습니다.
- 실타래 연기는 흡수 마법의 적중 이펙트를 바탕으로 색을 짙은 보라로 바꾼 것입니다. 원래 색은 얼음빛 파랑이라 전격 효과 사이에서 구분되지 않았습니다.

## 2.2.6에서 이어지는 수정

- 2.2.6의 덫 표시물 튕김 수정(충돌 없는 전용 메시)이 그대로 들어 있습니다. 2.2.5 이하에서 올라오는 경우 함께 받습니다.

## 호환성

- 기존 세이브를 그대로 쓸 수 있습니다. 코세이브 형식은 2.2.0과 같습니다.
- ESP에 레코드 8개(키워드 1, 마법 효과 3, 주문 2, 아트 오브젝트 1, 사운드 1)가 추가됩니다. 모두 기존 레코드 뒤에 붙으므로 기존 레코드의 FormID는 바뀌지 않습니다.
- 그림자 타격 이펙트 메시(`Meshes\CalamityAffixes\ShadowEchoHit.nif`), 텍스처(`Textures\CalamityAffixes\ShadowFistSmoke.dds`), 사운드(`Sound\FX\CalamityAffixes\ShadowPunch01~03.wav`)가 함께 들어갑니다. 베데스다 메시를 고친 사본과 게임의 연기 텍스처·소리로 만든 파일이라, 게임 본편이 있어야만 쓸 수 있습니다.
- SE 1.5.97, AE 1.6.x, AE 1.7.x 모두 2.2.6과 같은 방식으로 동작합니다.

## 확인한 범위

- AE 1.6.1170 인게임에서 여러 차례 시험했습니다.
  - 그림자 권투 창이 13번 열렸을 때 놓친 발동이 없었습니다. 그림자 타격은 전격·화염 효과 사이에서도 보라색으로 구분됐습니다.
  - 최종 이펙트와 소리로 다시 시험했습니다. 그림자 타격 33번 모두 칼질 0.40~0.42초 뒤에 주먹 이펙트와 소리가 함께 나왔습니다. 특대검으로 한 대씩 끊어 쳐서 주먹이 타격마다 한 번만 맺히는 것과, 전투 중에도 소리가 들리는 것을 확인했습니다.
  - 0.4초 사이에 대상이 죽으면 그림자 타격은 나가지 않습니다.
  - 그림자 타격이 다른 적중 효과나 다른 그림자 타격을 다시 일으키지 않는 것을 로그로 확인했습니다.
- 난이도는 전문가와 전설에서 측정했습니다. 엔진이 그림자 타격에도 칼질과 같은 배율(이 모드팩 기준 전문가 0.8, 전설 0.4)을 이미 적용하므로, 모드에서는 따로 배율을 걸지 않습니다. 측정용 로그는 이번 릴리스에서 뺐습니다.
- 새 레코드가 기존 레코드 번호를 밀지 않는 것을 생성기 테스트로 고정했습니다.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.2.7_2026-10-09.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
