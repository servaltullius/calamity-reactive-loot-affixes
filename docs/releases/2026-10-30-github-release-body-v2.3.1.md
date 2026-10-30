# Calamity - Reactive Loot & Affixes v2.3.1

v2.3.1은 [2.3.0](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.3.0) 출시 뒤 코드 검수와 전체 코드리뷰에서 나온 문제를 고친 핫픽스입니다. ESP와 코세이브 형식은 2.3.0과 같고, DLL과 패널 문구만 바뀝니다. 2.3.0을 쓰고 있다면 바로 교체하세요.

*English summary: a hotfix for 2.3.0. If part of an affix could not be read at load (for example an older CalamityAffixes.esp that lacks a spell, or a wrongly typed value in affixes.json), that affix was treated as retired and removed from saved gear for good; it now stays on the item, and crafting on that item is paused with a panel message until the cause is fixed. Low-health runewords measured health against base maximum and ignored Fortify Health, so they fired much later than their text says; Archmage likewise ignored Fortify Magicka. Shared-status records (Exposed, meters, Doom) kept running while a menu paused the game. The old-save stat cleanup could take off a matching bonus another mod or the console had added. Also fixed: a missed low-health check right after a load, the panel staying on screen after a death reload, and two runeword stand-in spells that pointed at records the plugin no longer has. ESP and co-save are unchanged; install over 2.3.0 with Replace.*

## 장비 어픽스 보존

- **설정에 있는 어픽스를 읽지 못하면, 그 어픽스가 저장된 장비에서 지워지던 문제를 고쳤습니다.** 예를 들어 다른 버전의 `CalamityAffixes.esp`가 로드돼 주문을 찾지 못하거나 `affixes.json` 값의 형식이 틀리면, 그 어픽스가 폐기된 것으로 취급돼 장비에서 영구히 빠졌습니다.
- 이제 설정에 있는 어픽스는 읽지 못해도 장비에 그대로 남고, 원인이 풀리면 다시 동작합니다.
- 그런 어픽스가 남아 있는 장비는 원인이 풀릴 때까지 확인·재련·확장·정제·룬워드 변환·룬워드 제거를 모두 막습니다. 패널에는 "선택한 장비의 어픽스 하나를 이번 세션에 불러오지 못했습니다. CalamityAffixes.esp가 이 버전과 맞는지 확인하세요."가 나옵니다. 제작이 그 어픽스의 칸을 잘못 짐작해 지우거나 룬워드를 둘 겹치지 않게 하기 위해서입니다.

## 전투

- **저체력 룬워드가 체력 강화를 빼고 체력 비율을 계산하던 문제를 고쳤습니다.** 사자의 심장·마지막 소원·추방 등이 기본 최대 체력만 기준으로 써서, 마법부여·물약·능력으로 최대 체력을 올리면 설명보다 훨씬 늦게 발동했습니다. 예를 들어 기본 300에 강화 150이면 "40% 이하"가 120 HP에서야 발동했습니다. 이제 체력 바와 같은 최대 체력을 씁니다. 체력 강화를 쓰는 분은 전보다 일찍 발동합니다.
- 출혈 축적이 터질 때의 '최대 체력 %' 피해도 같은 기준으로 맞췄습니다.
- **대마법사의 '최대 매지카 %' 비용과 피해가 매지카 강화를 빼고 계산되던 문제를 고쳤습니다.** 매지카 강화가 있으면 피해와 소모 매지카가 함께 늘어납니다.
- **게임을 멈추는 메뉴를 열어 둔 동안에도 공용 상태 기록의 시간이 흐르던 문제를 고쳤습니다.** 인벤토리·지도 등을 열어 두었다 닫으면 노출 기록이 효과보다 먼저 끝나 약한 노출이 겹쳐 걸리거나, 빙결·출혈·감전 미터가 줄어 있었습니다. 이제 상태 기록도 마법 효과처럼 메뉴에서 멈춥니다.
- 세이브를 불러오는 순간 받은 피해의 저체력 검사가 한 번 빠질 수 있던 문제를 고쳤습니다.

## 그 밖의 수정

- **옛 세이브 능력치 정리**가 버프 도중 저장한 그라운드·제피르의 남은 보정을 찾을 때, 다른 모드나 콘솔이 넣은 같은 능력치 보정까지 깎을 수 있던 문제를 고쳤습니다. 이제 마법 효과가 남기는 칸에 있고 지금 걸린 효과로 설명되지 않는 양만 정리합니다.
- 패널을 연 채 죽어 자동으로 다시 불러오면 패널이 화면에 남아 있을 수 있던 문제를 고쳤습니다.
- 룬워드를 불러오지 못했을 때 쓰는 대체 효과 중 보호막(엑자일·브램블·메타모포시스)과 회복(피닉스·왕의 은총·레인·라이언하트) 계열이 ESP에 없는 주문을 찾던 문제를 고쳤습니다. 코드가 찾는 칼라미티 레코드 이름이 모두 ESP에 있는지 검사하는 테스트도 더했습니다.

## 호환성

- 기존 세이브를 그대로 쓸 수 있고 새 게임은 필요 없습니다. ESP와 코세이브 형식은 2.3.0과 같습니다.
- SE 1.5.97, AE 1.6.x, AE 1.7.x를 2.3.0과 같은 방식으로 지원합니다.

## 확인한 범위

- 핫픽스 테스트 빌드를 AE 1.6.1170 인게임에서 써 보았고, 불러오기·장착·저장이 정상이었으며 이상은 보고되지 않았습니다.
- 상태 시계, 최대 체력·매지카 계산, 옛 세이브 정리 판정은 컴파일 시점 테스트로 확인했습니다. 실제 게임 객체가 필요한 부분(설정 일부 실패 시 보존, 제작 차단, 로드 직후 저체력 검사, 패널 닫기)은 코드 구조 검사로 고정했습니다.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.3.1_2026-10-30.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요. DLL만 바꾸지 마세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
