# Effect Wave 2 인게임 스모크 테스트

> 대상: Effect Wave 2와 함께 적용된 접미사 중복 억제, 무기 하위 유형 제한,
> 룬워드 베이스 경고, 기존 효과 회귀를 실제 Skyrim 런타임에서 확인한다.
>
> 이 문서는 기능 스모크 테스트용이다. 확률 효과를 한두 번 보지 못한 것만으로
> 실패 판정하지 않으며, 로그상 발동했는데 수치·대상·지속시간이 틀린 경우는 즉시
> 실패로 기록한다.

## 0. 테스트 기록과 준비

### 테스트 기록

- 게임/런타임 버전:
- Calamity Affixes 빌드 또는 커밋:
- 사용한 MO2 프로필:
- 테스트 세이브:
- 다른 전투·AV·소환 변경 모드:
- 테스트 날짜/테스터:

### 준비

1. 효과 수치에 영향을 주는 다른 장비, 포션, 축복, 능력, 전투 모드를 가능한 한
   제거한 통제 세이브를 사용한다.
2. MCM의 Debug Notifications를 켠다.
3. 장비를 바꿀 때마다 인벤토리를 닫고 1~2초 기다린 뒤 AV와 로그를 기록한다.
4. 같은 대상과 같은 무기·공격을 사용해 장착 전/후를 비교한다.
5. 확률 효과는 최소 30회의 유효 이벤트를 시도한다. 발동하지 않은 공격은 수치
   비교에서 제외하고, `CastSpellImmediate` 등 발동 로그가 남은 시점부터 실제
   효과를 확인한다.
6. 테스트 아이템이 필요하면 Prisma UI 디버그 화면의 `Spawn Test Item`,
   `+1 Recipe Set`, `+1 Next Fragment`, `Recover Currency`를 사용할 수 있다.
7. 각 항목은 `PASS`, `FAIL`, `INCONCLUSIVE` 중 하나로 기록한다.
   `INCONCLUSIVE`는 허용 결과가 표본 안에 나오지 않은 경우처럼 재시험이 필요한
   상황이며 `PASS`로 간주하지 않는다.

## 1. 같은 접미사 계열은 최고 티어 하나만 적용

같은 계열 접미사를 여러 부위에 장착해도 합산하지 않고, 현재 장비 중 가장 높은
티어 하나만 적용해야 한다. 서로 다른 계열은 각 계열의 최고 티어가 독립적으로
적용되어야 한다.

### 1.1 Vitality 패시브

| 조합 | 기대 최대 체력 보너스 |
|---|---:|
| `suffix_vitality_t1` | +25 |
| T1 + T1 | +25 |
| T1 + T2 | +50 |
| T2 + T3 | +75 |
| T1 + T2 + T3 | +75 |

확인 절차:

1. 모든 시험 장비를 벗고 `player.getav health` 값을 기록한다.
2. 위 조합을 순서대로 장착하고 최대 체력 변화가 표와 같은지 확인한다.
3. 높은 티어를 벗었을 때 남은 티어 중 최고 값으로 즉시 내려오는지 확인한다.
4. 전부 벗었을 때 기준값으로 돌아오는지 확인한다.
5. 저장 후 불러오기와 셀 이동 뒤에도 보너스가 중복 추가되지 않는지 확인한다.

판정:

- T1 두 개가 +50으로 합산되거나 T1+T2가 +75가 되면 `FAIL`.
- 장비를 벗은 뒤 보너스가 남거나 로드할 때마다 증가하면 `FAIL`.

### 1.2 Assassin 치명타 계열

| 티어 | ID | 키워드 | 기대 CriticalChance | 기대 런타임 치명타 피해 |
|---|---|---|---:|---:|
| T1 | `suffix_assassin_t1` | `CAFF_KYWD_SUFFIX_CRITICAL_DAMAGE_T1` | +10 | +10% |
| T2 | `suffix_assassin_t2` | `CAFF_KYWD_SUFFIX_CRITICAL_DAMAGE_T2` | +20 | +20% |
| T3 | `suffix_assassin_t3` | `CAFF_KYWD_SUFFIX_CRITICAL_DAMAGE_T3` | +30 | +30% |

필수 조합 판정:

| 조합 | 기대 CriticalChance | 기대 치명타 피해 보너스 |
|---|---:|---:|
| T1 + T1 | +10 | +10% |
| T1 + T2 | +20 | +20% |
| T2 + T3 | +30 | +30% |

확인 절차:

1. `player.getav criticalchance`로 표시 AV를 확인한다.
2. 동일 무기·동일 대상에서 치명타를 발생시키고
   `CalamityAffixes: crit damage bonus ... -> multiplier ...` 로그를 확인한다.
3. 낮은 티어를 추가해도 최고 티어 값이 변하지 않는지 확인한다.
4. 높은 티어를 벗으면 남은 최고 티어 값으로 내려오는지 확인한다.
5. `disablePassiveSuffixSpells`를 켜는 별도 회귀에서는 표시형
   CriticalChance 패시브만 없어지고, 숨은 런타임 치명타 피해는 여전히 최고 티어
   하나의 값으로 적용되는지 확인한다.

`disablePassiveSuffixSpells` 상태에서 치명타 피해까지 사라지거나, 반대로
CriticalChance 패시브가 남는 경우는 `FAIL`이다.

## 2. 무기 하위 유형별 4개 접미사 계열 롤 제한

현재 하위 유형 제한 대상은 다음 네 계열이다.

| 계열 | ID | 키워드 | 티어 수치 | 허용 베이스 |
|---|---|---|---|---|
| Swordsman | `suffix_swordsman_t1/t2/t3` | `CAFF_KYWD_SUFFIX_FIRE_RESIST_T1/T2/T3` | +5/+10/+15 | 한손 근접 무기 |
| Champion | `suffix_champion_t1/t2/t3` | `CAFF_KYWD_SUFFIX_FROST_RESIST_T1/T2/T3` | +5/+10/+15 | 양손 근접 무기 |
| Marksman | `suffix_marksman_t1/t2/t3` | `CAFF_KYWD_SUFFIX_SHOCK_RESIST_T1/T2/T3` | +5/+10/+15 | 활·석궁 |
| Eagle Eye | `suffix_eagle_eye_t1/t2/t3` | `CAFF_KYWD_SUFFIX_SHOUT_COOLDOWN_T1/T2/T3` | BowSpeedBonus +5/+10/+15 | 활·석궁 |

### 롤 매트릭스

| 생성 아이템 | Swordsman | Champion | Marksman | Eagle Eye |
|---|---:|---:|---:|---:|
| 한손 근접 | 허용 | 금지 | 금지 | 금지 |
| 양손 근접 | 금지 | 허용 | 금지 | 금지 |
| 활 | 금지 | 금지 | 허용 | 허용 |
| 석궁 | 금지 | 금지 | 허용 | 허용 |
| 지팡이 | 금지 | 금지 | 금지 | 금지 |

확인 절차:

1. 각 하위 유형에서 실제 랜덤 접미사 롤을 최소 30회 생성한다.
2. 아이템 이름 또는 상세 정보와 로그를 함께 기록해 네 계열의 출현 여부를 센다.
3. 허용 계열이 표본에 한 번도 나오지 않은 경우는 확률상 가능하므로
   `INCONCLUSIVE`로 두고 표본을 늘린다.
4. 금지 계열이 단 한 번이라도 나오면 즉시 `FAIL`로 기록하고 해당 아이템,
   베이스 FormID, 접미사 ID, 생성 로그를 보존한다.

수동으로 이미 붙어 있는 테스트 키워드를 장착하는 것은 효과 동작 확인에는 쓸 수
있지만, “랜덤 롤 제한”의 통과 근거로 사용하지 않는다.

## 3. 전문 룬워드 베이스 경고는 비차단이어야 함

아래 여섯 룬워드는 권장 베이스와 맞지 않아도 제작을 막지 않고 경고만 표시해야
한다.

| 룬워드 | ID | LoreBox 키워드 | 권장 베이스 |
|---|---|---|---|
| Strength | `runeword_strength_final` | `LoreBox_CAFF_AFFIX_RUNEWORD_STRENGTH` | 양손 근접 무기 |
| Edge | `runeword_edge_final` | `LoreBox_CAFF_AFFIX_RUNEWORD_EDGE` | 활·석궁 |
| Zephyr | `runeword_zephyr_final` | `LoreBox_CAFF_AFFIX_RUNEWORD_ZEPHYR` | 활·석궁 |
| Oath | `runeword_oath_final` | `LoreBox_CAFF_AFFIX_RUNEWORD_OATH` | 한손 근접 무기 |
| Myth | `runeword_myth_final` | `LoreBox_CAFF_AFFIX_RUNEWORD_MYTH` | 중갑 |
| Unbending Will | `runeword_unbending_will_final` | `LoreBox_CAFF_AFFIX_RUNEWORD_UNBENDING_WILL` | 방패 |

베이스 불일치 시 기대 경고:

> 베이스 불일치: 이 룬워드는 \<요구\>에 맞춰져 있습니다. 현재 베이스와 호환되지
> 않지만 변환은 계속할 수 있습니다.

확인 절차:

1. 각 룬워드를 맞는 베이스에서 조합해 경고 없이 변환되는지 확인한다.
2. 같은 룬워드를 명백히 맞지 않는 베이스에서 조합한다.
3. 위 경고가 실제 요구 베이스를 넣어 표시되는지 확인한다.
4. 경고 뒤 변환이 계속되어 완성 룬워드가 생성되고 재료·결과 상태가 정상인지
   확인한다.
5. Dragon(`runeword_dragon_final`)은 현재 빌드 비의존 룬워드이므로 경갑,
   중갑, 의복 어느 쪽에서도 위 전문 베이스 경고가 나오지 않아야 한다.

경고가 제작을 취소하거나, 맞는 베이스에서 경고가 뜨거나, Dragon에 전문 베이스
경고가 뜨면 `FAIL`이다.

## 4. Effect Wave 2 룬워드 13종 핵심 체감과 수치

확률 효과는 내부 쿨다운보다 짧은 간격의 미발동을 오류로 보지 않는다. 반대로
발동 로그가 확인된 뒤 효과가 빠지거나 대상·수치·지속시간이 다르면 `FAIL`이다.

| 룬워드 | ID / LoreBox 키워드 | 트리거·ICD | 기대 핵심 효과 |
|---|---|---|---|
| Destruction | `runeword_destruction_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_DESTRUCTION` | 적중 30%, 5초 | 반경 350 전격 폭풍. 물리 적중 피해의 4%를 초당 10~40으로 제한해 5초 적용 |
| Hand of Justice | `runeword_hand_of_justice_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_HAND_OF_JUSTICE` | 적중 28%, 4초 | 총 적중 피해의 18%를 40~250으로 제한한 화염 저항 반영 생명력 흡수 |
| Dragon | `runeword_dragon_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_DRAGON` | 피격 30%, 10초 | 방어력 +120, 화염·냉기·전격 저항 각 +25, 8초 |
| Mist | `runeword_mist_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_MIST` | 적중 30%, 6초 | 대상 매지카 즉시 -75, MagickaRateMult -100과 마법 저항 -30, 6초 |
| Famine | `runeword_famine_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_FAMINE` | 적중 30%, 7초 | 스태미나 초당 -30(5초), 공격력 -20%와 무기 속도 -15%(6초). 적중 피해 비례 스케일링 없음 |
| Beast | `runeword_beast_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_BEAST` | 적중 30%, 18초 | 착용자 공격력 +30%, 방어력 +150, 무기 속도 +15%, 10초 |
| Eternity | `runeword_eternity_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_ETERNITY` | 피격 25%, 15초 | 방어력 +300과 피해 반사 25%, 6초 |
| Enigma | `runeword_enigma_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_ENIGMA` | 피격 100%, 30초 | 투명화와 이동 속도 +45, 4초. 상시 이동 속도 +10. 무적·에테리얼 효과는 아님 |
| Last Wish | `runeword_last_wish_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_LAST_WISH` | 생명력 35% 이하, 45초 | 즉시 체력 +250, 방어력 +250과 마법 저항 +50, 12초 |
| Plague | `runeword_plague_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_PLAGUE` | 처치 40%, 4초 | 시체 생명력의 3%+12, 반경 450, 최대 12대상, 거리 감쇠 0.70, 최대 연쇄 깊이 2, 연쇄 창 0.6초, 1초 창당 최대 폭발 2회 |
| Pride | `runeword_pride_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_PRIDE` | 적중 30%, 4초 | 반경 250 냉기 충격파. 물리 적중 피해의 16%를 30~200으로 제한 |
| Mosaic | `runeword_mosaic_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_MOSAIC` | 적중 26%, 5초 | 화염·냉기·전격 각각 총 적중 피해의 5%, 각 8~80. 저항 전 합계 24~240, 적중 시각효과 표시 |
| Obsession | `runeword_obsession_final` / `LoreBox_CAFF_AFFIX_RUNEWORD_OBSESSION` | 적중 34%, 5초 | 총 적중 피해의 18%를 75~300으로 제한한 마법 피해. 상시 MagickaRateMult +25 |

### Wave 2 공통 확인 절차

1. 각 룬워드를 단독 장착하고 로그에서 해당 affix ID의 발동을 확인한다.
2. 낮은 피해와 높은 피해 두 조건을 만들어 최소값·최대값 제한이 있는 효과를
   각각 확인한다.
3. 저항 디버프와 원소 피해는 저항이 다른 두 대상을 사용해 방향성이 맞는지
   확인한다.
4. 버프·디버프는 콘솔 AV 또는 Active Effects에서 시작 수치와 종료 시점을
   기록한다.
5. 내부 쿨다운 직전과 직후에 다시 유효 이벤트를 발생시켜 쿨다운이 지나치게
   길거나 무시되지 않는지 확인한다.
6. 장비를 벗은 뒤 상시 효과가 제거되고, 저장 후 불러오기에 중복 적용되지 않는지
   확인한다.

### Wave 2 개별 필수 관찰

- Destruction은 단일 대상만이 아니라 반경 내 두 번째 대상에도 전격 지속 피해가
  들어가며, 초당 피해가 10 미만 또는 40 초과가 되지 않아야 한다.
- Hand of Justice는 피해와 회복이 함께 관찰되어야 한다. 대상 화염 저항 때문에
  실제 흡수량이 달라지는 조건도 기록한다.
- Famine은 적중 피해를 크게 바꿔도 스태미나·공격·속도 디버프 고정 수치가
  임의로 스케일하지 않아야 한다.
- Enigma는 4초 동안 빠르게 이탈할 수 있어야 하지만, 피해를 무시하거나 충돌을
  통과하는 무적/에테리얼 상태가 되면 안 된다.
- Last Wish는 35% 경계를 넘는 피격에서 발동하며, 45초 안에 반복 발동하지 않아야
  한다.
- Plague 단독 체감 시험은 다른 `CorpseExplosion` 어픽스를 모두 벗고 진행한다.
  Plague가 proc하면 selection priority 20으로 선택되어야 한다. 같은 시체 중복 폭발,
  1초 창당 2회 예산 초과, 연쇄 깊이 초과가 차단되어야 하며 로그의
  corpse/targets/dmg/radius/chain 값을 보존한다.
- Plague와 Death Pyre/Breath of the Dying 등 다른 `CorpseExplosion` 어픽스를
  함께 장착하는 회귀 시험에서는 각 후보의 proc 판정 뒤
  `SelectBestCorpseExplosionAffix`가 selection priority를 먼저, 동률이면 기본
  피해를 비교한다. Plague proc 성공 시 priority 20으로 피해가 더 큰 후보보다
  우선하고, Plague 실패 시 Breath of the Dying(priority 10), 그마저 실패하면
  일반 priority 0 후보 중 기본 피해가 가장 큰 하나가 선택되어야 한다.
  priority와 기본 피해가 모두 같으면 기존 config 순서의 첫 후보를 유지한다.
  어느 경우에도 한 처치당 한 폭발과 기존 중복/연쇄/레이트리밋 예산은 유지되어야
  하며, 로그의 selected affix가 실제 선택된 어픽스와 일치해야 한다.
- Mosaic은 한 번의 발동에서 세 원소가 모두 적용되어야 한다.

## 5. Core 6 개선 효과

| 효과 | ID / LoreBox 키워드 | 기대 동작 |
|---|---|---|
| 전격 취약 | `shock_weakness` / `LoreBox_CAFF_AFFIX_HIT_SHOCK_SHRED` | 적중 35%, ICD 4초. 대상 전격 저항 -25, 4초 |
| Shadow Stride | `shadow_stride` / `LoreBox_CAFF_AFFIX_HIT_SHADOWSTEP_BURST` | 처치 시 무기 속도 +15%, 4초, ICD 6초. 누적 발동 10/30회 뒤 +18%/+21% |
| Death Mark | `death_mark` / `LoreBox_CAFF_AFFIX_CONVERT_PHYS_TO_FIRE_75` | 적중 25%, 전역 ICD 1초·대상별 ICD 10초. 방어력 -200, 6초 |
| Ice Form | `ice_form` / `LoreBox_CAFF_AFFIX_CONVERT_PHYS_TO_FROST_25` | 적중 15%, ICD 10초. 이동 속도 -100%, 2초 |
| Nourishing Flame | `nourishing_flame` / `LoreBox_CAFF_AFFIX_KILL_CORPSE_EXPLOSION_T5` | 최근 5초 내 처치 조건을 만족한 적중, ICD 2초. 착용자 회복량 `6 + 물리 적중 피해 10%`, 6~50 제한 |
| Mana Knot | `mana_knot` / `LoreBox_CAFF_AFFIX_SUMMON_CORPSE_EXPLOSION_T3` | 적중 35%, ICD 5초. 대상 MagickaRateMult -100, 6초 |

확인 포인트:

1. Shadow Stride는 현재 구현 기준으로 “처치 후 무기 속도” 효과다. 이동/은신형
   효과로 오인하지 말고, 10회와 30회 경계 전후의 실제 공격 주기를 비교한다.
2. Death Mark는 여러 적에게 번갈아 적용할 수 있어도 같은 대상에는 10초 안에
   재적용되지 않아야 한다.
3. Ice Form은 2초 뒤 이동 속도가 정확히 복구되고 영구 정지가 남지 않아야 한다.
4. Nourishing Flame은 최근 처치 조건이 없을 때 발동하지 않으며, 낮은 피해에서
   6 미만·높은 피해에서 50 초과로 회복하지 않아야 한다.
5. Mana Knot 종료 후 대상의 매지카 재생이 정상으로 복구되어야 한다.

## 6. 기존 효과 회귀

### 6.1 Wave 1 룬워드

| 룬워드 | 기대 핵심 회귀 |
|---|---|
| Chaos | 적중 16%, ICD 5.5초, 가장 낮은 저항을 따라 원소를 선택해 피해 45 |
| Breath of the Dying | 처치 50%, ICD 3초, 독성 시체 폭발 `24 + 시체 생명력 6%`, 반경 600 |
| Rhyme | 피격 22%, ICD 12초, 방어력 +60(6초), 상시 냉기 저항 +40 |
| Faith | 적중 22%, ICD 12초, 공격력 +20%·무기 속도 +15%(6초), 상시 무기 속도 +15% |
| Call to Arms | 적중 24%, ICD 18초, 공격력 +20%(8초), 상시 체력 +50·매지카 +30 |
| Honor | 적중 26%, ICD 8초, HealRateMult +80(7초), 상시 HealRateMult +20 |
| Hustle Armor | 피격 26%, ICD 10초, 방어력 +60(5초), 상시 이동 속도 +8 |
| Chains of Honor | 피격 20%, ICD 16초, 이동 속도 +35(5초), 상시 화염·냉기·전격 저항 각 +15 |
| Treachery | 피격 26%, ICD 10초, 이동 속도 +28(5초), 상시 무기 속도 +10% |

Mosaic와 Obsession은 Wave 2 표의 항목으로 함께 회귀한다. 위 룬워드는 발동 여부만
보지 말고 장착 해제, 저장/로드, 셀 이동 후 상시 효과가 중복되거나 남지 않는지도
확인한다.

### 6.2 Spirit와 Insight AV

- Insight(`runeword_insight_final`)
  - 상시 `MagickaRate +25`
  - 적중 30%, ICD 10초, `MagickaRateMult +150`, 12초
- Spirit(`runeword_spirit_final`)
  - 상시 `MagickaRate +30`
  - 적중 28%, ICD 9초, `MagickaRateMult +120`, 10초

필수 판정:

1. 두 룬워드 동시 장착 시 상시 MagickaRate 증가는 합계 +55여야 한다.
2. 장비 교체, 저장/로드, 셀 이동, MCM 재적용 뒤 +110, +165처럼 중복 증가하면
   `FAIL`.
3. 발동형 MagickaRateMult는 각 지속시간 뒤 기준값으로 복구되어야 한다.
4. 한 룬워드를 벗으면 해당 상시 보너스만 제거되어야 한다.

### 6.3 BowSpeedBonus 실제 발사 주기

- Eagle Eye T1/T2/T3: BowSpeedBonus +5/+10/+15
- Zephyr(`runeword_zephyr_final`, `CAFF_MGEF_RW_ZEPHYR_RUSH`):
  적중 20%, ICD 8초, BowSpeedBonus +30, 6초

확인 절차:

1. 같은 활과 화살로 기준 10발의 완전 장전→발사 시간을 측정한다.
2. Eagle Eye 각 티어와 Zephyr 발동 중 같은 방식으로 측정한다.
3. `player.getav bowspeedbonus` 값과 실제 애니메이션 주기가 모두 올바른 방향으로
   변하는지 확인한다.
4. AV 숫자만 올라가고 발사 주기가 빨라지지 않으면 `ENGINE-SEMANTICS FAIL`로
   기록한다.
5. 효과 종료·장비 해제 뒤 기준 주기로 복구되는지 확인한다.

### 6.4 ShoutRecoveryMult 실제 재사용 대기시간

Ground(`runeword_ground_final`, `CAFF_MGEF_RW_GROUND_ANCHOR`)은 피격 22%,
ICD 11초로 ShoutRecoveryMult +50을 8초 부여한다.

1. 동일 포효의 기준 재사용 대기시간을 측정한다.
2. Ground 발동 중 같은 포효의 재사용 대기시간을 측정한다.
3. 실제 재사용 대기시간이 짧아져야 한다.
4. 수치만 바뀌고 실제 시간이 같거나 오히려 길어지면
   `ENGINE-SEMANTICS FAIL`이다.
5. 8초 종료 뒤 AV와 다음 포효 대기시간이 정상으로 복구되는지 확인한다.

### 6.5 NoRecast 없이 같은 효과 지속시간 새로고침

Doom(`runeword_doom_final`)을 대표 시험으로 사용한다. Doom은 매 적중 시 발동,
ICD 0.5초, 대상 SpeedMult -30, 4초다.

1. `t=0초`에 같은 대상을 적중시켜 SpeedMult -30을 확인한다.
2. `t=3초`에 다시 적중시킨다.
3. 디버프는 최초 시점의 `t≈4초`가 아니라 두 번째 적중 기준 `t≈7초`까지
   유지되어야 한다.
4. 재적중 뒤 -60으로 중첩되지 않고 -30을 유지해야 한다.
5. 최종 종료 뒤 기준 SpeedMult로 복구되어야 한다.

재적중을 거부해 `t≈4초`에 끝나거나, 중첩되어 -60이 되거나, 영구 잔류하면
`FAIL`이다. Ice/Flicker 계열도 같은 방식으로 추가 표본을 확인할 수 있다.

### 6.6 소환 슬롯 회귀

| 효과 | ID / LoreBox 키워드 | 소환 FormID |
|---|---|---|
| Wolf Spirit | `wolf_spirit` / `LoreBox_CAFF_AFFIX_HIT_CONJURE_FAMILIAR` | `Skyrim.esm\|000640B6` |
| Flame Sprite | `flame_sprite` / `LoreBox_CAFF_AFFIX_HIT_FLAMING_FAMILIAR` | `Skyrim.esm\|0009CE26` |
| Flame Atronach | `LoreBox_CAFF_AFFIX_HIT_CONJURE_FLAME_ATRONACH` | `Skyrim.esm\|000204C3` |
| Frost Atronach | `LoreBox_CAFF_AFFIX_HIT_CONJURE_FROST_ATRONACH` | `Skyrim.esm\|000204C4` |
| Storm Atronach | `LoreBox_CAFF_AFFIX_HIT_CONJURE_STORM_ATRONACH` | `Skyrim.esm\|000204C5` |
| Dremora Lord | `LoreBox_CAFF_AFFIX_HIT_CONJURE_DREMORA_LORD` | `Skyrim.esm\|0010DDEC` |

필수 판정:

1. Twin Souls가 없을 때 활성 소환은 최대 1체여야 한다.
2. Twin Souls가 있을 때 활성 소환은 최대 2체여야 한다.
3. 반복 발동, 장비 교체, 셀 이동, 저장/로드 뒤 보이지 않는 소환 슬롯 점유나
   소환 개체 누수가 없어야 한다.
4. `conjured_pyre_t1/t2` 폭발은 소환 슬롯을 소비하지 않아야 한다.
5. 제한을 넘는 개체가 남거나 기존 소환이 사라졌는데 슬롯이 계속 막히면
   `FAIL`이다.

## 7. 로그·콘솔 관찰과 최종 통과 기준

### 로그 위치와 실시간 관찰

기본 로그:

`%USERPROFILE%\Documents\My Games\Skyrim Special Edition\SKSE\CalamityAffixes.log`

PowerShell:

```powershell
Get-Content -LiteralPath "$env:USERPROFILE\Documents\My Games\Skyrim Special Edition\SKSE\CalamityAffixes.log" -Wait -Tail 100
```

WSL:

```bash
tail -F '/mnt/c/Users/<사용자명>/Documents/My Games/Skyrim Special Edition/SKSE/CalamityAffixes.log'
```

자동 로그 기본 검사:

```bash
python3 tools/qa_skse_logcheck.py --log '/path/to/CalamityAffixes.log'
```

### 찾아볼 로그 패턴

- `runtime config loaded`
- `installed HandleHealthDamage`
- `PrismaUI API acquired`
- `RebuildActiveCounts`
- `active affix (id=`
- `applied passive suffix spell`
- `removed passive suffix spell`
- `CastSpellImmediate (affix=`
- `magnitudeOverride`
- `evolutionStage`
- `CastSpellAdaptiveElement`
- `crit damage bonus`
- `CorpseExplosion fired`
- `denied by budget`

Plague/시체 폭발은 `affixId`, `corpse`, `targets`, `dmg`, `radius`, `chain`을
함께 기록한다. 차단 로그에서는 `DuplicateCorpse`, `RateLimited`,
`ChainDepthExceeded`가 상황에 맞게 나오는지 확인한다.

### 콘솔 관찰 권장값

- `player.getav health`
- `player.getav magicka`
- `player.getav magickarate`
- `player.getav magickaratemult`
- `player.getav criticalchance`
- `player.getav bowspeedbonus`
- `player.getav shoutrecoverymult`

대상 AV는 콘솔에서 대상을 선택한 뒤 같은 `getav` 명령으로 확인한다. 콘솔 출력은
효과 적용 직전, 발동 직후, 종료 직후 세 시점을 캡처한다.

### 전체 PASS 기준

아래 조건을 모두 만족해야 전체 `PASS`다.

1. 같은 접미사 계열이 최고 티어 하나만 적용되고 장착 해제 시 정확히 복구된다.
2. 네 전문 접미사 계열이 허용되지 않은 무기 하위 유형에 롤되지 않는다.
3. 여섯 전문 룬워드의 베이스 불일치 경고가 정확하고 제작을 차단하지 않으며,
   Dragon에는 해당 경고가 없다.
4. Wave 2 13종과 Core 6가 표의 대상·수치·지속시간·ICD로 작동한다.
5. Wave 1, Spirit/Insight, BowSpeedBonus, ShoutRecoveryMult, 재시전 새로고침,
   소환 슬롯 회귀가 모두 통과한다.
6. 장비 해제와 저장/로드 뒤 AV·패시브·소환 슬롯이 누적 또는 영구 잔류하지 않는다.
7. CTD, 프리즈, 누락된 Spell/MGEF, 알 수 없는 action token, 영구 AV 오염이 없다.
8. 확률상 표본 부족만 `INCONCLUSIVE`로 남길 수 있으며, 기능 오류가 하나라도
   확인되면 전체 릴리스 판정은 `FAIL`이다.

### 실패 보고에 반드시 포함할 정보

- 사용한 아이템과 베이스 FormID
- affix/runeword ID 및 LoreBox 키워드
- 재현 순서와 시점
- 기대값과 실제값
- 관련 AV 전/후 값
- 관련 로그 앞뒤 최소 20줄
- 저장/로드 또는 셀 이동 뒤에도 재현되는지
- 충돌 가능성이 있는 다른 장비·효과·모드
