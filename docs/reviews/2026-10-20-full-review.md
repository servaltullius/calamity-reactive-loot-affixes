# 칼라미티 전체 리뷰 (2026-10-19~20)

읽기 전용 리뷰입니다. 28개 영역 검토자가 찾은 지적 163건을 지적마다 독립 검증자 3명(코드 추적·의도·엔진 동작)이 반박을 시도했습니다. **확인** = 3명 중 2명 이상 동의, **논란** = 1명만 동의. 기각된 10건은 뺐고, 같은 원인은 합쳐 102건입니다.

| 심각도 | 확인 | 논란 |
|---|---|---|
| 높음 | 3 | 0 |
| 중간 | 18 | 0 |
| 낮음 | 72 | 9 |

## 반복되는 문제

- 같은 FormID·MGEF 재사용으로 옛 세이브와 다른 효과 사이에 상태가 새거나 덮어씌워짐
- 훅 경로와 TESHitEvent 폴백의 판정·중복 제거 규칙이 서로 달라 이중 처리나 누락이 생김
- 발동 확률 패널티·lane 같은 효과 종류 목록이 런타임·툴팁·패널·생성기에 따로 하드코딩되어 어긋남
- MCM으로 런타임을 끈 상태나 disable 플래그가 관련 없는 기능까지 끄거나 아이템 상태를 잃게 함
- 문서·패널 문구·CHANGELOG가 실제 데이터 값(lootWeight, 상한, 대상 조건)과 맞지 않음
- 영어/한국어 문구가 섞이거나 개발용 이름이 사용자 화면에 그대로 나옴
- 엔진이 읽지 않는 actorValue나 잘못된 단위·플래그(Recover, area, 레벨 상한)를 쓴 데이터가 검사 없이 출시됨
- 검증 도구(lint·CI·테스트 픽스처)가 실제 산출물이나 appendedRecords를 비교하지 않아 회귀를 못 잡음

## 높음

### H1. 플레이어가 직접 쓴 주문으로는 대마법사(Archmage)가 절대 발동하지 않음

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.HealthDamage.Routing.cpp:158`, `skse/CalamityAffixes/src/EventBridge.Triggers.HitEvent.cpp:199`

ProcessArchmageSpellHit를 부르는 곳은 두 군데뿐입니다. 기본 훅 모드에서는 Routing.cpp:158이 HitData::attackDataSpell만 보는데, 이 값은 근접 공격 데이터 주문에만 채워지고 일반 주문 시전에서는 비어 있습니다. TESHitEvent 쪽 경로(HitEvent.cpp:199-205)는 `!hookRouted`이면서 committed HitData가 있을 때만 돕니다. 그래서 대마법사 I~IV를 입고 화염탄, 얼음 가시, 연쇄 번개를 쏴도 추가 번개 피해도 매지카 소모도 생기지 않습니다. 폴백 모드에서는 그 적을 미리 무기로 때렸을 때만 오래된 근접 HitData로 발동하고, 이때 온힛 접두도 함께 다시 터집니다. (원래 지적 중 파멸의 낙인/약점 공략 관련 부분은 의도된 동작으로 기각되었습니다.)

**고칠 방향:** hookRouted 값과 상관없이, committed HitData를 요구하지 않고 TESHitEvent의 `a_event->source`로 대마법사를 처리합니다. HandleHitEvent에서 procOrigin/procDepth 조기 반환 뒤, `!hookRouted` 블록과 hasCommittedHitData 블록 밖에 블록을 하나 둡니다. 조건은 다음과 같습니다: `_configLoaded`, `aggressor->IsPlayerRef()`, 대상이 살아 있는 적대 행위자, source가 SpellItem으로 해석되고 IsCalamityProcSource가 아님(CAFF_ 주문 제외), GetSpellType()이 kSpell(필요하면 kScroll 포함, 외침·권능 제외). procDepth/재진입 가드는 유지하고 (target, spell) 단위로 짧은 시간 중복 제거를 둡니다. 이중 발동을 막기 위해 Routing.cpp:158-160의 attackDataSpell 경로와 HitEvent.cpp:199-205의 기존 호출은 지웁니다. 2.2.8 변경 기록의 해당 줄과 test_hit_source_policy.cpp의 'Archmage lane' 주석도 고칩니다. 마지막으로 인게임에서 평범한 화염탄으로 확인합니다(디버그 로그의 'HandleHealthDamage seen ... hasSpell=' 줄).

### H2. 같은 FormID로 내용을 바꾼 MGEF 때문에 옛 세이브에 옛 능력치 보정이 영구히 남음 (세이브 마이그레이션 없음)

- 영역: 아이템·저장 C++ · 확인 · save-data
- 위치: `affixes/modules/keywords.affixes.runewords.json:429`, `affixes/modules/keywords.affixes.suffixes.json:2616`, `affixes/modules/keywords.affixes.runewords.json:4140`, `skse/CalamityAffixes/src/EventBridge.Serialization.Lifecycle.cpp:57`, `skse/CalamityAffixes/src/EventBridge.Triggers.ActiveCounts.cpp:270`, `skse/CalamityAffixes/include/CalamityAffixes/SuffixFamilySelection.h:212`

rc3·정식에서 여러 MGEF가 FormID와 에디터 ID는 그대로 둔 채 액터 값이나 아키타입만 바뀌었다. 엔진은 로드한 효과의 Start를 다시 부르지 않는다. 옛 효과가 더한 값은 액터 저장 데이터에 남고, actorValue는 현재 MGEF에서 읽는다. 게다가 ApplyDesiredPassiveSpells는 이미 있는 상시 주문을 kKeep으로 그대로 둔다(refreshPassiveSpellOnPostLoad는 Spirit에만 있음). 그래서 아무것도 옛 보정을 걷어내지 못한다.
(1) 상시 효과, 시점 무관: Grief/Faith/Hustle-W/Treachery 상시 효과(CAFF_MGEF_RW_*_PASSIVE_IAS)가 WeaponSpeedMult +0.10/0.15/0.12/0.10에서 AttackDamageMult +0.06/0.08/0.06, StaminaRateMult +25로 바뀌었다. Gladiator T1-3도 WSM에서 ADM으로 바뀌었다. 2.2.8·rc1·rc2 캐릭터에는 WeaponSpeedMult +0.10 같은 값이 영구히 남는다. 엔진은 이 값을 배율로 읽으므로 활 당기기는 약 0.1배가 되고, MCO가 없으면 근접 공격도 그만큼 느려진다. 새 공격력 보너스는 장비를 다시 낄 때까지 없다. 장비를 벗으면 Finish가 옛 크기를 새 AV(AttackDamageMult 등)에서 영구히 빼 간다. Eagle Eye(SUFFIX_SHOUT_COOLDOWN_T1-3)는 BowSpeedBonus +5/10/15 ValueModifier에서 Script로 바뀌었다. 그래서 rc3를 포함한 모든 이전 세이브에서 BowSpeedBonus +5~15가 장비를 벗어도, 모드를 지워도 남는다. 이는 CHANGELOG의 'ID는 그대로라 가진 장비에도 바로 적용됩니다'와 어긋난다.
(2) 시한 버프, 버프 중 저장한 경우만: Leaf/Strength/Unbending Will/Passion/Oath/Myth/Cure/Hearth/Enlightenment는 기술 보정 +20~30이 영구히 남는다. 다시 로드된 효과는 Health/DamageResist/ResistMagic을 가리키지만, 적대 조건 때문에 플레이어에게는 대개 비활성이다. Fury, Faith Fanatic(Beast Rage), Jackpot은 WSM/ADM이 남는다. Ground(ShoutRecoveryMult +50, 함성 쿨다운 약 51배)와 Zephyr(BowSpeedBonus +30)는 아키타입이 달라졌다. 그래서 엔진이 로드할 때 효과를 통째로 버리고, 보정만 영원히 남는다. 이 경우는 4~10초 창 안에 저장해야 하므로 드물다. 엔진 동작 일부는 인게임에서 확인하지 못했다.

**고칠 방향:** OnPostLoadGame에서 RebuildActiveCounts(true)보다 먼저 한 번 도는 마이그레이션을 넣는다.
1) 새 MFLG 비트만으로 판정하지 않는다. 이미 공개된 rc3 세이브에도 옛 효과가 남아 있기 때문이다. 플레이어의 활성 효과를 돌면서 base MGEF(PluginEditorIds로 확인)가 대상 목록에 있고, 저장된 magnitude가 옛 값(Grief 0.10, Faith 0.15, Hustle-W 0.12, Treachery 0.10, Gladiator .05/.10/.15, Eagle Eye 5/10/15)과 같으면 옛 효과로 본다. 비트는 두 번 돌지 않게 막는 용도로만 쓴다.
2) 상시 효과: 옛 AV에서 저장된 크기를 뺀다. ValueModifierEffect가 쓴 것과 같은 modifier 슬롯이어야 하는데, permanent인지 temporary인지는 확인이 필요하다. 그다음 activeEffect->magnitude를 0으로 만들고 RemoveSpell을 부른다. 그래야 Finish가 새 AV에서 값을 빼지 않는다. 마지막으로 AddSpell로 새 크기를 적용한다. refreshPassiveSpellOnPostLoad만 켜는 방식은 쓰지 않는다. 첫 RemoveSpell이 옛 크기를 새 AV에서 빼서 AttackDamageMult가 0.04만큼 영구히 모자라게 된다.
3) Eagle Eye: Script 효과라 Finish가 아무것도 하지 않는다. 그래서 BowSpeedBonus에서 저장된 크기를 직접 뺀다. 이 작업은 보정값이 효과 크기 이상일 때만 한다(바닐라 값은 최대 0.5).
4) 시한 버프: Ground/Zephyr는 효과가 로드 때 사라져서 찾아서 해제할 수 없다. 대신 Ground 효과가 없고 ShoutRecoveryMult temporary 보정이 49.5 이상이면 50을 뺀다. Zephyr도 같은 방식으로 BowSpeedBonus 30을 뺀다. 나머지 기술 버프는 옛 크기와 일치하는 효과를 찾아 magnitude를 0으로 만든 뒤 해제한다. 옛 AV에서는 실제로 남은 만큼만 뺀다(해제 전후 temporary modifier 측정). 버프가 없을 때의 값보다 내려가지 않게 상한을 둔다. 가능하면 먼저 인게임에서 재현한다: 2.2.8에서 Leaf/Ground 버프 중 저장하고, 2.3.0에서 로드해 getavinfo로 확인한다.
5) 한계: rc3에서 장비를 다시 껴서 크기가 새 값으로 바뀐 세이브는 남은 WSM을 안전하게 찾을 수 없다. 이런 경우는 콘솔 수정법을 문서로 안내한다.
6) CHANGELOG.md:18 문구를 고친다. 릴리스 노트에 다음을 적는다: 업데이트 전에 Eagle Eye와 해당 룬워드 장비를 벗는다. 전투 밖에서 버프가 끝난 뒤(약 10초) 저장한다. 이미 문제가 생긴 사람은 콘솔에서 `player.modav ShoutRecoveryMult -50`, `player.modav BowSpeedBonus -30` 등으로 고친다.

### H3. 아이템·주문 이름에 UTF-8이 아닌 바이트가 있으면 패널 JSON dump()가 예외를 던져 게임이 튕김

- 영역: UI · 확인 · crash
- 위치: `skse/CalamityAffixes/src/PrismaTooltip.PanelData.inl:38`, `skse/CalamityAffixes/src/PrismaTooltip.PanelData.inl:73`, `skse/CalamityAffixes/src/PrismaTooltip.PanelData.inl:209`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.Detail.cpp:35`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.BaseSelection.cpp:16`

패널 payload 세 개(베이스 목록, 레시피 목록, 패널 상태)를 인자 없는 dump()로 직렬화한다. 동봉된 nlohmann 3.11.3은 기본값이 error_handler_t::strict라서 잘못된 UTF-8 바이트를 만나면 type_error 316을 던진다. 장착 중인 무기·방어구 이름은 ResolveInventoryDisplayName에서 검사 없이 그대로 들어온다. CollectEquippedRunewordBaseCandidates는 착용한 WEAP/ARMO를 조건 없이 모두 모은다. 빌드는 /EHsc이고 JSON_NOEXCEPTION을 정의하지 않으며, 이 경로에는 try/catch가 없다. 예를 들어 CK가 cp1252로 저장한 모드 아이템('Mjölnir'의 0xF6, 'Blade – Refined'의 0x96, 'é'의 0xE9)이나, cp1251/cp1250 문자열을 쓰는 러시아어·폴란드어·체코어 게임에서 그런 아이템을 입고 F11을 누르면 이렇게 된다. 첫 패널 갱신에서 예외가 SKSE 태스크 밖으로 빠져나가 바탕화면으로 튕긴다. 그 아이템을 입고 있는 동안에는 패널을 열 때마다 반복된다. 바닐라 주문 이름이 로컬라이즈돼 있으면 레시피 목록 payload만으로도 일어날 수 있다.

**고칠 방향:** Prisma payload용 헬퍼를 하나 만든다: `std::string DumpPanelJson(const nlohmann::json& j) { return j.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace); }`. PanelData.inl의 dump 세 곳(:38 베이스 목록, :73 레시피 목록, :209 패널 상태)과 다른 Prisma용 dump에 모두 이 헬퍼를 쓴다. ensure_ascii=false를 유지해야 한국어가 UTF-8 그대로 나간다. ::ignore는 바이트를 소리 없이 지우므로 쓰지 않는다. 다만 replace만 쓰면 러시아어 사용자에게는 이름 전체가 U+FFFD(�)로 보인다. 그래서 ResolveInventoryDisplayName에 EnsureUtf8(std::string_view) 정리를 함께 넣는 것이 좋다. 결과가 유효한 UTF-8이면 그대로 두고, 아니면 cp1252에서 UTF-8로 바꾼다. 그리고 payload 생성과 push를 try/catch로 감싼다. 예외는 로그로 남기고 그 push만 건너뛰어서, 잘못된 payload 하나 때문에 게임이 죽지 않게 한다.


## 중간

### M1. 확률형 시체 폭발이 훅으로 처리된 처치 한 번에 확률을 두 번 굴림

- 영역: 전투 C++ · 확인 · balance
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.HealthDamage.Routing.cpp:189`, `skse/CalamityAffixes/src/EventBridge.Actions.Corpse.cpp:260`

기본 HandleHealthDamage 훅에서 무기로 적을 죽이면 즉시 경로(치명타 직후 ProcessImmediateCorpseExplosionFromLethalHit)가 한 번 굴리고, 실패하면 TESDeathEvent 경로가 또 굴립니다. 그래서 잿불 장작(처치 시 18%)은 실제로 약 32.8%, 임종의 숨결(50%)은 75%, 역병(40%)은 64%로 터집니다. 툴팁과 문서는 한 번 굴린 확률을 보여 줍니다. 즉시 경로에서 속도 제한이나 연쇄 깊이로 거부된 처치가 나중에 사망 이벤트로 터지기도 합니다. 100% 확률인 죽음의 장작은 영향이 없습니다. 첫 릴리스(99b49f0)부터 있던 문제입니다.

**고칠 방향:** 두 경로는 유지합니다. 사망 이벤트 경로는 훅이 라우팅하지 않는 처치(TESHitEvent 폴백, 라우팅되지 않은 피해)를 맡는 유일한 경로이고, 훅 경로는 폭발을 즉시 보이게 해 줍니다. 대신 시체 하나당 '평가' 한 번만 허용합니다. corpseExplosionSeenCorpses에 실패를 적으면 안 됩니다. 그 맵은 예산·중복 장부이고 runtime_gate_store_checks_settings_policy.cpp:313-338 테스트가 리셋하기 때문입니다. 별도 맵(예: _combatState.corpseExplosionEvaluatedCorpses, FormID→time_point)을 만들고 seenCorpses와 같은 60초 TTL·같은 정리 방식을 씁니다. CombatRuntimeState 리셋(CombatRuntimeState.h:150 근처)에서 함께 비웁니다. ProcessCorpseExplosionAction의 소환 모드가 아닐 때만, SelectBestCorpseExplosionAffix 전에 살아 있는 항목이 있으면 반환하고, 선택을 돌렸으면 결과와 상관없이 기록합니다.

### M2. TESHitEvent 폴백에 HitData별 중복 제거가 없어, 주문·마법부여 적중 이벤트가 오래된 근접 HitData를 새 적중으로 다시 처리함

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.HitEvent.cpp:143`, `skse/CalamityAffixes/src/EventBridge.Triggers.HitEvent.cpp:145`, `skse/CalamityAffixes/src/EventBridge.Triggers.HitEvent.cpp:245`

다른 모드가 HandleHealthDamage 슬롯(0x104)을 먼저 차지했거나 disableHealthDamageRouting이 켜진 폴백 모드에서 생깁니다. 플레이어가 산적을 한 번 베고 화염 숨결이나 화염탄을 쓰면, 주문 TESHitEvent마다 옛 칼 HitData가 committed로 읽힙니다. 그래서 온힛 접두와 치명 시전이 약 150ms마다 다시 굴립니다(v2.2.4 thunder_mastery 연쇄가 재현됨). 파멸의 낙인은 칼 피해의 60%로 표식을 남깁니다. 마법부여 무기는 무기 이벤트와 마법부여 이벤트(source FormID가 다름)가 같은 HitData로 둘 다 통과해 한 번 휘두를 때 두 번 굴립니다(이 부분은 PLAUSIBLE). 피격 쪽도 같습니다. 적이 한 번 때린 뒤 주문이나 브레스를 쓰면 매 적중마다 kIncomingHit이 돌고, 마법사의 갑옷(MoM)은 그 옛 물리 피해로 체력을 다시 돌려주고 매지카를 다시 깎습니다. 브레스 피해는 물리 피해가 아닌데도 그렇습니다. v2.2.4 수정은 훅 경로만 다뤘습니다.

**고칠 방향:** 훅 경로의 'HitData 내용당 한 번' 규칙을 폴백 두 분기(outgoing, incoming)에 그대로 옮깁니다. (outgoing 여부, aggressor, target)마다 마지막으로 처리한 HitDataUtil::MakeHitContentSignature를 기록합니다. 가능하면 Hooks::detail에 작은 헬퍼(예: TryClaimHitContent)를 두어 훅이 쓰는 s_procDispatch의 dispatchedHitContent 장부를 함께 쓰게 해서 두 경로가 어긋나지 않게 합니다. 따로 둔다면 _combatState 안의 크기 제한 맵으로 만들고, 로드/되돌리기 때 CombatRuntimeState 리셋에서 비우며, s_procDispatch처럼 오래된 항목을 내보냅니다. hasCommittedHitData 확인 뒤 서명이 0이 아니고 저장값과 같으면 '이미 처리됨'으로 보고 kHit, 메아리, 치명 시전, 대마법사, EvaluateMindOverMatter, kIncomingHit을 건너뜁니다. 이때 lastHitAt도 리셋하지 않습니다. 서명은 실제로 작업이 돌았을 때만 기록해서, committed되지 않은 이벤트에서 lastHitAt을 리셋하는 기존 동작을 유지합니다. LowHealth 평가는 계속 돌립니다. 다른 방법(회의론자 1명 제안)으로는 HitData가 그 이벤트의 source 것일 때만 받는 방식이 있습니다. GetHitSourceFormID(hitData, aggressor) == a_event->source이거나 source가 해당 무기의 TESObjectWEAP일 때만 받습니다.

### M3. 훅이 적중 처리를 거부한 피해(주문·브레스·외침·DoT·공격자 없는 피해)에서는 저체력 룬워드가 평가되지 않음

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/Hooks.Dispatch.cpp:404`, `skse/CalamityAffixes/src/Hooks.cpp:338`, `skse/CalamityAffixes/src/Hooks.cpp:284`

저체력 검사가 적중 처리와 묶여 있습니다. 드래곤이 물면(처리됨, 70%에서 저체력 검사) 뒤이은 화염 브레스 틱마다 HandleHealthDamage가 그 물기 HitData를 다시 읽습니다. 그러면 재사용 가드(그리고 stale 비율 가드, 50ms 중복 가드)가 거부해서 저체력 검사까지 가지 않습니다. 사자의 심장, 마지막 소원, 추방(철피부·긴급 회복)은 플레이어가 30% 아래로 떨어지거나 죽어도 발동하지 않고, 다음 물리 적중 때야 발동합니다. 근접 공격 뒤 외침을 쓰는 드라우그, 할퀸 뒤 화염구를 던지는 해그레이븐, 무기로 건 독·화상 DoT 틱도 같습니다. 공격자 행위자가 없는 피해(낙하, 주인 없는 덫)는 Hooks.cpp:284의 ShouldProcessHealthDamageHookPointers에서 걸러져 아예 평가되지 않습니다. 한 회의론자가 1.6.1170 exe에서 낙하·덫 피해도 vfunc 0x820(HandleHealthDamage)을 거친다는 것을 확인했습니다.

**고칠 방향:** 적중 dedupe(재사용·stale·50ms 가드)는 그대로 두고, 피해 기반 저체력 검사만 따로 떼어 냅니다. ThunkImpl에서 CallOriginal 후 조기 반환하는 모든 분기(ShouldAllowProcDispatch 거부의 각 가드, 공격자 없음, 메아리 일격 경로. 시체 경로는 상관없음)에서 대상이 플레이어(context.targetIsPlayer)이면 처리합니다. 먼저 CallOriginal을 호출해 피해가 들어간 뒤 체력을 읽게 합니다. 그 다음 SchedulePostHealthDamageActions처럼 FormID와 s_runtimeGeneration을 캡처하는 SKSE 태스크를 큐에 넣습니다. 태스크는 새 EventBridge 진입점(예: OnPlayerHealthDamageLowHealthOnly)을 부릅니다. 이 진입점은 _stateMutex를 잡고 _configLoaded, enabled, IsPlayerRef를 확인한 뒤 ProcessTrigger(kLowHealth)만 실행합니다. 공격자가 null이면 플레이어를 대상과 소유자로 넘깁니다(대상이 null이면 거부되기 때문). HandleHealthDamage 안에서 동기로 평가하면 안 됩니다. 그 시점에는 체력이 아직 깎이지 않았고 훅 안에서 주문을 시전하게 됩니다. 브레스 틱마다 태스크가 쌓이지 않게 합치고(coalescing), 저체력 어픽스가 활성일 때만 예약합니다. 런타임 활성, inHook 아님, proc-dispatch 가드 밖일 때만 예약합니다.

### M4. 저체력 재무장이 쿨다운 중이거나 피해 없이 회복한 뒤에는 이뤄지지 않아 긴급 룬워드가 잠긴 채로 남음

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.Policy.cpp:201`

사자의 심장(문턱 30%, 재무장 40%, ICD 45초) 예입니다. 적중으로 25%가 되어 발동한 뒤 플레이어가 90%까지 회복하고 45초 동안 계속 맞아도, ICD 게이트가 먼저 반환하므로 재무장되지 않습니다. ICD가 끝났을 때 45%에서 35%로 맞으면 재무장도(40% 미만) 발동도(30% 초과) 안 됩니다. 다음 적중에 25%가 되면 previous=35<40이라 consumed가 그대로 true여서 발동하지 않습니다. 거의 1분 동안 재무장선 위에 있었는데도 그렇습니다. ICD가 없을 때도, 만피까지 회복한 뒤 첫 적중이 30~40% 사이에 떨어지면 같은 일이 생깁니다. 'previous'를 마지막 저장값으로 쓰기 때문입니다.

**고칠 방향:** PassesLowHealthTriggerGate를 둘로 나눕니다. (1) 상태만 쓰는 재무장 갱신: 현재 체력 ≥ 재무장선이거나, 적중 전 체력 ≥ 재무장선이면서 현재 ≤ 문턱이면 consumed=false로 둡니다. 적중 전 체력은 마지막 스냅샷이 아니라 현재 체력 + 이번 피해로 계산합니다. 이 갱신은 ProcessTrigger의 kLowHealth 처리에서 TryProcessTriggerAffix 루프 전에, nextAllowed(ICD), recently, lucky-hit, proc 예산 게이트보다 먼저 모든 활성 저체력 어픽스 인덱스에 한 번씩 돌립니다(또는 FinalizeTriggerDispatch에서). (2) 발동 판정은 기존 게이트가 맡습니다.

### M5. 치명 시전(CoC)의 위력 덮어쓰기가 다중 효과 주문의 모든 효과에 적용되어 냉기 CoC 둔화가 적중 피해에 비례해 완전 정지까지 커짐

- 영역: 전투 C++ · 확인 · balance
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Special.cpp:574`

EvaluateCastOnCrit는 max(기본값, 0.3×물리 피해)를 magnitudeOverride로 넘깁니다. 엔진의 ActorMagicCaster::CastSpellImmediate는 이 값을 캐스터(오프셋 0x3C)에 하나만 저장해 주문의 모든 효과에 씁니다(1.7.104 exe에서 확인). 얼음 가시 CoC(일반 근접 45%, ICD 0.15초)에서 물리 300짜리 적중이면 덮어쓰기 값이 90이 되어 '둔화 50%'가 둔화 90%가 됩니다. 물리 약 333을 넘으면(얼음 창은 최소 60) SpeedMult가 100% 이상 깎여 거의 매번 3~5초 동안 대상이 멈춥니다. 분해·강렬한 화염 특전 효과의 위력(200/99)도 적중 비례 값으로 낮아집니다.

**고칠 방향:** 적중 비례 덮어쓰기는 피해 효과에만 적용합니다. 기존 CAFF_SPEL_COC_* 레코드는 에디터 ID를 유지한 채 피해 효과만 남깁니다(레코드 내용만 바꾸면 FormID는 그대로). 냉기 둔화, Deep Freeze, 분해, 강렬한 화염, 충격 경직 같은 보조 효과는 CoC마다 별도의 단일 목적 주문으로 만듭니다. 새 주문은 spec.root.json의 keywords.appendedRecords 맨 끝에만 추가합니다(FormID가 위치 기반이기 때문). 그리고 덮어쓰기 0으로 함께 시전합니다.

### M6. 파멸 표식 폭발 시각 뒤에 dead=true 사망 이벤트가 오면, TickDoomBursts가 죽어가는 대상의 파멸을 먼저 버려 전염이 파멸을 옮기지 못함

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Status.cpp:690`, `skse/CalamityAffixes/src/EventBridge.Triggers.DeathEvent.cpp:50`

파멸의 낙인이 t=0에 적 A를 표시하고(폭발 t=1.5) 전염이 준비된 상태에서, t=0.8에 A를 죽였다고 합시다. A가 죽어가는 동안 t=1.5에 TickDoomBursts가 항목을 꺼내 A가 죽었으므로 버립니다. A의 dead=true 이벤트가 그 뒤에 오면 전염(Kill 트리거, DeathEvent.cpp:246에서만 실행)은 파멸을 찾지 못합니다. A에게 파멸만 있었다면 전염 자체가 발동하지 않습니다. 폭발 직전에 처치하는 흔한 경우에 '처치한 적의 상태(노출, 파멸...)가 최대 2명에게 옮겨 간다'는 설명이 파멸에 대해 성립하지 않습니다.

**고칠 방향:** 죽어가는 대상의 파멸을 dead=true 이벤트가 읽을 때까지 살려 두고, 터지지는 않게 합니다. HandleDeathEvent의 dead=false 분기(DeathEvent.cpp:50, 이미 상태 락을 잡은 곳)에서 _combatState.doomQueue.Find(victim)와 그 대상의 파멸 장부 항목을 FormID별 '죽어가는 파멸' 기록에 복사합니다. 유예 TTL은 5~10초 정도로 둡니다. TickDoomBursts는 죽어가는 대상의 기한 항목을 폭발시키지 않습니다. 전염(HasSpreadableStatus와 전파 실행)은 dead=true 시점에 그 기록을 읽습니다.

### M7. 화염·냉기·전격 일격의 lootWeight가 0이라 절대 나오지 않는데, 문서와 v2.3.0 조정은 일반 접두로 취급함

- 영역: 아이템·저장 C++ · 확인 · data-text
- 위치: `affixes/modules/keywords.affixes.core.json:99`, `affixes/modules/keywords.affixes.core.json:155`, `affixes/modules/keywords.affixes.core.json:213`, `Data/SKSE/Plugins/CalamityAffixes/affixes.json`

flame_strike, frost_strike, spark_strike의 runtime.lootWeight가 0.0이다. 공개 접두 76개 중 가중치가 0인 것은 이 셋뿐이다. 그래서 감정, 정제, 재련 어디에서도 나오지 않는다. 그런데 v2.3.0 노트와 넥서스 페이지는 냉기 일격과 전격 일격을 빙결·감전 누적 수단으로 소개한다. 플레이어는 얻을 수 없는 접두를 보고 빌드를 짜게 되고, 이 셋을 위해 한 누적 조정은 게임에서 아무 효과가 없다. 가중치 0은 9b8b916에서 일부러 은퇴시킨 것인데, 이후 작업에서 이를 놓쳤다.

**고칠 방향:** 설계 판단이므로 작성자에게 방향을 묻는다.
(a) 다시 나오게 하려면: runtime.lootWeight를 0보다 크게 준다. 동료 접두처럼 1.0을 주거나, 100% 적중·ICD 없음 특성을 고려해 0.3~1.0 정도로 준다. kid.chance도 같은 값으로 맞춘다. 그다음 affixes.json과 Data/SKSE/Plugins/CalamityAffixes/affixes.json을 재생성하고 문서 생성기를 다시 돌린다. ESP와 FormID는 바뀌지 않는다.
(b) 은퇴를 유지하려면: gen_prefix_doc.py/gen_effects_en_doc.py(update_docs.py)에서 가중치 0인 공개 접두를 공개 목록과 개수에서 빼거나(76 → 73), '루트 풀에 없음, 기존 장비만'으로 표시한다. CHANGELOG v2.3.0의 누적 수단 목록(CHANGELOG:43)과 계획 표에서 냉기 일격·전격 일격을 지운다.
어느 쪽이든 파이썬 계약 테스트를 추가한다. 공개 접두(internal_/runeword_ 제외)의 lootWeight가 0 이하인데 허용 목록에 없으면 실패하게 하고, kid.chance가 lootWeight와 같은지도 검사한다.

### M8. 소환 무기(Bound Sword 등)를 제작 베이스로 쓸 수 있어서, 주문이 끝나면 들인 조각·두루마리·오브가 사라짐

- 영역: 아이템·저장 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.Service.cpp:229`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.BaseSelection.cpp:35`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.BaseSelection.cpp:122`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.Detail.cpp:32`

IsLootObjectEligibleForAffixes는 TESObjectWEAP이면 무조건 true를 돌려준다. 방어구에만 GetPlayable/슬롯 검사가 있다. CollectEquippedRunewordBaseCandidates와 SelectRunewordBase는 IsWeaponOrArmor만 확인하고, 착용 중인 소환 무기에 ExtraUniqueID까지 붙인다. 그래서 Bound Sword/Bound Bow를 장착한 채 패널을 열면 이 무기를 베이스로 고를 수 있다. 여기에 룬워드를 변환하거나 감정 두루마리, 재련 오브, 칸 확장을 쓰면 재료가 소모된다. 120초가 지나거나 무기를 해제하면 아이템이 룬워드·어픽스와 함께 사라진다. 다시 시전하면 어픽스 없는 새 인스턴스가 나온다. 모드가 추가한 플레이 가능한 소환 방어구와, 모드가 플레이어에게 입힌 플레이 불가 방어구도 같은 경로를 탄다.

**고칠 방향:** 1) IsLootObjectEligibleForAffixes(Service.cpp:229)에서 무기 분기를 `if (const auto* weap = a_object->As<RE::TESObjectWEAP>()) { return !weap->IsBound() && weap->GetPlayable(); }`로 바꾼다.
2) CollectEquippedRunewordBaseCandidates(BaseSelection.cpp:35)와 SelectRunewordBase(BaseSelection.cpp:122)에서 IsWeaponOrArmor 대신 IsLootObjectEligibleForAffixes로 거른다. 거르는 시점은 ExtraUniqueID를 만들기 전이어야 한다. 그래야 소환·부적격 아이템에 키가 생기지 않고, 목록과 CycleRunewordBase에도 나오지 않는다.
주의: 이 함수는 ActiveCounts의 지연 정리(cleanupInvalidLegacyAffixes)에도 쓰인다. GetPlayable 조건을 공용 함수에 넣으면, 모드가 추가한 플레이 불가 무기에 이미 붙은 어픽스도 지워진다. 이 부작용을 받아들이기 어렵다면 공용 함수에는 IsBound만 넣고, playable 검사는 베이스 목록과 선택에만 적용한다. 소환 무기에 이미 저장된 어픽스가 지워지는 것은 의도한 결과다.
3) IsLootObjectEligibleForAffixes가 소환·플레이 불가 무기를 거부하는지 확인하는 단위 테스트를 추가한다. 모드의 플레이 가능 소환 방어구는 ARMO 플래그로 구별할 수 없으므로 범위 밖으로 둔다.

### M9. 암살자 접미사의 치명타 확률 +10/20/30%가 바닐라 치명타 퍽의 'Set Value'에 덮여 사라짐

- 영역: 데이터·생성기 · 확인 · correctness
- 위치: `affixes/modules/keywords.affixes.suffixes.json:2037`, `affixes/modules/keywords.affixes.suffixes.json:2086`, `affixes/modules/keywords.affixes.suffixes.json:2135`

suffix_assassin_t1~t3은 CriticalChance 액터값에 +10/20/30을 준다. 그런데 엔진은 크리 루틴(HitData::Populate → 0x7f17c0)에서 CriticalChance AV를 읽은 뒤 kCalculateMyCriticalHitChance 퍽 엔트리를 적용한다. 바닐라 Bladesman·Deep Wounds·Critical Shot은 이 엔트리에서 'Set Value'를 쓴다. 그래서 Bladesman 1랭크인 한손검 사용자가 Grand Assassin을 끼면 툴팁에는 +30%라고 나오지만 실제 확률은 10%다. 퍽이 없으면 30%였을 것이므로 퍽을 찍으면 오히려 확률이 내려간다. 룬워드 Pattern(CAFF_MGEF_RW_PATTERN_WARD)도 같은 AV를 쓰므로 같은 문제가 있다.

**고칠 방향:** Assassin T1~T3과 Pattern 룬워드를 함께 고친다. 보너스를 CriticalChance AV에서 빼서(활 속도 효과처럼 아무것도 하지 않는 운반용 효과로 바꿈) 퍽 단계보다 먼저 계산되지 않게 한다. 그리고 바닐라 Set 엔트리 뒤에 적용되는 방법으로 다시 더한다. 가장 견고한 방법은 DLL 훅이다. HitData::Populate가 부르는 크리 루틴 안의 kCalculateMyCriticalHitChance(EP 1) HandleEntryPoint 호출을 감싸서, 결과에 접미사·룬워드 보너스를 더한다(1.7.104 기준 ID 44016 부근). 퍽을 쓰는 방법이라면 'Calculate My Critical Hit Chance: Add Value' 엔트리를 단계에 맞춰 붙였다 뗀다. 고치지 않을 거라면 바닐라 치명타 퍽과 겹치지 않는다고 문구에 적는다. 인게임 확인: Bladesman + T3 + 디버그 크리 카운터.

### M10. 변환·시체 폭발·대마법사 피해가 1초 DoT라서 1초 안에 같은 주문을 다시 걸면 앞 피해의 남은 부분이 사라질 수 있음

- 영역: 데이터·생성기 · 확인 · correctness
- 위치: `affixes/modules/keywords.affixes.core.json:1658`

CAFF_SPEL_DMG_FIRE/FROST/SHOCK_DYNAMIC은 커밋 11f1c3d부터 magnitude 1·duration 1(Health ValueMod, Hostile|Detrimental, NoRecast 없음)이다. 다시 걸었을 때 앞 효과가 대체된다면 이런 일이 생긴다. 단검이나 쌍수로 0.5초마다 치면서 Fire Infusion 100%를 쓰면, 매 타격의 물리 피해가 빠지고 1초에 걸친 화염 피해로 다시 들어가는데 다음 타격이 0.5초 뒤 이를 덮어서 마지막 타격 말고는 절반쯤 사라진다. 그러면 어픽스가 없을 때보다 DPS가 낮아진다. Death Pyre 연쇄(0.9초 창)와 한 번 휘둘러 두 마리를 잡을 때 두 폭발에 모두 맞은 세 번째 적도 같다. 다만 한 검토자는 SkyrimSE 1.6.1170 분석에서 Value Modifier 재시전은 겹쳐 쌓인다고 반박했다.

**고칠 방향:** 먼저 같은 시전자의 같은 주문 재시전이 앞 효과를 대체하는지 쌓이는지 인게임에서 확인한다. 대체된다면 세 dynamic 피해 효과를 duration 0으로 되돌려 즉시 피해로 만들고 ESP와 affixes.json을 다시 생성한다. 원소 피격 셰이더는 따로 낸다. 방법은 DLL에서 변환·폭발·대마법사 시전 후 target->InstantiateHitShader를 부르거나, 기존 PlayActionFeedback 경로를 쓰거나, 피해 0·duration 1짜리 셰이더 전용 효과를 keywords.appendedRecords 맨 끝에 새 레코드로 추가하는 것이다. 다른 방법은 재시전 전에 살아 있는 인스턴스의 남은 피해를 새 override에 더하는 것이다. 이 코드베이스의 다른 곳(예: CAFF_MGEF_STATUS_BURNING)도 '재시전 = 대체'를 가정하므로 같이 점검한다.

### M11. Nadir(공포)·Peace(진정)·Delirium(광란)·Lawbringer(언데드 퇴치) MGEF에 Recover가 없음

- 영역: 데이터·생성기 · 확인 · correctness
- 위치: `affixes/modules/keywords.affixes.runewords.json:2240`, `affixes/modules/keywords.affixes.runewords.json:3639`, `affixes/modules/keywords.affixes.runewords.json:3994`, `affixes/modules/keywords.affixes.runewords.json:5092`

CAFF_MGEF_RW_LAWBRINGER_HEAL(2240, TurnUndead/Confidence), CAFF_MGEF_RW_NADIR_WARD(3639, Demoralize/Confidence), CAFF_MGEF_RW_PEACE_BARRIER(3998, Calm/Aggression), CAFF_MGEF_RW_DELIRIUM_CONFUSE(5092, Frenzy/Aggression)이 "recover": false다. ESP의 DATA 플래그는 0x5로 Recover(0x2)가 없다. 반면 바닐라의 Calm·Frenzy·Demoralize·TurnUndead MGEF 57개는 모두 Recover가 있다. 그래서 효과가 끝나도 대상의 Confidence/Aggression 변화가 되돌려지지 않고 세이브에 남을 수 있다. 예를 들어 Delirium이 현상금 걸린 경비병이나 고유 NPC에게 걸리면 계속 적대적인 상태로 남을 수 있고, Peace는 영구히 덜 공격적으로, Nadir·Lawbringer는 영구히 겁먹게 만들 수 있다. 한 검토자는 Delirium의 변화 방향이 지적과 다르고 변화 폭은 더 크다고 정정했다. 다른 검토자는 '영구히 남는다'는 결과는 반박했지만 Recover를 켜야 한다는 데는 동의했다. 2026-03 95b0d17부터 있던 문제다.

**고칠 방향:** keywords.affixes.runewords.json에서 CAFF_MGEF_RW_NADIR_WARD, CAFF_MGEF_RW_PEACE_BARRIER, CAFF_MGEF_RW_DELIRIUM_CONFUSE, CAFF_MGEF_RW_LAWBRINGER_HEAL을 "recover": true로 바꾸고 다시 생성한다. 플래그만 바뀌므로 FormID와 순서는 그대로다. Recover가 있으면 TargetValueModifierEffect::Start가 (목표값 - 현재값)을 Temporary 수정치로 한 번 적용하고 Finish가 이를 제거한다. tools/lint_affixes.py:468-476의 'Paralysis는 recover=true' 규칙을 Calm, Frenzy, Demoralize, TurnUndead(같은 맥락에서 Invisibility/Etherealize도)로 넓힌다. Calm·TurnUndead에 Detrimental을 강제로 붙이는 것(KeywordPluginBuilder.cs:672)도 다시 검토한다.

### M12. Melody·Death·Gloom의 즉시(duration 0) 흡수 효과에 Recover가 켜져 있어 흡수한 양이 같은 프레임에 되돌아감

- 영역: 데이터·생성기 · 확인 · correctness
- 위치: `affixes/modules/keywords.affixes.runewords.json:1922`, `affixes/modules/keywords.affixes.runewords.json:2703`, `affixes/modules/keywords.affixes.runewords.json:2707`, `affixes/modules/keywords.affixes.runewords.json:4770`

CAFF_MGEF_RW_MELODY_NOTE(01000943, Stamina), CAFF_MGEF_RW_DEATH_LIFESTEAL(01000972, Health), CAFF_MGEF_RW_GLOOM_VEIL(010009ED, Magicka)은 Absorb·hostile·recover true이고 ESP 플래그는 0x7(Hostile|Recover|Detrimental)이다. 주문은 duration 0(크기 20/15/30)이며 CastSpell 발동으로 걸린다. Death가 발동하면(타격 시 24%, ICD 5초) 대상이 체력 15를 잃고 플레이어가 15를 얻지만, 같은 업데이트 안에서 둘 다 되돌아가 아무 일도 일어나지 않는다. 패널과 문구는 'Absorb Health 15'를 약속한다. Melody와 Gloom도 같다.

**고칠 방향:** 세 MGEF를 "recover": false로 바꾼다. Hand of Justice와 바닐라처럼 Recover 없는 duration 0 흡수는 Start에서 Damage 수정치로 한 번 적용되고 되돌려지지 않는다. duration은 0으로 둬도 된다. 레코드 위치는 바꾸지 말고 그 자리에서 고친 뒤 affixes.json을 다시 합치고 ESP와 Data/SKSE affixes.json을 다시 생성한다. tools/lint_affixes.py(545행 부근)의 즉시 복원 규칙을 hostile 효과와 Absorb에도 적용한다. 또는 AffixSpecLoader.Validate에 검사를 넣는다. 조건은 'ConstantEffect가 아니고 duration 0인 주문이 쓰는 Health/Magicka/Stamina ValueModifier·Absorb MGEF에 Recover가 있으면 실패'이고, hostile 여부와 상관없이 적용한다.

### M13. Destruction·Pride 주문 범위 350/250이 피트 단위 area 필드에 들어가 반경이 약 107m·76m가 됨

- 영역: 데이터·생성기 · 확인 · balance
- 위치: `affixes/modules/keywords.affixes.runewords.json:3225`, `affixes/modules/keywords.affixes.runewords.json:3391`

CAFF_SPEL_RW_DESTRUCTION_BLEED(TargetActor, area 350)와 CAFF_SPEL_RW_PRIDE_CHILL(TargetActor, area 250)의 값이 그대로 EffectData.Area(KeywordPluginBuilder.cs:867)로 들어간다. 엔진은 area에 22를 곱하므로 반경이 약 7,700 유닛(약 107m)과 약 5,500 유닛(약 76m)이 된다. Destruction이 발동하면(타격 시 30%, ICD 5초) 던전이나 야영지 대부분의 적대 액터가 5초 동안 초당 10~40 전격 피해를 받는다. Pride는 76m 안의 모든 적대 대상에게 30~200 냉기 피해를 준다. 눈치채지 못한 먼 적까지 전투에 끌려온다. 패널 설명은 'Radius 350/250'이다. v1.3.0(d4db5b4)부터 있던 문제다.

**고칠 방향:** 방법 1, 엔진 범위를 피트로 고침: 22유닛/피트 기준으로 Destruction은 area 16(약 352유닛), Pride는 11(약 242유닛)로 바꾸고 문구와 단위를 맞춘다. 방법 2, 덫 선례(0b45b4e)를 따름(권장): 두 주문의 area를 0으로 하고, 덫·CorpseExplosion이 쓰는 적대 대상 선택으로 350/250 월드 유닛 안의 적을 maxTargets 제한과 함께 런타임에 고른다. 이 방법이면 설명과 패널 반경을 월드 유닛 하나로 통일할 수 있다.

### M14. 진정·공포·광란·언데드 퇴치 룬워드의 레벨 상한(Nadir 20, Peace 25, Delirium 20, Lawbringer 30)이 어디에도 안 적혀 있고 중후반 적에게 조용히 실패함

- 영역: 데이터·생성기 · 확인 · data-text
- 위치: `affixes/modules/keywords.affixes.runewords.json:2240`, `affixes/modules/keywords.affixes.runewords.json:3643`, `affixes/modules/keywords.affixes.runewords.json:3999`, `affixes/modules/keywords.affixes.runewords.json:5093`

CAFF_SPEL_RW_NADIR_WARD(Demoralize 20), CAFF_SPEL_RW_PEACE_BARRIER(Calm 25), CAFF_SPEL_RW_DELIRIUM_CONFUSE(Frenzy 20), CAFF_SPEL_RW_LAWBRINGER_HEAL(TurnUndead 30)은 이 크기를 대상 레벨 상한으로 쓰는 원형이다. magnitudeScaling이나 override도 없다. 레벨 40 플레이어가 레벨 35 산적 두목에게 맞아 Nadir 12%가 터지면, 공포 소리와 아트는 나오지만 적에게는 효과가 없고 16초 쿨다운만 소모된다. 초반이 지나면 대부분의 적이 레벨 20~25를 넘으므로 이 룬워드들은 거의 작동하지 않는다. 이름·요약·문서 어디에도 상한이 적혀 있지 않다.

**고칠 방향:** 먼저 밸런스를 정하고 문구를 맞춘다. (a) 상한 없이 쓰려면 네 주문의 records.spell.effect.magnitude를 바닐라 '모두에게 통하는' 파워처럼 99로 올린다. 레벨 스케일링 모드팩을 생각하면 100~255처럼 여유를 두는 게 더 안전하다. 값만 바꾸므로 FormID는 그대로이고, 다시 생성하면 된다. 보스·드래곤까지 걸리는 게 싫다면 대상 레벨로 런타임 magnitudeOverride를 계산하는 방법도 있다. (b) 레벨 제한을 유지한다면 name/nameEn/nameKo, SummaryText.h, docs/RUNEWORD_EFFECTS.md·EFFECTS_EN.md에 '공포 5초, 레벨 20 이하' / 'Fear 5s, up to level 20'처럼 상한을 적는다.

### M15. Insight 상시 패시브 'Magicka Regen +25'가 기본 3인 MagickaRate에 +25를 더해 마나 재생이 약 9배가 됨

- 영역: 데이터·생성기 · 확인 · balance
- 위치: `affixes/modules/keywords.affixes.runewords.json:278`

CAFF_MGEF_RW_INSIGHT_PASSIVE_MPREGEN(0x10008DC)은 actorValue MagickaRate(AV 28) Value Modifier다. CAFF_SPEL_RW_INSIGHT_PASSIVE(Ability/ConstantEffect)가 이를 크기 25로 건다. Insight 무기를 끼면 매지카 재생이 초당 최대치의 3%에서 28%로 오른다. 전투 밖에서는 약 3.6초, 전투 중에도 약 11초면 다 차서 사실상 무한 매지카가 되고, 하이 엘프 Highborn이 상시 켜진 것과 같다. 가장 강한 접미사인 Meditation T3도 +75%(1.75배)다. 문구는 +25%처럼 읽힌다.

**고칠 방향:** 레코드를 그 자리에서 고친다(에디터 ID와 FormID 유지, appendedRecords에 새 레코드를 추가하지 않음). keywords.affixes.runewords.json:280의 actorValue를 MagickaRate에서 MagickaRateMult로 바꾸고, 크기는 25(+25%)로 두거나 Obsession 패시브와 겹치지 않게 다른 값(예: 25~50)을 고른다. name/nameKo/nameEn을 '마나 재생률 +N%' / 'Magicka Regen Rate +N%'로 바꾸고, SummaryText와 문서도 맞춘 뒤 다시 생성한다.

### M16. Radiance의 생명 감지 주문이 area 0이고 피격 셰이더도 없어 아무것도 감지하지 못함

- 영역: 데이터·생성기 · 확인 · correctness
- 위치: `affixes/modules/keywords.affixes.runewords.json:3874`, `affixes/modules/keywords.affixes.runewords.json:3889`

CAFF_MGEF_RW_RADIANCE_GUARD(010009B4)는 DetectLife 원형, av 56(DetectLifeRange), hitShader 0, FXPersist 없음, 조건 없음이다. CAFF_SPEL_RW_RADIANCE_GUARD(010009B5)는 Self·magnitude 200·area 0·duration 30이다. 바닐라 생명 감지는 반경을 area에서 가져오고 강조 표시는 피격 셰이더로 그린다. 그래서 Radiance가 발동해도(피격 시 15%, ICD 40초) 아트와 소리만 나오고 30초 동안 아무 생물도 강조되지 않으며, 쿨다운만 소모된다. 문구 'Detect Life 200m'도 단위가 틀렸다(200피트는 약 61m).

**고칠 방향:** 레코드 내용만 그 자리에서 고친다(새 레코드 추가나 순서 변경 금지). MGEF: actorValue를 None으로(또는 생략), "hitShaderForm": "Skyrim.esm|0x000DC209"(LifeDetectedEnemy; 아군 모양은 0x00000146 LifeDetected), "fxPersist": true. 주문 효과: magnitude 0, duration 30, area 약 200(바닐라 실외 기준 피트; 원하면 IsInInterior로 실내 100 분리). 필요하면 바닐라식 생물 대상 조건을 넣는다. 문구 단위를 맞추고 다시 생성한다. area만 고치면 셰이더가 없어서 여전히 아무것도 보이지 않는다.

### M17. 인벤토리에서 다른 아이템을 가리키거나 이미 고른 장비를 다시 고르면 선택한 장비의 'Full Item Effects' 문구가 지워지고 다시 나오지 않음

- 영역: UI · 확인 · correctness
- 위치: `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:508`, `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:188`, `Data/PrismaUI/views/CalamityAffixes/scripts/ui-text.js:349`, `Data/PrismaUI/views/CalamityAffixes/scripts/commands.js:361`, `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:338`, `skse/CalamityAffixes/src/PrismaTooltip.PanelData.inl:459`, `skse/CalamityAffixes/src/PrismaTooltip.ViewLifecycle.inl:295`

JS에서 runewordAffixTextState에 값을 넣는 곳은 setRunewordAffixPreview(interop.js:338-342) 하나뿐이다. 반면 invalidateRunewordAffixPreview(ui-text.js:348-349)는 이 값을 ""로 비우고, 세 곳에서 불린다. setSelectedItemName은 받은 이름이 바뀔 때마다, setInventoryItems는 선택 이름이 바뀔 때, 그리고 미리보기를 무효화하는 명령(commands.js:361-362)에서 부른다. 그런데 setSelectedItemName으로 오는 이름은 작업 중인 베이스가 아니다. 인벤토리·거래·상자 메뉴에서 가리킨 아이템이다(PushSelectedTooltipSnapshot에서 SetSelectedItemContext로 전달). DLL은 마지막으로 보낸 문구를 g_runewordCache.affixPreview에 캐시하고, 같은 문구는 다시 보내지 않는다. 그래서 베이스를 고른 뒤 인벤토리를 열고(패널이 닫혀 있어도) 다른 아이템을 가리킨 다음, 인벤토리를 닫고 F11로 패널을 열면 이렇게 보인다. Item Affixes 탭의 'Full Item Effects'에는 'Pick an item to see its current affixes.'가 나온다. 'Inspect Highlighted Item'에는 'No item focused yet'이 나온다. 아이템 카드에는 선택한 장비가 그대로 보이는데도 그렇다. 'Change Item'에서 이미 체크된 장비를 다시 눌러도 바로 같은 상태가 된다. 이 상태는 로딩 화면을 거치거나 그 장비의 미리보기 문구가 바뀔 때(다른 베이스 선택, 제작, 언어 전환)까지 이어진다.

**고칠 방향:** 두 경로를 모두 고쳐야 하고, 한쪽만으로는 해결되지 않는다. (1) 마우스 오버 경로: setSelectedItemName(interop.js:508)에서 invalidateRunewordAffixPreview 호출을 뺀다. 가리킨 아이템은 작업 베이스가 아니며, prisma_script_order_test.js:244-262가 이미 둘이 분리돼야 한다고 고정하고 있다. setInventoryItems(interop.js:188-190)는 이름이 아니라 선택된 베이스 키가 바뀔 때만 무효화한다. (2) 명령 경로: 둘 중 하나를 고른다. 첫째, 미리보기를 무효화하는 명령 뒤에 새 미리보기 없이 feedback이나 타임아웃만 오면, 마지막으로 확정된 문구를 되살린다(JS는 대기 표시만 한다). 둘째, DLL에서 g_runewordCache.affixPreview를 비워 같은 문구도 다시 push되게 한다. 비우는 위치는 SetControlPanelOpenImpl(true)의 RequestRunewordPanelRefresh 옆(ViewLifecycle.inl:295)과, 미리보기를 무효화하는 명령 처리(베이스 선택, 제작)의 시작 부분이다.

### M18. '부여 +N%' 접미(Enchanter)는 엔진이 읽지 않는 EnchantingModifier를 써서 아무 효과가 없고, '중갑/경갑 +N%'(Myth 패시브·Fortitude·Evasion)는 실제로는 '받는 피해 감소'로 작동해 문구와 다르다

- 영역: 도구·문서 · 확인 · data-text
- 위치: `affixes/modules/spec.root.json:1122`, `affixes/modules/keywords.affixes.suffixes.json:2910`, `affixes/modules/keywords.affixes.suffixes.json:2958`, `affixes/modules/keywords.affixes.suffixes.json:3006`, `affixes/modules/keywords.affixes.suffixes.json:2184`, `affixes/modules/keywords.affixes.suffixes.json:1605`, `docs/SUFFIX_EFFECTS.md:28`

CAFF_MGEF_RW_MYTH_PASSIVE와 Fortitude 접미(CAFF_MGEF_SUFFIX_DESTRUCTION_COST_T1-3)는 HeavyArmorModifier(AV 101), Evasion 접미(CAFF_MGEF_SUFFIX_POISON_RESIST_T1-3)는 LightArmorModifier(102), Enchanter 접미(CAFF_MGEF_SUFFIX_ENCHANTING_T1-3)는 EnchantingModifier(113)를 쓴다. 바닐라 MGEF는 이 세 AV를 하나도 쓰지 않는다. 바닐라 강화 효과는 기술 AV를 쓴다: AlchFortifyHeavyArmor는 HeavyArmor(11), AlchFortifyEnchanting과 EnchRobesFortifyEnchantingConstantSelf는 Enchanting(23)에 Peak Value Modifier를 건다. 검증자 2명은 세 AV 모두 엔진이 읽지 않는다고 봤고, 1명은 중갑·경갑은 퍽이 읽는다고 반박했다. 의견이 갈려 Skyrim.esm의 PerkSkillBoosts(0x000CF788)를 직접 열어 봤다. Mod Incoming Damage 항목이 ArmorHeavy/ArmorLight 키워드 장비를 입고 있을 때 피해에 (1 - 0.01 x HeavyArmorModifier/LightArmorModifier)를 곱한다. EnchantingModifier(113)를 읽는 항목은 없다. 결과는 둘이다. 첫째, 'of Enchanter' 부여 +5/10/15%는 제작대에서 아무 효과가 없다. 둘째, Myth의 '중갑 +10%', Fortitude의 '중갑 +5/10/15%', Evasion의 '경갑 +5/10/15%'는 해당 갑옷을 입었을 때 받는 피해 -N%로 작동한다. 툴팁, CHANGELOG, SUFFIX_EFFECTS.md에는 막연히 '+N%'로만 적혀 있다.

**고칠 방향:** 모든 에디터 ID와 FormID는 그대로 두고 레코드 내용만 바꾼다. (1) CAFF_MGEF_SUFFIX_ENCHANTING_T1-3은 바닐라 Fortify Enchanting 방식을 따른다. actorValue를 Enchanting으로 하고 Peak Value Modifier로 건다. 다른 스탯으로 바꾸는 방법도 있다. (2) Myth 패시브와 Fortitude/Evasion은 HeavyArmorModifier/LightArmorModifier를 그대로 둔다. 문구만 실제 동작에 맞게 '중갑(경갑) 착용 시 받는 피해 -N%'처럼 바꾼다. 바꿀 곳은 nameKo/nameEn, 툴팁, CHANGELOG, SUFFIX_EFFECTS.md 생성기다. 바꾼 뒤 인게임에서 Fortitude를 낀 상태와 뺀 상태의 받는 피해를 비교해 확인한다.


## 낮음

### L1. 폴백 시체 적중 필터가 '처치 일격'이 아닌 시체 첫 적중을 통과시키고, 30초·로드·32칸 한계로 시체를 잊음

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/include/CalamityAffixes/CorpseHitFilter.h:20`, `skse/CalamityAffixes/include/CalamityAffixes/CorpseHitFilter.h:26`, `skse/CalamityAffixes/src/EventBridge.Triggers.HitEvent.cpp:159`

폴백 모드에서 CorpseHitFilter는 죽은 대상에 대한 첫 적중을 처치 일격으로 보고 통과시킵니다. 그래서 칼라미티 발동 주문, 화상 DoT, 동료가 죽인 시체를 플레이어가 처음 때리면 모든 온힛 발동이 돌고 쿨다운이 소모됩니다. 시체에 노출과 피드백도 걸립니다. 또 기록이 30초 뒤에 만료되고 로드 때 비워지며 32칸 링이라 큰 전투에서 밀려납니다. 그 결과 35초 뒤 시체에 맞은 강타·화살이나 로드 직후 시체 첫 적중에서 얼음 형상(행운 적중 15%, ICD 10초) 같은 온힛이 다시 터집니다. CHANGELOG 2.3.0 Fixed에는 '폴백 경로에서도' 시체 적중이 발동하지 않는다고 적혀 있습니다.

**고칠 방향:** '처음 본 시체 적중'이 아니라 사망 시점을 기준으로 받습니다. 킬러 일치만으로는 부족합니다. 화상 같은 발동 DoT 처치도 killer가 플레이어로 나오기 때문입니다. HandleDeathEvent에서 플레이어 소유가 아닌 killer 때문에 조기 반환하기 전(DeathEvent.cpp:92)에, 모든 사망(dead=false 시작 포함)에 대해 (죽는 FormID, StatusClockNowMs())를 기록합니다. 기록은 나이 기준으로 내보내는 작은 맵(몇 초면 충분)에 둡니다. 폴백 outgoing 경로에서는 사망 직후 짧은 창(약 200ms) 안에 온 적중만 처치 일격으로 받습니다. 또는 dead=false 시점에 저장한 HitDataUtil::MakeHitContentSignature(GetLastHitData)와 내용이 같은 적중만 받습니다. 사망 기록이 없는 시체 적중(로드 이전에 죽은 시체 포함)은 거부합니다. 로드 뒤 '미리 채우기'는 시점을 알 수 없어 쓸 수 없습니다. 고정 30초 만료와 32칸 링은 없애거나 보조 장치로만 남기고, CheckMemoryExpires 테스트를 맞춰 고칩니다. 먼저 처치 일격에서 TESDeathEvent와 TESHitEvent가 큐에 쌓이는 순서를 확인합니다. (메아리 관련 주장은 기각: ProcessEchoStrikeHit가 이미 죽은 대상을 거부합니다.)

### L2. disableTrapSystemTick=true가 활 당기기 속도(독수리 눈 접미, 제퍼)까지 조용히 꺼 버림

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/main.cpp:112`, `skse/CalamityAffixes/src/TrapSystem.cpp:41`

SyncBowDrawSpeed를 부르는 곳은 TrapSystem.cpp:41-42의 Tick() 하나뿐이고, 그 태스크는 TrapSystem 워커만 올립니다. main.cpp:112-116은 disableTrapSystemTick이 켜져 있으면 TrapSystem::Install()을 건너뜁니다. 덫 문제를 피하려고(또는 그렇게 안내받아) 이 설정을 켠 사용자는 독수리 눈 접미와 제퍼 룬워드가 툴팁과 패널에는 '활 속도 +X%'로 보이지만 WeaponSpeedMult는 바뀌지 않아 활이 평소 속도로 당겨집니다.

**고칠 방향:** 7b4e82a에서 설치 게이트를 넣은 이유를 지킵니다. 크래시를 추적하는 사용자가 덫 워커 스레드 자체를 원인에서 빼고 싶을 때 쓰는 설정이라는 점입니다. 다수안: main.cpp에서 TrapSystem::Install()을 항상 호출합니다. 워커 깨우기 조건에서는 플래그가 켜져 있으면 HasActiveTraps()를 false로 취급하고, WantsBowDrawSpeedSync()로는 계속 깨어나게 합니다. Tick()에서는 TickTraps()를 플래그가 꺼졌을 때만 부르고, SyncBowDrawSpeed()는 항상 부릅니다. TickTraps도 Traps.cpp:30에서 이미 플래그를 보고 상태를 비운 뒤 반환합니다. 로그 문구는 '덫 틱은 꺼짐, 워커는 활 속도용으로 동작'처럼 바꿉니다. 스레드를 아예 띄우지 않는 진단 의미를 그대로 두고 싶다면, 활 동기화를 항상 설치되는 EchoStrikeSystem 워커로 옮깁니다.

### L3. '검증 간격' 안전망 재검사가 훅 모드에서는 주기적으로 돌지 않고 처치 때만 돎

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.HitEvent.cpp:102`

기본 훅 모드에서는 MCM의 '초 단위 (안전망 재검사)' 옵션이 처치 없는 전투나 비전투 중에 재검사를 하지 않습니다. 장착이나 어픽스 변경을 놓치면 설정 간격과 상관없이 다음 처치나 재장착 때까지 발동이 틀린 채로 남습니다. 9227fe2에서 훅 경로 쪽 호출이 없어진 뒤 HitEvent.cpp의 호출만 `!hookRouted`로 막혀 있습니다.

**고칠 방향:** EventBridge.Triggers.HitEvent.cpp:102-105에서 MaybeResyncEquippedAffixes(now)를 감싸는 `if (!hookRouted)` 게이트를 없애 모든 적중에서 부를 수 있게 합니다. 9227fe2 이전 동작으로 돌아가는 것입니다. 이 핸들러는 이미 메인 스레드의 지연 SKSE 태스크 드레인에서 상태 락을 잡고 돌고, 간격 검사는 함수 안에서 합니다.

### L4. 발동 확률 패널티 대상 목록이 곳곳에서 어긋남: 메아리 일격·전염은 패널티 단계에 세지만 깎이지 않고, 파멸 표식 툴팁에는 조정된 확률이 없음

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.ActiveCounts.cpp:191`, `skse/CalamityAffixes/src/EventBridge.Loot.TooltipResolution.cpp:458`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.PanelState.cpp:127`

전염과 다른 발동 접두가 하나씩 달린 갑옷에서 다른 접두는 80%로 떨어지는데 전염은 100%로 남습니다. '한 아이템에 발동 접두 2개 → 각각 80%' 규칙과 맞지 않습니다. 파멸의 낙인과 다른 발동 접두가 달린 무기는 런타임이 20%로 굴리는데, 툴팁은 기본 25%만 보여 주고 조정 줄이 없습니다. rc3(0799eb3)에서 kDoomMark를 ActiveCounts.cpp와 PanelState.cpp에는 넣었지만 TooltipResolution.cpp:458-467에는 넣지 않았습니다. (회의론자 1명은 메아리 일격/전염 부분이 의도된 패턴일 수 있다고 봤고, 다른 1명은 이 시나리오가 2.3.0 이전에 만든 아이템에서만 생긴다고 했습니다.)

**고칠 방향:** '표준 트리거 확률 경로' 액션 집합을 constexpr 헬퍼 하나로 모읍니다. 예: ProcChancePolicy.h의 IsProcPenaltyEligible 옆에 IsStandardTriggerProcLaneAction(ActionType)을 두고 CastSpell, CastSpellAdaptiveElement, SpawnTrap, DoomMark, EchoStrike, SpreadStatus에 true를 반환하게 합니다. 지금 목록을 따로 두는 세 곳(ActiveCounts.cpp:191-195의 usesStandardTriggerProcLane, PanelState.cpp:127-131, TooltipResolution.cpp:458-467)이 모두 이 헬퍼를 쓰게 합니다. 최소 수정은 TooltipResolution.cpp의 usesStandardTriggerProcLane에 `ActionType::kDoomMark`를 넣는 것입니다.

### L5. 시체 폭발의 '시체 최대 체력 %'를 실제 시체가 아닌 NPC 기본 레코드에서 읽음

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Corpse.cpp:32`

죽음의 장작(12 + 시체 최대 체력 6%)이나 소환 장작을 레벨형 산적 두목, 드래곤, 드레모라 군주 소환수에 쓰면, % 부분이 기본 NPC 레코드 체력으로 계산됩니다. 플레이어가 실제로 깎은 체력 막대와 크게 다를 수 있어 폭발이 설명보다 약하거나 강해집니다.

**고칠 방향:** ResolveSafeCorpseMaxHealth에서 Status.cpp:471처럼 행위자 값을 먼저 읽습니다: `if (auto* avOwner = a_corpse->AsActorValueOwner()) { const float v = avOwner->GetPermanentActorValue(RE::ActorValue::kHealth); if (v > 0.0f) return v; }`(또는 Policy.cpp처럼 skyrim_cast<RE::ActorValueOwner*>). 결과가 0 이하일 때만 기존 GetActorBase() 경로로 돌아갑니다.

### L6. 물리→원소 전환이 칼라미티 치명 보너스로 늘어난 몫은 전환하지 않음

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Special.cpp:196`

화염 주입 100%와 치명 피해 +50% 접미를 같이 쓰면, 100 피해 치명타가 훅에서 150이 됩니다. 그런데 화염으로 전환되는 것은 약 100이고 50은 물리로 들어갑니다. 어픽스 문구는 '물리 100%를 화염으로 전환'입니다.

**고칠 방향:** 전환을 치명 배율이 반영된 물리 피해 기준으로 계산합니다. EvaluateConversion에 float a_physicalScale 인수(기본 1.0)를 추가합니다. AdjustDamageAndEvaluateSpecials에서 critMult가 유한하고 1보다 크면 그 값을 넘기고, Special.cpp에서 physicalDealt = max(0, physicalDamage - resistedPhysicalDamage)에 이 배율을 곱한 뒤 전환합니다. 다른 방법으로는 HitData는 물리 비율을 구하는 데만 쓰고, 최종 훅 피해에 그 비율을 곱해 전환할 양을 정하는 방식이 있습니다.

### L7. 덫이 거는 저항 감소가 노출 장부를 거치지 않아 노출 감소와 겹치고, 혼돈의 룬과 레코드를 공유해 장부가 어긋남

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Traps.cpp:491`, `affixes/modules/spec.root.json`

타르 역병 덫(방어도 -150, 장부 밖)을 밟은 적에게 죽음의 표식(노출, 방어도 -200)이나 힘(노출, -100)이 걸리면, 계획은 더 강한 항목이 없다고 보고 시전해 -350이나 -250이 됩니다. 덫이 노출 밖에 있는 것과 타르 역병이 겹치는 것은 의도된 설계입니다. 남는 문제는 공유 레코드입니다. 혼돈의 룬이 덫으로 거는 기본 150짜리 분쇄(Sunder)가 같은 주문·같은 시전자라서 엔진에서 죽음의 표식의 200 인스턴스를 대체하는데, 장부에는 200이 남습니다. 저장·로드 뒤 재구성은 그 덫 분쇄를 노출로 기록하므로, 같은 발동 조합이 로드 전후에 다르게 동작합니다.

**고칠 방향:** 덫의 추가 주문을 노출 경로로 보내지 마십시오. 덫은 설계상 상태 시스템 밖에 있습니다. 공유 레코드만 나눕니다. chaos_rune 전용 Sunder와 Fragile SPEL/MGEF 레코드를 만들어 spec.root.json의 keywords.appendedRecords 맨 끝에만 추가하고(FormID가 위치 기반), chaos_rune의 추가 주문이 그 레코드를 가리키게 합니다. 반대로 death_mark/spell_breach 쪽에 새 레코드를 주어도 됩니다.

### L8. 전염 사전 검사가 주변 적을 확인하지 않아, 근처에 아무도 없는 처치에서 3초 쿨다운을 써 버림

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Dispatch.cpp:114`

노출이나 화상이 걸린 외딴 궁수를 죽이면 퍼뜨릴 대상이 없어도 전염 쿨다운이 돕니다. 3초 안에 무리 속 적을 죽이면 전염이 아직 쿨다운이라 아무것도 퍼지지 않습니다. 정작 설명에 맞는 건 두 번째 처치입니다.

**고칠 방향:** CanExecuteAction의 kSpreadStatus 경우(Dispatch.cpp:114)에서, 값싼 HasSpreadableStatus 확인 뒤 실행 경로와 같은 규칙으로 살아 있는 적이 범위 안에 있는지도 요구합니다. 예: `&& !CollectNearbyHostiles(a_owner, a_target, action.spreadRadius, 1u).empty()`. 규칙은 소유자와 피해자 제외, 죽은 대상 제외, IsHostileEffectTarget(owner, a), 거리 제곱 ≤ spreadRadius 제곱입니다. spreadRadius > 0도 요구합니다. 첫 대상을 찾으면 바로 멈추는 전용 헬퍼를 둬도 됩니다.

### L9. 자기 버프 '종류'를 주 능력치만으로 묶어서, 변태(에테르화)를 투명화로 취급하고 위력 0 투명화는 항상 짐

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Cast.cpp:160`, `affixes/modules/keywords.affixes.runewords.json`

변태의 3초 에테르 무적 중에 소환수나 동료가 적을 죽이면, 고요한 걸음이 처치 시 발동해 에테르 효과를 해제하고 평범한 투명화를 겁니다. 그래서 플레이어가 다시 피해를 받을 수 있습니다. 반대로 그림자 장막, 고요한 걸음, 수수께끼 투명화가 걸려 있는 동안에는 변태와 은신 룬워드가 발동하지 못합니다.

**고칠 방향:** 가장 간단한 것은 데이터 수정입니다. affixes/modules/keywords.affixes.runewords.json에서 CAFF_MGEF_RW_METAMORPHOSIS_SHIFT의 actorValue를 None으로 바꿔 바닐라 VoiceMakeEthereal(av=-1)과 맞춥니다. 같은 에디터 ID의 내용만 바뀌므로 FormID는 그대로입니다. 그러면 IsReplaceableSelfBuffValue가 이 효과를 제외하고, SelfBuffPlan.cast 기본값이 true라 계속 시전되며 해제되지 않습니다. 코드로 고친다면 CollectCalamitySelfBuffs와 PlanSelfBuffCast(발생 효과 루프와 자기 효과 루프 모두)에서 `baseEffect->GetArchetype() == RE::EffectSetting::Archetype::kEtherealize`인 효과를 교체 불가로 취급합니다.

### L10. disableHealthDamageRouting=true가 MagicTarget::AddTarget 아군 마법 훅까지 설치하지 않음

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/main.cpp:106`

호환성 때문에 체력 피해 라우팅을 끄고 TESHitEvent로 돌아가면, summonfix3의 AddTarget 블록도 조용히 설치되지 않습니다. 그 결과 칼라미티 소환수의 적대 마법과 적대 전용 시전이 동료와 중립 NPC에게도 걸립니다.

**고칠 방향:** Hooks::Install을 둘로 나눕니다. InstallHealthDamageHooks()(ActorHandleHealthDamageHook::Install만)와 InstallFriendlyMagicHooks()(FriendlyMagicTargetHook<> 세 개)입니다. main.cpp에서 InstallFriendlyMagicHooks()는 항상 부르고, !IsHealthDamageRoutingDisabled 게이트는 InstallHealthDamageHooks()에만 둡니다.

### L11. proc-dispatch 맵 정리가 dispatchedHitContent까지 지워, 오래된 적중이 한 번 다시 처리될 수 있음

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/Hooks.Dispatch.cpp:434`

맵이 512개(kProcDispatchMaxEntries)를 넘으면 10초 이상 지난 쌍(kProcDispatchCleanupAge)의 레코드를 통째로 지우는데, dispatchedHitContent도 같은 레코드에 있습니다. 긴 세션에서 정리가 일어난 뒤 10초 넘게 전에 칼로 맞은 적에게 주문을 쓰면, 그 옛 칼 HitData가 한 번 새 적중으로 통과해 온힛 어픽스와 치명 시전이 발동합니다. '얼마나 오래전이든'이라는 보장과 맞지 않습니다. 드물고 영향도 작습니다.

**고칠 방향:** 정리는 유지하되, 아직 오래된 적중을 막고 있는 레코드는 지우지 않습니다. Hooks.Dispatch.cpp:435의 루프에서 지우기 전에 TESForm::LookupByID<RE::Actor>(key >> 32)로 대상을 찾습니다(훅이 게임 스레드에서 돌아 안전합니다). 대상이 없거나, 대상의 현재 HitData의 MakeHitContentSignature가 저장된 dispatchedHitContent와 다를 때만 지웁니다. 다른 방법은 내용 서명을 타이밍 레코드와 따로 두는 것입니다. 쌍→마지막 내용을 담는 별도 맵을 더 큰 상한(예: 4096)과 LRU 방식으로 두고 ClearDispatchRuntimeState에서 비웁니다.

### L12. 발동 출처 가드가 처치 트리거에는 적용되지 않는데, 주석은 발동 처치가 처치 연쇄를 시작하지 않는다고 약속함

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Status.cpp:698`, `skse/CalamityAffixes/src/EventBridge.Triggers.DeathEvent.cpp:29`

파멸 폭발, 그림자 권투가 메아리, 덫이 적을 죽이면 약 1초 뒤 TESDeathEvent(dead=true)가 오고, 이때 처치 시 어픽스가 발동합니다. 가드는 CastHostileOnlySpellImmediate를 감싸는 문법적 ScopedProcDepth뿐이라 지연된 사망 이벤트를 막지 못합니다. 처치 발동 주문이 두 번째 적을 죽이면 또 연쇄하고, 이를 막는 건 트리거 발동 예산과 ICD뿐입니다. DoT와 덫 처치가 이미 처치 어픽스를 발동하는 것과는 일관됩니다.

**고칠 방향:** 먼저 정책을 정합니다. 다수안은 동작을 그대로 두고 주석만 고치는 것입니다. Status.cpp:696-698, EchoStrike.cpp, DeathEvent.cpp:29-30의 주석을 '이 가드는 동기 재진입(폭발이 온힛 어픽스를 발동하거나 새 적중으로 라우팅되는 것)만 막는다. 폭발로 생긴 처치는 지연된 TESDeathEvent(dead=true)가 오면 플레이어 처치로 센다'로 바꿉니다. 발동 처치를 정말 막으려면, 발동 시전·폭발 때(또는 훅이 g_inProcDispatch/예상된 메아리 중 치명 피해를 볼 때) (대상 FormID, 만료) 장부를 기록하고 HandleDeathEvent가 그 대상의 ProcessTrigger(kKill)을 건너뛰게 합니다.

### L13. 예산에 막힌 시체 폭발 후보가 다른 후보로 넘어가지 않고 그 시체의 폭발을 통째로 취소함

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Corpse.cpp:270`

죽음의 장작 접두와 임종의 숨결 룬워드를 단 무기로 20명 무리를 상대합니다. t=0에 죽음의 장작 폭발이 나고 1초 DoT로 두 적이 t≈1.0에 죽습니다(DoT 처치는 훅 경로 재시도가 없어 TESDeathEvent 한 번뿐). 임종의 숨결이 50%를 통과해 선택되면, 직전 폭발이 0.5초 안이라 깊이 1 ≥ maxChainDepth 1로 kChainDepthExceeded가 되거나, 같은 1초 창에 이미 폭발이 있어 속도 제한에 걸립니다. 어느 쪽이든 그 시체는 폭발하지 않습니다. 죽음의 장작(깊이 1 < 5)이었다면 허용됐을 폭발입니다. 역병, 잿불 장작도 같습니다.

**고칠 방향:** SelectBestCorpseExplosionAffix가 확률과 ICD를 통과한 후보를 모두 돌려주게 합니다. 각 후보에 인덱스, 피해, 우선순위, perTargetKey, usesPerTargetIcd를 담고, IsPreferredCorpseExplosionCandidate로 정렬합니다(동률이면 설정 순서). ProcessCorpseExplosionAction에서는 이 순서대로 TryConsumeCorpseExplosionBudget을 불러 처음 허용되는 후보를 씁니다. 그 후보의 per-target ICD만 기록합니다.

### L14. 문서가 노출을 '저항 감소'로만 정의하지만 방어도·매지카 재생 감소도 노출이라 겹치지 않음. CHANGELOG에는 출혈 10~400 제한이 빠짐

- 영역: 전투 C++ · 확인 · docs
- 위치: `docs/EFFECTS_EN.md:32`, `docs/PREFIX_EFFECTS.md:37`, `CHANGELOG.md:39`, `CHANGELOG.md:41`, `tools/gen_effects_en_doc.py:230`, `tools/gen_prefix_doc.py:195`

죽음의 표식(방어도 -200)을 쓰는 플레이어가 문서에는 저항만 상한이 있다고 적혀 있어 룬워드 힘이나 돌로 방어도 감소가 더해질 거라 기대합니다. 실제로는 대상에 죽음의 표식이 있으면 힘의 발동이 조용히 건너뛰어집니다. 또 체력 20000짜리 보스에서 CHANGELOG의 '최대 체력 3%'는 출혈 600을 뜻하지만 실제로는 400이 들어갑니다.

**고칠 방향:** 생성된 문서가 아니라 생성기를 고칩니다. tools/gen_effects_en_doc.py:230의 Exposed 정의를 'Exposed: a lowered resistance or armor (Mist also lowers magicka regen). Only the strongest Calamity Exposed on each value applies.'로 바꿉니다. tools/gen_prefix_doc.py:195는 '노출: 저항·방어도 감소. 같은 능력치에는 가장 강한 칼라미티 노출 하나만 적용'처럼 바꿉니다. 그 다음 EFFECTS_EN.md와 PREFIX_EFFECTS.md를 재생성합니다. CHANGELOG.md:39도 같은 뜻으로 고치고, CHANGELOG.md:41의 출혈 줄에는 '(10~400)'을 붙입니다. 참조 문서에는 이미 있으니 이건 선택입니다.

### L15. 칼라미티 소환 발동이 우선순위 없이 플레이어 소환 슬롯을 같이 써서, 6초짜리 화염 정령이 플레이어의 60초 소환수나 더 강한 소환수를 몰아냄

- 영역: 전투 C++ · 확인 · balance
- 위치: `affixes/modules/keywords.affixes.core.json:432`, `skse/CalamityAffixes/src/EventBridge.Actions.Dispatch.cpp`

직접 부른 드레모라 군주(60초)가 있는 소환술사가 화염 정령 무기(행운 적중 36%, ICD 14초)를 쓰면, 전투 중 약 15초마다 6초짜리 정령이 소환되면서 드레모라 군주가 해제됩니다. 5초 뒤 정령이 자폭하면 소환수가 하나도 남지 않습니다. 드레모라 계약 군주 자신의 공격이 같은 장비의 늑대 영혼 접두를 발동해 군주를 몰아내기도 합니다. 아이템 문구 어디에도 발동이 현재 소환수를 대체한다는 말이 없습니다. (회의론자 1명은 이것이 의도된 설계라 문서만 고치면 된다고 봤습니다.)

**고칠 방향:** CanExecuteAction의 kCastSpell 경우에 소환 검사를 넣어, 확률과 ICD를 쓰기 전에 실행되게 합니다. 대상은 IsSummonCreatureSpell(action.spell)(또는 IsSummonLikeSpell/kSummonCreature)이고 자기 대상(applyToSelf)이 아닌 액션입니다. (a) 적중의 aggressor가 소유자가 아니라 지휘 중인 소환수이면 건너뜁니다. 소환수 자신의 적중으로 그 소환수를 대체하지 않게 하는 것입니다. (b) 플레이어가 지휘 행위자 상한에 차 있고, 현재 소환수 중 하나가 새 주문의 지속시간보다 오래 남거나 더 강하면 건너뜁니다. 또 문구에 발동 소환이 플레이어의 소환 슬롯을 쓴다는 점을 적습니다.

### L16. MCM에서 칼라미티를 끄거나 꺼진 상태로 저장을 불러오면, 살아 있는 칼라미티 소환수의 보호가 영구히 사라짐

- 영역: 전투 C++ · 확인 · save-data
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.ActiveCounts.cpp:31`

칼라미티 아트로나크가 나와 있는 상태에서 MCM으로 칼라미티를 껐다 켜거나, 저장하고 끈 뒤 다시 저장·로드하면 그렇습니다. 소환수는 최대 60초까지 살아 있지만 보호가 없어 플레이어와 동료의 공격이 피해를 주고, 광역 공격이 중립 NPC와 동료를 다치게 할 수 있습니다. 칼라미티를 다시 켜도 등록이 복구되지 않습니다. DeactivateRuntimeState가 소환 등록부(g_registeredSummons)까지 지우고, OnPostLoadGame이 g_loadedSummonFormIDs를 쓰기 전에 비울 수 있기 때문입니다.

**고칠 방향:** ClearHostileEffectGuardRuntimeState를 둘로 나눕니다. (1) 일시 상태 초기화: g_summonRuntimeGeneration 증가, 시전 범위 깊이·소유자, g_pendingSummonCasts, g_explosionDamageWindows, g_summonRefreshScheduled. DeactivateRuntimeState는 이것만 부르고 g_registeredSummons와 g_loadedSummonFormIDs는 유지합니다. (2) 전체 초기화: 등록부까지 지우며, PreLoad/Revert에서만 부릅니다. DeactivateRuntimeState의 Hooks::ClearRuntimeState 호출은 테스트가 확인하므로 남기고, 그 안에서 일시 초기화만 하도록 연결합니다. 또 RestoreCalamitySummonsAfterLoad가 OnPostLoadGame이 비우기 전에 g_loadedSummonFormIDs를 쓰도록 순서를 맞춥니다.

### L17. 100ms 단일 슬롯 중복 필터가 진짜 적중을 버림: 같은 베이스 무기 두 자루나 다중 화살이 한 대상에 동시에 맞으면 온힛과 메아리가 한 번만 생김

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.Runtime.cpp:208`

Runtime.cpp:208-234는 {outgoing, aggressor, target, source}를 100ms 창의 전역 슬롯 하나와 비교합니다. source는 베이스 TESObjectWEAP FormID(화살은 활)입니다. 그래서 같은 베이스(어픽스는 다름)의 데이드릭 단검 두 자루로 쌍수 강타를 넣으면 첫 칼날만 온힛을 굴리고 그림자 메아리를 얻습니다. 철 단검과 강철 단검이면 둘 다 셉니다. 근거리 3발 다중 사격도 한 번만 굴립니다.

**고칠 방향:** DuplicateHitKey/LastHitKey에 content 필드(기본 0)를 추가합니다. 훅 경로(ProcessOutgoingHealthDamageHit, ProcessIncomingHealthDamageHit)에서는 HitDataUtil::MakeHitContentSignature(a_hitData)를 넣습니다. TESHitEvent 폴백은 0으로 둬서 같은 이벤트 반복을 막는 기존 source 기준 중복 제거를 유지합니다. 키가 같고 content도 같을 때(또는 content가 0일 때)만 막으면, 두 지연 태스크가 같은 최신 적중을 본 경우는 계속 합쳐집니다. 한 발의 투사체·폭발 묶음까지 따로 세는 게 걱정되면, content를 kMeleeAttack이 있고 kExplosion이 없는 근접 무기 적중에만 넣는 방법(회의론자 1명 제안)도 있습니다.

### L18. 칼라미티 뒤에 HandleHealthDamage를 훅하는 모드가 있으면 훅 경로와 TESHitEvent 폴백이 둘 다 같은 적중을 처리함

- 영역: 전투 C++ · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/Hooks.cpp:178`

Hooks.cpp:167-189의 IsHooked는 vtbl[0x104]가 ThunkActor/ThunkCharacter/ThunkPlayerCharacter와 정확히 같을 때만 true입니다. CalamityAffixes.dll보다 늦게 로드되는 전투 플러그인이 kDataLoaded 이후 이 슬롯을 훅하고 이전 포인터로 이어 주면, 우리 thunk는 계속 돌지만 hookRouted는 false가 됩니다. 그 결과 플레이어의 칼 적중이 두 경로에서 모두 평가됩니다. 마법사의 갑옷이 피격마다 매지카를 두 번 깎고, 마법부여 무기는 한 번 휘두를 때 온힛을 최대 세 번 굴리고, 화염탄이 칼 발동을 다시 터뜨립니다. 다른 플러그인이 같은 슬롯을 뒤에 훅할 때만 생기고, 이 사용자의 모드 목록에서는 그런 흔적이 없었습니다.

**고칠 방향:** vtable 일치 검사는 주 판정으로 남겨 정상 훅 상황의 동작을 그대로 둡니다. 적중마다 타임스탬프로 판정하는 방식으로 완전히 바꾸지는 마십시오. 막힌 적중이나 0 피해 적중은 HandleHealthDamage를 부르지 않아 훅이 멀쩡해도 폴백으로 새고, 지연 태스크의 healthDamageHookLastAt은 인라인 TESHitEvent 폴백보다 늦게 기록되기 때문입니다. 대신 vtable 클래스마다 atomic 'thunk 호출됨' 플래그를 하나 둡니다. ThunkActor, ThunkCharacter, ThunkPlayerCharacter가 진입할 때 플래그를 세웁니다. IsHooked는 슬롯이 우리 thunk와 다르더라도 우리 thunk가 호출된 적이 있으면(다른 모드가 우리에게 이어 주는 경우) 라우팅된 것으로 봅니다. 슬롯이 바뀐 것을 발견하면 경고 로그를 남깁니다.

### L19. 로드할 때 정리 단계가 현재 설정으로 해석되지 않는 어픽스 토큰을 전부 영구 삭제함: ESP가 빠지거나 버전이 어긋나면 세이브의 어픽스가 지워짐

- 영역: 아이템·저장 C++ · 확인 · save-data
- 위치: `skse/CalamityAffixes/src/EventBridge.Serialization.Load.Records.inl:637`, `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp:155`, `skse/CalamityAffixes/src/EventBridge.Config.cpp:134`, `skse/CalamityAffixes/src/EventBridge.Config.AffixParsing.cpp:527`, `skse/CalamityAffixes/src/PluginEditorIds.cpp:58`, `skse/CalamityAffixes/src/main.cpp:100`

FinalizeLoadedSerializationState(Records.inl:635-639)는 레지스트리가 비어 있지 않으면 로드할 때마다 SanitizeAllTrackedLootInstancesForCurrentLootRules를 돌린다. 이때 ShouldKeepTrackedLootAffixToken(Assign.cpp:155-163)은 affixIndexByToken에 없는 토큰에 false를 돌려준다. CalamityAffixes.esp가 체크 해제되거나, 이름이 바뀌거나, 병합되면 PluginEditorIds::Load가 빈 색인으로 끝난다. MO2 충돌로 옛 ESP가 이긴 경우도 같다. 그러면 JSON 어픽스 대부분이 InitializeAffixFromJson에서 실패해 건너뛰어진다. 바닐라 주문으로 합성된 룬워드 어픽스가 레지스트리를 채우므로 '비어 있음' 검사는 통과하고, 'no affixes loaded' 오류도 나오지 않는다. 결국 모든 추적 아이템에서 접두·접미·룬워드 토큰과 재련 횟수가 지워지고, 슬롯이 빈 인스턴스는 추적에서 빠진다. 다음 저장이나 자동 저장 때 이 상태가 확정되어, ESP를 고쳐도 돌아오지 않는다. 정상 설치에서는 일어나지 않고, 모르는 토큰 정리 자체는 은퇴한 어픽스를 걷어내려는 의도된 동작이다. 2.3.0에서 2.2.x로 되돌리면 contagion, doom_brand, exploit_weakness가 장비에서 지워지는 것도 같은 경로다.

**고칠 방향:** 모르는 토큰 정리는 유지하되, 실행 조건을 '레지스트리가 비어 있지 않음'에서 '설정이 믿을 만함'으로 바꾼다.
1) PluginEditorIds에 색인 생성 여부를 알려 주는 플래그를 추가한다(예: `bool PluginEditorIds::IsIndexed()`, LookupModByName 성공과 파싱 성공일 때만 true). 레지스트리를 채우는 BuildConfigDerivedAffixState에서는 SynthesizeRunewordRuntimeAffixes 이전에 JSON에서 파싱한 어픽스 수를 따로 기록한다.
2) FinalizeLoadedSerializationState(Records.inl:637)와 LoadConfig.postIndex 정리(Config.cpp:134)는 색인이 비었거나 JSON 파싱 수가 0이면 건너뛴다. 이때 instanceAffixes와 IRFC(selectedReforgeCounts)는 그대로 두어 Save가 원래대로 다시 쓰게 하고, 경고를 한 번 남긴다. 알려진 접미에 대한 stripTrackedSuffixSlots 규칙은 계속 적용한다.
3) 'no affixes loaded. Is the generated CalamityAffixes plugin enabled?' 오류도 합성 이전 개수를 기준으로 판정해서, ESP가 없는 상황이 가려지지 않게 한다.
4) 모르는 토큰을 무조건 휴면으로 두지는 않는다. 진짜로 제거한 어픽스의 토큰이 슬롯에 계속 남게 된다. 항목별 파싱 실패 카운터로 정리를 막는 방안은 평가자 의견이 갈렸다. 선택 레코드(상시 주문 등)가 빠져도 정리가 꺼져 버린다는 반대가 있으므로, 쓰려면 실패한 어픽스 ID만 MakeAffixToken으로 집합에 넣어 그 토큰만 보존하는 식으로 좁힌다.
5) v2.3.0 CHANGELOG의 롤백 문구에 '2.2.x로 되돌리면 contagion·doom_brand·exploit_weakness가 장비에서 지워진다'를 한 줄 추가한다.

### L20. 런타임을 끈 상태에서 버리거나 주운 어픽스 아이템은 키가 옮겨지지 않아, 다음 로드나 재활성화 때 어픽스가 지워짐

- 영역: 아이템·저장 C++ · 확인 · save-data
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.cpp:335`, `skse/CalamityAffixes/src/EventBridge.Loot.cpp:399`, `skse/CalamityAffixes/src/EventBridge.Loot.cpp:536`, `skse/CalamityAffixes/src/EventBridge.Triggers.ModCallback.Handlers.cpp:74`, `skse/CalamityAffixes/src/EventBridge.Serialization.Lifecycle.cpp:208`

HandleContainerChangedEvent는 `!_runtimeSettings.enabled`이면 Loot.cpp:335에서 바로 return한다. 그래서 플레이어→월드 경로(1a: RemapInstanceKey(dropSourceKey, detachedWorldKey), NotePlayerDrop), 상자 보관 기록(1b), 월드→플레이어 재부착 경로(3: ConsumePlayerDropPickupInstanceKey와 RemapInstanceKey(detachedWorldKey, pickupKey))가 모두 실행되지 않는다. MCM에서 Calamity를 끄고 어픽스 검을 버리거나 무기 거치대에 올리면(거치대도 드롭 경로를 탄다), 상태가 (player, uid) 키에 그대로 남는다. 이후 저장·재로드하거나 다시 켜면 PostLoad 또는 활성화 시점의 PruneOrphanedPlayerInstanceKeys가 이 상태를 지운다. 검은 이름만 남고 어픽스와 룬워드 진행이 사라지며, 되돌릴 방법이 없다. 켠 상태에서 버리고 끈 상태에서 주운 경우도, 월드 레퍼런스가 삭제될 때 같은 결과가 된다.

**고칠 방향:** HandleContainerChangedEvent 맨 위에는 `_configLoaded`와 a_event, 플레이어 null 검사만 남긴다. 다음 소유권 처리는 enabled와 상관없이 항상 실행한다.
- 1a: detached world 키로 옮기기, dropSourceKey==0 정리 작업, NotePlayerDrop
- 1b: playerContainerStash 기록
- 3: ConsumePlayerDropPickupInstanceKey와 pickup 키로 옮기기
- 3b: stash 소비
`_runtimeSettings.enabled` 검사는 아이템을 주는 두루마리 보존 분기와 마지막 ProcessLootAcquired(루트·화폐·조각 굴림)에만 둔다. 두루마리 분기는 소유권 경로 아래로 옮기거나 자체 enabled 검사를 준다. 드롭과 줍기 분기는 반드시 함께 바꾼다. 드롭만 옮기고 줍기를 안 옮기면, 끈 상태에서 주운 아이템의 상태가 detached 키에 남아 영영 다시 붙지 않는다. OnPostLoadGame과 활성화 토글의 정리는 그대로 둔다. 드롭이 항상 키를 옮기게 되면 그 정리가 안전해지고, v1.2.24 UID 재활용 오염을 막는 역할도 유지된다. 로드 때 정리를 건너뛰는 대안은 쓰지 않는다. 다시 켤 때 정리가 돌기 때문에 충분하지 않다. runtime_gate_store_checks_loot_policy.cpp(:783 등)의 소스 문자열 검사를 함께 갱신하고, enabled return이 다시 드롭·줍기 분기 위로 올라가지 않도록 고정하는 테스트를 추가한다.

### L21. 플레이어가 버린 어픽스 아이템을 동료나 NPC가 주우면 상태를 잃음 (월드→NPC 이전 경로 없음)

- 영역: 아이템·저장 C++ · 확인 · save-data
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.cpp:533`, `skse/CalamityAffixes/src/EventBridge.Loot.cpp:472`, `skse/CalamityAffixes/include/CalamityAffixes/InstanceKeyTransferPolicy.h:54`

플레이어가 아이템을 버리면 상태가 MakeDetachedWorldInstanceKey(refFormID)로 옮겨진다(Loot.cpp:472). 이를 되돌리는 경로는 월드→플레이어 분기(Loot.cpp:537-598) 하나뿐이다. oldContainer=월드, newContainer=NPC인 이벤트는 `newContainer != playerId`에서 return된다. 그래서 동료에게 '저거 주워' 하거나 드라우그 무장해제 함성으로 떨어진 무기를 NPC가 주우면, 상태는 detached 키에 남는다. 나중에 아이템이 플레이어에게 돌아와도 어픽스가 없다.

**고칠 방향:** HandleContainerChangedEvent에서 `newContainer != playerId` return 앞에 분기를 하나 추가한다. 조건은 `oldContainer == kWorldContainer && newContainer != 0 && newContainer != playerId`이고, 플레이어 줍기(step 3)와 같은 방식으로 처리한다.
1) 원본 키는 guard에 기록된 드롭 키나 MakeDetachedWorldInstanceKey(referenceFormID)를 쓰되, IsDetachedWorldReferenceMatch로 검증한다. ConsumePlayerDropPickupInstanceKey를 대상과 무관하게 동작하는 변형으로 일반화해서, 해당 refHandle의 reroll-guard 항목을 소비한다.
2) 원본이 추적 중이고 uniqueID가 0이 아니면 대상 키는 MakeInstanceKey(newContainer, uniqueID)다. 대상에 이미 실물 상태가 있으면 Loot.cpp:574-582처럼 둘 다 버린다. 없으면 RemapInstanceKey(detached, target)를 부른다.
3) uniqueID가 0이면 detached 상태를 버리지 말고 로그만 남긴다. 이후 UniqueIDChange 이벤트나 NPC→플레이어 이전이 정확 키 규칙으로 이어받는다. referenceUniqueKey의 소유자 비트가 newContainer와 같을 때만 그것을 쓴다.
4) 이 이벤트에서는 루트를 굴리지 않는다. remap은 지연 form-delete 처리보다 먼저, 같은 FIFO 순서에서 일어나야 한다. NPC 소유 키는 PruneOrphanedPlayerInstanceKeys가 정리하지 않으므로 코세이브가 조금 커질 수 있다.

### L22. 플레이어 외 소유자(상인 상자·NPC)의 상태는 정리되지 않고, UID가 다시 쓰이면 kConflict로 양쪽 상태가 모두 버려짐

- 영역: 아이템·저장 C++ · 확인 · save-data
- 위치: `skse/CalamityAffixes/include/CalamityAffixes/InstanceKeyTransferPolicy.h:123`, `skse/CalamityAffixes/include/CalamityAffixes/InstanceKeyTransferPolicy.h:74`, `skse/CalamityAffixes/src/EventBridge.Serialization.Lifecycle.cpp:208`

PruneOrphanedPlayerInstanceKeys는 소유자가 플레이어인 키만 지운다. 상인 상자나 NPC 키는 쌓이기만 한다. 엔진에는 컨테이너별 UID 카운터가 없고, GetNextUniqueID는 현재 그 인벤토리에서 쓰이지 않는 가장 낮은 UID를 고른다. 그래서 어픽스 아이템을 팔고, 상자가 리셋된 뒤 다른 어픽스 아이템을 팔면 옛 UID가 다시 쓰일 수 있다. 그러면 새 아이템이 kConflict에 걸려 원본과 대상 상태가 모두 버려지고, 되사도 어픽스가 없다. 지워지지 않은 오래된 항목은 코세이브에도 계속 쌓인다.

**고칠 방향:** fail-closed 규칙을 전부 없애지는 않는다. test_instance_key_transfer_policy.cpp:192-215는 원본이 미추적이고 대상이 추적 중인 중복 이벤트가 kIgnore로 남을 것을 요구한다. 그래서 '무시할 때 대상을 버리는' 방식은 올바르게 옮겨진 상태를 지워 버린다.
1) kConflict이고 원본이 정확한 owner+UID 키(policyOldTracked, 레거시 item-form 키 아님)일 때 새 결정(예: kReplaceStaleDestination)을 돌려준다. 그러면 HandleUniqueIDChangeEvent는 DiscardInstanceKeyState(newKey) 후 RemapInstanceKey(oldKey, newKey)를 실행한다. 엔진이 방금 그 UID를 빈 값으로 내줬으므로, 그 키에 이미 있던 상태는 정의상 오래된 것이다. 레거시 원본 충돌과 canonical+legacy 모호성 차단은 kConflict로 둔다.
2) 다른 소유자의 인벤토리를 로드 시점에 훑어 '살아 있는 아이템 없음'을 판정하지는 않는다. InventoryChanges가 아직 로드되지 않아 비어 보일 수 있다는 지적이 있다. 원하면 플레이어 외 소유자의 제거 이벤트(newUniqueID==0이고 oldBaseID/oldUniqueID가 유효함)에서 MakeInstanceKey(oldBaseID, oldUniqueID)를 버려 쌓임을 줄인다. 단, 플레이어가 그 키를 갖고 있지 않고 플레이어 드롭 경로가 처리하는 이벤트가 아닐 때만 한다.
3) 상인→상인, 플레이어→상인 덮어쓰기 경우에 대한 정책 테스트를 추가하고, 중복 이벤트 테스트는 그대로 통과해야 한다.

### L23. 한국어 룬워드 상세에 영어 레코드 이름이 나옴: HarvestRecordDisplayNames가 keywords.appendedRecords/appendedMagicEffects를 읽지 않음

- 영역: 아이템·저장 C++ · 확인 · data-text
- 위치: `skse/CalamityAffixes/src/EventBridge.Config.AffixParsing.cpp:433`, `skse/CalamityAffixes/src/EventBridge.Config.AffixParsing.cpp:523`, `skse/CalamityAffixes/src/EventBridge.Config.cpp:182`

recordDisplayNames를 채우는 곳은 HarvestRecordDisplayNames뿐이고, 이 함수는 각 어픽스의 `records`만 읽는다. 런타임 affixes.json에 있는 keywords.appendedRecords(96개)와 appendedMagicEffects의 주문·효과는 이름 색인에 들어가지 않는다. 그래서 UI를 한국어로 두고 룬워드 탭에서 Leaf를 고르면 '상시 효과 (Passive): Runeword Leaf Passive ...'처럼 영어 이름이 보인다. 약 12개 룬워드가 이렇다. Shadow Boxer 자세, Spirit의 행동 주문, Doom 레코드도 이름이 빠진다.

**고칠 방향:** ParseConfiguredAffixesFromJson은 keywords.affixes 배열만 받는다. 그래서 수정은 전체 설정을 볼 수 있는 EventBridge.Config.cpp에서, recordDisplayNames를 비우는 Reset 이후인 BuildConfigDerivedAffixState(*affixes, handler) 바로 뒤에 넣는다.
1) 안쪽 harvest 람다를 멤버 헬퍼(예: HarvestRecordDisplayName(const json& record, bool isSpell, TESDataHandler*))로 빼고, HarvestRecordDisplayNames도 이 헬퍼를 쓰게 한다.
2) LoadConfig에서 j["keywords"]["appendedRecords"]의 각 항목에 이 헬퍼를 부른다. type이 "MagicEffect"면 entry.magicEffect(isSpell=false), "Spell"이면 entry.spell(isSpell=true)이고, 다른 type은 건너뛴다. appendedMagicEffects의 각 객체에도 isSpell=false로 부른다.
3) 어픽스 경로와 마찬가지로, 복사본에 ConfigShared::StripNullValues를 먼저 적용해 null 때문에 value()가 예외를 던지지 않게 한다.
4) 조회는 기존 ConfigShared::LookupFormFromSpec(PluginEditorIds) 경로로만 한다. LookupByEditorID를 직접 부르면 안 된다는 규칙을 지킨다.

### L24. Infinity의 수동 모드 이름이 한국어로만 들어 있어 영어 툴팁에 한글이 나오고, 'Mode'/'(Manual)'은 항상 영어임

- 영역: 아이템·저장 C++ · 확인 · data-text
- 위치: `affixes/modules/keywords.affixes.runewords.json:174`, `skse/CalamityAffixes/src/EventBridge.Config.AffixActionParsing.cpp:148`, `skse/CalamityAffixes/src/EventBridge.Loot.TooltipResolution.cpp:162`, `Data/SKSE/Plugins/CalamityAffixes/affixes.json`

runeword_infinity_final의 modeCycle.labels가 ["화염 노출","냉기 노출","번개 노출"]이다. 이 값은 배포용 런타임 JSON에도 그대로 들어 있다. 파서는 이 문자열을 영어/한국어 구분 없이 modeCycleLabels 하나에 그대로 복사하고, formatAffixDetail은 'Mode', 'Auto', '(Manual)'을 항상 영어로 붙인다. 그래서 영어 UI에서 Infinity를 장착하고 'Manual Next'를 누르면 'Mode 화염 노출 (Manual)'이 나온다. 반대로 한국어 UI에서 Elemental Attunement는 'Mode Flame (Manual)'처럼 영어로 나온다. 이 데이터는 이번 브랜치 이전(6eff740, v1.2.18-rc6)부터 있었다.

**고칠 방향:** 가장 작은 수정은 데이터만 바꾸는 것이다. Elemental Attunement처럼 Infinity의 labels를 영어(예: ["Fire Exposed","Frost Exposed","Shock Exposed"])로 바꾸고 compose와 생성기를 다시 돌려 런타임 affixes.json을 재생성한다. 이렇게 하면 영어 모드는 고쳐지지만, 한국어 모드는 여전히 영어로 나온다.
완전한 수정은 하위 호환을 유지하며 현지화하는 것이다.
1) `labels`는 기본값으로 두고, 선택 항목인 `labelsEn`/`labelsKo`를 추가한다. ParseActionEvolutionAndModeCycle(AffixActionParsing.cpp)에서 modeCycleLabelsEn/modeCycleLabelsKo로 따로 파싱한다.
2) formatAffixDetail에서 a_uiLanguageMode로 라벨을 고른다(0=En, 1=Ko, 2="Ko / En"). 한쪽이 비면 `labels`를 쓴다. 'Mode'/'모드', 'Auto'/'자동', '(Manual)'/'(수동)'도 같은 방식으로 현지화한다.
3) Infinity에는 labelsEn을, Elemental Attunement에는 기존 원소 이름에 맞춘 labelsKo(화염/냉기/번개)를 채운다.
4) tools/lint_affixes.py가 새 키를 받아들이게 하고, labels나 labelsEn에 한글이 들어가면 잡도록 한다.

### L25. 설정 로드 중 nlohmann 예외를 잡지 않아서, 값 하나가 잘못되면 건너뛰지 못하고 kDataLoaded에서 게임이 튕김

- 영역: 아이템·저장 C++ · 확인 · crash
- 위치: `skse/CalamityAffixes/src/EventBridge.Config.LootRuntime.cpp:82`, `skse/CalamityAffixes/src/EventBridge.Config.cpp:110`, `skse/CalamityAffixes/src/EventBridge.Config.AffixParsing.cpp:525`, `skse/CalamityAffixes/src/main.cpp:102`

ApplyLootConfigFromJson은 `loot` 객체에서 loot.value(...)를 약 25번 부른다. 그런데 try/catch가 없고 StripNullValues도 거치지 않는다. LoadConfig와 이를 부르는 main.cpp:102(kDataLoaded 핸들러)도 예외를 잡지 않는다. nlohmann의 value()는 키가 있으면 get<T>()를 호출해서, 타입이 틀리거나 null이면 type_error.302를 던진다. 사용자가 affixes.json을 손으로 고치다 "chancePercent": "50"이나 "renameItem": null을 넣으면 메인 메뉴 직전에 게임이 튕긴다. 생성기 버그로 레코드 이름에 문자열이 아닌 값이 들어가도 같다. 이때 원인을 알려 주는 Calamity 로그는 남지 않는다.

**고칠 방향:** ApplyLootConfigFromJson 안에서 예외를 잡고 그냥 계속하면 부족하다. 중간에 예외가 나면 일부 _loot 필드에는 검증 전 값이 남고, 끝부분의 범위 제한(LootRuntime.cpp:190-260)이 실행되지 않는다. 둘 중 하나를 택한다.
(A) LoadRuntimeConfigJson 이후 LoadConfig 본문을 try { ... } catch (const nlohmann::json::exception& e)로 감싼다. 예외가 나면 e.what()을 로그로 남기고, _configLoaded를 설정하지 않은 채 return한다. 기존 파싱 실패 경로와 같은 동작이다. 다만 잘못된 loot 필드 하나 때문에 모드 전체가 조용히 로드되지 않는다는 반대 의견이 있다.
(B) 권장안: `loot`를 복사하고 그 복사본에 ConfigShared::StripNullValues를 적용한다. 각 필드는 타입을 확인하는 헬퍼(is_number/is_boolean/is_string)로 읽고, 틀리면 경고를 남기고 기본값을 유지한다. 끝의 범위 제한은 항상 실행되게 한다. HarvestRecordDisplayNames(a, a_handler) 호출은 ParseConfiguredAffixesFromJson의 어픽스별 try 안(AffixParsing.cpp:525)으로 옮긴다. 그러면 레코드 이름이 하나 잘못돼도 그 어픽스의 표시 이름만 빠진다. 마지막 안전망으로 LoadConfig에 로그만 남기는 catch를 둘 수도 있다.

### L26. 영어 레시피 요약에서 95개 중 80개가 'cast Calamity: Runeword X'로 나와 효과 설명이 없고 개발용 이름이 보임

- 영역: 아이템·저장 C++ · 확인 · data-text
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.RecipeEntries.cpp:425`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.RecipeEntries.cpp:619`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.SummaryText.h:330`

ActionSummaryTextEnByKey(SummaryText.h:330-348)에는 키가 15개뿐이다. 한국어 ActionSummaryTextByKey는 거의 모든 키를 갖고 있다. buildEffectSummaryTextEn은 영어 문구가 없는 키를 만나면 actionSummaryTextEn으로 넘어가고, kCastSpell은 여기서 'cast ' + 주문 원본 이름이 된다. 그래서 영어나 영어+한국어 UI에서 Nadir 버튼은 'On taking a hit 12% chance to cast Calamity: Runeword Nadir (Cooldown 16s) (Duration 5s)'로 나온다. Pattern을 포함한 80개 룬워드(CastSpell 75, adaptive 3, 시체 폭발 2)가 효과를 설명하지 않고 'Calamity: ' 접두를 그대로 보여 준다.

**고칠 방향:** 주된 수정은 kEffectSummaryOverrides가 쓰는 키 중 영어 표에 없는 모든 키(75개 CastSpell 키와 signature_infinity, signature_heart_of_the_oak, signature_chaos, signature_breath_of_the_dying, signature_plague)에 대해, 한국어 ActionSummaryTextByKey와 같은 내용의 영어 문구를 ActionSummaryTextEnByKey에 추가하는 것이다. 이름만 정리하면 'cast Runeword Nadir'가 되어 여전히 효과를 말하지 않는다.
임시 처리로, 영어 문구가 없는 키는 a_affix.displayNameEn의 ' - ' 뒤 효과 구절(예: 'Nadir of Fear (Fear 5s)')을 먼저 쓰게 할 수 있다. 안전망으로 actionSummaryTextEn/Ko의 kCastSpell, kSpawnTrap, kCastOnCrit에서는 이름을 ResolveRecordDisplayName(spell, pickLocalizedRecordName(raw, korean), korean)로 정리해 'Calamity: ' 접두가 플레이어에게 보이지 않게 한다.
모든 오버라이드 키에 비어 있지 않은 영어 문구가 있는지 검사하는 ctest를 추가한다. 영어 문자열이 늘어나므로 C++ ctest, 파이썬 UI 문자열 검사, 소스 문자열 래칫도 함께 갱신한다.

### L27. 한국어 레시피 요약에 발동 조건과 지속시간이 두 번 나옴: '피격 시 25% 확률로 피격 시 ...(6초) (지속 6초)'

- 영역: 아이템·저장 C++ · 확인 · data-text
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.RecipeEntries.cpp:577`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.SummaryText.h`, `skse/CalamityAffixes/src/EventBridge.Loot.TooltipResolution.cpp:415`, `RepoSpecRegressionTests.cs:1434`

buildEffectSummaryTextKo는 상시 효과 전용 항목만 빼고 모든 항목에 triggerTextKo(적중 시/피격 시/처치 시/저체력 시) + 'N% 확률로 ' + 매핑 문구 + ' (쿨타임 N초)' + ' (지속 N초)'를 붙인다. 그런데 매핑된 한국어 문구 약 18개가 이미 '피격 시', '처치 시', '체력 위급 시' 같은 조건으로 시작하고 '(6초)' 같은 지속시간도 갖고 있다. 그래서 Eternity는 '피격 시 25% 확률로 피격 시 방어도 +300·피해 반사 +25%(6초) (쿨타임 15초) (지속 6초)'로, Lawbringer는 '처치 시 40% 확률로 처치 시 성스러운 빛이 ...'로 나온다.

**고칠 방향:** 접두 검사에만 기대지 말고 데이터 문자열을 고친다. triggerTextKo(kLowHealth)는 '저체력 시'인데 매핑 문구는 '체력 위급 시'라서, 접두 비교로는 저체력 항목을 놓친다.
1) SummaryText.h의 ActionSummaryTextByKey에서 맨 앞의 조건 구절만 지운다: '피격 시 ', '적중 시 ', '처치 시 ', '체력 위급 시 ', 그리고 signature_last_wish의 '체력 35% 이하에서'. 대상은 signature_enigma, call_to_arms, faith, chains_of_honor, hustle_a, treachery, honor, rhyme, destruction, dragon, eternity, pride, obsession, kill_*, lowhealth_*, long_cd_* 등이다. 두 호출부(RecipeEntries.cpp:589, TooltipResolution.cpp:415)가 이미 조건과 확률을 앞에 붙인다.
2) 모든 매핑 문구에서 '(N초)'를 일괄 삭제하지는 않는다. signature_famine '기력 -30/초(5초), 공격력 -20%(6초)'처럼 지속시간이 둘인 문구는 정보를 잃는다. 대신 조합기를 고쳐서, 매핑 문구가 같은 N의 '(N초)'를 이미 갖고 있으면 ' (지속 N초)'를 붙이지 않는다. TooltipResolution.cpp에도 똑같이 적용해 두 경로를 맞춘다.
3) RepoSpecRegressionTests.cs(약 1434-1450행)에 고정된 Text 값을 새 문자열에 맞게 고친다. rw_eternity와 rw_lawbringer의 한국어 요약에 조건 구절이 한 번만 나오는지 검사하는 ctest를 추가한다.

### L28. 장착 빌드 요약이 EchoStrike·SpreadStatus 어픽스(Shadow Boxer, Contagion)의 발동 확률을 숨기고 '항상' 칩을 붙임

- 영역: 아이템·저장 C++ · 확인 · ui
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.PanelState.cpp:125`, `Data/PrismaUI/views/CalamityAffixes/scripts/build-summary.js:417`, `skse/CalamityAffixes/src/EventBridge.Triggers.cpp:113`

resolveProcLane(PanelState.cpp:125-143)은 CastSpell, AdaptiveElement, SpawnTrap, DoomMark만 kStandard로 넣는다. EchoStrike와 SpreadStatus는 기본값인 kNone이 된다. 하지만 런타임은 이 둘도 TryProcessTriggerAffix에서 ResolveTriggerProcChancePct(procChanceMult 반영)로 굴린다. 그래서 MCM 발동 확률 배율이 0.5이면 Shadow Boxer(100%)와 Contagion(100%)은 실제로 50%만 발동하는데, 장착 빌드 목록에는 확률 없이 '항상' 칩(build-summary.js:417)이 붙는다.

**고칠 방향:** resolveProcLane(PanelState.cpp:127-131)의 kStandard 그룹에 `case ActionType::kEchoStrike:`와 `case ActionType::kSpreadStatus:`를 추가한다. 그러면 패널이 런타임이 실제로 굴리는 ResolveTriggerProcChancePct를 보여 준다. 패널티 목록이 비어 있어 procRollStackCount는 1로 남고, 이것도 런타임과 같다. 100% 미만이면 '50% 발동'이 나오고, 100%면 다른 100% 어픽스처럼 칩이 붙지 않는다.
툴팁 쪽 주의: 툴팁의 usesStandardTriggerProcLane에 이 두 타입을 넣으면 '단독 장착 시' 추정에 아이템별 칸 패널티가 들어간다. 그런데 런타임은 이 둘에 칸 패널티를 적용하지 않는다(DoomMark 항목 참고). 툴팁에서도 확률을 보이려면 패널티 1.0을 넘기거나, 배율만 반영하는 special lane에 넣는다.

### L29. Unbending Will의 추천 베이스가 문서에는 무기인데, 패널은 모든 무기에 '추천 베이스는 방패' 경고를 띄움

- 영역: 아이템·저장 C++ · 확인 · data-text
- 위치: `skse/CalamityAffixes/include/CalamityAffixes/ItemSubtypePolicy.h:184`, `affixes/runeword.contract.json`, `RunewordCatalogRows.inl:37`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.RecipeEntries.cpp:90`, `docs/RUNEWORD_EFFECTS.md`, `docs/EFFECTS_EN.md`

ItemSubtypePolicy.h:184-186은 rw_unbending_will을 kShield에 매핑하고, 테스트도 이를 고정한다. 그래서 PopulateSpecializedBaseWarning이 무기마다 'Base mismatch: recommended base is shield'를 띄운다. 그런데 runeword.contract.json의 recommendedBase는 'Weapon'이고, 여기서 만들어진 공개 문서도 무기를 권장한다. 문서대로 무기를 고른 플레이어는 경고를 보고, 방패를 고른 플레이어는 공개 권장과 반대로 가게 된다. 게임 안에서는 방패로 일관되므로, 틀린 쪽은 데이터와 문서다.

**고칠 방향:** 방패를 유지한다. 게임 코드, Block +15% 효과, Selection.cpp:103-107 주석이 모두 방패를 가리키고, 테스트도 이를 고정한다. 데이터는 다음처럼 고친다.
1) affixes/runeword.contract.json의 rw_unbending_will "recommendedBase"를 "Weapon"에서 "Armor"로 바꾸고, 생성기가 만드는 runtime_contract.json도 갱신한다.
2) RunewordCatalogRows.inl:37을 LootItemType::kArmor로 바꿔 contract/catalog 일치 검사(RunewordContractCatalogTests)를 맞춘다.
3) docs/RUNEWORD_EFFECTS.md와 docs/EFFECTS_EN.md를 재생성한다. 가능하면 gen_runeword_doc.py/gen_effects_en_doc.py가 무기/방어구 대분류 대신 특화 베이스(방패 등)를 출력하게 한다.
recommendedBaseType 변경은 이 레시피에 런타임 부작용이 없다. 명시적 최종 어픽스가 있어 합성되지 않기 때문이다. 반대로 설계자가 무기 룬워드를 원한다면, ResolveSpecializedRunewordBase의 rw_unbending_will 분기와 kRecommendedBaseOverrides의 'shield'를 지우고 test_item_subtype_policy.cpp:35를 고친다. 이 방향은 사용자 확인 없이 하지 않는다.

### L30. 제어 패널을 연 채로 세이브를 로드하면 패널이 다시 열리고 포커스를 가져감

- 영역: 아이템·저장 C++ · 확인 · ui
- 위치: `skse/CalamityAffixes/src/PrismaTooltip.Tick.inl:187`, `skse/CalamityAffixes/src/PrismaTooltip.ViewLifecycle.inl:285`, `skse/CalamityAffixes/src/main.cpp:129`

g_controlPanelOpen은 명시적으로 닫거나 토글할 때(ViewLifecycle.inl:285-291), EnsurePrisma가 실패할 때, 뷰를 초기화할 때만 지워진다. 세이브 로드 경로에서는 아무도 건드리지 않는다. main.cpp의 kPreLoadGame/kPostLoadGame은 EventBridge로만 넘어간다. 그래서 전투 중 패널을 연 채 죽어서 자동 로드되거나, 패널을 연 채 퀵로드하면 새로 로드된 게임에서 패널이 다시 뜨고 포커스를 가져간다. 데이터가 오래된다는 부분은 사실이 아니다. LoadingMenu 중 ClearTooltipViewState(true)가 이미 룬워드 캐시를 비우고 새로 고치게 한다.

**고칠 방향:** 모든 LoadingMenu가 아니라 실제 세이브 로드(kPreLoadGame, 필요하면 kNewGame)에서만 패널 요청을 지운다. 문이나 순간이동 로딩 화면에서 숨겼다가 되살리는 동작은 의도된 것이므로 유지한다. 예를 들어 main.cpp의 kPreLoadGame 분기(또는 EventBridge::OnPreLoadGame)에서 CalamityAffixes::PrismaTooltip::SetControlPanelOpen(false)를 부른다. 이 호출은 게임 스레드에 SetControlPanelOpenImpl(false)를 올리고, 게임 진행이 준비되지 않았으면 ApplyControlPanelState의 !IsGameplayReady 분기가 안전하게 숨긴다. '패널 닫힘' 피드백까지 피하려면 전용 PrismaTooltip::OnPreLoadGame()을 만들어 g_controlPanelOpen=false, g_lastPanelState=false, g_lastPanelStateKnown=true만 설정한다. 룬워드 캐시를 따로 무효화할 필요는 없다.

### L31. 제거 절차 안내가 없어서, 끄지 않고 모드를 지우면 Calamity 상시 보너스가 캐릭터에 남을 수 있음

- 영역: 아이템·저장 C++ · 확인 · docs
- 위치: `README.md:153`, `docs/NEXUS_DESCRIPTION.md`, `Data/MCM/Config/CalamityAffixes/config.json:35`

접미와 룬워드 상시 효과는 기술, 저항, 체력, 소지 무게 등 액터 값을 바꾸는 진짜 능력이다(passiveSpellEditorId 93개). README, docs, 넥서스 설명, MCM 문구 어디에도 제거 절차가 없다. MCM 토글 도움말은 'Master enable/disable switch'뿐이다. 그래서 Calamity 접미 장비를 여러 개 낀 사용자가 바로 모드를 지우면, 상시 기술·저항 보너스가 캐릭터에 영구히 남고 콘솔 modav로만 고칠 수 있다. 위치로 적힌 README.md:153은 정확하지 않다. 해당 내용이 그 줄에 있지 않다.

**고칠 방향:** README.md와 docs/NEXUS_DESCRIPTION.md에 한국어·영어로 짧은 '제거' 절을 추가한다.
1) MCM에서 'Enable Mod'를 끈다('Calamity: disabled' 메시지가 나오면 상시 효과와 활 속도 보너스가 제거된 것이다). 또는 Calamity 장비를 모두 벗는다.
2) 메뉴 밖에서 약 10초 기다려 시한 버프와 노출 디버프가 끝나게 한다.
3) 새 슬롯에 저장한다.
4) ESP, DLL, 나머지 파일을 함께 지운다. ESP를 남기고 DLL만 지우면 상시 능력이 영구히 남는다.
이 절차를 건너뛰면 보너스가 남아 콘솔 modav로만 고칠 수 있다고 경고한다. 활 속도는 저장 전마다 이미 제거되므로 별도 단계가 필요 없다. MCM 토글 도움말에도 '제거 전에 꺼 둘 것'을 한 줄 넣는다. 끈 설정은 사용자 설정 파일에 남아서, 다시 설치하면 꺼진 상태로 시작한다는 점도 적는다. 코드 변경은 필요 없다.

### L32. 2.3.0에서 2.2.8이나 rc로 되돌리면 Edge 상시 효과가 배운 주문으로 남고, 새 룬워드 상시 효과의 보너스도 남음

- 영역: 아이템·저장 C++ · 확인 · save-data
- 위치: `affixes/modules/keywords.affixes.runewords.json:1662`, `CHANGELOG.md:34`, `README.md`

정식 빌드는 CAFF_SPEL_RW_EDGE_BLADE를 Ability(ConstantEffect, Marksman +10)로 바꾸고 runeword_edge_final의 passiveSpellEditorId로 쓴다. 2.2.8과 rc3에서는 같은 레코드가 7초짜리 시전형 주문(Marksman +20)이었다. 그래서 정식에서 Edge를 낀 채 2.2.8로 되돌리고 로드하면, 'Calamity: Runeword Edge'가 마법 메뉴에 시전 가능한 자기 버프로 남는다. 2.2.8에는 이를 지울 코드가 없다. Leaf 등 새 상시 효과(9개)를 낀 캐릭터도 파괴 비용 -10% 같은 보너스가 영구히 남는다.

**고칠 방향:** 문서만 고친다. v2.3.0 릴리스 본문, CHANGELOG.md:34의 롤백 문구 옆, README에 '2.3.0에서 되돌리기' 절차를 추가한다.
1) 2.3.0 상태에서 MCM으로 Calamity를 끈다. 그러면 DeactivateRuntimeState가 2.3.0 ESP 기준으로 EDGE_BLADE와 새 *_PASSIVE 능력 9개를 포함한 모든 상시 효과를 제거하고, 런타임 활 속도 보너스도 걷어낸다.
2) 시한 버프가 끝날 때까지 기다린 뒤 저장한다.
3) 옛 버전을 설치하고 그 세이브를 로드한 뒤 다시 켠다. 끈 설정은 사용자 설정 파일에 남아 있어서, 옛 DLL이 꺼진 채로 시작할 수 있다.
이 절차를 건너뛰면 Edge 주문이 남거나 작은 기술·저항 보너스가 남을 수 있고, `player.removespell`/`player.modav`로 고칠 수 있다고 적는다. 코세이브에 마이그레이션 표식을 두고 경고를 남기는 안은 필요 없다.

### L33. 옛 CalamityAffixes_MCM_GrantRecoveryPack 이벤트가 아직 처리됨: 디버그 확인도 1회 제한도 없이 이벤트마다 오브 5개와 모든 룬 1개씩 지급

- 영역: 아이템·저장 C++ · 확인 · security
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.ModCallback.Handlers.cpp:298`, `skse/CalamityAffixes/include/CalamityAffixes/EventNames.h:25`, `skse/CalamityAffixes/include/CalamityAffixes/detail/EventBridge.Constants.inl:45`

Handlers.cpp:298-321은 kMcmGrantRecoveryPackEvent를 받으면 GrantReforgeOrbs(5u)를 부르고, runeNameByToken의 모든 룬마다 조각을 1개씩 준다. 디버그 플래그, 1회 제한, 보유량 확인이 전혀 없다. HandleDebugUtilityModCallback은 enabled 검사보다 먼저 실행된다. 그래서 콘솔에서 `cgf "CalamityAffixes_ModeControl.Emit" "CalamityAffixes_MCM_GrantRecoveryPack"`을 치거나 다른 모드가 이 이름을 보내면, 디버그 도구가 꺼져 있어도 오브 5개와 룬 조각 33개를 무제한으로 받는다. 이 이벤트를 보내는 곳은 없고, v1.3.1에서 RecoverMiscCurrency로 대체되었다. 싱글 게임이라 보안 문제라기보다 남은 치트 경로다.

**고칠 방향:** kMcmGrantRecoveryPackEvent 분기(Handlers.cpp:298-321)와 상수 EventNames.h:25 kMcmGrantRecoveryPack, EventBridge.Constants.inl:45 kMcmGrantRecoveryPackEvent를 지운다. RecoverMiscCurrency가 이미 이 역할을 하고, test_prisma_failsoft_contract.py:171도 이 분기 없이 통과한다. CHANGELOG에는 보안 수정이 아니라 죽은 코드 정리로 적는다. QA 도구로 남기고 싶다면 다른 도구처럼 `_loot.debugHudNotifications || _loot.debugLog`로 막는다. 다만 플레이어가 MCM이나 콘솔에서 그 토글을 켤 수 있으므로 실질적인 보호는 아니다. 같은 맥락에서, kRunewordGrantStarterOrbsEvent의 기본값이 아닌 a_numArg 수량도 상한을 두거나 디버그로 막는다.

### L34. '+1 다음 조각'은 빠진 룬이 아니라 항상 첫 룬을 주고, '+1 레시피 세트'는 패널에 보이는 것과 다른 레시피의 룬을 줄 수 있음

- 영역: 아이템·저장 C++ · 확인 · tooling
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.Crafting.cpp:29`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.RecipeUi.cpp:47`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.PanelState.cpp`

GrantNextRequiredRuneFragment(Crafting.cpp:24-45)는 nextIndex로 instanceStates[selected].insertedRunes를 쓰고 runeTokens[nextIndex]를 지급한다. 그런데 insertedRunes를 늘리는 코드가 없다. 쓰는 곳은 0으로 초기화(RecipeUi.cpp:47), 범위 제한(Catalog.cpp:486), 세이브 복원뿐이다. 그래서 Ground(Shael-Io-Ort)를 고르고 '+1 다음 조각'을 세 번 누르면 Shael만 3개가 나오고, 패널은 계속 '부족한 조각 Io'를 보여 준다. 레시피 세트 쪽은 GetCurrentRunewordRecipe를 쓰기 때문에 이렇게 된다: 베이스 A에서 레시피 X, 베이스 B에서 레시피 Y를 고른 뒤 A로 돌아오면 패널은 X를 보여 주는데, '+1 레시피 세트'는 Y의 룬을 준다.

**고칠 방향:** 두 함수 모두 selectedBaseKey가 있으면 ResolveSelectedRunewordRecipe(*selectedBaseKey)로 레시피를 찾고, 선택된 베이스가 없을 때만 GetCurrentRunewordRecipe()를 쓴다. GrantNextRequiredRuneFragment에서는 PanelState.cpp처럼 BuildRuneTokenCounts<16>(recipe->runeTokens, insertedRunes)로 요구량 표를 만들어, insertedRunes가 0이 아닌 옛 세이브도 처리되게 한다. 보유량이 요구량보다 적은 첫 토큰을 지급하고, 모두 갖췄을 때만 runeTokens[insertedRunes 또는 0]으로 돌아간다.

### L35. 디버그가 꺼져 있어도 발동 경로마다 디버그 테스트 이벤트 키를 힙 문자열로 만듦

- 영역: 아이템·저장 C++ · 확인 · performance
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Cast.cpp:632`, `skse/CalamityAffixes/src/EventBridge.Actions.Cast.cpp:635`, `skse/CalamityAffixes/src/EventBridge.Actions.Cast.cpp:795`, `skse/CalamityAffixes/src/EventBridge.Actions.Cast.cpp:798`, `skse/CalamityAffixes/src/EventBridge.Actions.Status.cpp:339`

NoteDebugTestEvent(Status.cpp:339-346)는 std::string_view를 받고, _loot.debugLog나 _loot.debugHudNotifications가 켜져 있을 때만 무언가를 기록한다. 하지만 Cast.cpp 632/635/795/798에서는 호출 전에 `"affix:" + a_affix.id`를 먼저 만든다. 그래서 디버그가 꺼진 평소 플레이에서도 자기 버프나 상태 시전이 일어날 때마다 작은 힙 할당이 한 번 생기고 곧 버려진다. 비용은 무시할 만하고 게임에 영향은 없다. Status.cpp:493의 payoff 키는 StatusName이 최대 'payoff:Burning' 14자라 MSVC SSO 버퍼에 들어가므로 힙 할당이 없다.

**고칠 방향:** Cast.cpp의 네 곳만 고친다. 각 호출을 `if (_loot.debugLog || _loot.debugHudNotifications) NoteDebugTestEvent("affix:" + a_affix.id);`처럼 감싼다. 또는 플래그를 먼저 확인하고 기록할 때만 문자열을 합치는 헬퍼(NoteDebugAffixEvent(std::string_view id)나 `NoteDebugTestEvent(std::string_view prefix, std::string_view name)` 오버로드)를 둔다. Status.cpp:493은 그대로 둔다. 급하지 않은 정리 항목이다.

### L36. README는 'Reset State / 상태 초기화'를 누르라고 하지만, 실제 버튼 이름은 'Reset Selected Item / 선택한 장비 초기화'임

- 영역: 아이템·저장 C++ · 확인 · docs
- 위치: `README.md:99`, `README.md:107`, `Data/PrismaUI/views/CalamityAffixes/index.html:634`

README.md 99행과 107행은 잘못 붙은 어픽스를 고치려면 **Reset State / 상태 초기화**를 두 번 누르라고 안내한다. 그런데 패널에 있는 초기화 버튼은 runewordResetButton 하나이고, 이름은 'Reset Selected Item / 선택한 장비 초기화'다(index.html:634, ui-text.js:201, commands.js:122, runeword.js:1116). 처음 누르면 'Confirm Full Reset / 전체 초기화 확인'으로 바뀐다. 사용자는 존재하지 않는 버튼을 찾게 된다.

**고칠 방향:** README.md:99와 :107의 **Reset State / 상태 초기화**를 **Reset Selected Item / 선택한 장비 초기화**로 바꾼다. README가 한국어 문서이므로 **선택한 장비 초기화**만 써도 된다. 원하면 '복구 및 초기화' 섹션이 접힌 채 시작하니 먼저 펼치라는 것과, 처음 누르면 '전체 초기화 확인'으로 바뀐다는 것을 덧붙인다. CHANGELOG.md와 docs/releases/*는 당시 이름을 기록한 것이므로 그대로 둔다.

### L37. 공개 룬워드 문서가 Spirit·Fortitude·Phoenix의 추천 베이스를 파이썬 값 'None'으로 출력함 (패널은 검/방패, 혼합, 무기/방패)

- 영역: 아이템·저장 C++ · 확인 · docs
- 위치: `docs/RUNEWORD_EFFECTS.md:105`, `docs/RUNEWORD_EFFECTS.md:476`, `docs/RUNEWORD_EFFECTS.md:609`, `tools/gen_runeword_doc.py:30`, `affixes/runeword.contract.json`

tools/gen_runeword_doc.py:30은 recipe['recommendedBase']를 f-string에 그대로 넣는다. 95개 중 rw_spirit, rw_fortitude, rw_phoenix 3개는 이 값이 JSON null이라서 파이썬이 'None'으로 찍는다. 그래서 docs/RUNEWORD_EFFECTS.md 105/476/609행에 '- 추천 베이스: None'이 나오고, '베이스 없음'이나 '어느 장비에도 안 됨'으로 읽힌다. 게임 패널은 kRecommendedBaseOverrides로 Spirit에 검/방패, Phoenix에 무기/방패, Fortitude에 혼합을 권장한다.

**고칠 방향:** gen_runeword_doc.py가 null을 그대로 찍지 않게 한다. 최소 수정은 render_entry에서 Weapon→무기, Armor→방어구, None→혼합으로 옮기는 것이다(예: `recipe.get('recommendedBase') or '혼합 (무기·방어구)'`). 다만 이렇게 하면 Spirit와 Phoenix는 여전히 패널과 다르게 보인다. 패널과 맞추려면 kRecommendedBaseOverrides의 레시피별 베이스 키를 runeword.contract.json으로 옮기거나, 생성기에 같은 표를 복사하고 두 표가 같은지 검사하는 테스트를 둔다. 이건 더 큰 별도 작업이다. tools/tests/test_gen_runeword_doc.py에 recommendedBase가 None인 경우와 '생성된 문서에 None이 없음' 검사를 추가하고, tools/update_docs.py로 문서를 재생성한다.

### L38. 패널이 바닐라 스카이림에 없는 '클로'와 '장창/창' 베이스를 추천함 (C++ 분류기도 인식 못 함)

- 영역: 아이템·저장 C++ · 확인 · data-text
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.RecipeEntries.cpp:68`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.RecipeEntries.cpp:85`, `Data/PrismaUI/views/CalamityAffixes/scripts/recipe-text.js:440`, `Data/PrismaUI/views/CalamityAffixes/scripts/recipe-text.js:450`

kRecommendedBaseOverrides가 rw_insight, rw_infinity, rw_pride, rw_obedience를 'polearm'에, rw_chaos와 rw_mosaic을 'claw'에 매핑한다. 이 키는 PrismaTooltip.PanelData.inl:60을 거쳐 패널로 가고, recipe-text.js가 'Base: Polearm/Spear', 'Claw' 같은 배지로 바꾼다. 무기 확장 모드가 없으면 이런 아이템은 존재하지 않고, 문서는 'Weapon'이라고 적는다. 베이스를 검사하는 코드는 없어서 문구만 틀린 문제다. 최초 공개 릴리스(99b49f0)부터 있었다.

**고칠 방향:** 가장 간단한 방법은 RecipeEntries.cpp:68-71과 85-86의 'polearm'·'claw' 항목 6개를 지우는 것이다. 그러면 resolveRecommendedBaseKey가 'weapon'으로 돌아가 docs/RUNEWORD_EFFECTS.md와 일치한다. 스카이림 무기 분류로 안내하고 싶다면 패널이 이미 쓰는 키만 쓴다: Chaos·Mosaic은 'one_handed_melee', Infinity·Pride·Obedience는 'two_handed_melee', Insight는 마력 회복 효과이므로 'staff_wand'나 'weapon'. 그 뒤 recipe-text.js resolveRecipeBaseBadge(440-441, 450-451)에서 쓰지 않게 된 'polearm'/'claw' 경우를 지우고, 허용 배지 문구를 검사하는 C++·JS UI 문자열 테스트를 확인한다. 이 레시피들을 ResolveSpecializedRunewordBase에 넣지는 않는다. 새 'Base mismatch' 경고가 생겨 문구 수정의 범위를 넘는다. 문서도 같은 표기로 맞춘다.

### L39. 보스 상자 이름 검사와 보스 컨테이너 1.15 배율은 아무 효과가 없는데, 넥서스 페이지와 2.2.8 변경 기록은 적용된다고 함

- 영역: 아이템·저장 C++ · 확인 · docs
- 위치: `docs/NEXUS_DESCRIPTION.md:79`, `docs/NEXUS_DESCRIPTION.md:174`, `README.md:227`, `skse/CalamityAffixes/src/EventBridge.Loot.Service.cpp:419`, `skse/CalamityAffixes/src/EventBridge.Loot.Service.cpp:447`, `affixes/modules/spec.root.json`

보스 상자 분류기 ResolveLootCurrencySourceTier는 ProcessLootAcquired(Service.cpp:447)에서만 불린다. 그런데 그 전에 :419의 IsRuntimeCurrencyDropRollEnabled가 _loot.runtimeCurrencyDropsEnabled를 보고 return한다. 이 플래그는 항상 ResolveCorpseDeathOnlyCurrencyDropPolicy()로 설정되므로 화폐는 시체 사망 때만 굴린다. 그래서 넥서스 페이지가 보스 상자 감지를 켜 준다고 해서 powerofthree's Tweaks를 설치해도, lootSourceChanceMultBossContainer를 고쳐도 보스 상자를 포함한 모든 상자에서 화폐가 나오지 않고 두 설정 모두 효과가 없다.

**고칠 방향:** 문서만 고친다.
- NEXUS_DESCRIPTION.md:79(KO)와 :174(EN), README.md:227에서 보스 상자 구절을 지운다. powerofthree's Tweaks의 이점은 방어구 제외 목록(rewardbox/lootbox)을 다른 모드 폼까지 넓혀 준다는 것만 남긴다.
- 이미 배포된 2.2.8 CHANGELOG 항목은 고치지 않는다. 대신 2.3.0 노트에 '상자·보스 상자 분류는 효과가 없다. 화폐는 시체에서만 나온다'는 정정을 한 줄 넣는다.
- spec.root.json의 lootSourceChanceMult{Container,BossContainer,World}와 bossContainer* 키는 이번에 단독으로 지우지 않는다. 생성기 spec, 스키마, lint, LootDefaultsTests.cs:17/32, tools/tests/test_lint_affixes.py:431-434, GeneratorRunnerTests가 이 키들을 참조한다. 조정 가능한 값처럼 보이지 않게 하려면 README 설정 절에서 '동작하지 않는 예약 키'로 표시하거나, 나중에 이 모든 곳에서 함께 지운다.

### L40. 패널의 천장(피티) 안내는 '나올 수 있었는데 안 나온 처치마다 +1'이라는데, 네임드·보스 확정 처치와 미리 채워진 시체는 카운터를 움직이지 않음

- 영역: 아이템·저장 C++ · 확인 · ui
- 위치: `Data/PrismaUI/views/CalamityAffixes/scripts/resource-dashboard.js:324`, `Data/PrismaUI/views/CalamityAffixes/index.html`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.Crafting.cpp:201`, `skse/CalamityAffixes/src/EventBridge.Triggers.DeathEvent.cpp:143`

resource-dashboard.js:324-331의 안내는 '룬 조각/재련 오브가 나올 수 있는 처치에서 안 나오면 1씩 오른다'고 말한다(한국어도 같은 뜻). 그런데 카운터를 올리는 곳은 TryRollRunewordFragmentToken과 TryRollReforgeOrbGrant뿐이고, 일반 적의 굴림에서만 불린다. 보스 위치 ref, 드래곤, 네임드 적은 확정 보상 경로를 탄다. 그래서 네임드 산적을 잡아 60% 분기에서 재련 오브만 받고 룬 조각을 못 받아도 '룬 조각 천장' 바는 그대로다. 확정 보상을 천장에서 빼는 것 자체는 의도된 설계다(Loot.Service.cpp:589, NEXUS_DESCRIPTION.md:142). 틀린 건 문구다.

**고칠 방향:** C++는 건드리지 않는다. resource-dashboard.js(323-332)의 두 안내와 index.html의 기본 문구를 똑같이 고친다. Ultralight 패널은 공간이 좁으니 짧고 쉬운 말로 쓰고, 테스트가 금지하는 단어('eligible ordinary', '적격', '피티')는 피한다. 문구 정리 원칙과 calamity-ui-style 기준을 따른다. 예시: EN 'Goes up by 1 each time an ordinary enemy could drop a rune fragment and doesn't. Named enemies, bosses and dragons don't change it. When full, the next one is guaranteed.' / KO '일반 적에게서 룬 조각이 안 나오면 1씩 오릅니다. 이름 있는 적·보스·드래곤은 영향 없음. 다 차면 다음엔 반드시 나옵니다.' 재련 오브 안내는 이름만 바꿔 같은 문장으로 쓴다. resource-dashboard.js:165의 마우스오버 문구('Fills on each miss')는 그대로 둔다. test_prisma_resource_dashboard.py와 C++ UI 소스 문자열 검사가 새 문구를 요구하면 같은 변경에서 갱신한다.

### L41. 소환술사 접미사의 MGEF·SPEL 이름이 아직 'Conjuration +5/10/15%'로 남아 있음

- 영역: 데이터·생성기 · 확인 · data-text
- 위치: `affixes/modules/keywords.affixes.suffixes.json:3052`, `affixes/modules/keywords.affixes.suffixes.json:3053`, `affixes/modules/keywords.affixes.suffixes.json:3101`, `affixes/modules/keywords.affixes.suffixes.json:3149`

suffix_conjurer_t1~t3의 name/nameEn/nameKo(3047-3049, 3095-3097, 3143-3145)는 'Conjuration spell cost -N%'로 바뀌었지만, records.magicEffect.name과 records.spell.name(3053/3062, 3101/3110, 3149/3158)은 아직 'Calamity Suffix: Conjuration +N% / 소환 +N%'다. 그래서 활성 효과 창에는 'Calamity Suffix: Conjuration +15%'가 나오고 아이템에는 '소환 마법 비용 -15%'가 나온다. 비용 감소를 '+N%'로 쓰지 말라는 설계 원칙에도 맞지 않는다.

**고칠 방향:** keywords.affixes.suffixes.json에서 CAFF_MGEF_SUFFIX_CONJURATION_T1-3과 CAFF_SPEL_SUFFIX_CONJURATION_T1-3의 name을 'Calamity Suffix: Conjuration spell cost -5% / 소환 마법 비용 -5%'(-10%, -15%)로 바꾼다. 에디터 ID는 그대로 둔다(FULL 문자열만 바뀌고 FormID는 그대로). 그 다음 tools/compose_affixes.py와 생성기를 다시 돌려 affixes/affixes.json, Data/SKSE/Plugins/CalamityAffixes/affixes.json, ESP에 반영하고, 생성물은 CRLF→LF로 정규화한다.

### L42. 게임을 로드해도 MCM 값이 런타임에 다시 전달되지 않아, 2%→4% 정화의 오브 변환이 사용자가 직접 고른 2%를 실행할 때마다 덮어씀

- 영역: 데이터·생성기 · 확인 · correctness
- 위치: `tools/CalamityAffixes.Generator/Writers/McmPluginBuilder.cs:43`

ESP의 QUST 0x01000800에는 로드 시 실행되는 별칭 스크립트가 없다. McmPluginBuilder.cs:42-51은 PlayerAlias_MCM을 스크립트 없이 추가하고, 9d17a8d에서 SKI_PlayerLoadGameAlias를 일부러 뺐다. MCM Helper의 mcm_configbase도 대신 해주지 않는다. 사용자가 'Scouring Orb Chance'를 2%로 정하면 2.0이 저장되는데, 다음 실행 때 C++가 2.0을 읽어 4.0으로 바꾼다. MCM은 여전히 2.0%로 보이지만 오브는 두 배로 떨어지고, 슬라이더를 움직여야 다음 재시작 전까지만 맞는다. 또 MO2에서 Overwrite를 정리해 user_settings.json이 사라졌지만 MCM ini는 남아 있으면, 런타임은 기본 확률과 Enable=true로 돌고 MCM에는 사용자 설정이 보인다. (한 검토자는 'SKSE가 RegisterForModEvent를 로드 간에 유지하지 않는다'는 전제가 틀렸다고 반박했다.)

**고칠 방향:** 값 기준의 2.0→4.0 재작성을 없애고 잘못된 주석을 고친다. json "version" 하나로 판단하기도 어렵다. PersistRuntimeUserSettings가 이미 version=1을 쓰므로, v2.2.x에서 사용자가 직접 2.0을 고른 파일과 손대지 않은 옛 기본값을 구분할 수 없다. 변환을 유지하려면 version을 2로 올리고, 저장된 버전이 없거나 2 미만일 때만 한 번 바꾼 뒤 곧바로 저장한다. 이 한계는 감수해야 한다. MCM→런타임 동기화는 실제로 로드 때 도는 경로에 둔다. 가장 간단한 방법은 kDataLoaded/로드 시점에 DLL이 Data/MCM/Settings/CalamityAffixes.ini를 읽는 것이다(iUiLanguage 때문에 이미 이 파일을 파싱함). 다른 방법은 MCM Helper와 함께 동작하는 로드 시 별칭 스크립트를 되살리는 것이다.

### L43. 검증 주기 기본값이 MCM은 5초, 런타임은 8초로 다름

- 영역: 데이터·생성기 · 확인 · correctness
- 위치: `Data/MCM/Config/CalamityAffixes/config.json:56`

Data/MCM/Config/CalamityAffixes/config.json:56의 defaultValue와 settings.ini:7의 fValidationInterval은 5.0이다. DLL은 EventBridge.Constants.inl:88의 kEquipResyncInterval{8000}으로 시작한다(EventBridge.h:195, EventBridge.Config.Reset.cpp:22). 새로 설치하면 MCM에는 5초로 보이지만, 사용자가 슬라이더를 움직이기 전까지 장비 재검사는 8초마다 돈다.

**고칠 방향:** C++ 8000ms는 v1.2.18-rc1(9227fe2)에서 일부러 넣은 성능 보호이므로 그대로 둔다. Data/MCM/Config/CalamityAffixes/config.json의 fValidationInterval defaultValue와 Data/MCM/Config/CalamityAffixes/settings.ini의 fValidationInterval을 8.0으로 맞춘다. 이미 값을 고른 사용자는 Data/MCM/Settings/CalamityAffixes.ini에 저장된 값을 그대로 쓴다.

### L44. '잃어버린 화폐 복구' MCM 버튼이 영어 전용 HUD에 한국어 문구를 보냄

- 영역: 데이터·생성기 · 확인 · ui
- 위치: `skse/CalamityAffixes/src/EventBridge.Serialization.Lifecycle.cpp:439`

_miscCurrencyRecovered가 켜진 뒤(476행에서 설정, 코세이브에 저장·로드) 같은 플레이에서 'Recover Missing Currency'를 다시 누르면, RecoverMiscCurrency()가 "Already recovered this save. / 이미 복구 완료된 세이브입니다."를 돌려준다. 그래서 HUD에 'Already recovered this save. / ' 뒤로 빈칸이나 네모 글자가 찍힌다. 1472517부터 있던 순수 표시 문제다.

**고칠 방향:** Lifecycle.cpp:439에서 영어만 돌려준다: `return "Already recovered this save.";`. 패널 쪽은 기존 방식대로 Data/PrismaUI/views/CalamityAffixes/scripts/i18n.js의 engineFeedbackLocalizations에 `"Already recovered this save.": ["Already recovered this save.", "이미 복구 완료된 세이브입니다."]`를 추가해 언어에 맞게 보여준다.

### L45. MCM 확률·발동 배율 HUD 알림이 소수점 6자리로 찍힘(예: '8.500000%')

- 영역: 데이터·생성기 · 확인 · ui
- 위치: `skse/CalamityAffixes/src/EventBridge.Triggers.ModCallback.Handlers.cpp:146`

EventBridge.Triggers.ModCallback.Handlers.cpp의 네 알림(131행 procChanceMult, 146행 룬워드 조각 확률, 약 164행 재련 오브, 약 182행 감정/정화 오브)이 std::to_string(float)으로 글자를 만든다. std::to_string은 "%f" 형식이라 항상 소수점 6자리다. 그래서 슬라이더를 8.5로 옮기면 'Calamity: runeword fragment chance 8.500000%', 배율은 'Calamity: proc x1.000000'으로 나온다.

**고칠 방향:** 네 곳의 std::to_string(x)를 소수 첫째 자리 고정 형식으로 바꾼다: std::format("{:.1f}", x) 또는 spdlog에 포함된 fmt::format("{:.1f}", x). 슬라이더 단계(배율 0.1, 확률 0.5)와 맞는다. 원하면 끝의 '.0'을 지워 정수는 '8%'처럼 보여준다.

### L46. 일부 접두 설명이 실제 동작과 다름 (Mana Burn 한국어는 '흡수', Mage Armor는 1회 상한 누락, Elemental Attunement 영어는 ICD 누락)

- 영역: 데이터·생성기 · 확인 · data-text
- 위치: `affixes/modules/keywords.affixes.core.json:1139`, `affixes/modules/keywords.affixes.core.json:1141`

Mana Burn의 name·nameKo(core.json:1139/1141)는 '매지카 60 흡수'인데, CAFF_MGEF_HIT_SHOCK_OVERLOAD는 hostile Magicka ValueModifier 하나라서 피해만 주고 플레이어 매지카는 차지 않는다(생성기에는 Absorb 플래그도 없음). Mage Armor I/II는 1회 상한(35/50)이 문구에 없어서, 600 피해를 받고 90 전환을 기대한 II 사용자는 50만 받는다. Elemental Attunement의 nameEn에는 ICD 0.2초가 빠져 있다(59b3335부터).

**고칠 방향:** mana_burn의 name과 nameKo를 '매지카 60 감소'(또는 바닐라 용어 '매지카 피해 60')로 바꾼다. Mage Armor I/II의 EN·KO 설명에 Life Drain처럼 상한을 넣는다. 예: '물리 피해 10%를 매지카로 전환 (타격당 최대 35)' / 'Redirect 10% of physical hit damage to Magicka (max 35 per hit)', II는 50. Elemental Attunement nameEn에 ' / ICD 0.2s'를 붙인다(예: '(100% on hit / ICD 0.2s)'). 모듈 JSON을 고친 뒤 다시 합치고 생성한다. Bear Trap의 비슷한 '흡수' 문구와 한국어 레코드 이름은 따로 판단한다.

### L47. Exploit Weakness가 피해 0인 타격에도 발동해 1 피해만 주고 쿨다운을 써버림

- 영역: 데이터·생성기 · 확인 · correctness
- 위치: `affixes/modules/keywords.affixes.status.json:56`, `skse/CalamityAffixes/src/EventBridge.Actions.Dispatch.cpp:21`

exploit_weakness는 magnitudeScaling {HitTotalDealt, mult 0.15}만 있고, spellBaseAsMin은 기본 false(Config.AffixActionParsing.cpp:66), add/min/max는 0이다. 저항이나 막기로 피해가 0인 타격이 상태이상 2개 걸린 적에게 맞으면, ResolveSpellMagnitudeOverride가 0을 돌려주는데 CanExecuteAction의 kCastSpell 분기(Dispatch.cpp:21-48)에는 DoomMark(:111)와 달리 '크기 > 0' 검사가 없다. 그래서 효과가 레코드 기본값(1 피해)으로 걸리고 0.5초 쿨다운이 소모되어 바로 뒤의 진짜 타격을 막는다. (한 검토자는 인게임에서 이 상황이 실제로 생기는지 반박했다.)

**고칠 방향:** CanExecuteAction의 kCastSpell(그리고 kCastSpellAdaptiveElement) 분기 중 mode-cycle이 아닌 쪽에서 `return action.spell != nullptr;` 앞에 다음을 넣는다: `if (action.spell && action.magnitudeScaling.source != MagnitudeScaling::Source::kNone && ResolveSpellMagnitudeOverride(action, action.spell, a_hitData, false) <= 0.0f) return false;`. 진화 적용 전의 원래 스케일 값으로 판단하며, DoomMark와 같은 방식이다.

### L48. Lawbringer의 '언데드 퇴치' MGEF에 언데드 조건이 없어 살아 있는 적도 도망가게 할 수 있음

- 영역: 데이터·생성기 · 확인 · data-text
- 위치: `affixes/modules/keywords.affixes.runewords.json:2240`

CAFF_MGEF_RW_LAWBRINGER_HEAL(01000957)은 TurnUndead·Confidence·hostile·recover false이고, 주문은 크기 30·지속 8·범위 15다. 조건은 KeywordPluginBuilder.cs:676-705가 쓰는 IsHostileToActor==1, GetPlayerTeammate==0 두 개뿐이다. 바닐라 TurnUndead MGEF 13개는 모두 HasKeyword(ActorTypeUndead 0x13796) 또는 IsUndead 조건이 있다. 산적 옆에서 다른 산적을 죽여 Lawbringer(40%)가 발동하면, 레벨 30 이하의 살아 있는 산적이 8초 동안 도망칠 수 있다. 문구는 '주변 언데드를 퇴치'이고, 레코드 이름은 아직 'Soul Harvest / 영혼 수확'이다. 95b0d17부터 그대로다. 단, 엔진 검토자들은 엔진이 Turn Undead를 이미 언데드에게만 적용한다고 반박했다.

**고칠 방향:** 먼저 살아 있는 대상에게 실제로 걸리는지 인게임에서 확인한다. 걸린다면 MagicEffectRecordSpec에 MGEF별 추가 조건 필드를 만들고, AddMagicEffect에서 hostile 조건 두 개 뒤에 AND 플래그를 유지한 채 바닐라 EnchTurnUndeadFFContact 조건을 덧붙인다: HasKeyword(Skyrim.esm|0x13797 ActorTypeDaedra)==0 AND (HasKeyword(Skyrim.esm|0x13796 ActorTypeUndead)==1 OR IsUndead==1). 에디터 ID와 레코드 위치는 그대로 두고, 레코드 이름은 실제 효과에 맞게 바꾼다.

### L49. 패널 설명 문구: Last Wish는 없는 원소 모드를 설명하고, Heart of the Oak는 모든 대상에 적용되는데 '보스 상대'라고 씀

- 영역: 데이터·생성기 · 확인 · data-text
- 위치: `Data/PrismaUI/views/CalamityAffixes/scripts/recipe-text.js:353`

rw_last_wish는 SummaryText.h:70에서 signature_last_wish 키를 받고, recipe-text.js:353-357(resolveRecipeFlavorDetailText)은 '고급 탭에서 원소를 Manual Mode로 고정할 수 있다'고 보여준다. 하지만 runeword_last_wish_final은 LowHealth(35%) 트리거 CastSpell(CAFF_SPEL_RW_LASTWISH_FADE: DamageResist +250, ResistMagic +50 12초, 체력 250 회복)이라 그런 옵션이 없다. signature_heart_of_the_oak(SummaryText.h:69)는 '보스 상대'라고 써서, 일반 적에게는 가장 약한 원소 처리가 안 된다고 오해하게 만든다(거짓이라기보다 오해를 부르는 문구).

**고칠 방향:** resolveRecipeFlavorDetailText(recipe-text.js:353-361)에서 signature_last_wish 분기를 지운다(숫자 요약이 이미 효과를 설명함). 한 줄이 필요하면 t("Triggers on its own when your health drops below 35%.", "체력이 35% 아래로 떨어지면 저절로 발동합니다.")로 바꾼다. signature_heart_of_the_oak는 '타격 시 항상 대상의 가장 높은 원소 저항을 낮춘다'는 뜻으로 고쳐 쓴다.

### L50. Mist의 '즉시 매지카 소모 75'가 Mana Burn MGEF를 재사용해서 전격 저항에 깎임

- 영역: 데이터·생성기 · 확인 · data-text
- 위치: `affixes/modules/keywords.affixes.runewords.json:3580`, `affixes/modules/keywords.affixes.runewords.json:3591`

CAFF_SPEL_RW_MIST_FOG의 두 번째 효과는 CAFF_MGEF_HIT_SHOCK_OVERLOAD(크기 75, 지속 0)다. 이 MGEF는 Mana Burn 접두용으로, core.json:1143-1150에서 actorValue Magicka·resistValue ResistShock·Destruction으로 정의되어 있다. 그래서 KeywordPluginBuilder.cs:625-630이 전격 마법부여 피격 셰이더도 붙인다. 폭풍 정령이나 전격 저항 50%인 대상에게는 매지카 소모가 75가 아니라 0이나 37이 된다. (한 검토자는 의도된 재사용이라고 반박했다.)

**고칠 방향:** spec.root.json의 keywords.appendedRecords 맨 끝에만 새 MGEF를 추가한다(예: CAFF_MGEF_RW_MIST_MANA_DRAIN). 설정은 actorValue Magicka, hostile, 지속 0, resistValue 없음으로 한다. Arcane Collapse의 다른 효과처럼 마법 저항으로 줄게 하려면 ResistMagic을 쓴다. hitShaderForm은 비워 전격 셰이더가 붙지 않게 한다. CAFF_SPEL_RW_MIST_FOG의 두 번째 효과를 이 MGEF로 바꾸고, CAFF_MGEF_HIT_SHOCK_OVERLOAD 자체는 손대지 않는다.

### L51. Wealth의 'Speechcraft +15'는 화술 기술이 아니라 SpeechcraftModifier(바닐라 Fortify Barter, 거래 가격)를 올림

- 영역: 데이터·생성기 · 확인 · data-text
- 위치: `affixes/modules/spec.root.json:159`

spec.root.json:157-160의 CAFF_MGEF_RW_WEALTH_PASSIVE_SPEECHCRAFT(0x1000AE4)는 AV 107(SpeechcraftModifier) Value Modifier이고, CAFF_SPEL_RW_WEALTH_PASSIVE가 크기 15로 건다(affixes/affixes.json:159도 같음). 화술 기술은 AV 17이다. 플레이어는 설득 판정이나 기술 제한 대화에 쓰이는 화술 +15를 기대하지만, 실제로는 매매 가격이 15% 유리해질 뿐이다.

**고칠 방향:** 효과는 그대로 두고 문구를 고치는 쪽을 권장한다. AV 107·15는 바닐라 Fortify Barter 장비와 같고 v1.5.0부터 이렇게 출시됐다. AV 17로 바꾸면 기술 레벨이 올라 설득과 거래에 모두 영향을 주므로 게임플레이가 달라진다. 문구는 '소지 무게 +75·거래 가격 15% 유리' / 'Carry Weight +75 and prices 15% better'처럼 바꾸고, 나오는 곳을 모두 고친다: keywords.affixes.runewords.json:4501-4503의 name/nameEn/nameKo, SummaryText.h:300·:345, recipe-text.js. 생성기 테스트(EffectReworkContractTests.cs:150/156, RepoSpecRegressionTests.cs:263)도 확인한다.

### L52. 여러 룬워드 레코드의 옛 이름이 패널 상세와 활성 효과 창에 그대로 나옴

- 영역: 데이터·생성기 · 확인 · data-text
- 위치: `affixes/modules/keywords.affixes.runewords.json:3210`, `affixes/modules/keywords.affixes.runewords.json:3219`

HarvestRecordDisplayNames(EventBridge.Config.AffixParsing.cpp:432-488)가 레코드 name의 EN/KO 부분을 recordDisplayNames로 복사하고, buildSpellProfileText(RecipeEntries.cpp:247-34x)가 이를 룬워드 탭 상세에 보여준다. 그런데 Destruction의 MGEF·SPEL(3210/3219)은 'Runeword Destruction Bleed / 룬워드 파괴 출혈'인데, 실제로는 ResistShock 저항·statusTag Shock인 전격 효과다. Obsession 상세에는 Chaos 룬워드의 CAFF_MGEF/SPEL_RW_CHAOS_BLEED 이름이 나온다. Lawbringer(LAWBRINGER_HEAL, 'Soul Harvest')와 Faith(FAITH_FANATIC_WARD) 등도 실제 효과와 이름이 다르다. 새 공용 상태이상 체계를 읽는 플레이어가 헷갈린다.

**고칠 방향:** keywords.affixes.runewords.json의 name 필드만 고친다. 에디터 ID는 모두 유지한다(FULL 이름은 FormID와 상관없고, 테스트는 에디터 ID만 확인함). 예: CAFF_MGEF/SPEL_RW_DESTRUCTION_BLEED → 'Calamity: Runeword Destruction Shock Storm / 룬워드 파괴 전격 폭풍', CAFF_MGEF/SPEL_RW_CHAOS_BLEED → Obsession에 맞는 이름(또는 Obsession 전용 주문을 따로 주거나 공용이 티 나지 않는 중립 이름), CAFF_MGEF/SPEL_RW_LAWBRINGER_HEAL → 'Lawbringer Justice / 로브링어 정의 집행', CAFF_MGEF_RW_FAITH_FANATIC_WARD → 'Faith Fanaticism / 신념 광신'. 그 다음 affixes.json, Data 사본, ESP를 다시 합치고 생성한다.

### L53. 상태가 바뀌지 않은 제작 실패는 패널 상태 push가 캐시에 막혀, 제작 버튼이 8초간 잠기고 실제 실패 사유가 'No response from crafting'으로 덮임

- 영역: UI · 확인 · ui
- 위치: `skse/CalamityAffixes/src/PrismaTooltip.HandleUiCommand.inl:258`, `skse/CalamityAffixes/src/PrismaTooltip.HandleUiCommand.inl:273`, `skse/CalamityAffixes/src/PrismaTooltip.PanelData.inl:1`, `skse/CalamityAffixes/src/PrismaTooltip.PanelData.inl:204`, `Data/PrismaUI/views/CalamityAffixes/scripts/commands.js:316`, `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:316`, `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:463`

감정·재련·정제·룬워드 제거·교환 버튼을 누르면 JS가 affixCraftPendingState를 설정한다(commands.js:316-324). 이 상태는 setRunewordPanelState(interop.js:316)를 받거나 8초 타임아웃이 지나야만 풀린다. setActionFeedback은 이 상태를 풀지 않는다. C++ 제작·교환 핸들러는 RefreshRunewordPanelBindings, SetRunewordPanelState, PushCachedInteropPayload 순서로 상태를 보낸다. 실패해서 상태가 그대로면 JSON이 직전과 같아 push가 생략된다. 이 문제는 JS는 허용했지만 C++가 거부하는 경우에 나타난다. 예를 들면 무기를 해제했거나 무장해제당한 지 2초가 안 됐을 때, stripTrackedSuffixSlots 정책이 켜져 있을 때, 설정에서 사라진 어픽스 토큰이 남은 옛 세이브라서 'legacy affix layout. Use a Scouring Orb to reroll it.'이 뜰 때다. 이때 올바른 실패 문구가 잠깐 뜬 뒤 제작·변환 버튼이 모두 8초간 회색으로 잠긴다. 슬롯 미터는 'Unlocking Slot 3…'에 머문다. 8초가 지나면 실제 사유가 'No response from crafting. Check the item and try again.'으로 덮인다. 2.0 제작 분리(6e70e2a/4e01ac7) 때부터 있던 문제다.

**고칠 방향:** C++에서 고친다. HandleUiCommand.inl의 제작 분기(affix.identify/reforge/scour/runeword.remove, 약 257행)와 교환 분기(currency.exchange, 약 272행)에서, RefreshRunewordPanelBindings(*bridge, false) 바로 앞에 g_runewordCache.panelStateJson.clear()를 부른다. !outcome.success일 때만 불러도 충분하다. 그러면 명령마다 setRunewordPanelState가 feedback보다 먼저 도착하고, JS가 affixCraftPendingState를 풀고 다시 그린다. setActionFeedback 안에서 affixCraftPendingState를 지우는 방식은 쓰지 않는다. JS가 이 함수를 로컬에서도 부르기 때문이다(tooltip.js:143/172/198, panel-layout.js:530, commands.js:163/284). 그러면 요청 도중 버튼이 풀려, 두 번 눌러 재화를 두 번 쓸 수 있다. JS에서 고쳐야 한다면 엔진 응답인 actionFeedback interop 진입점(bootstrap.js:27/44)만 감싼다. 그 래퍼에서 pending을 풀고 runewordPanelState를 다시 그린다. 이와 별도로, 런타임이 동작별 가능 여부(CanCraftAffixLayout 기준 canReforge/canIdentify 등)를 보내는 것도 고려한다. 그러면 서버가 늘 거부하는 레이아웃에서 Reforge 버튼이 켜지지 않는다.

### L54. 'Inspect Highlighted Item' 카드가 작업 중인 선택 장비를 '인벤토리에서 가리킨 아이템'이라고 표시함

- 영역: UI · 확인 · ui
- 위치: `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:182`, `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:494`, `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:504`, `Data/PrismaUI/views/CalamityAffixes/scripts/recipe-text.js:53`

selectedItemNameState 변수 하나에 두 가지 의미가 섞여 있다. setSelectedItemName은 DLL이 보낸 '가리킨 메뉴 아이템'을 여기에 쓴다. setInventoryItems(interop.js:180-186)와 setUiLanguage(interop.js:494-499)는 같은 변수에 선택된 작업 베이스 이름을 쓰고, 출처를 'equipped'로 바꾼다. renderSelectedItemContext(recipe-text.js:48-57)는 hasSelection만 보고 affixSelectedItemName/Meta를 채운다. 그래서 아무것도 가리키지 않은 채 게임 중에 패널을 열거나 'Change Item'에서 장비를 고르면 이렇게 나온다. 'Inspected Inventory Item' 칸에 'Iron Sword'가 표시되고, 그 아래에 'The item you're pointing at in your inventory.'(인벤토리에서 가리킨 아이템입니다.)라고 적힌다. 인벤토리에서 단검을 가리키고 있을 때도, 다른 장비를 장착해서 베이스 목록 push가 오면 이 칸이 작업 베이스로 바뀐다.

**고칠 방향:** 검사 카드용 상태를 따로 둔다. hoveredItemNameState/hoveredItemSourceState(이름은 예시)를 추가하고, setSelectedItemName/setSelectedItemSource(interop.js:504-522)만 이 값을 쓰게 한다. 비우는 일은 ClearTooltipViewState가 보내는 같은 interop 호출이 맡고, setInventoryItems는 이 값을 건드리지 않는다. renderSelectedItemContext의 affixSelectedItemName/affixSelectedItemMeta는 이 상태만 읽는다. 그러면 카드가 panelTooltipText와 늘 일치한다. selectedItemNameState와 setInventoryItems의 기존 쓰기는 그대로 둔다. 미리보기 대기 로직과 보조 라벨이 이 값에 의존한다. 최소 수정으로는 selectedItemSourceState === "equipped"일 때 메타 문구를 '선택한 장비' 계열 문구로 바꾸는 방법도 있다.

### L55. 패널 단축키 라벨이 Insert·Home·Delete 같은 흔한 키를 'Key210'처럼 표시하고, 게임패드 버튼은 'MouseN'으로 표시함

- 영역: UI · 확인 · ui
- 위치: `skse/CalamityAffixes/src/PrismaTooltip.SettingsLayout.inl:160`, `skse/CalamityAffixes/src/PrismaTooltip.SettingsLayout.inl:7`, `skse/CalamityAffixes/src/PrismaTooltip.SettingsLayout.inl:279`, `skse/CalamityAffixes/src/PrismaTooltip.MenuInput.inl:1`

DescribeHotkey(SettingsLayout.inl:1-161)가 이름을 아는 키는 숫자 줄, 글자, F1-F12, 수정 키, Esc/Enter/Space/Tab/Backspace, -/=, 대괄호, 방향키뿐이다. 나머지 DIK 코드는 160행에서 "Key" + 숫자가 된다. MCM 키맵에서 패널 키를 Insert(0xD2=210)·Home·Delete 등에 지정하면, 패널과 툴팁의 안내(i18n.js updatePanelHotkeyHints)가 'Open/Close Panel: Key210'으로 나온다. 마우스 분기는 kMouseButtonOffset(256) 이상을 모두 마우스로 취급한다. 그래서 MCM Helper가 저장한 게임패드 코드(266 이상)에 'Mouse11' 같은 라벨이 붙는다. 게임패드 버튼은 NormalizeButtonKeyCode가 0을 돌려줘서 실제로 동작하지 않는다. 하지만 이는 문서화된 제한(NEXUS_DESCRIPTION.md:16/111)이므로, 결함은 라벨뿐이다.

**고칠 방향:** 게임패드 입력 지원은 이번 수정에 넣지 않는다. 문서상 미지원이고, 지원하려면 XInput 마스크를 SKSE 코드 266-281로 매핑하고 문서도 바꿔야 한다. DescribeHotkey만 고친다. (1) 이름이 없는 키에 이름을 붙인다. Insert, Delete, Home, End, PgUp, PgDn, `, ;, ', \, ',', ., /, CapsLock, 숫자패드 키, Pause/ScrollLock, F13-F15가 대상이다. (2) 마우스 분기를 256-265로 제한한다. (3) 266 이상(SKSE 게임패드 오프셋)은 마우스 분기보다 먼저 검사해서 'Gamepad (not supported)' 같은 게임패드 라벨을 돌려준다.

### L56. 레시피 검색창이나 콘솔에 글자를 입력하는 중에도 패널 단축키가 반응해 패널이 열리고 닫힘

- 영역: UI · 확인 · ui
- 위치: `skse/CalamityAffixes/src/PrismaTooltip.MenuInput.inl:45`, `skse/CalamityAffixes/src/PrismaTooltip.MenuInput.inl:113`, `skse/CalamityAffixes/src/PrismaTooltip.cpp:155`

PanelHotkeyEventSink::ProcessEvent(MenuInput.inl:113-134)는 BSInputDeviceManager의 원시 싱크라서, 어떤 메뉴가 열려 있든 모든 버튼 이벤트를 받는다. 유일한 필터인 ShouldToggleControlPanelForButton(MenuInput.inl:32-46)은 IsDown, 키 코드, IsGameplayReady만 확인한다. IsGameplayReady(PrismaTooltip.cpp:155-171)는 콘솔이나 텍스트 입력 상태를 걸러내지 않는다. 그래서 단축키를 K 같은 글자에 지정한 플레이어가 그 글자가 든 레시피 이름을 패널 검색창에 치면, 입력 도중에 패널이 닫힌다. 콘솔에서 그 글자를 쳐도 패널이 토글된다. 기본 키는 F11이라 기본 설정에서는 거의 겪지 않는다. 원래 보고에 있던 'MCM 키맵에서 다시 지정할 때 저널 위로 패널이 뜬다'는 부분은, 검증자 한 명이 대체로 틀렸다고 봤다.

**고칠 방향:** ShouldToggleControlPanelForButton이 두 경우에 false를 돌려주게 한다. (1) 콘솔이 열려 있을 때: ui->IsMenuOpen(RE::Console::MENU_NAME)으로 확인한다. CommonLibSSE-NG가 노출한다면 RE::ControlMap::GetSingleton()->textEntryCount > 0도 함께 확인해 게임 텍스트 입력 전반을 막는다. 필요하면 JournalMenu가 열려 있을 때도 막는다(MCM에는 이미 Toggle Panel 버튼이 있다). (2) 패널 자체의 입력창: 네이티브 쪽은 DOM 포커스를 볼 수 없다. 그래서 JS가 input/textarea의 focusin/focusout을 명령으로 알리고, C++가 그 플래그를 보고 토글을 건너뛴다. '포커스 중에는 출력되지 않는 키만 받는다'는 방식은 쓰지 않는다. 검증자 지적대로 글자 키에 지정한 단축키가 동작하지 않게 된다.

### L57. MCM에서 Calamity를 끄면 패널의 Transmute 등 이벤트 기반 명령이 아무 일도 하지 않으면서 '요청됨'이라고 표시함

- 영역: UI · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/PrismaTooltip.HandleUiCommand.inl:207`, `skse/CalamityAffixes/src/PrismaTooltip.HandleUiCommand.inl:13`, `skse/CalamityAffixes/src/PrismaTooltip.cpp:243`, `skse/CalamityAffixes/src/EventBridge.Events.cpp:115`

HandleImmediateRunewordCommand의 'runeword.insert' 분기는 EmitModEvent(kRunewordInsertRune)를 보내고 나서 상태를 다시 읽는다. EmitModEvent(PrismaTooltip.cpp:243-259)는 이벤트 소스만 있으면 항상 true를 돌려준다. 즉 'sent'가 true라고 이벤트가 처리됐다는 뜻이 아니다. 런타임이 꺼져 있으면 EventBridge의 모드 이벤트 처리가 UI·런타임 설정·디버그 이벤트를 뺀 게임플레이 이벤트를 버린다. 상태가 변하지 않았으니 코드는 마지막의 'Runeword -> transmute requested'로 떨어진다. 그래서 MCM에서 'Enabled'를 끈 상태로 Transmute를 두 번 눌러 확인하면, 패널은 변환을 요청했다고 표시한다. 하지만 조각은 쓰이지 않고 룬워드도 들어가지 않으며 HUD 메시지도 없다. 같은 패널의 재련·정제는 EventBridge를 직접 호출하므로 그대로 동작한다. 베이스·레시피 넘기기, mode.*, runeword.status, grant.next/set 같은 다른 이벤트 기반 명령도 같은 식으로 조용히 무시된다. 모드 이벤트 처리가 미뤄지는 경우에도, 결과가 확정되기 전에 feedback이 먼저 나간다.

**고칠 방향:** 'runeword.insert' 분기 맨 앞에서 bridge && !bridge->IsRuntimeEnabled()인지 확인한다. 그렇다면 EmitModEvent를 보내지 않는다. 대신 'Calamity is disabled in the MCM.' 같은 문구를 PushUiFeedback으로 보내고 return true 한다. 이 문구의 번역 항목을 i18n.js에 추가해야 한다. HandleEventBackedUiCommand의 게임플레이 명령(베이스·레시피 넘기기, mode.*, runeword.status, grant.next/set)에도 같은 검사를 넣는다. 더 나은 방법은 InsertRunewordRuneIntoSelectedBase를 제작 명령처럼 EventBridge의 동기 메서드로 직접 호출하는 것이다. 그러면 feedback과 갱신을 보내기 전에 결과를 알 수 있다.

### L58. 크거나 넓은 툴팁 하나가 그 세션의 툴팁 위치를 계속 옮겨 두고, 다음에 툴팁을 조정하면 옮겨진 위치가 저장됨

- 영역: UI · 확인 · ui
- 위치: `Data/PrismaUI/views/CalamityAffixes/scripts/tooltip.js:119`, `Data/PrismaUI/views/CalamityAffixes/scripts/tooltip.js:62`, `Data/PrismaUI/views/CalamityAffixes/scripts/state.js:99`

applyTooltipLayout()은 tooltip.js:113-130에 있다(보고된 315/320행은 틀렸다). 이 함수는 `tooltipLayout = fitTooltipLayoutToViewport(tooltipLayout, measureTooltipRect())`로 화면 맞춤 결과를 세션 전역 tooltipLayout(state.js:99)에 덮어쓴다. 맞춤 함수(tooltip.js:62-84)는 지금 뜬 툴팁 크기를 기준으로 top을 innerHeight-8-높이 이하로 자르고, right도 같은 식으로 자른다. 예를 들어 1080p에서 툴팁을 top=600까지 내려 뒀다고 하자. 1.25 자동 배율이 적용돼 약 700px이 된 긴 룬워드 툴팁이 뜨면 top이 372로 잘리고, 그 뒤로 모든 툴팁이 372에서 열린다. 이 상태에서 고급 탭의 'Text +'를 누르면, 플레이어가 고른 600 대신 372가 저장된다.

**고칠 방향:** tooltipLayout은 플레이어가 고른 값으로 유지한다. applyTooltipLayout에서 `const fitted = fitTooltipLayoutToViewport(tooltipLayout, measureTooltipRect())`를 계산한다. fitted는 tooltipPanel과 quickLaunch의 style.right/top에만 쓰고, tooltipLayout에 다시 대입하지 않는다. 마지막 맞춤 결과가 필요한 곳을 위해 tooltipRenderedLayout 같은 별도 변수에 보관한다. persistTooltipLayout은 지금처럼 플레이어가 정한 값만 저장한다.

### L59. 디버그 테스트 목록의 한국어 안내는 '준비'를 누르고 '확인됨'을 기다리라고 하지만, 기본 이중 언어 모드에서 버튼과 상태 칩은 영어('Set up', 'Seen', 'Waiting')로 나옴

- 영역: UI · 확인 · ui
- 위치: `Data/PrismaUI/views/CalamityAffixes/scripts/ui-text.js:405`, `Data/PrismaUI/views/CalamityAffixes/scripts/ui-text.js:426`, `Data/PrismaUI/views/CalamityAffixes/scripts/ui-text.js:431`, `Data/PrismaUI/views/CalamityAffixes/scripts/i18n.js:39`, `Data/MCM/Config/CalamityAffixes/settings.ini:16`

안내 문구는 t()로 만들기 때문에 'both' 모드에서 'EN / KO' 형태로 나온다. 그 한국어 절반이 "준비"와 "확인됨"을 따옴표로 인용한다. 반면 상태 칩(ui-text.js:426)과 버튼(:431)은 tCompact를 쓴다. tCompact(i18n.js:39-41)는 uiLang === "ko"일 때만 한국어를 돌려준다. MCM 기본값은 iUiLanguage=2(이중 언어)다. 그래서 기본 설정의 한국어 테스터가 고급 > 디버그 도구를 열면, 안내에 나온 두 단어를 어느 줄에서도 찾을 수 없다. 디버그 전용 화면의 표시 문제다.

**고칠 방향:** 안내의 한국어 절반(ui-text.js:405)에서 따옴표 안 단어를 컨트롤과 같은 헬퍼로 만든다: `"${tCompact("Set up", "준비")}"`, `"${tCompact("Seen", "확인됨")}"`. 'ko' 모드에서는 지금과 같고, 'both' 모드에서는 실제 버튼·칩 라벨과 일치한다. 칩과 버튼은 스타일 규칙대로 tCompact를 유지한다. 다른 방법으로, 컨트롤 이름 인용을 빼고 '줄의 버튼을 누르세요… 줄이 확인 표시로 바뀝니다'처럼 써도 모든 언어 모드에서 맞는다.

### L60. PR CI가 plugin_editor_id_tests를 빌드·실행하지 않는다. 에디터 ID 직접 조회 금지와 출시 ESP 레코드 수(829) 검사가 릴리스 때에만 돈다

- 영역: 도구·문서 · 확인 · tooling
- 위치: `skse/CalamityAffixes/tests/CMakeLists.txt:14`, `skse/CalamityAffixes/tests/run_runtime_gate_store_checks.sh:87`, `skse/CalamityAffixes/CMakeLists.txt:305`, `skse/CalamityAffixes/tests/plugin_editor_id_tests.cpp:108`, `.github/workflows/ci-verify.yml:134`

plugin_editor_id_tests.cpp는 run_runtime_gate_store_checks.sh 안에서만 빌드되고 실행된다. 이 스크립트는 메인 프로젝트의 ctest(CalamityAffixesRuntimeGateStoreChecks, CMAKE_HOST_UNIX 조건)에만 등록돼 있다. 그런데 ci-verify의 runtime-gate-tests 잡은 메인 빌드에서 StaticChecks와 플러그인 타깃만 빌드하고, ctest는 별도 프로젝트 skse/CalamityAffixes/tests/build에서만 돌린다. 그래서 PR에 `RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_...")` 같은 직접 호출을 넣거나, ESP에 레코드를 추가하면서 829 개수를 고치지 않아도 PR CI는 통과한다. 실패는 병합 후 릴리스 태그를 밀 때 release_build의 ctest에서 처음 드러난다.

**고칠 방향:** skse/CalamityAffixes/tests/CMakeLists.txt의 if(BUILD_TESTING) 블록 안에 세 줄을 추가한다: `add_executable(plugin_editor_id_tests plugin_editor_id_tests.cpp)`, `target_include_directories(plugin_editor_id_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)`, `add_test(NAME CalamityAffixesPluginEditorIdChecks COMMAND plugin_editor_id_tests)`. 테스트는 `__FILE__`을 기준으로 저장소 루트를 찾아 Data/CalamityAffixes.esp를 읽으므로 작업 디렉터리는 따로 정하지 않아도 된다. 다른 방법으로 ci-verify에서 `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel`을 함께 돌려도 된다.

### L61. 새 lint 검사 4종(1 기준 배율 정수 크기, BowSpeedBonus 거부, 미등록 actorValue, 적대 효과 음수 크기)이 keywords.appendedRecords와 다른 어픽스의 MGEF를 쓰는 효과를 건너뛴다

- 영역: 도구·문서 · 확인 · tooling
- 위치: `tools/lint_affixes.py:539`, `tools/lint_affixes.py:454`, `tools/lint_affixes.py:521`, `tools/lint_affixes.py:486`, `tools/lint_affixes.py:329`

`mgef_by_editor_id`는 어픽스마다 그 어픽스 자신의 records.magicEffect(s)만으로 새로 만들어진다(454-466행). 주문 효과가 가리키는 MGEF가 이 맵에 없으면 521-525행에서 `continue`로 넘어간다. 그래서 다른 어픽스나 appendedRecords에 선언된 MGEF를 쓰는 효과에는 539행의 정수 크기 검사와 적대 음수 검사가 돌지 않는다. AV 허용·거부 검사(486-498행)도 그 어픽스의 로컬 MGEF만 본다. appendedRecords 처리(329-401행)는 아트·정적 오브젝트·사운드 등만 모은다. 예를 들어 spec.root.json의 Freeze 공격 피해 약화나 Beast Rage가 공유하는 MGEF에 `"magnitude": 30`을 적어도 lint가 통과한다. 새 규칙 주석이 막겠다던 Ground식 실수가 그대로 빠져나가고, 출시 값은 공격 피해를 약 31배로 곱한다. 지금 게임에서 생기는 버그는 아니다. 방어선에 빈틈이 있다는 문제다.

**고칠 방향:** 어픽스 루프에 들어가기 전에 editorId → (경로, MGEF) 전역 맵을 하나 만든다. 이 맵은 모든 어픽스의 records.magicEffect(s), keywords.appendedMagicEffects, keywords.appendedRecords의 type MagicEffect 항목을 모두 담는다. 거부 AV와 미등록 AV 검사는 헬퍼로 빼서 MGEF가 선언된 곳마다 한 번씩 돌린다. 적대 음수, 1 기준 배율 정수, 즉시 recover 검사는 어픽스와 appendedRecords의 모든 주문 효과에 돌리되, MGEF는 전역 맵에서 찾는다. 찾지 못한 CAFF_ MGEF 참조는 오류로 보고한다.

### L62. lint와 생성기 어느 쪽도 runtime.action.passiveSpellEditorId(참조 93개)를 검사하지 않는다. 오타가 나면 상시 보너스가 조용히 빠진다

- 영역: 도구·문서 · 확인 · tooling
- 위치: `tools/lint_affixes.py:129`, `tools/lint_affixes.py:1029`

`_collect_spell_refs_from_action`(tools/lint_affixes.py:129-166)은 spellEditorId, stanceSpellEditorId, markSpellEditorId, adaptive spells, extraSpells, modeCycle spells만 모은다. passiveSpellEditorId는 읽지 않는다. 그래서 1029-1035행의 'Missing generated spell' 검사가 passive 참조를 보지 못한다. tools/CalamityAffixes.Generator에도 이 필드를 검사하는 코드가 없다. passiveSpellEditorId가 생성되지 않는 주문 이름을 가리켜도 lint, compose, 생성기가 모두 통과한다. 그러면 게임에서 해당 룬워드나 접미의 상시 효과(예: 한손 +10%)가 적용되지 않고, 흔적은 SKSE 로그의 warn 한 줄뿐이다. 원 리뷰가 든 구체 예시는 검증자들이 맞지 않는다고 했지만, 이 필드를 검사하는 경로가 없다는 점은 셋 다 확인했다.

**고칠 방향:** `_collect_spell_refs_from_action` 안에서 중첩 `add` 정의 바로 뒤, 모든 action_type 분기보다 앞에 다음 줄을 넣는다: `add(_extract_spell_editor_id(action.get("passiveSpellEditorId")), f"{affix_id}:action.passiveSpellEditorId")`. 런타임은 이 필드를 모든 슬롯과 모든 액션 타입에서 읽으므로 DebugNotify 접미를 포함한 모든 액션에 적용되어야 한다.

### L63. CI가 커밋된 Data/CalamityAffixes.esp와 KID/DISTR ini, InventoryInjector json을 생성기 출력과 비교하지 않는다. 비교 대상은 runtime_contract.json과 affixes.json뿐이다

- 영역: 도구·문서 · 확인 · tooling
- 위치: `tools/verify_runtime_contract_sync.py:74`, `tools/CalamityAffixes.Generator/GeneratorRunner.cs:16`, `.github/workflows/ci-verify.yml:111`

verify_runtime_contract_sync.py는 생성기 전체를 임시 Data 폴더에 돌린 뒤 runtime_contract.json 하나만 읽어 비교한다(67-80행). 생성기는 같은 실행에서 CalamityAffixes_KID.ini, CalamityAffixes_DISTR.ini, ESP, affixes.json, InventoryInjector json도 쓰지만, 이것들을 저장소 Data/와 비교하는 CI 단계는 없다. 모듈 스펙만 고치고 생성기를 안 돌린 경우는 lint의 생성 스냅샷 비교에 걸린다(원 리뷰의 예시는 실제로 잡힌다). 잡히지 않는 경우는 둘이다: 생성기 C# 코드나 Mutagen이 바뀌어 ESP 출력만 달라진 경우, 그리고 ESP를 스테이징하지 않은 부분 커밋. 이때 CI는 통과하고, 저장소를 그대로 받아 쓰는 사람은 릴리스 zip과 다른 옛 ESP 값을 받는다.

**고칠 방향:** verify_runtime_contract_sync.py는 이미 임시 출력을 만들고 있으므로 비교 대상만 늘린다. 생성된 CalamityAffixes.esp를 Data/CalamityAffixes.esp와 바이트 단위로 비교한다. Mutagen 출력이 결정적이지 않으면 FormKey, EditorID, 레코드 바이트 단위 비교로 바꾼다. CalamityAffixes_KID.ini, CalamityAffixes_DISTR.ini, InventoryInjector json도 Data/와 비교한다. 검증자 1명은 그 전에 커밋된 Data/를 그대로 받아 쓰는 경로를 지원할지부터 정하라고 했다. 커밋된 Data/의 DLL은 v2.2.2 이후 갱신되지 않았다.

### L64. rc 태그가 붙은 HEAD에서 build_mo2_zip.sh와 release_verify.sh가 버전을 다르게 정해 `release_verify.sh --with-mo2-zip`이 항상 VERSION MISMATCH를 낸다. rc와 정식 태그가 함께 붙어 있으면 zip 이름이 rc로 붙는다

- 영역: 도구·문서 · 확인 · tooling
- 위치: `tools/release_verify.sh:86`, `tools/build_mo2_zip.sh:14`, `tools/release_verify.sh:161`

build_mo2_zip.sh(14-29행)는 CAFF_PACKAGE_VERSION, HEAD를 가리키는 `v*` 태그 중 `sort -V`의 마지막 것, CMake 버전 순으로 버전을 정한다. release_verify.sh의 resolve_expected_package_version(86-100행)은 CAFF_PACKAGE_VERSION 다음에 바로 CMake(2.3.0)로 가고 태그는 보지 않는다. 그래서 v2.3.0-rc4를 태깅하고 `--with-mo2-zip`을 다시 돌리면 zip은 2.3.0-rc4, 기대값은 2.3.0이 되어 근거 없는 실패가 난다. 같은 커밋에 v2.3.0을 더 붙여도 `sort -V`는 v2.3.0-rc4를 마지막으로 고른다. 그 결과 정식 릴리스용 로컬 zip과 넥서스 업로드 파일이 rc4 이름으로 나온다.

**고칠 방향:** tools/path_helpers.sh에 공용 resolve_package_version 함수를 두고 두 스크립트가 함께 쓴다. 순서는 CAFF_PACKAGE_VERSION, HEAD 태그, CMake다. HEAD에 `v*` 태그가 여러 개면 `sort -V`의 마지막 것 대신 pre-release 접미사가 없는 태그를 우선한다. release_verify.sh는 버전을 한 번만 정해서 `CAFF_PACKAGE_VERSION="${expected_version}" CAFF_PACKAGE_OUTPUT_FILE=... build_mo2_zip.sh`처럼 빌더에 넘긴다. 이렇게 하면 두 스크립트의 버전이 어긋날 수 없다.

### L65. path_helpers의 임시 별칭 디렉터리가 명령 치환 안에서 만들어져서 cleanup_repo_run_root가 지우지 못한다

- 영역: 도구·문서 · 확인 · tooling
- 위치: `tools/path_helpers.sh:37`, `tools/build_mo2_zip.sh:7`, `tools/release_verify.sh:6`

resolve_repo_run_root는 저장소 경로에 공백이 있고 ../calamity나 ../projects/calamity 별칭이 없으면, mktemp 디렉터리와 저장소 심링크를 만들고 CAFF_REPO_RUN_ROOT_TEMP_DIR에 경로를 넣는다(37-39행). 그런데 두 호출부가 `repo_run_root="$(resolve_repo_run_root "${repo_root}")"` 형태로 서브셸에서 부른다. 그래서 이 변수는 부모 셸에 전달되지 않고, EXIT trap의 cleanup_repo_run_root는 지울 대상을 모른다. 해당 조건에서는 build_mo2_zip.sh나 release_verify.sh를 돌릴 때마다 /tmp/caff-repo-link.XXXXXX 디렉터리가 하나씩 남는다. 이 사용자 환경에는 ../projects/calamity가 있어서 지금은 생기지 않는다.

**고칠 방향:** 함수가 printf로 출력하는 대신 전역 결과 변수(예: CAFF_REPO_RUN_ROOT_RESULT)에 값을 넣게 한다. 호출부는 서브셸 없이 `resolve_repo_run_root "${repo_root}"; repo_run_root="${CAFF_REPO_RUN_ROOT_RESULT}"`로 부른다. 호출부를 바꾸지 않으려면 cleanup_repo_run_root에서 처리해도 된다: CAFF_REPO_RUN_ROOT_TEMP_DIR가 비어 있고 repo_run_root가 `${TMPDIR:-/tmp}/caff-repo-link.*/calamity` 형태이면 그 dirname을 지운다.

### L66. qa_skse_logcheck가 DLL이 더 이상 쓰지 않는 훅 설치 로그 줄을 찾는다. 테스트 픽스처도 같은 옛 줄이라 테스트는 통과하고, 실제 시작 로그는 hit_pipeline에서 실패한다

- 영역: 도구·문서 · 확인 · tooling
- 위치: `tools/qa_skse_logcheck.py:161`, `tools/tests/test_qa_skse_logcheck.py:21`, `skse/CalamityAffixes/src/Hooks.cpp:159`

qa_skse_logcheck.py:158-165의 필수 그룹 hit_pipeline은 `installed HandleHealthDamage vfunc hooks`를 찾는다. 하지만 커밋 9890572(2026-02-25, HandleHealthDamage 훅 체인 호환성 강화) 이후 Hooks.cpp:159-164가 쓰는 줄은 `CalamityAffixes: installed HandleHealthDamage hooks (idx=0x..., actor=..., character=..., player=...)`다. test_qa_skse_logcheck.py:21의 픽스처도 옛 문구라 테스트로는 드러나지 않는다. 게임을 켜고 전투 없이 세이브만 불러온 뒤 CalamityAffixes.log에 이 도구를 돌리면, 훅이 제대로 설치됐는데도 `missing: hit_pipeline`과 `RESULT: FAIL`이 나온다.

**고칠 방향:** qa_skse_logcheck.py의 첫 hit_pipeline 정규식을 `r"installed HandleHealthDamage (?:vfunc )?hooks"`로 바꾼다. 그러면 지금 문구와 옛 로그를 모두 받는다. test_qa_skse_logcheck.py:21의 픽스처는 Hooks.cpp가 실제로 쓰는 줄로 바꾼다. 예: `[info] CalamityAffixes: installed HandleHealthDamage hooks (idx=0x..., actor=..., character=..., player=...)`.

### L67. 전염이 미스트의 지속 노출은 옮기지 않으면서, 미스트만 걸린 적을 처치해도 발동과 ICD를 소모한다. CHANGELOG 문구와도 다르다

- 영역: 도구·문서 · 확인 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Status.cpp:796`, `skse/CalamityAffixes/src/EventBridge.Actions.Status.cpp:355`, `skse/CalamityAffixes/src/EventBridge.Actions.Status.cpp:188`, `affixes/modules/keywords.affixes.runewords.json:3580`, `CHANGELOG.md:59`

CAFF_SPEL_RW_MIST_FOG는 statusTag가 Exposed인 주문이고 효과가 셋이다: MagickaRateMult 6초, HIT_SHOCK_OVERLOAD 지속 0(즉시), RIFT_SPARK 6초. 전염 처리는 즉시 효과가 섞인 주문을 통째로 건너뛴다(Status.cpp:794-797). 이것은 docs/plans/2026-10-09-shared-status-effects.md:177에 적힌 의도된 설계다. 문제는 사전 검사 HasSpreadableStatus(Status.cpp:355-372)다. 화상이나 누적형이 아닌 장부 항목이 하나라도 있으면 true를 돌려준다. 그래서 미스트만 걸린 적을 처치하면 전염이 발동은 하지만 주변 2명에게 아무것도 옮기지 않고 3초 ICD만 써 버린다. 그 3초 안에 진짜 상태(화상, 화염 약화의 노출 등)가 걸린 적을 처치해도 아무것도 옮겨지지 않는다. 게다가 CHANGELOG.md:59는 "전염은 이제 지속되는 상태만 옮깁니다"라고 적어서, 미스트의 지속 노출은 옮겨지는 것처럼 읽힌다.

**고칠 방향:** 주문을 통째로 건너뛰는 지금 설계는 그대로 두고, 시간제 효과만 따로 옮기는 경로는 만들지 않는다. 대신 사전 검사를 실제 루프와 맞춘다. HasSpreadableStatus의 장부 루프에서 화상, 누적형, kDoom이 아닌 항목마다 `RE::TESForm::LookupByID<RE::SpellItem>(statuses[i].spellFormID)`로 주문을 찾는다. 결과가 null이거나 HasInstantEffect(spell)가 true면 그 항목을 건너뛴다. kDoom은 실제 루프에서 즉시 효과 검사보다 먼저 별도 경로로 처리되므로 제외한다. HasInstantEffect는 같은 파일 188행의 익명 네임스페이스에 이미 앞서 정의돼 있다. CHANGELOG.md:59는 '즉시 효과가 섞인 주문(미스트)은 통째로 옮기지 않습니다'처럼 고친다.

### L68. 공개 SUFFIX_EFFECTS.md에 Eagle Eye 계열이 영향을 주는 능력치가 `None`으로 나온다. 문서 생성기가 빈 운반용 효과의 actorValue를 그대로 찍는다

- 영역: 도구·문서 · 확인 · docs
- 위치: `tools/gen_suffix_doc.py:94`, `tools/gen_suffix_doc.py:113`, `docs/SUFFIX_EFFECTS.md:27`, `affixes/modules/keywords.affixes.suffixes.json:2616`

eagle_eye 세 단계는 이제 archetype이 Script이고 actorValue가 "None"인 운반용(no-op) MGEF다. 활 당기기 속도는 런타임(EventBridge)이 적용한다. 그런데 tools/gen_suffix_doc.py는 entries[0].records.magicEffect.actorValue를 가공 없이 두 곳에 넣는다: 요약 표(94·102행)와 계열별 섹션의 '영향 능력치' 줄(113행). 그래서 공개 접미 목록을 읽는 플레이어에게는 Eagle Eye 계열이 `None`에 영향을 준다고 보인다.

**고칠 방향:** gen_suffix_doc.py에 계열별 표시 라벨 헬퍼(예: display_stat(fam, entries))를 하나 만들어 요약 행과 '영향 능력치' 줄 양쪽에서 쓴다. actorValue가 "None"이거나 archetype이 Script면 `{"eagle_eye": "활 당기기 속도 (런타임 적용)"}` 같은 대응표에서 라벨을 가져온다. 대응표에 없는 None 계열이 나오면 생성을 멈추게 해서, 새 운반용 계열이 조용히 None으로 나가지 않게 한다. display_magnitude()에는 원래 actorValue를 그대로 넘긴다. 그다음 문서를 다시 생성한다.

### L69. NEXUS_DESCRIPTION.md의 제작·룬워드 설명이 2.3.0 동작과 룬워드 수에 맞지 않는다

- 영역: 도구·문서 · 확인 · docs
- 위치: `docs/NEXUS_DESCRIPTION.md:25`, `docs/NEXUS_DESCRIPTION.md:120`, `docs/NEXUS_DESCRIPTION.md:21`, `docs/NEXUS_DESCRIPTION.md:116`, `docs/NEXUS_DESCRIPTION.md:30`, `docs/NEXUS_DESCRIPTION.md:125`

한국어 25행과 영어 120행은 모든 제작 작업이 완성 룬워드와 그 성장 상태를 보존한다고 적혀 있다. 하지만 2.3.0에는 정제 오브 1개를 써서 룬워드를 지우는 '룬워드 제거'(AffixCraftAction::kRemoveRuneword, 패널 명령 runeword.remove:)가 있다. 확인 스크롤 줄(21/116행)은 일반 어픽스 1~3개를 붙인다고만 적었지만, 룬워드 장비를 확인하면 접미가 최대 2개만 붙는다. 룬워드 수도 94개로 적혀 있다(30·34·125·129행). 쉐도우 복서가 추가돼 실제로는 95개다. 넥서스 독자는 어떤 제작 작업도 룬워드를 지우지 않는다고 믿게 된다.

**고칠 방향:** 한국어와 영어 두 섹션에 같은 변경을 한다. 25/120행은 '룬워드 제거를 뺀 모든 작업은 룬워드를 보존'으로 바꾼다. 룬워드 제거 항목을 추가한다: 정제 오브 1개, 비워진 머리 칸에 새 접두를 굴림(이미 접두가 있는 옛 장비는 룬워드만 빠짐), 접미와 남은 재련 횟수 유지, 6초 안에 한 번 더 눌러야 실행. 확인 스크롤 항목에 룬워드 장비는 접미 최대 2개라는 경우를 더한다. 94는 모두 95로 바꾼다.

### L70. NEXUS_DESCRIPTION_BBCODE.txt(넥서스 BBCode 사본)가 낡았다: 옛 발동 감쇠 단계, 2.0 이전 제작 설명, 룬워드 94개가 그대로다

- 영역: 도구·문서 · 확인 · docs
- 위치: `docs/NEXUS_DESCRIPTION_BBCODE.txt:15`, `docs/NEXUS_DESCRIPTION_BBCODE.txt:115`, `docs/NEXUS_DESCRIPTION_BBCODE.txt:50`, `docs/NEXUS_DESCRIPTION_BBCODE.txt:150`, `docs/INDEX.md:19`

한국어 15행과 영어 115행은 재련 오브가 선택 장비에 어픽스를 부여하거나 다시 굴린다고 적었다. 지금은 빈 장비에는 확인 스크롤(kIdentify)만 쓸 수 있고, 재련은 선택한 어픽스가 있어야 한다(AffixCraftingPolicy.h:176-188). 확인, 선택 재련, 정제, 교환 설명도 빠져 있다. 50·150행의 Best Slot Wins는 아직 '1개=100%, 2개=80%, 3개=65%, 4개=50%'다. 2.3.0 머리 칸 문구로 바뀐 NEXUS_DESCRIPTION.md와 맞지 않는다. 룬워드도 94개로 적혀 있다. docs/INDEX.md:19는 여전히 이 파일을 넥서스 BBCode 설명으로 안내한다. 이 파일을 2.3.0 넥서스 페이지에 붙이면, 플레이어는 재련 오브로 새 장비에 어픽스를 붙이고 장비 하나에 발동 어픽스가 3~4개 붙는다고 읽게 된다. 다만 실제로 넥서스에 붙이는 것은 릴리스 키트의 BBCode라서 실제 피해는 원 리뷰가 말한 것보다 작다(검증자 2명).

**고칠 방향:** 다시 만들기보다 퇴역시킨다. 파일을 지우거나, 릴리스 키트(docs/releases/...-nexus-vX.md) 5절을 보라는 한 줄 안내로 바꾼다. docs/INDEX.md:19도 키트를 가리키게 고친다. 2.3.0 정식 때는 키트의 BBCode 절과 NEXUS_DESCRIPTION.md를 함께 갱신한다. 갱신 내용은 룬워드 94→95, 머리 칸 기준 Best Slot Wins 문구, 확인·선택 재련·정제·교환 설명이다. 파일을 남기기로 하면 NEXUS_DESCRIPTION.md를 바탕으로 다시 만든다.

### L71. README의 다운로드·설치 안내와 개수가 낡았다(v2.2.2 링크, 접두 73개, 룬워드 94개, 생성기 계약 94)

- 영역: 도구·문서 · 확인 · docs
- 위치: `README.md:19`, `README.md:3`, `README.md:4`, `README.md:31`, `README.md:179`, `README.md:343`, `README.md:344`

README.md에는 커밋되지 않은 변경이 없다. 그런데 3·4행 배지와 19-23행의 '현재 정식 릴리스', MO2 ZIP, SHA-256 링크가 모두 v2.2.2를 가리킨다. 저장소에는 v2.2.3~v2.2.8과 v2.3.0-rc1~rc3 태그가 있다. README의 '다운로드'를 따라 설치한 플레이어는 2.2.2를 받는다. 2.2.2에는 1.7.x 런타임 수정도, PluginEditorIds(po3 Tweaks 없이 자기 폼 찾기)도 없다. 31행은 접두 73개·룬워드 94개, 179·343·344행도 94개로 적혀 있다(실제는 접두 76개, 룬워드 95개). 특히 344행은 생성기가 '정확히 94개 레시피' 계약을 검사한다고 적고 있어서, 이 숫자를 믿고 작업한 기여자는 생성기 검사에서 막힌다.

**고칠 방향:** ZIP과 SHA-256 링크에 releases/latest는 쓰지 않는다. 자산 이름에 버전과 날짜가 들어가 latest/download 링크가 깨지고, latest는 프리릴리스를 건너뛰어 지금은 v2.2.8로 간다. v2.3.0 정식을 낼 때 배지(3·4행)와 다운로드 블록(19-23행)을 v2.3.0 태그와 실제 ZIP 이름으로 바꾼다. 그 전까지는 v2.2.8로 맞춘다. 31행은 접두 76개·룬워드 95개로, 179·343·344행의 94는 95로 바꾼다. 344행의 계약 문장도 포함이다.

### L72. 접두에 붙은 장비 종류 표시([무기]/[방어구], CHANGELOG의 '무기'/'방어구')가 실제로 지켜지지 않는다. 공용 풀이라 모든 접두가 어느 장비에나 붙는다

- 영역: 도구·문서 · 확인 · docs
- 위치: `CHANGELOG.md:44`, `affixes/modules/spec.root.json:18`, `Data/SKSE/Plugins/CalamityAffixes/affixes.json:17`, `skse/CalamityAffixes/src/EventBridge.Loot.Service.cpp:262`, `skse/CalamityAffixes/src/EventBridge.Config.LootPools.cpp:17`, `docs/PREFIX_EFFECTS.md`

spec.root.json:18과 출시본 Data/SKSE/Plugins/CalamityAffixes/affixes.json:17은 모두 "sharedPool": true다. 그래서 RollLootAffixIndex(EventBridge.Loot.Service.cpp:262-300)는 무기 풀과 방어구 풀을 합친 lootSharedAffixes(EventBridge.Config.LootPools.cpp:17-20)에서 접두를 뽑는다. 검을 확인하면 전염이나 Stone Ward 같은 [방어구] 접두가, 투구를 확인하면 파멸의 낙인이나 약점 포착 같은 [무기] 접두가 나올 수 있다. 하지만 CHANGELOG.md:44와 접두 목록은 무기용·방어구용으로 정해진 것처럼 적혀 있다. 효과 자체는 어느 칸에 있어도 동작한다.

**고칠 방향:** 코드는 그대로 두고 문서만 고친다. sharedPool은 1.0부터의 의도된 설계이고, 장비 종류로 거르면 드롭·재련 풀의 밸런스가 바뀐다. CHANGELOG.md:44의 새 접두 3종에서 '무기, '/'방어구, '를 빼거나, '(무기용)' 같은 테마 표시로 바꾸고 어느 장비에나 붙을 수 있다는 설명을 단다. v2.3.0 정식 릴리스 노트도 똑같이 고친다. docs/PREFIX_EFFECTS.md에도 같은 안내를 넣는다. 다시 생성해도 남도록 tools/gen_prefix_doc.py에 넣는다.

### L73. 활 속도 기록이 오래된 applied 값을 믿어서, 다른 모드가 WeaponSpeedMult를 덮어쓰면 저장 전 제거가 음수 오프셋을 저장에 남김

- 영역: 전투 C++ · 논란 · save-data
- 위치: `skse/CalamityAffixes/src/EventBridge.BowDrawSpeed.cpp:113`

기본값 0에 우리 +1.15가 적용된 상태에서 다른 모드가 WeaponSpeedMult를 1.0으로 강제하면 우리 몫이 사라집니다. 다음 동기화에서 withoutOurs=-0.15, desired=1.30이 되어 값은 다시 1.15로 보이고 applied=1.30이 됩니다. 저장할 때 ModActorValue(-1.30)이 -0.15를 남기고, 이 값이 저장 파일에 들어갑니다. 다른 모드의 1.0은 사라지고, 캐릭터는 로드 뒤와 제거 뒤에도 음수 WeaponSpeedMult를 지닙니다. 외부에서 ForceActorValue로 덮어쓸 때만 생깁니다.

**고칠 방향:** 코드는 바꾸지 않습니다(회의론자 다수 의견). 마지막으로 본 값과 다르면 applied를 0으로 리셋하는 방식이나 저장 전 제거를 제한하는 방식은 쓰지 마십시오. 코드는 덧셈 변화(원소의 분노, 마법부여, 능력, 다른 플러그인의 ModAV, 베이스 SetAV, 둔화 효과)와 덮어쓰기를 구분하지 못합니다. 그래서 그 방식은 흔한 경우에 우리 보너스를 저장에 양수로 남기는 더 나쁜 누수를 만듭니다. 할 수 있는 건 EventBridge.BowDrawSpeed.cpp에 '외부 ForceActorValue가 WeaponSpeedMult를 덮어쓰면 이 계산이 어긋날 수 있다'는 주석을 다는 정도입니다.

### L74. 시체 폭발 '연쇄' 감쇠와 깊이 상한이 폭발 처치로 생긴 연쇄가 아니라 시간 창 안의 모든 폭발에 적용됨

- 영역: 전투 C++ · 논란 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Actions.Corpse.cpp:381`

죽음의 장작 T3만 있을 때, 강타 한 번에 같은 프레임에서 셋을 죽이면 폭발이 1.0배, 0.7배, 0.49배로 들어갑니다. 어느 것도 다른 폭발이 일으킨 게 아닌데도 그렇습니다. 큰 전투에서 약 0.8초마다 칼로 처치하면 2~5번째 폭발은 0.7, 0.49, 0.34, 0.24배이고, 6번째는 폭발 없이 0.9초가 지날 때까지 거부됩니다(깊이 5 ≥ maxChainDepth 5). 툴팁은 폭발로 생긴 처치만 줄어드는 것처럼 읽힙니다.

**고칠 방향:** 런타임은 그대로 둡니다. 원인 태깅은 넣지 마십시오. 의도된 같은 프레임 폭발 완충을 없애고, 엔진이 폭발 처치와 무기 처치를 같은 방식으로 귀속시키기 때문에 상태만 늘고 얻는 게 없습니다. 원하면 문구만 바꿉니다. 영어는 'Successive explosions within 0.9s deal 0.7x each (max 5 in a row), 4/sec limit', 한국어는 '0.9초 안에 이어진 폭발은 위력 30%씩 감소, 최대 5연속, 초당 4회 제한'입니다. 소환 장작 등 같은 규칙을 쓰는 문구와 문서도 같이 바꿉니다.

### L75. 명시적 룬워드 어픽스 파싱이 실패하면, 합성 경로가 같은 토큰으로 다른 효과의 대체 어픽스를 조용히 등록함

- 영역: 아이템·저장 C++ · 논란 · correctness
- 위치: `skse/CalamityAffixes/src/EventBridge.Config.RunewordSynthesis.cpp:44`, `skse/CalamityAffixes/src/EventBridge.Config.RunewordSynthesis.cpp:114`, `skse/CalamityAffixes/src/SpellSet.cpp:27`, `skse/CalamityAffixes/src/EventBridge.Config.AffixParsing.cpp:85`

레시피의 명시적 결과 어픽스가 없으면 SynthesizeRunewordRuntimeAffixes(RunewordSynthesis.cpp:44-118)가 스타일 기반 대체 어픽스를 만들고, info 수준으로만 로그를 남긴다. 부분 업데이트나 버전 불일치로 참조 레코드가 빠지면 이렇게 된다. 룬워드 아이템은 이름을 유지하고 패널 레시피 문구도 원래 효과를 설명하지만, 전투에서는 엉뚱한 적응 타격이나 자기 버프를 시전한다. 대체 어픽스는 접두 칸 룬워드로도 세지 않는다. 대체에 쓰는 SpellSet 에디터 ID 2개(CAFF_SPEL_RW_EXILE_BARRIER, CAFF_SPEL_RW_PHOENIX_SURGE)는 현재 ESP에 없다. 다만 배포 데이터에서는 95개 결과 어픽스가 모두 존재하므로, 정상 설치에서는 이 경로가 실행되지 않는다(평가자 2명이 반박).

**고칠 방향:** 선택적 정리 항목이다. 최소한 RunewordSynthesis.cpp:114의 레시피별 합성 로그를 info에서 warn(또는 error)으로 올리고, '작성된 결과 어픽스 <id>가 없어 합성 대체물로 바꿨다'고 레시피와 resultAffixId를 밝힌다. AffixParsing.cpp:85에서 키워드 조회가 실패할 때도 warn을 남겨, 어픽스가 조용히 빠지지 않게 한다. SpellSet.cpp:27-28의 죽은 조회 2개는 지우거나 고친다. 둘 다 실패해도 안전하다. 합성 대체 자체를 없애고 기존 kMissingResultAffix 경로가 레시피를 막게 하는 안(평가자 1명)도 있다. 이 경우 runtime_gate_store_checks_runtime.cpp의 파이프라인 소스 문자열 테스트도 갱신한다. 대체를 유지한다면 합성 어픽스에 AffixSlot::kPrefix를 준다.

### L76. 룬 조각은 다 있는데 변환이 막힌 경우에도 패널은 '룬 조각이 부족합니다'라고 표시함

- 영역: 아이템·저장 C++ · 논란 · ui
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.PanelState.cpp:631`, `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.PanelState.cpp:575`, `Data/PrismaUI/views/CalamityAffixes/scripts/runeword.js:434`, `Data/PrismaUI/views/CalamityAffixes/scripts/runeword.js:1046`

PanelState.cpp:629-633(레거시 분기 575-577 포함)은 부족한 조각이 없는데 canApplyResult가 false이면 BuildRunewordApplyBlockMessage(reason)를 panelState.missingSummary에 넣는다. runeword.js:434는 missingSummary가 비어 있지 않으면 무조건 '룬 조각이 부족합니다'로 바꾸고, 1046행은 'Missing: Affix slots full (max 3)'를 보여 준다. 그래서 접미 3개에 접두가 없는 옛 아이템에 필요한 룬을 모두 가진 경우, 버튼은 꺼지고 안내는 조각 부족을 말한다. 실제 해결책인 '먼저 정제'는 안내에 나오지 않는다. 다만 평가자 2명은 현재 실제 아이템으로는 이 상태(kAffixSlotsFull)에 도달할 수 없다고 반박했다.

**고칠 방향:** 지금 당장 필요하지 않다는 의견이 다수다. 고친다면 패널 데이터 형식을 바꾸지 않아 옛 DLL과도 맞는 가장 작은 방법을 쓴다. resolveRunewordPanelActionState에서 state.requiredRunes 중 owned < required인 항목이 있을 때만 '룬 조각이 부족합니다'를 보여 준다. 그렇지 않은데 missingSummary가 있으면 막힌 이유를 영어·한국어로 안내한다. 예: 'Affix slots are full. Scour the item first.' / '어픽스 칸이 가득 찼습니다. 먼저 정제하세요.' runeword.js:1046 상태줄에도 같은 규칙을 적용해, 막힌 이유가 'Missing: ...'으로 나오지 않게 한다. 더 깔끔한 방법은 막힌 이유를 별도 패널 필드(예: applyBlockReason 열거 문자열 또는 applyBlockMessageEn/Ko)로 보내고, missingSummary는 조각 전용으로 두는 것이다. 이 경우 HandleUiCommand.inl:187-193도 고치고, 필드가 없을 때는 JS 판정으로 돌아가게 한다.

### L77. 지연 처리된 줍기 이벤트에서 월드 레퍼런스를 더 이상 찾을 수 없으면, guard에 기록된 드롭 키를 거부해 상태를 잃음

- 영역: 아이템·저장 C++ · 논란 · save-data
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.cpp:549`, `skse/CalamityAffixes/src/EventBridge.Events.cpp:27`, `skse/CalamityAffixes/include/CalamityAffixes/DeferredEventDispatcher.h:62`

DispatchEngineEvent는 TESContainerChangedEvent를 값으로 복사하고, `reference`는 ObjectRefHandle로 들고 있다. 다른 작업 스레드가 _stateMutex를 잠깐 잡고 있으면 이벤트가 지연 큐로 들어간다. 큐를 비울 때 레퍼런스가 이미 사라졌으면 referenceFormID가 0이 되고, IsDetachedWorldReferenceMatch가 guard에 기록된 드롭 키를 거부한다. 그러면 버렸다가 다시 주운 어픽스 아이템이 어픽스 없이 인벤토리에 들어온다. 그런데 엔진 평가자는 Ghidra(SkyrimSE 1.6.1170)로 확인한 결과를 근거로 반박했다. 게임 바이너리는 이 필드(offset 0x10)에 핸들이 아니라 FormID를 넣는다는 것이다. 그렇다면 문제의 원인은 지연이 아니라 필드 타입을 잘못 해석한 데 있다. 다른 평가자 1명은 엔진 조건을 확인하지 못해 판단을 보류했다.

**고칠 방향:** IsDetachedWorldReferenceMatch의 '참조 키 0이면 거부' 규칙은 완화하지 않는다. test_instance_key_transfer_policy.cpp:43이 고정하고 있다.
(1) 지연 경로 보완(평가자 2명): DispatchEngineEvent에 `if constexpr (std::is_same_v<Event, RE::TESContainerChangedEvent>)` 분기를 넣는다. 이벤트를 받는 시점, 즉 레퍼런스가 아직 살아 있는 동기 콜백 안에서 event.reference를 한 번 해석해 FormID와 ExtraUniqueID 키(baseID/uniqueID)를 EngineEventContext(예: containerRefFormID, containerRefUniqueKey)에 담는다. HandleContainerChangedEvent는 큐를 비울 때 get()을 다시 부르지 않고 이 값을 쓴다. NiPointer로 바꾸는 방식은 쓰지 않는다. 보조 장치로, 담아 둔 FormID가 0이라도 원래 핸들이 guard 항목과 일치하면 그 키를 받아들일 수 있다.
(2) 필드 타입 수정(엔진 평가자): offset 0x10을 FormID로 읽는다. 공급된 헤더를 `RE::FormID reference; // 10`으로 고치거나 `std::bit_cast<RE::FormID>(a_event->reference)`를 쓴다. 드롭 키는 이 FormID로 MakeDetachedWorldInstanceKey를 만들고, ExtraUniqueID는 LookupByID<RE::TESObjectREFR>로 얻는다. 드롭 guard의 키도 가짜 핸들 대신 FormID로 바꾼다. 드롭의 TESUniqueIDChangeEvent(newUniqueID==0)도 활용할 수 있다. 비영속 레퍼런스의 줍기는 reference가 0이므로 드롭 때 기록한 ExtraUniqueID로 맞춰야 한다. 어느 쪽이든 인게임에서 드롭 후 다시 줍기와 무기 거치대를 다시 확인한다.

### L78. 테스트 세트와 '테스트 어픽스 부여'가 실제로는 나올 수 없는 3~4개 발동 접두 아이템을 만들어, QA가 65%/50% 패널티가 걸린 확률을 보게 됨

- 영역: 아이템·저장 C++ · 논란 · tooling
- 위치: `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp:489`, `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp:407`, `skse/CalamityAffixes/src/EventBridge.Triggers.ActiveCounts.cpp:101`

kTestSets 11~14(Assign.cpp:488-495)는 아이템 하나에 발동 접두를 3~4개 넣는다. GrantTrapAffixToSelectedBase도 최대 4개까지 쌓는다. 이 접두들은 모두 CountProcPenaltySlots에 들어가서, ResolveMultiAffixProcPenalty가 0.65나 0.5를 적용한다. 그래서 테스트 세트 12('누적 미터')의 Frost Strike·Spark Strike(적중 시 100%)는 절반쯤만 발동하고, 미터가 실제 장비보다 두 배쯤 느리게 찬다. 세트 11의 Shadow Veil(피격 시 100%)은 65%로 발동한다. 그러면 QA가 플레이어는 볼 일 없는 수치로 임계값이나 버프 교체를 판단하게 된다. 다만 평가자 2명은 현재 테스트 계획에 이 확률에 의존하는 항목이 없어 실제 피해가 없다고 반박했다.

**고칠 방향:** 디버그로 만든 아이템을 패널티에서 빼지는 않는다. 인스턴스별 영구 플래그가 필요하고, QA가 실제 규칙을 시험하지 않게 된다. 코드 변경은 필수가 아니다. 다음 중 필요한 것만 한다.
- 발동 빈도나 미터가 차는 속도를 판단하는 테스트 계획 항목에는 발동 접두 하나에 접미를 붙인 세트(예: 세트 15, ice_form + frost_strike)를 쓰거나, 발동 접두를 두 개로 줄여 0.8배로 맞춘다.
- spawn.testset을 쓰는 항목의 todo 문구에 아이템별 패널티나 툴팁의 유효 확률을 적는다.
- 원하면 GrantTrapAffixToSelectedBase와 GrantTestSetToSelectedBase의 HUD 안내 '(N/4 slots)' 뒤에 ResolveMultiAffixProcPenalty(CountProcPenaltySlots(slots))로 계산한 'proc x0.5' 같은 배율을 붙인다.

### L79. 생성기의 하드코딩된 대체 검증 계약에 DoomMark·SpreadStatus가 빠져 있음

- 영역: 데이터·생성기 · 논란 · tooling
- 위치: `tools/CalamityAffixes.Generator/Spec/AffixSpecLoader.cs:1362`

GetFallbackContract(AffixSpecLoader.cs:1362-1386)의 동작 목록은 EchoStrike에서 끝난다. 0799eb3(rc3)에서 tools/affix_validation_contract.json에는 DoomMark와 SpreadStatus를 추가했지만 이 대체 목록은 고치지 않았다. 원래 지적은 저장소 밖에서 생성기를 돌리면 정상적인 DoomMark/SpreadStatus 어픽스에 'runtime.action.type must be one of [...]' 오류가 나서 계약 파일이 없다는 게 아니라 스펙이 틀린 것처럼 보인다는 것이었다. 검토자 3명 모두 이 시나리오는 실제로 일어나지 않는다고 봤다(저장소 밖에서는 더 일찍 실패하고, 모든 실행이 저장소 안에서 이루어짐). 남는 문제는 쓰이지 않는 오래된 목록이다.

**고칠 방향:** 선택적 정리. GetFallbackContract를 지우고, tools/affix_validation_contract.json이 없거나 파싱되지 않으면 LoadValidationContract가 경로를 밝힌 FileNotFoundException/InvalidDataException을 던지게 한다. RunewordContractCatalog.Load의 엄격한 기본값과 같은 방식이다. 두 목록을 손으로 맞추는 것보다 간단하고, 저장소 안의 실행에는 영향이 없다. 빠진 두 항목만 추가하는 방식은 다시 어긋날 수 있으므로 피한다.

### L80. 패널을 닫을 때 피드백 줄의 hover title이 지워지지 않아, 빈 줄에 마우스를 올리면 이전 메시지가 보일 수 있음 (로딩 화면 같은 예외 경로에서만)

- 영역: UI · 논란 · ui
- 위치: `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:370`, `Data/PrismaUI/views/CalamityAffixes/scripts/interop.js:467`, `skse/CalamityAffixes/src/PrismaTooltip.ViewLifecycle.inl:285`

이번 변경으로 setActionFeedback이 `feedback.title = feedback.textContent`를 설정한다(interop.js:465-467). 그런데 applyControlPanelOpenState(false)(interop.js:370)는 textContent만 비우고 title은 남긴다. #feedback은 min-height가 1.4em이라 비어 있어도 마우스를 올릴 수 있다. 예를 들어 정제의 6초 확인 문구가 떠 있을 때 패널을 닫았다가 다시 열면, 빈 줄 위에 옛 'Click Scour again within 6 seconds…' 경고가 보일 수 있다. 다만 검증자 3명 중 2명은 보고된 시나리오가 실제로는 일어나지 않는다고 봤다. 단축키와 UI 명령을 포함한 일반적인 닫기·열기는 모두 SetControlPanelOpenImpl(ViewLifecycle.inl:285-310)을 거친다. 이때 C++가 'Prisma panel closed.' / 'Prisma panel opened.' feedback을 push해서 title을 덮어쓴다. 문제가 남는 곳은 로딩 화면 같은 예외 경로뿐이다.

**고칠 방향:** applyControlPanelOpenState(false)(interop.js:370)에서 `feedback.textContent = "";` 옆에 `feedback.title = "";`(또는 `feedback.removeAttribute("title")`)를 추가한다. setActionFeedback("")를 대신 부르지는 않는다. 그렇게 하면 닫기 경로에서 어픽스 확장 대기 상태와 룬워드 미리보기 대기 상태까지 함께 지워진다.

### L81. test_runtime_contract_sync_script_passes_for_repo_snapshot는 가짜 dotnet이 커밋된 계약 파일을 그대로 복사하기 때문에, 계약이 낡아도 실패할 수 없다

- 영역: 도구·문서 · 논란 · test-coverage
- 위치: `tools/tests/test_release_verify.py:26`, `tools/tests/test_release_verify.py:797`, `tools/tests/test_release_verify.py:833`

이 테스트(tools/tests/test_release_verify.py:26-43)는 CAFF_DOTNET을 _write_fake_dotnet(797-841행)이 만든 가짜로 바꾼다. 가짜는 `run`으로 불리면 저장소의 runtime_contract.json을 --data 출력 폴더에 복사할 뿐이다(833-834행). 그래서 verify_runtime_contract_sync.py는 파일을 자기 복사본과 비교하게 된다. release_verify.sh처럼 `python3 -m unittest discover -s tools/tests`를 로컬에서 돌리면, runtime_contract.json이 낡아 있어도 이름이 '저장소 스냅샷 통과'인 이 테스트는 통과로 나온다. 검증자 2명은 이것이 의도된 밀폐형 배선 테스트라고 반박했다. 이전 버전에서 일부러 바꾼 것이고, 실제 생성기 비교는 ci-verify의 'Verify runtime contract snapshot sync' 단계에서 따로 돈다는 이유다. 그래서 이견 있음으로 남는다.

**고칠 방향:** 테스트 이름을 실제로 확인하는 내용에 맞게 바꾼다. 예: test_runtime_contract_sync_routes_generator_output_through_run_root. 기존 테스트는 배선 스모크 테스트로 남긴다. 선택 사항으로 부정 테스트를 하나 추가한다. 가짜 dotnet에 환경 변수 스위치(예: CAFF_TEST_FAKE_CONTRACT_MUTATE=1)를 두어 다른 내용의 계약을 쓰게 하고, 스크립트가 종료 코드 1과 'checked-in contract is stale'을 내는지 확인한다.

