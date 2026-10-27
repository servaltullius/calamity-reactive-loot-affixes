# Calamity - Reactive Loot & Affixes v2.3.0

v2.3.0은 **룬워드가 접두 칸을 차지하도록** 바꾼 제작 개편과, 아이템끼리 맞물리는 **공용 상태**를 더한 효과 개편입니다. 패널도 게임 UI에 어울리는 색과 배치로 바꿨습니다. [rc1](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.3.0-rc1)·[rc2](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.3.0-rc2)·[rc3](https://github.com/servaltullius/calamity-reactive-loot-affixes/releases/tag/v2.3.0-rc3)에서 공개 테스트한 내용에, 겹치던 스킬 버프 룬워드 재설계와 전체 점검에서 나온 수정을 더했습니다. 이 노트는 2.2.8에서 올라오는 기준으로 씁니다.

*English summary: a runeword now takes an item's prefix slot, so it is a choice against a prefix instead of a free extra; items made before 2.3.0 keep their old layout. Shared statuses let affixes on different items work together: Freeze, Bleed and Shock fill a per-enemy meter that pays off at 100, Burning stacks, Exposed keeps only the strongest cut, Doom bursts after a mark. Three new prefixes use them. Every weapon attack-speed effect, which MCO-style combat ignores, now does something else; Eagle Eye and Zephyr now really raise bow draw speed. Only one Crit Cast spell fires per attack, and a new self-buff keeps the stronger of two of the same kind. Ten "+skill for a few seconds" runewords now apply or exploit statuses and keep a smaller passive bonus. The panel is restyled to match Skyrim's menus and opens with one short frame instead of several. A full review fixed over a hundred smaller issues, including Ground making shout cooldowns longer, Archmage never firing on the player's own spells, a crash on items with non-UTF-8 names, a 2-4 s stall on the first inventory open, and tooltips cut off at the bottom. The MCM follows the panel language. Existing saves work; install over 2.2.x with Replace.*

## 룬워드가 접두 칸을 차지합니다

- 장비의 어픽스 칸 3개 중 **첫 칸에는 룬워드나 접두 중 하나**만 들어가고, 나머지 두 칸은 접미입니다. 지금까지는 룬워드가 접두 위에 덤으로 얹혀 늘 이득이었습니다. 이제는 공격 접두를 노릴지, 상시 보너스까지 붙은 룬워드를 확정으로 만들지 고르는 선택입니다.
- 접두가 있는 장비에 룬워드를 입히면 그 접두가 사라집니다. 패널이 무엇이 사라지는지 알려 주고, 6초 안에 한 번 더 눌러야 진행합니다. 접미는 남습니다.
- 룬워드 장비를 정제하면 룬워드는 두고 접미만 다시 굴립니다. 일반 어픽스가 없는 룬워드 장비에 확인 스크롤을 쓰면 접미 1개(75%)나 2개(25%)가 붙습니다. 슬롯 확장은 룬워드를 한 칸으로 셉니다.
- **룬워드 제거:** 정제 오브 1개로 룬워드를 지우고 빈 칸에 새 접두를 굴립니다. 접미와 남은 선택 재련 횟수는 유지되고, 쓴 룬 조각은 돌려받지 못합니다. 2.3.0 전 장비에서 지우면 룬워드만 사라지고 있던 접두·접미가 남습니다.
- 툴팁의 룬워드 줄은 `[R]`로 표시합니다.
- **2.3.0 전에 만든 장비는 그대로입니다.** 룬워드와 접두를 함께 가진 장비는 전투·정제·재련·확장 모두 예전 규칙대로 동작합니다. 다른 룬워드로 바꿀 때 새 규칙으로 넘어가며, 이때도 확인창이 사라질 접두를 알려 줍니다.

## 공용 상태

상태를 거는 효과와 상태를 이용하는 효과가 **서로 다른 장비에서도** 맞물립니다. 상태를 거는 어픽스는 설명 끝에 상태를 적습니다(예: `빙결 축적 +20.`).

| 상태 | 규칙 |
|---|---|
| 빙결 축적 | 미터가 100이 되면 3초 동안 이동속도 -80%, 공격력 -30% |
| 출혈 축적 | 미터가 100이 되면 최대 체력 8% 피해(강한 적 3%, 10~400) |
| 감전 축적 | 미터가 100이 되면 대상과 주변 2명에게 번개 40 |
| 화상 | 1중첩당 초당 화염 피해 4, 최대 5중첩, 4초 |
| 노출 | 저항·방어도 감소. 같은 능력치에는 가장 강한 칼라미티 노출 하나만 적용 |
| 파멸 | 표식 1.5초 뒤 그 적중 피해의 일부가 폭발 |

- 보스·드래곤·고유 적은 축적 문턱이 150에서 시작해 터질 때마다 50씩 오릅니다(최대 300).
- 미터는 3초 동안 쌓이지 않으면 초당 15씩 줄어듭니다. 동료와 중립 NPC에게는 쌓이지 않고, 상태는 세이브에 저장하지 않습니다.
- **새 접두 3종:** 파멸의 낙인(25% 확률로 파멸, 적중 피해의 60%), 약점 포착(상태가 2가지 이상인 적에게 적중 피해의 15% 추가), 전염(상태가 걸린 적을 처치하면 주변 2명에게 옮김).

## 바뀐 효과

- **무기 공격속도 효과를 모두 바꿨습니다.** MCO 같은 전투 모드에서는 무기 공격속도 증감이 공격 동작에 반영되지 않아 사실상 아무 일도 하지 않았습니다. 대부분 공격력으로 바뀌었고(검투사 접미 +3/6/9%, 슬픔·허슬 +6%, 신념 등), ID는 그대로라 가진 장비에도 바로 적용됩니다. 전체 표는 [CHANGELOG](https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/CHANGELOG.md)에 있습니다.
- **활 속도가 실제로 오릅니다.** 독수리눈 접미와 룬워드 제피르가 쓰던 값은 확대 조준 시간 감속이었습니다. 이제 활·석궁을 들고 있을 때 당기기 속도가 5/10/15%(제피르 30%) 오릅니다.
- **스킬 버프 룬워드 10개를 다시 만들었습니다.** "확률로 스킬 +20~30"으로 모두 같은 모양이던 리프·스트렝스·엣지·오쓰·패션·언벤딩 윌·큐어·하스·인라이트먼트·미스가 이제 상태를 걸거나 이용하고, 작은 상시 보너스가 남습니다. 퓨리는 출혈 일격이 됐습니다.
- **같은 종류의 자기 버프는 더 강한 쪽 하나만 남습니다.** 같은 능력치를 올리는 칼라미티 발동 버프가 겹치면 능력치마다 세기를 비교합니다. 모든 면에서 지는 버프는 발동하지 않고 확률·재사용 대기도 쓰지 않습니다.
- **치명 시전은 한 번의 공격에 주문 하나만** 시전합니다. 여러 개를 끼면 차례로 돌아갑니다. 치명 시전·대마법사 접두는 덜 나옵니다(가중치 합 7.0→2.0, 3.7→1.0).
- **노출이 이미 더 강하게 걸려 있으면** 새 노출은 확률과 재사용 대기를 쓰지 않습니다.

## 패널

- 색을 인벤토리·MCM 테마에 맞춘 갈색 연기 바탕·뼈색 글자·바랜 금빛 강조로 바꿨습니다.
- 아이템 어픽스 탭은 왼쪽에 작업하는 장비의 칸과 작업 버튼, 오른쪽에 제작 재료와 장착 빌드를 둡니다. 룬워드 탭은 레시피 목록이 더 길게 보입니다.
- 패널을 열 때 60ms 안팎의 끊김이 3~4프레임 이어지던 것이 한 프레임(측정 약 51ms)으로 줄었습니다.

## 주요 수정

- 룬워드 그라운드가 용언 대기시간을 오히려 크게 늘리던 것을 고쳤습니다. 이제 탈로스의 축복처럼 50% 빠르게 합니다.
- 대마법사가 플레이어가 직접 쓴 주문(주문서 포함)에도 발동합니다. 일반 주문 적중에서는 전혀 발동하지 않았습니다.
- 사자의 심장·마지막 소원·추방이 브레스·주문·DoT·낙하 피해로 체력이 떨어져도 발동합니다.
- 이미 죽은 적의 시체를 때려 적중 효과와 재사용 대기를 써 버리던 것을 고쳤습니다.
- 아이템·주문 이름에 UTF-8이 아닌 문자(예: 'Mjölnir')가 있는 장비로 패널을 열면 게임이 튕기던 문제를 고쳤습니다.
- 세이브를 불러온 뒤 장비창을 처음 열 때 2~4초 멈추던 것을 없앴습니다. 칼라미티 화면을 불러오는 중에 미리 만듭니다.
- 어픽스가 많은 아이템 툴팁의 아래쪽이 잘리거나 깜박이던 문제를 고쳤습니다. 넘치면 글자를 조금 줄여 다 보여 줍니다.
- **MCM이 패널 언어 설정(영어/한국어/둘 다)을 따릅니다.** 이중 표기로 길어진 설명의 한국어 뒷부분이 잘리던 문제입니다.
- 나디르·피스·딜리리엄·로브링어는 레벨 60 이하에게만 통하고, 바닐라 환혹 주문처럼 언데드·데이드라·드래곤·드워프 기계에게는 걸리지 않습니다.
- 멜로디·데스·글룸의 흡수, 통찰의 마나 재생(약 9배로 들어가던 것), 부여술사 접미, 파괴·프라이드의 범위(약 107m·76m → 5m·3.5m)를 설명대로 고쳤습니다.
- ESP가 빠지거나 이름이 바뀐 상태로 불러와도, 또는 동료가 주운 장비라도 어픽스가 지워지지 않습니다.

그 밖의 수정은 [CHANGELOG](https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/CHANGELOG.md)에 있습니다.

## 호환성

- **기존 세이브를 그대로 쓸 수 있고 새 게임은 필요 없습니다.** 코세이브 형식은 2.2.8과 같습니다.
- ESP에는 레코드 53개를 기존 목록 끝에만 추가했습니다(총 846개). 기존 FormID는 그대로입니다.
- **옛 세이브의 능력치 정리:** 능력치를 바꾼 효과(검투사·슬픔·신념·허슬·트래셔리의 무기 속도, 독수리눈, 부여술사, 통찰, 그리고 버프 도중 저장한 그라운드·제피르·퓨리·신념·잭팟과 재설계 전 스킬 버프 룬워드)는 예전 세이브에 옛 보정이 남아 있을 수 있습니다. 2.3.0으로 처음 불러올 때 한 번만, 예전 버전이 넣은 양만큼 걷어 내고 로그에 `stale stat migration` 줄을 남깁니다. 알아볼 수 없는 경우(그 장비를 rc에서 다시 끼었거나 이미 콘솔로 고친 경우 등)는 건드리지 않으며, 남은 보정은 콘솔로 지울 수 있습니다(예: `player.modav WeaponSpeedMult -0.1`).
- 새 규칙으로 만든 룬워드 장비(접두 없음)를 2.2.x DLL로 불러오면 비정상 구성으로 보여 정제만 할 수 있습니다. 2.3.0에서 되돌리거나 모드를 지울 때는 README의 ["제거하거나 이전 버전으로 되돌릴 때"](https://github.com/servaltullius/calamity-reactive-loot-affixes#%EC%A0%9C%EA%B1%B0%ED%95%98%EA%B1%B0%EB%82%98-%EC%9D%B4%EC%A0%84-%EB%B2%84%EC%A0%84%EC%9C%BC%EB%A1%9C-%EB%90%98%EB%8F%8C%EB%A6%B4-%EB%95%8C) 순서를 따라 주세요.
- SE 1.5.97, AE 1.6.x, AE 1.7.x를 2.2.8과 같은 방식으로 지원합니다.

## 확인한 범위

- AE 1.6.1170 인게임(MCO 환경)에서 rc1~rc3의 제작·패널·상태 효과와, 정식 전 수정(버프 대체·노출, 재설계 룬워드 7개, 그라운드·제피르·대마법사·사자의 심장, 시체 타격 무시, MCM 언어 전환, 툴팁, 첫 장비창·패널 끊김, 피스·화염 전환·투명화 종료)을 확인했습니다.
- 옛 세이브 능력치 정리는 정리할 것이 없는 세이브에서만 게임으로 확인했고, 실제로 걷어 내는 경로는 단위 테스트로 확인했습니다. 2.2.8 세이브에서 능력치가 이상하면 로그의 `stale stat migration` 줄과 함께 알려 주세요.
- Python·생성기·SKSE 검사와 배포 패키지 검증을 통과했습니다.

## 설치와 업데이트

1. **`CalamityAffixes_MO2_v2.3.0_2026-10-27.zip`** 전체를 설치하고, MO2에서 기존 Calamity 모드를 **Replace**하세요. DLL만 바꾸지 마세요.
2. 새 게임은 필요하지 않습니다.

Prisma UI는 이 ZIP에 포함되지 않습니다. 공개 자산은 **MO2 ZIP, ZIP SHA256, DLL, ESP** 네 개입니다.
