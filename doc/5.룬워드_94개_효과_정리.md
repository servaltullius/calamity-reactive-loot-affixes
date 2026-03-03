# 룬워드 94개 효과 정리 (v1.2.20)

> 업데이트: 2026-03-04
> 기준 코드:
> - 효과 정의: `affixes/modules/keywords.affixes.runewords.json`
> - 룬 카탈로그: `skse/CalamityAffixes/src/RunewordCatalogRows.inl`
> - 요약 텍스트: `skse/CalamityAffixes/src/EventBridge.Loot.Runeword.SummaryText.h`

- 총 룬워드: **94개**
- 모든 룬워드는 **JSON 개별 정의** — 자동 합성 없음
- 시그니처 21개(복합 효과) + 일반 73개(단일 효과)
- C++ IdentityOverride: 3개만 사용 (Honor, Eternity, Chaos)

## 아키텍처 변경 (v1.2.20)

v1.2.19까지 "직접 정의 16개 + 자동 합성 78개" → v1.2.20에서 **94개 전부 JSON 개별 정의**로 전환.

1. 스카이림 마법 아키타입 활용 (Paralysis, Calm, Demoralize, Frenzy, TurnUndead, Absorb, Invisibility, Etherealize)
2. 다양한 actorValue (BowSpeedBonus, CriticalChance, PoisonResist, CarryWeight, 스킬 Modifier 등)
3. 원소 데미지 타이핑: `actorValue=Health` + `hostile=true` + `resistValue` + `magicSkill=Destruction`
4. 스타일 그룹은 **시각 이펙트 전용** (효과 결정에 관여하지 않음)

### v1.2.20 중복 효과 19종 리워크

같은 AV에 수치만 다른 중복을 해소하여 19개 룬워드를 완전히 다른 효과로 교체:

| 유형 | 룬워드 | 기존 → 새 효과 |
|------|--------|---------------|
| 자신 버프 | Ground | ResistShock → ShoutRecoveryMult +50 |
| 자신 버프 | Cure | HealRate → RestorationModifier +20 |
| 자신 버프 | Leaf | Fire Strike → DestructionModifier +20 |
| 자신 버프 | Hearth | ResistFrost → ConjurationModifier +20 |
| 자신 버프 | Passion | AttackDamageMult → IllusionModifier +20 |
| 자신 버프 | Temper | AttackDamageMult → StaminaRate +5 |
| 자신 버프 | Wisdom | HealRate → MagickaRate +5 |
| 흡수 | Gloom | Invisibility → Absorb Magicka 30 |
| 흡수 | Melody | BowSpeedBonus → Absorb Stamina 20 |
| 대상 디버프 | Duress | DamageResist Self → AttackDamageMult -25 Target |
| 대상 디버프 | Wind | SpeedMult -40 → WeaponSpeedMult -20 |
| 대상 디버프 | Stone | DamageResist Self → DamageResist -50 Target |
| 대상 디버프 | Venom | Poison DoT → PoisonResist -40 |
| 대상 디버프 | Crescent Moon | Shock 35 → ResistShock -35 |
| 대상 디버프 | Voice of Reason | Frost 20/3s → ResistFrost -35 |
| 대상 디버프 | Rift | Calm → ResistMagic -30 |
| 대상 디버프 | Steel | WeaponSpeedMult Self → HealRateMult -80 Target |
| 대상 디버프 | Harmony | SpeedMult Self → StaminaRateMult -80 Target |
| 대상 디버프 | Mist | SpeedMult -30 → MagickaRateMult -80 Target |

## 시그니처 (복합 효과)

- **Enigma** (`Jah,Ith,Ber`) — 피격 100% / ICD 90초 → 자신
  - SpeedMult +45, 8초, + 패시브, 복합
  - 위험 순간 위상 전환으로 즉시 이탈

- **Infinity** (`Ber,Mal,Ber,Ist`) — 적중 100% / ICD 0.5초 → 대상
  - ResistFire 20, 5초, 복합
  - 저항이 높은 축을 먼저 찢어 약점 개방

- **Fortitude** (`El,Sol,Dol,Lo`) — 피격 25% / ICD 16초 → 자신
  - DamageResist +120, 8초, + 패시브, 복합
  - 강철 의지 버프로 난전 생존력 상승

- **Insight** (`Ral,Tir,Tal,Sol`) — 적중 30% / ICD 10초 → 자신
  - MagickaRateMult +150, 12초, + 패시브, 복합
  - 명상 파동으로 자원 고갈 구간을 완화

- **Spirit** (`Tal,Thul,Ort,Amn`) — 적중 28% / ICD 9초 → 자신
  - MagickaRateMult +120, 10초, + 패시브, 복합
  - 집중력 흐름을 올려 전투 리듬 안정화

- **Grief** (`Eth,Tir,Lo,Mal,Ral`) — 적중 30% / ICD 0.5초 → 자신
  - Health +5, + 패시브, 복합
  - 빈틈 없이 몰아치는 초고속 참격 연타

- **Call to Arms** (`Amn,Ral,Mal,Ist,Ohm`) — 적중 24% / ICD 18초 → 자신
  - DamageResist +150, 8초, + 패시브, 복합
  - 함성으로 아군 페이스를 끌어올려 공방 동시 강화

- **Chains of Honor** (`Dol,Um,Ber,Ist`) — 피격 20% / ICD 16초 → 자신
  - SpeedMult +35, 5초, + 패시브, 복합
  - 명예의 사슬로 성역의 보호를 단단히 고정

- **Heart of the Oak** (`Ko,Vex,Pul,Thul`) — 적중 32% / ICD 8초 → 대상
  - ResistFire 18, 5초, 복합
  - 견고한 저항 축부터 깎아 후속 딜 창출

- **Faith** (`Ohm,Jah,Lem,Eld`) — 적중 22% / ICD 12초 → 자신
  - + 패시브, 복합
  - 광신의 돌격으로 기동과 생존을 동시에 끌어올림

- **Last Wish** (`Jah,Mal,Jah,Sur,Jah,Ber`) — 체력 위급 100% / ICD 60초 → 자신
  - 복합
  - 체력 위급 시 방어·저항 대폭 버프로 생존

- **Phoenix** (`Vex,Vex,Lo,Jah`) — 처치 60% / ICD 2초 → 자신
  - 복합
  - 처치 시 생명력과 마나를 즉시 회복

- **Dream** (`Io,Jah,Pul`) — 적중 30% / ICD 0.8초 → 대상
  - Health(ResistShock) 10, + 패시브, 복합
  - 번개 공명을 축적해 전격 일격을 터뜨림

- **Honor** (`Amn,El,Ith,Tir,Sol`) — 적중 26% / ICD 8초 → 자신
  - HealRateMult +80, 7초, + 패시브, 복합
  - 소울트랩으로 영혼을 묶어 포획

- **Hustle-W** (`Shael,Ko,Eld`) — 적중 26% / ICD 7초 → 자신
  - SpeedMult +30, 5초, + 패시브, 복합
  - 가속으로 빈틈을 메워 기동 우위 확보

- **Beast** (`Ber,Tir,Um,Mal,Lum`) — 적중 100% / ICD 180초 → 자신
  - 복합
  - 타격 시 야수의 격노로 폭발적 강화 (긴 재사용)

- **Obsession** (`Zod,Ist,Lem,Lum,Io,Nef`) — 적중 34% / ICD 5초 → 자신
  - MagickaRateMult +200, 10초, + 패시브, 복합
  - 흡수 꽃망울이 자원을 빨아 약화

- **Rhyme** (`Shael,Eth`) — 피격 22% / ICD 12초 → 자신
  - ResistFrost +30, 6초, + 패시브, 복합
  - 위상 이동으로 즉시 이탈 각을 만듦

- **Hustle-A** (`Shael,Ko,Eld`) — 피격 26% / ICD 10초 → 자신
  - SpeedMult +30, 5초, + 패시브, 복합
  - 위상 이동으로 즉시 이탈 각을 만듦

- **Treachery** (`Shael,Thul,Lem`) — 피격 26% / ICD 10초 → 자신
  - SpeedMult +28, 5초, + 패시브, 복합
  - 머플로 발소리를 지워 은신 진입 보조

- **Sanctuary** (`Ko,Ko,Mal`) — 피격 100% / ICD 0.5초 → 대상
  - Health 3, + 패시브, 복합
  - 받은 피해 일부를 공격자에게 되돌려 보복

## 화염 타격

- **Wrath** (`Pul,Lum,Ber,Mal`) — 적중 26% / ICD 5초 → 대상
  - Health(ResistFire) 30
  - 화염을 실은 일격으로 적에게 추가 화염 피해

- **Brand** (`Jah,Lo,Mal,Gul`) — 적중 28% / ICD 5초 → 대상
  - Health(ResistFire) 10, 6초
  - 화염 낙인을 새겨 지속 화상 피해

## 냉기 타격

- **Doom** (`Hel,Ohm,Um,Lo,Cham`) — 적중 100% / ICD 0.5초 → 대상
  - SpeedMult 30, 4초
  - 타격마다 냉기 감속장을 씌워 둔화 유지

- **Pride** (`Cham,Sur,Io,Lo`) — 적중 28% / ICD 4초 → 대상
  - Health(ResistFrost) 25
  - 냉기를 실은 일격으로 적에게 추가 냉기 피해

## 전격 타격

- **Holy Thunder** (`Eth,Ral,Ort,Tal`) — 적중 22% / ICD 7초 → 대상
  - Health(ResistShock) 30
  - 번개를 실은 일격으로 적에게 추가 전격 피해

- **Destruction** (`Vex,Lo,Ber,Jah,Ko`) — 적중 28% / ICD 5초 → 대상
  - Health(ResistShock) 8, 5초
  - 전류를 흘려보내 지속 전격 피해

## 독 계열

- **Malice** (`Ith,El,Eth`) — 적중 20% / ICD 9초 → 대상
  - Health(PoisonResist) 8, 6초
  - 독이 혈류를 타고 번져 지속 중독 피해

- **Plague** (`Cham,Shael,Um`) — 처치 40% / ICD 4초 → 대상
  - Health(PoisonResist) 6, 5초, 범위20
  - 처치 시 독 구름이 퍼져 잔여 적 압박

## 저항 분쇄 디버프

- **Venom** (`Tal,Dol,Mal`) — 적중 22% / ICD 7초 → 대상
  - PoisonResist -40, 6초
  - 맹독의 침식으로 적의 독 저항을 와해

- **Crescent Moon** (`Shael,Um,Tir`) — 적중 22% / ICD 7초 → 대상
  - ResistShock -35, 6초
  - 달빛 전류가 적의 전격 방어를 붕괴

- **Voice of Reason** (`Lem,Ko,El,Eld`) — 적중 22% / ICD 7초 → 대상
  - ResistFrost -35, 6초
  - 이성의 냉기가 적의 냉기 방어를 관통

- **Rift** (`Hel,Ko,Lem,Gul`) — 적중 20% / ICD 8초 → 대상
  - ResistMagic -30, 6초
  - 차원의 균열이 적의 마법 방어를 분쇄

- **Stone** (`Shael,Um,Pul,Lum`) — 피격 22% / ICD 8초 → 대상
  - DamageResist -50, 6초
  - 돌의 무게가 적의 방어구를 압쇄

## 공격/재생 억제 디버프

- **Duress** (`Shael,Um,Thul`) — 피격 20% / ICD 12초 → 대상
  - AttackDamageMult -25, 5초
  - 강압의 기운으로 공격자의 공격력을 약화

- **Wind** (`Sur,El`) — 적중 20% / ICD 5초 → 대상
  - WeaponSpeedMult -20, 5초
  - 질풍의 압박이 적의 공격 속도를 둔화

- **Steel** (`Tir,El`) — 적중 18% / ICD 8초 → 대상
  - HealRateMult -80, 5초
  - 강철 출혈이 적의 상처 회복을 방해

- **Harmony** (`Tir,Ith,Sol,Ko`) — 적중 20% / ICD 6초 → 대상
  - StaminaRateMult -80, 5초
  - 조화의 교란이 적의 기력 재생을 억제

- **Mist** (`Cham,Shael,Gul,Thul,Ith`) — 적중 18% / ICD 6초 → 대상
  - MagickaRateMult -80, 5초
  - 안개가 적의 마력 흐름을 차단

## 군중 제어: 공포/진정

- **Nadir** (`Nef,Tir`) — 피격 12% / ICD 16초 → 대상
  - Demoralize, Confidence 20, 5초
  - 공포의 파동으로 적을 도주하게 만듦

- **Peace** (`Shael,Thul,Amn`) — 피격 10% / ICD 16초 → 대상
  - Calm, Aggression 25, 6초
  - 평화의 파동으로 적의 적대심을 일시 해제

## 군중 제어: 광란/마비

- **Black** (`Thul,Io,Nef`) — 적중 8% / ICD 20초 → 대상
  - Paralysis, Paralysis, 2초
  - 암흑 충격으로 적의 몸을 순간 마비시킴

- **Delirium** (`Lem,Ist,Io`) — 적중 10% / ICD 15초 → 대상
  - Frenzy, Aggression 20, 5초
  - 광란의 파동으로 적을 동료에게 돌아서게 만듦

## 원소 저항 오라/마력 고갈

- **Silence** (`Dol,Eld,Hel,Ist,Tir,Vex`) — 적중 26% / ICD 7초 → 대상
  - Magicka 50, 5초
  - 침묵의 파동으로 적의 마력을 고갈시킴

- **Ice** (`Amn,Shael,Jah,Lo`) — 적중 100% / ICD 0.5초 → 대상
  - ResistFrost -30, 6초
  - 타격마다 냉기 저항을 깎아 후속 동결 극대화

- **Flickering Flame** (`Nef,Pul,Vex`) — 적중 100% / ICD 0.5초 → 대상
  - ResistFire -25, 6초
  - 타격마다 화염 저항을 깎아 후속 화상 극대화

## 이동속도/스태미나 디버프

- **Obedience** (`Hel,Ko,Thul,Eth,Fal`) — 적중 18% / ICD 5초 → 대상
  - SpeedMult -50, 3초
  - 묵직한 압박으로 적의 이동을 크게 둔화

- **Kingslayer** (`Mal,Um,Gul,Fal`) — 적중 30% / ICD 0.6초 → 대상
  - Health 3, 8초
  - 열린 상처에서 피가 흘러 지속 출혈 피해

- **Famine** (`Fal,Ohm,Ort,Jah`) — 적중 26% / ICD 6초 → 대상
  - Stamina(ResistFire) 30, 5초
  - 기근의 저주로 적의 체력을 소진시킴

## 흡수

- **Death** (`Hel,El,Vex,Ort,Gul`) — 적중 24% / ICD 5초 → 대상
  - Absorb, Health 15
  - 사신의 손길로 적의 생명력을 빼앗아 흡수

- **Gloom** (`Fal,Um,Pul`) — 피격 12% / ICD 18초 → 대상
  - Absorb, Magicka 30
  - 암흑의 흡인으로 적의 마력을 빼앗아 전환

- **Melody** (`Shael,Ko,Nef`) — 적중 18% / ICD 8초 → 대상
  - Absorb, Stamina 20
  - 선율의 울림으로 적의 활력을 흡수해 전환

## 혼합/적응형/스킬

- **Breath of the Dying** (`Vex,Hel,El,Eld,Zod,Eth`) — 처치 50% / ICD 3초 → 대상
  - Health 40, 3초, 범위15
  - 처치 시 독성 폭발로 주변 적에게 피해

- **Zephyr** (`Ort,Eth`) — 적중 20% / ICD 8초 → 자신
  - BowSpeedBonus +30, 6초
  - 질풍의 흐름으로 활 공격 속도 상승

- **Pattern** (`Tal,Ort,Thul`) — 적중 20% / ICD 8초 → 자신
  - CriticalChance +15, 6초
  - 연계 리듬을 살려 치명타 확률 상승

- **Strength** (`Amn,Tir`) — 적중 20% / ICD 9초 → 자신
  - TwoHandedModifier +25, 6초
  - 양손 전투 감각을 극대화해 타격력 강화

- **Edge** (`Tir,Tal,Amn`) — 적중 20% / ICD 8초 → 자신
  - MarksmanModifier +20, 7초
  - 예리한 집중으로 궁술 정확도 상승

- **Unbending Will** (`Fal,Io,Ith,Eld,El,Hel`) — 적중 22% / ICD 9초 → 자신
  - BlockModifier +30, 7초
  - 굳건한 의지로 방패 기술 대폭 강화

- **Lawbringer** (`Amn,Lem,Ko`) — 처치 40% / ICD 6초 → 대상
  - TurnUndead, Confidence 30, 8초, 범위15
  - 처치 시 성스러운 빛이 주변 언데드를 퇴치

- **Oath** (`Shael,Pul,Mal,Lum`) — 적중 24% / ICD 7초 → 자신
  - OneHandedModifier +30, 7초
  - 서약의 힘으로 한손 전투 기술 강화

- **Mosaic** (`Mal,Gul,Amn`) — 적중 26% / ICD 5초 → 대상
  - Health 20
  - 복합 원소 파편이 한꺼번에 작렬

- **Chaos** (`Fal,Ohm,Um`) — 적중 16% / ICD 5.5초 → 대상
  - Health 30, 5초
  - 적의 가장 약한 원소 저항 축을 자동 추적해 관통 타격

- **Eternity** (`Amn,Ber,Ist,Sol,Sur`) — 피격 100% / ICD 120초 → 자신
  - DamageResist +300, 10초
  - 피격 시 대형 방어 결계 발동 (긴 재사용)

## 공격/속도 버프

- **Memory** (`Lum,Io,Sol,Eth`) — 적중 20% / ICD 9초 → 자신
  - StaminaRateMult +100, 8초
  - 전투 기억을 되살려 스태미나 회복 가속

- **Fury** (`Jah,Gul,Eth`) — 적중 24% / ICD 3.5초 → 자신
  - WeaponSpeedMult +15, 8초
  - 격노의 흐름으로 무기 공격 속도 상승

- **Hand of Justice** (`Sur,Cham,Amn,Lo`) — 적중 20% / ICD 7초 → 자신
  - AttackDamageMult +20, 6초
  - 심판의 기세를 실어 공격력을 대폭 끌어올림

## 마법 학파/자연 재생 버프

- **Ground** (`Shael,Io,Ort`) — 피격 22% / ICD 11초 → 자신
  - ShoutRecoveryMult +50, 8초
  - 접지의 힘으로 용언 재충전 속도를 가속

- **Cure** (`Shael,Io,Tal`) — 피격 22% / ICD 11초 → 자신
  - RestorationModifier +20, 8초
  - 치유의 각성으로 회복 마법 위력을 증폭

- **Leaf** (`Tir,Ral`) — 적중 22% / ICD 7초 → 자신
  - DestructionModifier +20, 8초
  - 나뭇잎의 불꽃으로 파괴 마법 위력을 증폭

- **Hearth** (`Shael,Io,Thul`) — 피격 22% / ICD 11초 → 자신
  - ConjurationModifier +20, 7초
  - 화덕의 불씨가 소환의 가교를 열어 강화

- **Passion** (`Dol,Ort,Eld,Lem`) — 적중 22% / ICD 8초 → 자신
  - IllusionModifier +20, 7초
  - 정열의 환상이 환혹 마법 역량을 증폭

- **Temper** (`Shael,Io,Ral`) — 피격 20% / ICD 9초 → 자신
  - StaminaRate +5, 8초
  - 단련의 기운으로 기력 자연 재생을 가속

- **Wisdom** (`Pul,Ith,Eld`) — 피격 20% / ICD 10초 → 자신
  - MagickaRate +5, 7초
  - 지혜의 깊이로 마력 자연 재생을 가속

## 방어 버프

- **Ancient's Pledge** (`Ral,Ort,Tal`) — 피격 20% / ICD 11초 → 자신
  - PoisonResist +60, 7초
  - 고대 서약으로 독과 질병에 대한 저항력 획득

- **Myth** (`Hel,Amn,Nef`) — 피격 20% / ICD 11초 → 자신
  - HeavyArmorModifier +25, 7초
  - 신화적 가호로 중갑 방호 기술 강화

- **Bulwark** (`Shael,Io,Sol`) — 피격 22% / ICD 11초 → 자신
  - DamageResist +80, 7초
  - 수호막으로 방어도를 일시 대폭 상승

- **Splendor** (`Eth,Lum`) — 피격 18% / ICD 10초 → 자신
  - ResistMagic +30, 7초
  - 마력 방패로 마법 저항 전반을 강화

- **Bone** (`Sol,Um,Um`) — 피격 16% / ICD 11초 → 자신
  - ResistFrost +45, 7초
  - 냉기 보호막으로 동결 피해를 차단

- **Prudence** (`Mal,Tir`) — 피격 17% / ICD 11초 → 자신
  - ResistShock +40, 7초
  - 전기 보호막으로 전격 피해를 크게 경감

- **Principle** (`Ral,Gul,Eld`) — 피격 28% / ICD 10초 → 자신
  - ResistFire +50, 7초
  - 원칙의 불꽃으로 화염 저항을 크게 강화

- **Dragon** (`Sur,Lo,Sol`) — 피격 24% / ICD 11초 → 자신
  - LightArmorModifier +30, 8초
  - 용의 비늘로 경갑 방호 기술 강화

## 특수 방어 (반사/이더리얼)

- **Exile** (`Vex,Ohm,Ist,Dol`) — 체력 위급 100% / ICD 50초 → 자신
  - DamageResist +250, 10초
  - 체력 위급 시 강철 피부로 추가 피해 차단

- **Bramble** (`Ral,Ohm,Sur,Eth`) — 피격 22% / ICD 10초 → 자신
  - ReflectDamage +25, 6초
  - 가시 갑옷으로 받은 피해 일부를 공격자에게 반사

- **Metamorphosis** (`Io,Cham,Fal`) — 피격 5% / ICD 30초 → 자신
  - Etherealize, Invisibility, 3초
  - 에테리얼 변신으로 잠시 무적 상태에 진입

## 마나/마법

- **White** (`Dol,Io`) — 적중 18% / ICD 8초 → 자신
  - Magicka +50, 10초
  - 마나 쇄도로 마력 총량을 일시 증폭

- **Lore** (`Ort,Sol`) — 피격 20% / ICD 11초 → 자신
  - MagickaRateMult +80, 7초
  - 지식의 빛으로 마나 재생을 가속

- **Enlightenment** (`Pul,Ral,Sol`) — 피격 22% / ICD 11초 → 자신
  - AlterationModifier +25, 8초
  - 깨달음의 파동으로 변화마법 역량 강화

## 체력/재생

- **King's Grace** (`Amn,Ral,Thul`) — 적중 22% / ICD 9초 → 자신
  - HealRate +8, 7초
  - 치유의 흐름으로 체력 회복 속도 상승

- **Lionheart** (`Hel,Lum,Fal`) — 체력 위급 100% / ICD 45초 → 자신
  - Health +200
  - 체력 위급 시 긴급 대량 회복으로 사선 탈출

- **Rain** (`Ort,Mal,Ith`) — 피격 22% / ICD 11초 → 자신
  - HealRateMult +100, 8초
  - 생명의 비로 체력 재생을 대폭 가속

## 유틸리티

- **Radiance** (`Nef,Sol,Ith`) — 피격 15% / ICD 40초 → 자신
  - DetectLife, DetectLifeRange +200, 30초
  - 광채로 주변 생명체를 감지해 정보 우위 확보

- **Wealth** (`Lem,Ko,Tir`) — 피격 20% / ICD 18초 → 자신
  - CarryWeight +100, 15초
  - 부의 축적으로 소지무게를 크게 늘림

## 은신

- **Stealth** (`Tal,Eth`) — 피격 10% / ICD 20초 → 자신
  - Invisibility, Invisibility, 2초
  - 투명화로 전투를 끊고 각 재설정

- **Smoke** (`Nef,Lum`) — 피격 22% / ICD 12초 → 자신
  - SpeedMult +22, 5초
  - 연막 속에서 이동 속도를 올려 긴급 이탈

## 참고

- proc/ICD 수치는 `kRecipeTunings` 배열에서 94개 개별 조정됩니다.
- 시그니처 룬워드는 여러 MGEF/SPEL + 패시브를 조합합니다.
- C++ IdentityOverride: Honor(ChaosSunder), Eternity(Meditation), Chaos(AdaptiveElement) 3개만 유지.
- 스타일 그룹은 시각 이펙트 전용이며 효과 결정에 관여하지 않습니다.
