# 프리픽스 효과 정리 (공개용)

> 업데이트: 2026-03-22
> 기준 버전: `v1.2.21`
> 기준 코드:
> - 효과 정의: `affixes/modules/keywords.affixes.core.json`
> - 변환 스크립트: `tools/transform_prefixes.py`

- 총 공개 프리픽스: **73개**
- INTERNAL 항목은 공개 문서에서 숨김
- 스카이림 로어 기반 전면 리네이밍 (v1.2.21)
- 9개 freed slot → Thu'um 5종 + 마법학파 4종 신규 효과
- 각 효과 설명은 인게임 표시 문자열(`nameKo`/`nameEn`)을 그대로 사용

### 카테고리별 수량

| 카테고리 | 수 |
|----------|---|
| 원소 타격 | 4 |
| 원소 취약 | 3 |
| 소환 | 7 |
| 피격 방어 | 6 |
| 두루마리 숙련 | 4 |
| 화염 DoT | 2 |
| CC / 디버프 | 3 |
| 유틸리티 | 4 |
| 덫 / 룬 | 3 |
| 원소 주입 (전환) | 6 |
| 대마법사 | 4 |
| 치명 시전 (Crit Cast) | 7 |
| 시체 소각 (죽음의 화장) | 3 |
| 소환 화장 | 2 |
| 역병 / 적응 | 4 |
| 성장형 | 2 |
| Thu'um (외침) — 신규 | 5 |
| 마법학파 — 신규 | 4 |
| **합계** | **73** |

## 원소 타격

- **`storm_call`** [Weapon]
  - 한글 표시: 폭풍 소환: 38% 확률로 번개 피해 10 + 적중의 10%. 1.5초마다 발동.
  - 영문 표시: Storm Call (Lucky Hit 38% / ICD 1.5s): 10 Lightning Damage + 10% of Hit Damage
  - 대표 스펠: `CAFF_SPEL_ARC_LIGHTNING`

- **`flame_strike`** [Weapon]
  - 한글 표시: 화염 일격: 매 적중 시 화염 피해 6 + 적중의 10%.
  - 영문 표시: Flame Strike (100% on hit): 6 Fire Damage + 10% of Hit Damage
  - 대표 스펠: `CAFF_SPEL_FIRE_DAMAGE`

- **`frost_strike`** [Weapon]
  - 한글 표시: 냉기 일격: 매 적중 시 냉기 피해 6 + 적중의 10%.
  - 영문 표시: Frost Strike (100% on hit): 6 Frost Damage + 10% of Hit Damage
  - 대표 스펠: `CAFF_SPEL_FROST_DAMAGE`

- **`spark_strike`** [Weapon]
  - 한글 표시: 전격 일격: 매 적중 시 번개 피해 6 + 적중의 10%.
  - 영문 표시: Spark Strike (100% on hit): 6 Shock Damage + 10% of Hit Damage
  - 대표 스펠: `CAFF_SPEL_SHOCK_DAMAGE`

## 원소 취약

- **`flame_weakness`** [Weapon]
  - 한글 표시: 화염 취약: 36% 확률로 화염 저항 -15 (4초). 2.8초마다 발동.
  - 영문 표시: Flame Weakness (Lucky Hit 36% / ICD 2.8s): Fire Resist -15 (4s)
  - 대표 스펠: `CAFF_SPEL_FIRE_SHRED`

- **`frost_weakness`** [Weapon]
  - 한글 표시: 냉기 취약: 최근 2초 내 적중 유지 시 24% 확률로 냉기 저항 -15 (4초). 2.4초마다 발동.
  - 영문 표시: Frost Weakness (24% on hit while chaining hits in the last 2s / ICD 2.4s): Frost Resist -15 (4s)
  - 대표 스펠: `CAFF_SPEL_FROST_SHRED`

- **`shock_weakness`** [Weapon]
  - 한글 표시: 전격 취약: 최근 4초 내 처치 후 30% 확률로 번개 저항 -15 (4초). 2초마다 발동.
  - 영문 표시: Shock Weakness (30% on hit after a kill in the last 4s / ICD 2s): Shock Resist -15 (4s)
  - 대표 스펠: `CAFF_SPEL_SHOCK_SHRED`

## 소환

- **`wolf_spirit`** [Weapon]
  - 한글 표시: 늑대 혼령: 적중 시 30% 확률로 패밀리어 소환 (60초). 24초마다 발동.
  - 영문 표시: Wolf Spirit (30% on hit / ICD 24s): Summon Familiar (60s)

- **`flame_sprite`** [Weapon]
  - 한글 표시: 화염 정령: 36% 확률로 폭발하는 불꽃 패밀리어 소환. 14초마다 발동.
  - 영문 표시: Flame Sprite (Lucky Hit 36% / ICD 14s): Summon Exploding Flame Familiar

- **`flame_atronach`** [Weapon]
  - 한글 표시: 화염 아트로나크: 30% 확률로 화염 아트로낙 소환 (60초). 32초마다 발동.
  - 영문 표시: Flame Atronach (Lucky Hit 30% / ICD 32s): Summon Flame Atronach (60s)

- **`frost_atronach`** [Weapon]
  - 한글 표시: 냉기 아트로나크: 30% 확률로 냉기 아트로낙 소환 (60초). 32초마다 발동.
  - 영문 표시: Frost Atronach (Lucky Hit 30% / ICD 32s): Summon Frost Atronach (60s)

- **`storm_atronach`** [Weapon]
  - 한글 표시: 폭풍 아트로나크: 26% 확률로 폭풍 아트로낙 소환 (60초). 40초마다 발동.
  - 영문 표시: Storm Atronach (Lucky Hit 26% / ICD 40s): Summon Storm Atronach (60s)

- **`dremora_pact`** [Weapon]
  - 한글 표시: 드레모라 서약: 22% 확률로 드레모라 군주 소환 (60초). 52초마다 발동.
  - 영문 표시: Dremora Pact (Lucky Hit 22% / ICD 52s): Summon Dremora Lord (60s)

- **`soul_snare`** [Weapon]
  - 한글 표시: 영혼 수확: 30% 확률로 마력·기력 각 8 + 물리 피해의 3% 회복 (최대 24). 1초마다 발동.
  - 영문 표시: Soul Harvest (Lucky Hit 30% / ICD 1s): Restore Magicka & Stamina 8 + 3% of Physical Damage (max 24)
  - 대표 스펠: `CAFF_SPEL_SOUL_EXPLOIT`

## 피격 방어

- **`shadow_veil`** [Weapon]
  - 한글 표시: 그림자 반격: 피격 시 투명화 3초 + 공격력 25% 증가 6초. 30초마다 발동.
  - 영문 표시: Shadow Counter (on hit taken / ICD 30s): Invisibility 3s + Attack Damage +25% 6s
  - 대표 스펠: `CAFF_SPEL_INVISIBILITY`

- **`stone_ward`** [Armor]
  - 한글 표시: 돌의 수호: 피격 시 10% 확률로 피해 저항 +120 (3초). 6초마다 발동.
  - 영문 표시: Stone Ward (10% on hit taken / ICD 6s): Damage Resist +120 (3s)
  - 대표 스펠: `CAFF_SPEL_INCOMING_WARDEN_SHELL`

- **`arcane_ward`** [Armor]
  - 한글 표시: 마법 방벽: 피격 시 18% 확률로 마법 저항 +25% (4초). 8초마다 발동.
  - 영문 표시: Arcane Ward (18% on hit taken / ICD 8s): Magic Resist +25% (4s)
  - 대표 스펠: `CAFF_SPEL_INCOMING_RESOLVE_BARRIER`

- **`healing_surge`** [Armor]
  - 한글 표시: 치유의 파도: 피격 시 20% 확률로 체력 재생 +140% (5초). 9초마다 발동.
  - 영문 표시: Healing Surge (20% on hit taken / ICD 9s): Health Regen +140% (5s)
  - 대표 스펠: `CAFF_SPEL_INCOMING_VITAL_REBOUND`

- **`mage_armor_t1`** [Armor]
  - 한글 표시: 마법사의 갑옷 I: 물리 피해 10%를 매지카로 전환
  - 영문 표시: Mage Armor I: Redirect 10% of physical hit damage to Magicka

- **`mage_armor_t2`** [Armor]
  - 한글 표시: 마법사의 갑옷 II: 물리 피해 15%를 매지카로 전환
  - 영문 표시: Mage Armor II: Redirect 15% of physical hit damage to Magicka

## 두루마리 숙련

- **`scroll_mastery_t1`** [Armor]
  - 한글 표시: 두루마리 숙련 I: 스크롤 비소모 +20%
  - 영문 표시: Scroll Mastery I: +20% chance to not consume scrolls

- **`scroll_mastery_t2`** [Armor]
  - 한글 표시: 두루마리 숙련 II: 스크롤 비소모 +25%
  - 영문 표시: Scroll Mastery II: +25% chance to not consume scrolls

- **`scroll_mastery_t3`** [Armor]
  - 한글 표시: 두루마리 숙련 III: 스크롤 비소모 +30%
  - 영문 표시: Scroll Mastery III: +30% chance to not consume scrolls

- **`scroll_mastery_t4`** [Armor]
  - 한글 표시: 두루마리 숙련 IV: 스크롤 비소모 +35%
  - 영문 표시: Scroll Mastery IV: +35% chance to not consume scrolls

## 화염 DoT

- **`ember_brand`** [Weapon]
  - 한글 표시: 잔불 낙인: 적중 시 40% 확률로 점화 피해 4 + 적중의 4% (4초). 0.8초마다 발동.
  - 영문 표시: Ember Brand (40% on hit / ICD 0.8s): Ignite 4 + 4% of Hit Damage (4s)
  - 대표 스펠: `CAFF_SPEL_HIT_EMBERBRAND_IGNITE`

- **`ember_pyre`** [Weapon]
  - 한글 표시: 잔불 장작: 처치 시 18% 확률로 시체 폭발 (반경 320, 최대체력 8% + 14 화염). 1초마다 발동.
  - 영문 표시: Ember Pyre (18% on kill / ICD 1s): Corpse explosion (320 radius, 8% max health + 14 fire)
  - 대표 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`

## CC / 디버프

- **`ice_shackle`** [Weapon]
  - 한글 표시: 얼음 족쇄: 적중 시 24% 확률로 냉기 감속 -35% (2초). 2.5초마다 발동.
  - 영문 표시: Ice Shackle (24% on hit / ICD 2.5s): Frost snare -35% (2s)
  - 대표 스펠: `CAFF_SPEL_HIT_FROSTLOCK_SNARE`

- **`mana_burn`** [Weapon]
  - 한글 표시: 마나 소진: 35% 확률로 매지카 60 흡수. 1.2초마다 발동.
  - 영문 표시: Mana Burn (Lucky Hit 35% / ICD 1.2s): Drain 60 Magicka
  - 대표 스펠: `CAFF_SPEL_HIT_SHOCK_OVERLOAD`

- **`life_drain`** [Weapon]
  - 한글 표시: 생명 흡수: 38% 확률로 체력 회복 4 + 물리 피해의 4.5% (최대 24). 0.7초마다 발동.
  - 영문 표시: Life Drain (Lucky Hit 38% / ICD 0.7s): Heal 4 + 4.5% of physical hit damage (max 24)
  - 대표 스펠: `CAFF_SPEL_HIT_BLOOD_SIPHON`

## 유틸리티

- **`soul_siphon`** [Weapon]
  - 한글 표시: 영혼 착취: 처치 시 30% 확률로 마나 재생 +35% (8초). 2초마다 발동.
  - 영문 표시: Soul Siphon (30% on kill / ICD 2s): Magicka Regen +35% (8s)
  - 대표 스펠: `CAFF_SPEL_KILL_SOUL_SURGE`

- **`shadow_stride`** [Weapon]
  - 한글 표시: 그림자 질주: 최근 4초 내 처치 후 20% 확률로 이동속도 +20% (4초). 12초마다 발동.
  - 영문 표시: Shadow Stride (20% on hit after a kill in the last 4s / ICD 12s): Move Speed +20% (4s)
  - 대표 스펠: `CAFF_SPEL_HIT_SHADOWSTEP_BURST`

- **`silent_step`** [Weapon]
  - 한글 표시: 은신 습격: 처치 시 투명화 3초 + 이동속도 30% 증가 6초. 8초마다 발동.
  - 영문 표시: Stealth Assault (on kill / ICD 8s): Invisibility 3s + Move Speed +30% 6s
  - 대표 스펠: `CAFF_SPEL_STEALTH_ASSAULT`

- **`battle_frenzy`** [Weapon]
  - 한글 표시: 전투 광란: 적중 시 20% 확률로 무기 공격속도 +15% (4초). 0.6초마다 발동. 같은 대상 12초.
  - 영문 표시: Battle Frenzy (20% on hit / ICD 0.6s / per-target ICD 12s): Weapon Attack Speed +15% (4s)
  - 대표 스펠: `CAFF_SPEL_SWAP_JACKPOT_HASTE`

## 덫 / 룬

- **`bear_trap`** [Weapon]
  - 한글 표시: 곰 덫: 치명타/강공 시 20% 확률로 적 발밑에 덫 설치(재장전 2.5초) → 밟은 적 이동속도 -100% (2초) + 랜덤 추가 효과. 6초마다 발동.
  - 영문 표시: Bear Trap (20% crit/power attack / ICD 6s): Deploy trap at enemy feet (2.5s reload) -> Snare -100% Move Speed (2s) + Random bonus effect
  - 대표 스펠: `CAFF_SPEL_TRAP_IRONJAW_SNARE`

- **`rune_trap`** [Weapon]
  - 한글 표시: 룬 함정: 적중 시 12% 확률로 대상 위치에 지뢰 설치(0.6초 후, 8초, 1회) → 반경 150 둔화(-40%, 2초). 1.5초마다 발동.
  - 영문 표시: Rune Trap (12% on hit / ICD 1.5s): Deploy mine at target (0.6s delay, 8s, 1 charge) -> Radius 150 Slow (-40%, 2s)
  - 대표 스펠: `CAFF_SPEL_MINEFIELD_SNARE`

- **`chaos_rune`** [Weapon]
  - 한글 표시: 혼돈의 룬: 적중 시 12% 확률로 대상 위치에 지뢰 설치(0.8초 후, 10초, 1회) → 둔화 + 무작위 저주. 2초마다 발동. 같은 대상 8초.
  - 영문 표시: Chaos Rune (12% on hit / ICD 2s / per-target ICD 8s): Deploy mine at target (0.8s delay, 10s, 1 charge) -> Slow + Random curse
  - 대표 스펠: `CAFF_SPEL_CHAOS_MINE_SNARE`

## 원소 주입 (전환)

- **`fire_infusion_50`** [Weapon]
  - 한글 표시: 화염 주입: 물리 피해 50%를 화염으로 전환
  - 영문 표시: Fire Infusion: Convert 50% Physical to Fire
  - 대표 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`

- **`fire_infusion_100`** [Weapon]
  - 한글 표시: 화염 주입: 물리 피해 100%를 화염으로 전환
  - 영문 표시: Fire Infusion: Convert 100% Physical to Fire
  - 대표 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`

- **`frost_infusion_50`** [Weapon]
  - 한글 표시: 냉기 주입: 물리 피해 50%를 냉기로 전환
  - 영문 표시: Frost Infusion: Convert 50% Physical to Frost
  - 대표 스펠: `CAFF_SPEL_DMG_FROST_DYNAMIC`

- **`frost_infusion_100`** [Weapon]
  - 한글 표시: 냉기 주입: 물리 피해 100%를 냉기로 전환
  - 영문 표시: Frost Infusion: Convert 100% Physical to Frost
  - 대표 스펠: `CAFF_SPEL_DMG_FROST_DYNAMIC`

- **`shock_infusion_50`** [Weapon]
  - 한글 표시: 전격 주입: 물리 피해 50%를 전격으로 전환
  - 영문 표시: Shock Infusion: Convert 50% Physical to Shock
  - 대표 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`

- **`shock_infusion_100`** [Weapon]
  - 한글 표시: 전격 주입: 물리 피해 100%를 전격으로 전환
  - 영문 표시: Shock Infusion: Convert 100% Physical to Shock
  - 대표 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`

## 대마법사

- **`archmage_t1`** [Armor]
  - 한글 표시: 대마법사: 주문 적중 시 최대 매지카의 10%를 번개 피해로 가하고, 5%를 추가 소모한다.
  - 영문 표시: Archmage (on spell hit): 10% Max Magicka as Lightning Damage / 5% Max Magicka extra cost
  - 대표 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`

- **`archmage_t2`** [Armor]
  - 한글 표시: 대마법사: 주문 적중 시 최대 매지카의 12%를 번개 피해로 가하고, 5%를 추가 소모한다.
  - 영문 표시: Archmage (on spell hit): 12% Max Magicka as Lightning Damage / 5% Max Magicka extra cost
  - 대표 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`

- **`archmage_t3`** [Armor]
  - 한글 표시: 대마법사: 주문 적중 시 최대 매지카의 14%를 번개 피해로 가하고, 5%를 추가 소모한다.
  - 영문 표시: Archmage (on spell hit): 14% Max Magicka as Lightning Damage / 5% Max Magicka extra cost
  - 대표 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`

- **`archmage_t4`** [Armor]
  - 한글 표시: 대마법사: 주문 적중 시 최대 매지카의 16%를 번개 피해로 가하고, 5%를 추가 소모한다.
  - 영문 표시: Archmage (on spell hit): 16% Max Magicka as Lightning Damage / 5% Max Magicka extra cost
  - 대표 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`

## 치명 시전 (Crit Cast)

- **`crit_cast_firebolt`** [Armor]
  - 한글 표시: 치명 시전: 파이어볼트: 치명타/강공 시 파이어볼트 시전 (적중의 30%). 0.15초마다 발동.
  - 영문 표시: Crit Cast: Firebolt (Crit/Power Attack, ICD 0.15s): Firebolt + 30% of Hit Damage

- **`crit_cast_ice_spike`** [Armor]
  - 한글 표시: 치명 시전: 아이스 스파이크: 치명타/강공 시 아이스 스파이크 시전 (적중의 30%). 0.15초마다 발동.
  - 영문 표시: Crit Cast: Ice Spike (Crit/Power Attack, ICD 0.15s): Ice Spike + 30% of Hit Damage

- **`crit_cast_lightning_bolt`** [Armor]
  - 한글 표시: 치명 시전: 라이트닝 볼트: 치명타/강공 시 라이트닝 볼트 시전 (적중의 30%). 0.15초마다 발동.
  - 영문 표시: Crit Cast: Lightning Bolt (Crit/Power Attack, ICD 0.15s): Lightning Bolt + 30% of Hit Damage

- **`crit_cast_thunderbolt`** [Armor]
  - 한글 표시: 치명 시전: 썬더볼트: 치명타/강공 시 썬더볼트 시전 (적중의 30%). 0.15초마다 발동.
  - 영문 표시: Crit Cast: Thunderbolt (Crit/Power Attack, ICD 0.15s): Thunderbolt + 30% of Hit Damage

- **`crit_cast_icy_spear`** [Armor]
  - 한글 표시: 치명 시전: 아이시 스피어: 치명타/강공 시 아이시 스피어 시전 (적중의 30%). 0.15초마다 발동.
  - 영문 표시: Crit Cast: Icy Spear (Crit/Power Attack, ICD 0.15s): Icy Spear + 30% of Hit Damage

- **`crit_cast_chain_lightning`** [Armor]
  - 한글 표시: 치명 시전: 체인 라이트닝: 치명타/강공 시 체인 라이트닝 시전 (적중의 30%). 0.15초마다 발동.
  - 영문 표시: Crit Cast: Chain Lightning (Crit/Power Attack, ICD 0.15s): Chain Lightning + 30% of Hit Damage

- **`crit_cast_ice_storm`** [Armor]
  - 한글 표시: 치명 시전: 아이스 스톰: 치명타/강공 시 아이스 스톰 시전 (적중의 30%). 0.15초마다 발동.
  - 영문 표시: Crit Cast: Ice Storm (Crit/Power Attack, ICD 0.15s): Ice Storm + 30% of Hit Damage

## 시체 소각 (죽음의 화장)

- **`death_pyre_t1`** [Weapon]
  - 한글 표시: 죽음의 화장 T1: 처치 시 반경 600, (12 + 시체 최대체력 6%) 화염 피해. 연쇄 폭발(위력 0.7x) 최대 5회, 초당 4회 제한.
  - 영문 표시: Death Pyre T1 (on kill): Radius 600, (12 + 6% corpse max HP) Fire Damage. Chain(0.7x) max 5, 4/sec limit
  - 대표 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`

- **`death_pyre_t2`** [Weapon]
  - 한글 표시: 죽음의 화장 T2: 처치 시 반경 600, (14 + 시체 최대체력 7%) 화염 피해. 연쇄 폭발(위력 0.7x) 최대 5회, 초당 4회 제한.
  - 영문 표시: Death Pyre T2 (on kill): Radius 600, (14 + 7% corpse max HP) Fire Damage. Chain(0.7x) max 5, 4/sec limit
  - 대표 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`

- **`death_pyre_t3`** [Weapon]
  - 한글 표시: 죽음의 화장 T3: 처치 시 반경 600, (16 + 시체 최대체력 8%) 화염 피해. 연쇄 폭발(위력 0.7x) 최대 5회, 초당 4회 제한.
  - 영문 표시: Death Pyre T3 (on kill): Radius 600, (16 + 8% corpse max HP) Fire Damage. Chain(0.7x) max 5, 4/sec limit
  - 대표 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`

## 소환 화장

- **`conjured_pyre_t1`** [Armor]
  - 한글 표시: 소환 화장 T1: 소환물 사망 시 반경 600, (14 + 소환물 최대체력 7%) 화염 피해. 연쇄 폭발(위력 0.8x) 최대 3회, 초당 3회 제한.
  - 영문 표시: Conjured Pyre T1 (on summon death): Radius 600, (14 + 7% summon max HP) Fire Damage. Chain(0.8x) max 3, 3/sec limit
  - 대표 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`

- **`conjured_pyre_t2`** [Armor]
  - 한글 표시: 소환 화장 T2: 소환물 사망 시 반경 600, (18 + 소환물 최대체력 8.5%) 화염 피해. 연쇄 폭발(위력 0.8x) 최대 3회, 초당 3회 제한.
  - 영문 표시: Conjured Pyre T2 (on summon death): Radius 600, (18 + 8.5% summon max HP) Fire Damage. Chain(0.8x) max 3, 3/sec limit
  - 대표 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`

## 역병 / 적응

- **`elemental_bane`** [Weapon]
  - 한글 표시: 원소 저주: 적중 시 20% 확률로 가장 높은 원소 저항 -15 (4초). 1초마다 발동. 같은 대상 12초.
  - 영문 표시: Elemental Bane (20% on hit / ICD 1s / per-target ICD 12s): Highest elemental resist -15 (4s)

- **`plague_spore`** [Weapon]
  - 한글 표시: 역병 포자: 적중 시 20% 확률로 0.6초 후 포자 폭발(반경 150, 독 피해 3/s, 4초). 1.5초마다 발동.
  - 영문 표시: Plague Spore (20% on hit / ICD 1.5s): Spore burst after 0.6s (Radius 150, Poison 3/s, 4s)
  - 대표 스펠: `CAFF_SPEL_DOT_BLOOM_POISON`

- **`tar_blight`** [Weapon]
  - 한글 표시: 역청 황폐: 적중 시 18% 확률로 0.6초 후 타르 폭발(반경 170, 둔화 -35% + 방어 -150, 4초). 2초마다 발동.
  - 영문 표시: Tar Blight (18% on hit / ICD 2s): Tar burst after 0.6s (Radius 170, Slow -35% + Armor -150, 4s)
  - 대표 스펠: `CAFF_SPEL_DOT_BLOOM_TAR_SLOW`

- **`siphon_spore`** [Weapon]
  - 한글 표시: 흡수 포자: 적중 시 18% 확률로 0.6초 후 흡수 폭발(반경 170, 매지카/스태미나 재생 -60%, 6초). 2초마다 발동.
  - 영문 표시: Siphon Spore (18% on hit / ICD 2s): Siphon burst after 0.6s (Radius 170, Magicka/Stamina Regen -60%, 6s)
  - 대표 스펠: `CAFF_SPEL_DOT_BLOOM_SIPHON_MAG`

## 성장형

- **`thunder_mastery`** [Weapon]
  - 한글 표시: 뇌격 숙련: 매 적중 시 번개 피해 누적 성장 (×1.0→1.25→1.6→2.1, 단계: 15/45/90회). 0.2초마다 발동.
  - 영문 표시: Thunder Mastery (100% on hit): Lightning Damage Growth (x1.0→1.25→1.6→2.1, stages: 15/45/90 hits)
  - 대표 스펠: `CAFF_SPEL_SHOCK_DAMAGE`

- **`elemental_attunement`** [Weapon]
  - 한글 표시: 원소 조율: 매 적중 시 화염/냉기/번개 수동 전환 + 성장 (×1.0→1.3→1.7→2.2, 단계: 20/60/120회). 0.2초마다 발동.
  - 영문 표시: Elemental Attunement (100% on hit): Fire/Frost/Shock manual switch + Growth (x1.0→1.3→1.7→2.2, stages: 20/60/120 hits)
  - 대표 스펠: `CAFF_SPEL_FIRE_DAMAGE`

## Thu'um (외침) — 신규

> v1.2.21 신규 추가 — freed slot 활용, 기존 스펠만 참조

- **`voice_of_power`** [Armor]
  - 한글 표시: 힘의 외침: 피격 시 25% 확률로 이동 +30%, 공격속도 +20% (6초). 6초마다 발동.
  - 영문 표시: Voice of Power (25% on hit taken / ICD 6s): Move Speed +30%, Attack Speed +20% (6s)
  - 대표 스펠: `CAFF_SPEL_SWAP_JACKPOT_HASTE`

- **`death_mark`** [Weapon]
  - 한글 표시: 죽음의 표식: 15% 확률로 방어력 파쇄. 1초마다 발동. 같은 대상 25초.
  - 영문 표시: Death's Mark (Lucky Hit 15% / per-target ICD 25s): Armor Shred
  - 대표 스펠: `CAFF_SPEL_CHAOS_CURSE_SUNDER`

- **`ice_form`** [Weapon]
  - 한글 표시: 얼음 형상: 8% 확률로 극한 동결. 15초마다 발동.
  - 영문 표시: Ice Form (Lucky Hit 8% / ICD 15s): Deep Freeze
  - 대표 스펠: `CAFF_SPEL_TRAP_IRONJAW_SNARE`

- **`disarming_shout`** [Weapon]
  - 한글 표시: 무장 해제: 10% 확률로 공격속도 감소. 10초마다 발동.
  - 영문 표시: Disarming Shout (Lucky Hit 10% / ICD 10s): Attack Speed Reduction
  - 대표 스펠: `CAFF_SPEL_CHAOS_CURSE_SLOW_ATTACK`

- **`whirlwind_sprint`** [Weapon]
  - 한글 표시: 질풍 외침: 처치 시 이동속도 강화 (성장: ×1.0→1.25→1.55→2.0). 3초마다 발동.
  - 영문 표시: Whirlwind Sprint (100% on kill / ICD 3s): Move Speed Boost (Growth: x1.0→1.25→1.55→2.0)
  - 대표 스펠: `CAFF_SPEL_HIT_SHADOWSTEP_BURST`

## 마법학파 — 신규

> v1.2.21 신규 추가 — freed slot 활용, 기존 스펠만 참조

- **`spell_breach`** [Weapon]
  - 한글 표시: 마법 약화: 12% 확률로 마법 저항 감소. 10초마다 발동.
  - 영문 표시: Spell Breach (Lucky Hit 12% / ICD 10s): Magic Resist Reduction
  - 대표 스펠: `CAFF_SPEL_CHAOS_CURSE_FRAGILE`

- **`stamina_drain`** [Weapon]
  - 한글 표시: 기력 고갈: 20% 확률로 스태미나 드레인. 4초마다 발동.
  - 영문 표시: Stamina Drain (Lucky Hit 20% / ICD 4s): Stamina Drain
  - 대표 스펠: `CAFF_SPEL_TRAP_DRAGONTEETH_DRAIN_STAMINA`

- **`nourishing_flame`** [Weapon]
  - 한글 표시: 자양의 불꽃: 최근 5초 내 처치 후 15% 확률로 자가 치유 + 적중의 6%. 1.5초마다 발동.
  - 영문 표시: Nourishing Flame (15% on hit / ICD 1.5s / requires recent kill): Self Heal + 6% of Hit Damage
  - 대표 스펠: `CAFF_SPEL_HIT_BLOOD_SIPHON`

- **`mana_knot`** [Weapon]
  - 한글 표시: 마나 매듭: 15% 확률로 매지카 재생 감소. 8초마다 발동.
  - 영문 표시: Mana Knot (Lucky Hit 15% / ICD 8s): Magicka Regen Reduction
  - 대표 스펠: `CAFF_SPEL_CHAOS_CURSE_DRAIN_MAGREGEN`

