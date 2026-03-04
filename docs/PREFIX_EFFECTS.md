# 프리픽스 83개 효과 정리 (v1.2.21)

> 업데이트: 2026-03-04
> 기준 코드:
> - 효과 정의: `affixes/modules/keywords.affixes.core.json`
> - 변환 스크립트: `tools/transform_prefixes.py`

- 총 프리픽스: **83개** (INTERNAL 10개 포함)
- 스카이림 로어 기반 전면 리네이밍 (v1.2.21)
- 9개 freed slot → Thu'um 5종 + 마법학파 4종 신규 효과
- EditorID 100% 보존 (FormID 무변동)

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
| 치명 시전 | 7 |
| 시체 소각 (죽음의 화장) | 3 |
| 소환 화장 | 2 |
| 역병 / 적응 | 4 |
| 성장형 | 2 |
| Thu'um (외침) — 신규 | 5 |
| 마법학파 — 신규 | 4 |
| INTERNAL (내부 전용) | 10 |
| **합계** | **83** |

## 원소 타격

- **폭풍 소환** (`storm_call`) [Weapon]
  - 38% 확률 / 쿨타임 1.5초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_ARC_LIGHTNING`
  - 스케일: HitPhysicalDealt ×0.1

- **화염 일격** (`flame_strike`) [Weapon]
  - 적중 100.0% → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_FIRE_DAMAGE`
  - 스케일: HitPhysicalDealt ×0.1
  - 드롭 가중치: 0 (하위 전용)

- **냉기 일격** (`frost_strike`) [Weapon]
  - 적중 100.0% → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_FROST_DAMAGE`
  - 스케일: HitPhysicalDealt ×0.1
  - 드롭 가중치: 0 (하위 전용)

- **전격 일격** (`spark_strike`) [Weapon]
  - 적중 100.0% → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_SHOCK_DAMAGE`
  - 스케일: HitPhysicalDealt ×0.1
  - 드롭 가중치: 0 (하위 전용)

## 원소 취약

- **화염 취약** (`flame_weakness`) [Weapon]
  - 36% 확률 / 쿨타임 2.8초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_FIRE_SHRED`

- **냉기 취약** (`frost_weakness`) [Weapon]
  - 적중 24.0% / 쿨타임 2.4초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_FROST_SHRED`

- **전격 취약** (`shock_weakness`) [Weapon]
  - 적중 30.0% / 쿨타임 2초 / 최근 처치 4.0초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_SHOCK_SHRED`

## 소환

- **늑대 혼령** (`wolf_spirit`) [Weapon]
  - 적중 30.0% / 쿨타임 24초 → 대상 | CastSpell

- **화염 정령** (`flame_sprite`) [Weapon]
  - 36% 확률 / 쿨타임 14초 → 대상 | CastSpell

- **화염 아트로나크** (`flame_atronach`) [Weapon]
  - 30% 확률 / 쿨타임 32초 → 대상 | CastSpell

- **냉기 아트로나크** (`frost_atronach`) [Weapon]
  - 30% 확률 / 쿨타임 32초 → 대상 | CastSpell

- **폭풍 아트로나크** (`storm_atronach`) [Weapon]
  - 26% 확률 / 쿨타임 40초 → 대상 | CastSpell

- **드레모라 서약** (`dremora_pact`) [Weapon]
  - 22% 확률 / 쿨타임 52초 → 대상 | CastSpell

- **영혼 수확** (`soul_snare`) [Weapon]
  - 30% 확률 / 쿨타임 1초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_SOUL_EXPLOIT` (마력·기력 각 8 + 물리 피해 3%)
  - 드롭 가중치: 1.0

## 피격 방어

- **그림자 반격** (`shadow_veil`) [Weapon]
  - 피격 100% / 쿨타임 30초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_INVISIBILITY` (투명화 3초 + 공격력 +25% 6초)
  - 드롭 가중치: 1.0

- **돌의 수호** (`stone_ward`) [Armor]
  - 피격 10.0% / 쿨타임 6초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_INCOMING_WARDEN_SHELL`

- **마법 방벽** (`arcane_ward`) [Armor]
  - 피격 18.0% / 쿨타임 8초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_INCOMING_RESOLVE_BARRIER`

- **치유의 파도** (`healing_surge`) [Armor]
  - 피격 20.0% / 쿨타임 9초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_INCOMING_VITAL_REBOUND`

- **마법사의 갑옷 I: 물리 피해 10%를 매지카로 전환** (`mage_armor_t1`) [Armor]
  - 피격 100.0% →  | MindOverMatter
  - 물리 피해 0% → 매지카 전환

- **마법사의 갑옷 II: 물리 피해 15%를 매지카로 전환** (`mage_armor_t2`) [Armor]
  - 피격 100.0% →  | MindOverMatter
  - 물리 피해 0% → 매지카 전환

## 두루마리 숙련

- **두루마리 숙련 I: 스크롤 비소모 +20%** (`scroll_mastery_t1`) [Armor]
  - 적중 →  | DebugNotify

- **두루마리 숙련 II: 스크롤 비소모 +25%** (`scroll_mastery_t2`) [Armor]
  - 적중 →  | DebugNotify

- **두루마리 숙련 III: 스크롤 비소모 +30%** (`scroll_mastery_t3`) [Armor]
  - 적중 →  | DebugNotify

- **두루마리 숙련 IV: 스크롤 비소모 +35%** (`scroll_mastery_t4`) [Armor]
  - 적중 →  | DebugNotify

## 화염 DoT

- **잔불 낙인** (`ember_brand`) [Weapon]
  - 적중 40.0% / 쿨타임 0.8초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_HIT_EMBERBRAND_IGNITE`
  - 스케일: HitPhysicalDealt ×0.04

- **잔불 장작** (`ember_pyre`) [Weapon]
  - 처치 18.0% / 쿨타임 1초 →  | CorpseExplosion
  - 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`
  - 반경 320.0, 14.0+0.08% 시체HP, 연쇄 2회

## CC / 디버프

- **얼음 족쇄** (`ice_shackle`) [Weapon]
  - 적중 24.0% / 쿨타임 2.5초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_HIT_FROSTLOCK_SNARE`

- **마나 소진** (`mana_burn`) [Weapon]
  - 35% 확률 / 쿨타임 1.2초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_HIT_SHOCK_OVERLOAD`

- **생명 흡수** (`life_drain`) [Weapon]
  - 38% 확률 / 쿨타임 0.7초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_HIT_BLOOD_SIPHON`
  - 스케일: HitPhysicalDealt ×0.045 +4.0 (max 24.0)

## 유틸리티

- **영혼 착취** (`soul_siphon`) [Weapon]
  - 처치 30.0% / 쿨타임 2초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_KILL_SOUL_SURGE`

- **그림자 질주** (`shadow_stride`) [Weapon]
  - 적중 20.0% / 쿨타임 12초 / 최근 처치 4.0초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_HIT_SHADOWSTEP_BURST`

- **은신 습격** (`silent_step`) [Weapon]
  - 처치 100% / 쿨타임 8초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_STEALTH_ASSAULT` (투명화 3초 + 이동속도 +30% 6초)
  - 드롭 가중치: 1.0

- **전투 광란** (`battle_frenzy`) [Weapon]
  - 적중 20.0% / 쿨타임 0.6초 / 표적별 쿨타임 12초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_SWAP_JACKPOT_HASTE`

## 덫 / 룬

- **곰 덫** (`bear_trap`) [Weapon]
  - 적중 20.0% / 쿨타임 6초 →  | SpawnTrap
  - 스펠: `CAFF_SPEL_TRAP_IRONJAW_SNARE`
  - 덫 설치: 0초 후, 0초 유지

- **룬 함정** (`rune_trap`) [Weapon]
  - 적중 12.0% / 쿨타임 1.5초 →  | SpawnTrap
  - 스펠: `CAFF_SPEL_MINEFIELD_SNARE`
  - 덫 설치: 0초 후, 0초 유지

- **혼돈의 룬** (`chaos_rune`) [Weapon]
  - 적중 12.0% / 쿨타임 2초 / 표적별 쿨타임 8초 →  | SpawnTrap
  - 스펠: `CAFF_SPEL_CHAOS_MINE_SNARE`
  - 덫 설치: 0초 후, 0초 유지

## 원소 주입 (전환)

- **화염 주입** (`fire_infusion_50`) [Weapon]
  - 적중 →  | ConvertDamage
  - 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`
  - 전환: Fire 50.0%

- **화염 주입** (`fire_infusion_100`) [Weapon]
  - 적중 →  | ConvertDamage
  - 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`
  - 전환: Fire 100.0%

- **냉기 주입** (`frost_infusion_50`) [Weapon]
  - 적중 →  | ConvertDamage
  - 스펠: `CAFF_SPEL_DMG_FROST_DYNAMIC`
  - 전환: Frost 50.0%

- **냉기 주입** (`frost_infusion_100`) [Weapon]
  - 적중 →  | ConvertDamage
  - 스펠: `CAFF_SPEL_DMG_FROST_DYNAMIC`
  - 전환: Frost 100.0%

- **전격 주입** (`shock_infusion_50`) [Weapon]
  - 적중 →  | ConvertDamage
  - 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`
  - 전환: Shock 50.0%

- **전격 주입** (`shock_infusion_100`) [Weapon]
  - 적중 →  | ConvertDamage
  - 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`
  - 전환: Shock 100.0%

## 대마법사

- **대마법사** (`archmage_t1`) [Armor]
  - 적중 →  | Archmage
  - 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`
  - 대마법사: 티어 ?

- **대마법사** (`archmage_t2`) [Armor]
  - 적중 →  | Archmage
  - 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`
  - 대마법사: 티어 ?

- **대마법사** (`archmage_t3`) [Armor]
  - 적중 →  | Archmage
  - 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`
  - 대마법사: 티어 ?

- **대마법사** (`archmage_t4`) [Armor]
  - 적중 →  | Archmage
  - 스펠: `CAFF_SPEL_DMG_SHOCK_DYNAMIC`
  - 대마법사: 티어 ?

## 치명 시전

- **치명 시전: 파이어볼트** (`crit_cast_firebolt`) [Armor]
  - 적중 →  | CastOnCrit
  - 스케일: HitPhysicalDealt ×0.3

- **치명 시전: 아이스 스파이크** (`crit_cast_ice_spike`) [Armor]
  - 적중 →  | CastOnCrit
  - 스케일: HitPhysicalDealt ×0.3

- **치명 시전: 라이트닝 볼트** (`crit_cast_lightning_bolt`) [Armor]
  - 적중 →  | CastOnCrit
  - 스케일: HitPhysicalDealt ×0.3

- **치명 시전: 썬더볼트** (`crit_cast_thunderbolt`) [Armor]
  - 적중 →  | CastOnCrit
  - 스케일: HitPhysicalDealt ×0.3

- **치명 시전: 아이시 스피어** (`crit_cast_icy_spear`) [Armor]
  - 적중 →  | CastOnCrit
  - 스케일: HitPhysicalDealt ×0.3

- **치명 시전: 체인 라이트닝** (`crit_cast_chain_lightning`) [Armor]
  - 적중 →  | CastOnCrit
  - 스케일: HitPhysicalDealt ×0.3

- **치명 시전: 아이스 스톰** (`crit_cast_ice_storm`) [Armor]
  - 적중 →  | CastOnCrit
  - 스케일: HitPhysicalDealt ×0.3

## 시체 소각 (죽음의 화장)

- **죽음의 화장 T1** (`death_pyre_t1`) [Weapon]
  - 처치 →  | CorpseExplosion
  - 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`
  - 반경 600.0, 12.0+6.0% 시체HP, 연쇄 5회

- **죽음의 화장 T2** (`death_pyre_t2`) [Weapon]
  - 처치 →  | CorpseExplosion
  - 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`
  - 반경 600.0, 14.0+7.0% 시체HP, 연쇄 5회

- **죽음의 화장 T3** (`death_pyre_t3`) [Weapon]
  - 처치 →  | CorpseExplosion
  - 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`
  - 반경 600.0, 16.0+8.0% 시체HP, 연쇄 5회

## 소환 화장

- **소환 화장 T1** (`conjured_pyre_t1`) [Armor]
  - 처치 →  | SummonCorpseExplosion
  - 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`
  - 반경 600.0, 14.0+7.0% 소환물HP, 연쇄 3회

- **소환 화장 T2** (`conjured_pyre_t2`) [Armor]
  - 처치 →  | SummonCorpseExplosion
  - 스펠: `CAFF_SPEL_DMG_FIRE_DYNAMIC`
  - 반경 600.0, 18.0+8.5% 소환물HP, 연쇄 3회

## 역병 / 적응

- **원소 저주** (`elemental_bane`) [Weapon]
  - 적중 20.0% / 쿨타임 1초 / 표적별 쿨타임 12초 → 대상 | CastSpellAdaptiveElement

- **역병 포자** (`plague_spore`) [Weapon]
  - 적중 20.0% / 쿨타임 1.5초 →  | SpawnTrap
  - 스펠: `CAFF_SPEL_DOT_BLOOM_POISON`
  - 덫 설치: 0초 후, 0초 유지

- **역청 황폐** (`tar_blight`) [Weapon]
  - 적중 18.0% / 쿨타임 2초 →  | SpawnTrap
  - 스펠: `CAFF_SPEL_DOT_BLOOM_TAR_SLOW`
  - 덫 설치: 0초 후, 0초 유지

- **흡수 포자** (`siphon_spore`) [Weapon]
  - 적중 18.0% / 쿨타임 2초 →  | SpawnTrap
  - 스펠: `CAFF_SPEL_DOT_BLOOM_SIPHON_MAG`
  - 덫 설치: 0초 후, 0초 유지

## 성장형

- **뇌격 숙련** (`thunder_mastery`) [Weapon]
  - 적중 100.0% / 쿨타임 0.2초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_SHOCK_DAMAGE`
  - 성장형: 단계 [0, 15, 45, 90], 배수 [1.0, 1.25, 1.6, 2.1]
  - 스케일: HitPhysicalDealt ×0.08

- **원소 조율** (`elemental_attunement`) [Weapon]
  - 적중 100.0% / 쿨타임 0.2초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_FIRE_DAMAGE`
  - 성장형: 단계 [0, 20, 60, 120], 배수 [1.0, 1.3, 1.7, 2.2]
  - 모드 순환: ['Flame', 'Frost', 'Storm']
  - 스케일: HitPhysicalDealt ×0.08

## Thu'um (외침) — 신규

> v1.2.21 신규 추가 — freed slot 활용, 기존 스펠만 참조

- **힘의 외침** (`voice_of_power`) [Armor]
  - 피격 25.0% / 쿨타임 6초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_SWAP_JACKPOT_HASTE`

- **죽음의 표식** (`death_mark`) [Weapon]
  - 15% 확률 / 쿨타임 1초 / 표적별 쿨타임 25초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_CHAOS_CURSE_SUNDER`

- **얼음 형상** (`ice_form`) [Weapon]
  - 8% 확률 / 쿨타임 15초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_TRAP_IRONJAW_SNARE`

- **무장 해제** (`disarming_shout`) [Weapon]
  - 10% 확률 / 쿨타임 10초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_CHAOS_CURSE_SLOW_ATTACK`

- **질풍 외침** (`whirlwind_sprint`) [Weapon]
  - 처치 100.0% / 쿨타임 3초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_HIT_SHADOWSTEP_BURST`
  - 성장형: 단계 [0, 8, 24, 50], 배수 [1.0, 1.25, 1.55, 2.0]

## 마법학파 — 신규

> v1.2.21 신규 추가 — freed slot 활용, 기존 스펠만 참조

- **마법 약화** (`spell_breach`) [Weapon]
  - 12% 확률 / 쿨타임 10초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_CHAOS_CURSE_FRAGILE`

- **기력 고갈** (`stamina_drain`) [Weapon]
  - 20% 확률 / 쿨타임 4초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_TRAP_DRAGONTEETH_DRAIN_STAMINA`

- **자양의 불꽃** (`nourishing_flame`) [Weapon]
  - 적중 15.0% / 쿨타임 1.5초 / 최근 처치 5.0초 → 자신 | CastSpell
  - 스펠: `CAFF_SPEL_HIT_BLOOD_SIPHON`
  - 스케일: HitPhysicalDealt ×0.06 +3.0 (max 20.0)

- **마나 매듭** (`mana_knot`) [Weapon]
  - 15% 확률 / 쿨타임 8초 → 대상 | CastSpell
  - 스펠: `CAFF_SPEL_CHAOS_CURSE_DRAIN_MAGREGEN`

## INTERNAL (내부 전용)

- **INTERNAL: Bear Trap** (`internal_spell_bear_trap_poison`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

- **INTERNAL: Bear Trap** (`internal_spell_bear_trap_drain_magicka`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

- **INTERNAL: Bear Trap** (`internal_spell_bear_trap_drain_stamina`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

- **INTERNAL: Tar Blight** (`internal_spell_tar_blight_sunder`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

- **INTERNAL: Siphon Spore** (`internal_spell_siphon_spore_sta`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

- **INTERNAL: Chaos Rune** (`internal_spell_chaos_rune_sunder`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

- **INTERNAL: Chaos Rune** (`internal_spell_chaos_rune_drain_magregen`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

- **INTERNAL: Chaos Rune** (`internal_spell_chaos_rune_drain_stamregen`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

- **INTERNAL: Chaos Rune** (`internal_spell_chaos_rune_slow_attack`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

- **INTERNAL: Chaos Rune** (`internal_spell_chaos_rune_fragile`) [None]
  - 적중 →  | DebugNotify
  - 드롭 가중치: 0 (하위 전용)

---

## 전체 ID 매핑 (이전 → 현재)

| # | 이전 ID | 현재 ID | 유형 |
|---|---------|---------|------|
| 0 | `storm_call` | `storm_call` | 유지 |
| 1 | `flame_strike` | `flame_strike` | 유지 |
| 2 | `frost_strike` | `frost_strike` | 유지 |
| 3 | `spark_strike` | `spark_strike` | 유지 |
| 4 | `flame_weakness` | `flame_weakness` | 유지 |
| 5 | `frost_weakness` | `frost_weakness` | 유지 |
| 6 | `shock_weakness` | `shock_weakness` | 유지 |
| 7 | `wolf_spirit` | `wolf_spirit` | 유지 |
| 8 | `flame_sprite` | `flame_sprite` | 유지 |
| 9 | `flame_atronach` | `flame_atronach` | 유지 |
| 10 | `frost_atronach` | `frost_atronach` | 유지 |
| 11 | `storm_atronach` | `storm_atronach` | 유지 |
| 12 | `dremora_pact` | `dremora_pact` | 유지 |
| 13 | `soul_snare` | `soul_snare` | 유지 |
| 14 | `shadow_veil` | `shadow_veil` | 유지 |
| 15 | `stone_ward` | `stone_ward` | 유지 |
| 16 | `arcane_ward` | `arcane_ward` | 유지 |
| 17 | `healing_surge` | `healing_surge` | 유지 |
| 18 | `mage_armor_t1` | `mage_armor_t1` | 유지 |
| 19 | `mage_armor_t2` | `mage_armor_t2` | 유지 |
| 20 | `scroll_mastery_t1` | `scroll_mastery_t1` | 유지 |
| 21 | `scroll_mastery_t2` | `scroll_mastery_t2` | 유지 |
| 22 | `scroll_mastery_t3` | `scroll_mastery_t3` | 유지 |
| 23 | `scroll_mastery_t4` | `scroll_mastery_t4` | 유지 |
| 24 | `ember_brand` | `ember_brand` | 유지 |
| 25 | `ember_pyre` | `ember_pyre` | 유지 |
| 26 | `ice_shackle` | `ice_shackle` | 유지 |
| 27 | `mana_burn` | `mana_burn` | 유지 |
| 28 | `life_drain` | `life_drain` | 유지 |
| 29 | `soul_siphon` | `soul_siphon` | 유지 |
| 30 | `shadow_stride` | `shadow_stride` | 유지 |
| 31 | `silent_step` | `silent_step` | 유지 |
| 32 | `bear_trap` | `bear_trap` | 유지 |
| 33 | `internal_spell_bear_trap_poison` | `internal_spell_bear_trap_poison` | 유지 |
| 34 | `internal_spell_bear_trap_drain_magicka` | `internal_spell_bear_trap_drain_magicka` | 유지 |
| 35 | `internal_spell_bear_trap_drain_stamina` | `internal_spell_bear_trap_drain_stamina` | 유지 |
| 36 | `voice_of_power` | `voice_of_power` | 유지 |
| 37 | `fire_infusion_50` | `fire_infusion_50` | 유지 |
| 38 | `death_mark` | `death_mark` | 유지 |
| 39 | `fire_infusion_100` | `fire_infusion_100` | 유지 |
| 40 | `ice_form` | `ice_form` | 유지 |
| 41 | `frost_infusion_50` | `frost_infusion_50` | 유지 |
| 42 | `disarming_shout` | `disarming_shout` | 유지 |
| 43 | `frost_infusion_100` | `frost_infusion_100` | 유지 |
| 44 | `whirlwind_sprint` | `whirlwind_sprint` | 유지 |
| 45 | `shock_infusion_50` | `shock_infusion_50` | 유지 |
| 46 | `spell_breach` | `spell_breach` | 유지 |
| 47 | `shock_infusion_100` | `shock_infusion_100` | 유지 |
| 48 | `archmage_t1` | `archmage_t1` | 유지 |
| 49 | `archmage_t2` | `archmage_t2` | 유지 |
| 50 | `archmage_t3` | `archmage_t3` | 유지 |
| 51 | `archmage_t4` | `archmage_t4` | 유지 |
| 52 | `crit_cast_firebolt` | `crit_cast_firebolt` | 유지 |
| 53 | `crit_cast_ice_spike` | `crit_cast_ice_spike` | 유지 |
| 54 | `crit_cast_lightning_bolt` | `crit_cast_lightning_bolt` | 유지 |
| 55 | `crit_cast_thunderbolt` | `crit_cast_thunderbolt` | 유지 |
| 56 | `crit_cast_icy_spear` | `crit_cast_icy_spear` | 유지 |
| 57 | `crit_cast_chain_lightning` | `crit_cast_chain_lightning` | 유지 |
| 58 | `crit_cast_ice_storm` | `crit_cast_ice_storm` | 유지 |
| 59 | `death_pyre_t1` | `death_pyre_t1` | 유지 |
| 60 | `death_pyre_t2` | `death_pyre_t2` | 유지 |
| 61 | `death_pyre_t3` | `death_pyre_t3` | 유지 |
| 62 | `stamina_drain` | `stamina_drain` | 유지 |
| 63 | `nourishing_flame` | `nourishing_flame` | 유지 |
| 64 | `conjured_pyre_t1` | `conjured_pyre_t1` | 유지 |
| 65 | `conjured_pyre_t2` | `conjured_pyre_t2` | 유지 |
| 66 | `mana_knot` | `mana_knot` | 유지 |
| 67 | `rune_trap` | `rune_trap` | 유지 |
| 68 | `elemental_bane` | `elemental_bane` | 유지 |
| 69 | `plague_spore` | `plague_spore` | 유지 |
| 70 | `tar_blight` | `tar_blight` | 유지 |
| 71 | `internal_spell_tar_blight_sunder` | `internal_spell_tar_blight_sunder` | 유지 |
| 72 | `siphon_spore` | `siphon_spore` | 유지 |
| 73 | `internal_spell_siphon_spore_sta` | `internal_spell_siphon_spore_sta` | 유지 |
| 74 | `chaos_rune` | `chaos_rune` | 유지 |
| 75 | `internal_spell_chaos_rune_sunder` | `internal_spell_chaos_rune_sunder` | 유지 |
| 76 | `internal_spell_chaos_rune_drain_magregen` | `internal_spell_chaos_rune_drain_magregen` | 유지 |
| 77 | `internal_spell_chaos_rune_drain_stamregen` | `internal_spell_chaos_rune_drain_stamregen` | 유지 |
| 78 | `internal_spell_chaos_rune_slow_attack` | `internal_spell_chaos_rune_slow_attack` | 유지 |
| 79 | `internal_spell_chaos_rune_fragile` | `internal_spell_chaos_rune_fragile` | 유지 |
| 80 | `battle_frenzy` | `battle_frenzy` | 유지 |
| 81 | `thunder_mastery` | `thunder_mastery` | 유지 |
| 82 | `elemental_attunement` | `elemental_attunement` | 유지 |

