# v2.1.1 넥서스 페이지 문구·미디어

새 넥서스 페이지에 그대로 붙여 넣는 설명문, 이미지 순서와 캡션, 고정 댓글이다. 페이지 개설·옛 페이지 은퇴 절차는 [2026-09-26-nexus-v2.0.0-launch.md](2026-09-26-nexus-v2.0.0-launch.md)를 따른다.

가장 많이 헷갈리는 **패널 단축키**를 설명문 맨 앞과 고정 댓글에 둔다. 사실관계는 코드 기준이다.

- 기본 키는 `F11`이다. MCM에 키를 지정하지 않았으면 이 키로 열린다.
- 월드와 인벤토리 어디서든 열린다. 메인 메뉴·로딩 화면·캐릭터 생성 중에는 열리지 않는다.
- 게임패드 버튼으로는 열 수 없다. 키보드 키를 써야 한다.
- MCM `Calamity Affixes › Hotkey & Debug › Toggle Calamity Panel`에서 바꾼다. MCM(저널 메뉴)을 닫는 순간 새 키가 적용된다.
- 키를 바꾸려면 SkyUI와 MCM Helper가 필요하다. 둘이 없으면 F11로 열린다.
- Calamity 아이템 툴팁 아래에 현재 키가 `Open/Close Panel: <키>`(한국어 UI는 `패널 열기/닫기: <키>`)로 표시된다.
- 단축키가 먹지 않으면 MCM `Prisma UI › Toggle Panel (Button)`으로 바로 열 수 있고, 같은 페이지의 `Check Prisma UI Status`로 Prisma UI 상태를 확인한다.

## 1. 미디어

파일 위치: `G:\TAKEALOOK\downloads\Calamity-2.1.1-nexus-media\` (2.0 대표 이미지는 `Calamity-2.0-nexus-images\`)

| 순서 | 파일 | 캡션(영어) | 캡션(한국어) |
| --- | --- | --- | --- |
| 1 (대표) | `Calamity-2.0-nexus-images\00-hero-en.png` | Calamity 2 - Reactive Loot & Affixes | 칼래미티 2 |
| 2 | `00-hotkey-guide.png` | Opening the panel: F11 by default, change it in MCM | 패널 여는 법: 기본 F11, MCM에서 변경 |
| 3 | `10-ingame-combat.jpg` | Affixes fire spells, traps, and summons in combat | 어픽스가 전투 중 주문·함정·소환을 발동 |
| 4 | `11-ingame-crafting.png` | Crafting panel: currencies, slots, and the affix to reforge | 제작 패널: 재화·슬롯·재련할 어픽스 |
| 5 | `12-ingame-runeword.png` | Runeword workbench with 94 recipes | 룬워드 작업대(레시피 94종) |
| 6 | `13-ingame-tooltip.png` | Affixes and runewords in the item tooltip | 아이템 툴팁의 어픽스·룬워드 |
| 7~ | `Calamity-2.0-nexus-images\01~04-*-en.png` | (2.0 패널 캡처) | |

GIF 8개(`gif-01`~`gif-08`, 각 2.7~6.2MB)는 이미지 갤러리보다 **설명문 안**에 넣는 편이 잘 보인다. 넥서스 이미지 탭에 올린 GIF가 움직이지 않으면 imgur 같은 외부 호스팅에 올리고 그 주소를 아래 `[img]`에 넣는다. 모든 GIF는 아래쪽 자막 띠에 영어·한국어 설명이 들어 있어 화면을 가리지 않는다.

| 파일 | 내용 |
| --- | --- |
| `gif-01-open-panel.gif` | 월드에서 키를 눌러 패널이 열리는 장면 |
| `gif-02-loot.gif` | 시체에서 확인 스크롤·재련 오브 획득 |
| `gif-03-identify.gif` | 확인 스크롤로 어픽스 2개 부여 |
| `gif-04-reforge.gif` | 선택 재련: Bear Trap → Voice of Power, 오브 4→2 |
| `gif-05-expand.gif` | 슬롯 확장: 검투사 접미 추가 |
| `gif-06-scour.gif` | 정제: 두 번 눌러 전체 리롤 |
| `gif-07-exchange.gif` | 스크롤 3개 → 재련 오브 1개 교환 |
| `gif-08-runeword.gif` | 룬워드 카오스 선택 → 변환 |

설명문의 `[img]` 자리에는 이미지 탭에 올린 뒤 이미지를 열어 복사한 주소를 넣는다.

효과 목록은 설명문에 붙이지 않고 GitHub 링크로 건다. 영어는 `docs/EFFECTS_EN.md`(게임 내 영문 문구, `tools/update_docs.py`가 릴리스마다 다시 만든다), 한국어는 기존 접두·접미·룬워드 문서다.

## 2. 설명문 (BBCode)

넥서스 설명 편집기를 BBCode 모드로 바꾸고 아래 두 블록을 차례로 붙여 넣는다. 영어를 먼저 두고 한국어를 뒤에 둔다.

```bbcode
[center][img]HERO_IMAGE_URL[/img]

[size=5][b]Calamity 2 - Reactive Loot & Affixes[/b][/size]
Diablo 2 / Path of Exile-style affixes and 94 runewords for the gear you already own.
[i]This page continues Calamity from 2.0. Saves from 1.x (the old page) carry over.[/i][/center]

[size=4][b]Getting started: open the panel[/b][/size]
[img]HOTKEY_GUIDE_URL[/img]
[list]
[*][b]Default key: F11.[/b] It works in the world and in your inventory. Press it again, press ESC, or click Close to close the panel.
[*][b]Change the key:[/b] ESC > System > Mod Configuration > Calamity Affixes > Hotkey & Debug > Toggle Calamity Panel, then press a new key. The new key works as soon as you close the menu.
[*]Changing the key needs SkyUI and MCM Helper. Without them the panel stays on F11.
[*]Your current key is shown at the bottom of the Calamity item tooltip ("Open/Close Panel: ...").
[*]Gamepad buttons cannot open the panel. Use a keyboard key.
[*][b]Nothing happens?[/b] Another mod may already use F11, so pick a different key. You can also open the panel from MCM > Prisma UI > Toggle Panel (Button), and check MCM > Prisma UI > Check Prisma UI Status. The panel and tooltips need Prisma UI.
[/list]

[size=4][b]What it does[/b][/size]
Items never roll affixes on their own. You pick an equipped item and build it up with currencies that drop only from enemies you kill.
[list]
[*]73 prefixes, 66 suffixes, and 94 runewords made from 33 runes
[*]Effects fire on hit, when you are hit, on kill, or at low health: spells, traps, elemental conversion, summons, corpse explosions, and more
[*]Each item holds 1 runeword and up to 3 regular affixes (1 prefix + 2 suffixes)
[*]Item tooltips show every affix; the panel shows your whole build
[*]English and Korean UI
[/list]

[size=4][b]Crafting[/b][/size]
[list]
[*][b]Identify Scroll:[/b] gives an item with no affixes 1-3 affixes (60% / 30% / 10%).
[*][b]Reforge (2 Reforge Orbs):[/b] rerolls one affix you choose. The others stay. No attempt limit.
[*][b]Unlock slots (2, then 4 Reforge Orbs):[/b] adds a suffix and keeps what you have.
[*][b]Scour (1 Scouring Orb, rare):[/b] rerolls every regular affix at once and keeps the slot count. Click twice within 6 seconds to confirm.
[*][b]Trade:[/b] spare Identify Scrolls become Reforge Orbs (3:1) or Scouring Orbs (15:1). One way only.
[*][b]Runewords:[/b] collect rune fragments and transmute one of 94 runewords onto the item. Identify, reforge, and scour never remove a finished runeword.
[/list]
[img]GIF_REFORGE_URL[/img]
[img]GIF_SCOUR_URL[/img]
[img]GIF_EXCHANGE_URL[/img]
[img]GIF_RUNEWORD_URL[/img]

[size=4][b]Where currencies come from[/b][/size]
Only the corpses of hostile enemies killed by you or your summons. Containers, the world, and followers' kills never drop them.
[list]
[*]Default chances per corpse: rune fragment 8%, Reforge Orb 12%, Identify Scroll 20%, Scouring Orb 2% (all adjustable in MCM)
[*]Unique and named enemies always drop a rune fragment or a Reforge Orb; bosses and dragons drop one of each
[*]Pity: a rune fragment is guaranteed after 99 misses, a Reforge Orb after 39. The panel shows both counters.
[/list]
[img]GIF_LOOT_URL[/img]

[size=4][b]Requirements[/b][/size]
[list]
[*]SKSE64
[*]Address Library for SKSE Plugins
[*]Prisma UI (tooltips and the panel)
[*]Recommended: SkyUI and MCM Helper (settings and the panel key), KID
[/list]

[size=4][b]Install and update[/b][/size]
[list=1]
[*]Install the full ZIP with MO2 or Vortex and enable CalamityAffixes.esp.
[*]When updating, replace the old version. Do not swap only the DLL.
[*]Coming from 1.x (the old page): disable or remove that mod so only one Calamity is active. Your gear, currencies, and runewords carry over, and no new game is needed.
[/list]

[size=4][b]Scope[/b][/size]
Calamity works on your gear and on kills by you and your summons. It is not an NPC or follower gear overhaul.

[size=4][b]FAQ[/b][/size]
[b]The panel does not open.[/b] See "Getting started" above. The usual causes are another mod using F11 or a missing Prisma UI.
[b]No tooltips.[/b] Check that Prisma UI is installed and enabled. The log is at Documents/My Games/Skyrim Special Edition/SKSE/CalamityAffixes.log.
[b]Do I need a new game?[/b] No.

[b]Full effect list:[/b] every prefix, suffix, and runeword with its in-game text - [url=https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/EFFECTS_EN.md]Effect List[/url]
Source and full changelog: [url=https://github.com/servaltullius/calamity-reactive-loot-affixes]GitHub[/url]
```

```bbcode
[size=5][b]한국어[/b][/size]
[i]이 페이지는 Calamity 2.0 이후 버전입니다. 1.x(구 페이지)의 세이브를 그대로 이어서 쓸 수 있습니다.[/i]

[size=4][b]시작하기: 패널 열기[/b][/size]
[list]
[*][b]기본 키는 F11입니다.[/b] 월드와 인벤토리 어디서든 열립니다. 다시 누르거나 ESC, 닫기 버튼으로 닫습니다.
[*][b]키 바꾸기:[/b] ESC > 시스템 > 모드 설정 > Calamity Affixes > 단축키·디버그 > 칼래미티 패널 토글에서 새 키를 누릅니다. 메뉴를 닫는 순간 적용됩니다.
[*]키를 바꾸려면 SkyUI와 MCM Helper가 필요합니다. 없으면 F11로 열립니다.
[*]Calamity 아이템 툴팁 아래에 현재 키가 표시됩니다("패널 열기/닫기: ...").
[*]게임패드 버튼으로는 열 수 없습니다. 키보드 키를 쓰세요.
[*][b]반응이 없다면:[/b] 다른 모드가 F11을 쓰고 있을 수 있으니 다른 키로 바꾸세요. MCM > 프리즈마 UI > 패널 토글(버튼)으로도 열 수 있고, 같은 페이지의 Prisma UI 상태 확인으로 설치 상태를 볼 수 있습니다. 패널과 툴팁은 Prisma UI가 있어야 동작합니다.
[/list]

[size=4][b]어떤 모드인가요[/b][/size]
디아블로 2·패스 오브 엑자일식 어픽스와 룬워드 94종을 내가 가진 장비에 붙이는 SKSE 모드입니다. 아이템에 어픽스가 저절로 붙지 않습니다. 착용한 장비를 골라, 처치한 적의 시체에서만 나오는 재화로 직접 키웁니다.
[list]
[*]접두 73개, 접미 66개, 룬 33종으로 만드는 룬워드 94개
[*]적중·피격·처치·저체력 때 주문, 함정, 원소 전환, 소환, 시체 폭발 등이 발동
[*]장비마다 룬워드 1개 + 일반 어픽스 최대 3개(접두 1 + 접미 2)
[*]툴팁에서 어픽스를, 패널에서 전체 빌드를 확인
[*]영어·한국어 UI
[/list]

[size=4][b]제작[/b][/size]
[list]
[*][b]확인 스크롤:[/b] 어픽스가 없는 장비에 1~3개 부여(60% / 30% / 10%)
[*][b]선택 재련(재련 오브 2개):[/b] 고른 어픽스 하나만 교체, 나머지는 유지, 횟수 제한 없음
[*][b]슬롯 확장(재련 오브 2개, 다음은 4개):[/b] 기존 어픽스를 유지하고 접미 추가
[*][b]정제(정제 오브 1개, 희귀):[/b] 슬롯 수를 유지한 채 일반 어픽스 전부를 다시 굴림. 6초 안에 두 번 눌러 확정
[*][b]교환:[/b] 남는 확인 스크롤을 재련 오브(3:1)나 정제 오브(15:1)로. 한 방향만 가능
[*][b]룬워드:[/b] 룬 조각을 모아 94종 중 하나를 변환. 확인·재련·정제는 완성된 룬워드를 지우지 않음
[/list]

[size=4][b]재화 획득[/b][/size]
나나 내 소환수가 처치한 적대 대상의 시체에서만 나옵니다. 상자, 월드, 팔로워의 처치로는 나오지 않습니다.
[list]
[*]시체당 기본 확률: 룬 조각 8%, 재련 오브 12%, 확인 스크롤 20%, 정제 오브 2%(모두 MCM에서 조정)
[*]고유·네임드 적은 룬 조각이나 재련 오브 중 하나를 확정 지급, 보스와 드래곤은 둘 다 1개씩 확정 지급
[*]천장: 룬 조각은 99회, 재련 오브는 39회 연속 실패 뒤 확정. 패널에서 두 카운터를 확인할 수 있습니다
[/list]

[size=4][b]필수·권장[/b][/size]
[list]
[*]필수: SKSE64, Address Library for SKSE Plugins, Prisma UI
[*]권장: SkyUI·MCM Helper(설정과 패널 키 변경), KID
[/list]

[size=4][b]설치와 업데이트[/b][/size]
[list=1]
[*]전체 ZIP을 MO2나 Vortex로 설치하고 CalamityAffixes.esp를 켭니다.
[*]업데이트할 때는 이전 버전을 교체합니다. DLL만 바꾸지 마세요.
[*]1.x(구 페이지)에서 올라오면 구 모드를 끄거나 지워 Calamity가 하나만 켜지게 하세요. 장비·재화·룬워드는 그대로 이어지고 새 게임은 필요 없습니다.
[/list]

[size=4][b]자주 묻는 질문[/b][/size]
[b]패널이 안 열려요.[/b] 위 "시작하기"를 보세요. 대부분 다른 모드가 F11을 쓰고 있거나 Prisma UI가 없는 경우입니다.
[b]툴팁이 안 보여요.[/b] Prisma UI가 설치·활성화돼 있는지 확인하세요. 로그: 문서/My Games/Skyrim Special Edition/SKSE/CalamityAffixes.log
[b]새 게임이 필요한가요?[/b] 아니요.

[b]전체 효과 목록:[/b] [url=https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/PREFIX_EFFECTS.md]접두[/url] · [url=https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/SUFFIX_EFFECTS.md]접미[/url] · [url=https://github.com/servaltullius/calamity-reactive-loot-affixes/blob/main/docs/RUNEWORD_EFFECTS.md]룬워드[/url]
```

## 3. 고정 댓글 (Sticky)

```bbcode
[b]Can't open the panel? / 패널이 안 열리나요?[/b]
[list]
[*]Default key is [b]F11[/b]. Change it: ESC > System > Mod Configuration > Calamity Affixes > Hotkey & Debug > Toggle Calamity Panel. It works once you close the menu (needs SkyUI + MCM Helper).
[*]The current key is shown at the bottom of the Calamity item tooltip. Gamepad buttons can't open it.
[*]Still nothing: MCM > Prisma UI > Toggle Panel (Button), then Check Prisma UI Status. Prisma UI is required.
[/list]
[list]
[*]기본 키는 [b]F11[/b]입니다. 변경: ESC > 시스템 > 모드 설정 > Calamity Affixes > 단축키·디버그 > 칼래미티 패널 토글. 메뉴를 닫으면 적용됩니다(SkyUI + MCM Helper 필요).
[*]Calamity 아이템 툴팁 아래에 현재 키가 표시됩니다. 게임패드 버튼으로는 열 수 없습니다.
[*]그래도 안 되면 MCM > 프리즈마 UI > 패널 토글(버튼), 이어서 Prisma UI 상태 확인. Prisma UI는 필수입니다.
[/list]
```

## 4. 파일 설명과 변경 이력(2.1.1)

- 메인 파일 이름·설명과 전체 변경 이력은 [launch 키트](2026-09-26-nexus-v2.0.0-launch.md) 1절을 그대로 쓴다(2.1.1로 갱신됨).
- 2.1.1 변경 한 줄: `2.1.1: Fixed the Selected Base Affixes box hiding every affix after a long first one. / 선택 베이스 어픽스 상자에서 첫 어픽스가 길면 나머지가 가려지던 문제 수정`
- 2.1.2 변경 한 줄: `2.1.2: Every runeword now shows one full name in tooltips, the panel, and active effects (e.g. "Runeword Holy" is now "Runeword Holy Thunder"). / 룬워드 이름을 툴팁·패널·마법 효과 창에서 하나로 통일`
- 메인 파일은 2.1.2 ZIP으로 올린다(내용물 설명은 launch 키트 1절).
