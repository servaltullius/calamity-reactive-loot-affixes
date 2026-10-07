# Calamity - Reactive Loot & Affixes v2.2.6

v2.2.6은 **함정 어픽스의 덫 표시물이 사라질 때 게임이 튕길 수 있던 문제**를 고친 핫픽스입니다. 덫 표시물 6종이 충돌 없는 전용 메시를 쓰도록 ESP의 메시 경로만 바뀌었고, 레코드 번호와 코세이브 형식은 2.2.5와 같습니다.

*English summary: fixes a crash when a trap affix's ground marker (most often the bear trap) expired while an actor, such as a follower, was touching it. The marker reused the vanilla bear trap mesh, whose jaws sit on the engine's trap collision layer, so the game tracked our marker as a live trap and crashed when it was removed (SkyrimSE.exe+3FF101 with TrapEntry/TrapTargetEntry in the registers). All six trap markers now use collision-free copies under Meshes\CalamityAffixes\TrapMarkers with the same look and animations. Trap damage is unchanged. No save changes; install over 2.2.x with Replace, no new game needed.*

## 고친 점

- **함정 어픽스(특히 곰덫)가 깔아 둔 덫이 사라지는 순간 게임이 튕길 수 있던 문제를 고쳤습니다.**

  덫 표시용으로 바닐라 곰덫 메시를 그대로 썼는데, 이 메시의 턱은 게임의 덫 충돌 레이어에 들어 있습니다. 그래서 게임이 우리 표시물을 진짜 덫처럼 추적했습니다. 동료처럼 지나가던 캐릭터가 턱에 닿아 있는 상태에서 표시물이 12초 수명을 다해 지워지면, 게임의 물리 처리가 지워진 덫을 읽다가 튕겼습니다. 크래시 로그에서는 `SkyrimSE.exe+3FF101`, 레지스터의 `TrapEntry`/`TrapTargetEntry`, 그리고 덫에 닿은 캐릭터로 보입니다. 크래시 로그가 그 캐릭터의 모드를 원인으로 지목하더라도 실제 원인은 이 덫 표시물입니다.

  이제 덫 표시물 6종(곰덫·룬·역병·타르·흡수·혼돈)은 충돌과 물리를 뺀 전용 메시(`Meshes\CalamityAffixes\TrapMarkers`)를 씁니다. 겉모습과 열리고 닫히는 애니메이션은 그대로입니다. 덫 피해 판정은 원래부터 모드가 따로 처리하므로 달라지지 않습니다. 달라지는 점은 표시물에 더는 부딪히거나 걸리지 않는다는 것뿐입니다.

## 호환성

- 기존 세이브를 그대로 쓸 수 있습니다. 코세이브 형식은 2.2.0과 같고, 덫 표시물 레코드의 FormID도 그대로입니다.
- SE 1.5.97, AE 1.6.x, AE 1.7.x 모두 2.2.5와 같은 방식으로 동작합니다.
- 이 ZIP에는 처음으로 `Meshes` 폴더가 들어갑니다. 베데스다 메시를 고친 사본이라 게임 본편이 있어야만 쓸 수 있습니다.

## 확인한 범위

- 변환한 메시 6개를 별도 NIF 라이브러리(Nifly)로 다시 읽어 확인했습니다. 블록 구조와 노드·형상·애니메이션 참조는 원본과 같고, 충돌 블록을 가리키는 곳은 하나도 남지 않았습니다.
- 출고 메시에 충돌 블록이 다시 섞이면 실패하는 테스트를 추가했습니다. 바닐라 메시를 넣으면 실제로 실패하는 것도 확인했습니다.
- AE 1.6.1170 인게임에서 곰덫 9개가 열림·닫힘·재장전 애니메이션을 모두 정상 재생하고 12초 뒤 정상 정리되는 것, 튕김이 없는 것을 확인했습니다.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.2.6_2026-10-07.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
