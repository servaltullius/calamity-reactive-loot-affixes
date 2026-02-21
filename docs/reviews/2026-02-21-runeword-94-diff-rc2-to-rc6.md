# 룬워드 94개 변경 비교표 (v1.2.18-rc2 -> v1.2.18-rc6)

- 기준 태그: `v1.2.18-rc2` -> `v1.2.18-rc6`
- 비교 대상: `skse/CalamityAffixes/src/RunewordCatalogRows.inl` (94개 카탈로그), `affixes/modules/keywords.affixes.runewords.json` (직접 정의 16개), `skse/CalamityAffixes/src/RunewordIdentityOverrides.cpp` (자동 합성 오버라이드)
- 결과 요약: 변경 `29`개 / 유지 `65`개

## 변경된 항목 상세 (29개)

| No | 룬워드 | Recipe ID | 변경 유형 | 변경 요약 |
|---:|---|---|---|---|
| 9 | Pattern | `rw_pattern` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 가속` (Self) |
| 11 | Strength | `rw_strength` | 자동 합성 변경 | RunewordIdentityOverride 적용: `공속 저하 저주` (Target) |
| 12 | Edge | `rw_edge` | 자동 합성 변경 | RunewordIdentityOverride 적용: `취약 저주` (Target) |
| 15 | Honor | `rw_honor` | 자동 합성 변경 | RunewordIdentityOverride 적용: `파쇄 디버프` (Target) |
| 16 | Lore | `rw_lore` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 방호` (Self) |
| 28 | Memory | `rw_memory` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 방호` (Self) |
| 31 | Melody | `rw_melody` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 가속` (Self) |
| 32 | Harmony | `rw_harmony` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 가속` (Self) |
| 36 | Obedience | `rw_obedience` | 자동 합성 변경 | RunewordIdentityOverride 적용: `파쇄 디버프` (Target) |
| 38 | Wealth | `rw_wealth` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 위상` (Self) |
| 40 | Lawbringer | `rw_lawbringer` | 자동 합성 변경 | RunewordIdentityOverride 적용: `파쇄 디버프` (Target) |
| 43 | Enlightenment | `rw_enlightenment` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 명상` (Self) |
| 44 | Wisdom | `rw_wisdom` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 위상` (Self) |
| 50 | Venom | `rw_venom` | 자동 합성 변경 | RunewordIdentityOverride 적용: `흡수 블룸` (Target) |
| 55 | Call to Arms | `rw_call_to_arms` | 직접 정의 변경 | 트리거 `IncomingHit` -> `Hit`, 효과 설명 텍스트 변경 |
| 57 | Kingslayer | `rw_kingslayer` | 자동 합성 변경 | RunewordIdentityOverride 적용: `공속 저하 저주` (Target) |
| 59 | Principle | `rw_principle` | 자동 합성 변경 | RunewordIdentityOverride 적용: `공포` (Target) |
| 62 | Heart of the Oak | `rw_heart_of_the_oak` | 직접 정의 변경 | Adaptive 모드 `WeakestResist` -> `StrongestResist` |
| 69 | Bramble | `rw_bramble` | 자동 합성 변경 | RunewordIdentityOverride 적용: `타르 블룸` (Target) |
| 74 | Eternity | `rw_eternity` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 명상` (Self) |
| 75 | Infinity | `rw_infinity` | 직접 정의 변경 | modeCycle(수동 오버라이드) 추가 |
| 76 | Wrath | `rw_wrath` | 자동 합성 변경 | RunewordIdentityOverride 적용: `취약 저주` (Target) |
| 77 | Fury | `rw_fury` | 자동 합성 변경 | RunewordIdentityOverride 적용: `연쇄 번개` (Target) |
| 82 | Dream | `rw_dream` | 직접 정의 변경 | 전용 스펠 교체 |
| 83 | Faith | `rw_faith` | 직접 정의 변경 | 전용 스펠 교체, 효과 설명 텍스트 변경 |
| 85 | Last Wish | `rw_last_wish` | 직접 정의 변경 | modeCycle(수동 오버라이드) 추가 |
| 87 | Doom | `rw_doom` | 직접 정의 변경 | 전용 스펠 교체 |
| 89 | Pride | `rw_pride` | 자동 합성 변경 | RunewordIdentityOverride 적용: `자가 방호` (Self) |
| 93 | Mist | `rw_mist` | 자동 합성 변경 | RunewordIdentityOverride 적용: `냉기 동적 타격` (Target) |

## 직접 정의 16개 중 실제 효과 텍스트/런타임 변경

| 룬워드 | 이전(rc2) | 현재(rc6) |
|---|---|---|
| Call to Arms (`rw_call_to_arms`) | 룬워드 콜 투 암스 (Amn-Ral-Mal-Ist-Ohm): 피격 24% / ICD 18초 - 전투 함성(방어도 +150, 8초) | 룬워드 콜 투 암스 (Amn-Ral-Mal-Ist-Ohm): 적중 24% / ICD 18초 - 전투 함성(방어도 +150, 8초) |
| Heart of the Oak (`rw_heart_of_the_oak`) | 룬워드 참나무의 심장 (Ko-Vex-Pul-Thul): 적중 32% / ICD 8초 - 비전 균열(적응형 원소 노출) | 룬워드 참나무의 심장 (Ko-Vex-Pul-Thul): 적중 32% / ICD 8초 - 비전 균열(적응형 원소 노출) |
| Infinity (`rw_infinity`) | 룬워드 무한 (Ber-Mal-Ber-Ist): 적중 35% / ICD 0.5초 - 약점 원소 노출(적 저항 감소) | 룬워드 무한 (Ber-Mal-Ber-Ist): 적중 35% / ICD 0.5초 - 약점 원소 노출(적 저항 감소) |
| Dream (`rw_dream`) | 룬워드 드림 (Io-Jah-Pul): 적중 30% / ICD 0.8초 - 번개의 꿈 일격 | 룬워드 드림 (Io-Jah-Pul): 적중 30% / ICD 0.8초 - 번개의 꿈 일격 |
| Faith (`rw_faith`) | 룬워드 신념 (Ohm-Jah-Lem-Eld): 적중 22% / ICD 12초 - 위상 사냥꾼(이동속도 +45%, 6초) | 룬워드 신념 (Ohm-Jah-Lem-Eld): 적중 22% / ICD 12초 - 광신의 돌격(이속 +35%, 피해저항 +60, 6초) |
| Last Wish (`rw_last_wish`) | 룬워드 라스트 위시 (Jah-Mal-Jah-Sur-Jah-Ber): 적중 18% / ICD 3초 - 소원 붕괴(적응형 원소 노출) | 룬워드 라스트 위시 (Jah-Mal-Jah-Sur-Jah-Ber): 적중 18% / ICD 3초 - 소원 붕괴(적응형 원소 노출) |
| Doom (`rw_doom`) | 룬워드 둠 (Hel-Ohm-Um-Lo-Cham): 적중 30% / ICD 0.8초 - 절망의 냉기 일격 | 룬워드 둠 (Hel-Ohm-Um-Lo-Cham): 적중 30% / ICD 0.8초 - 절망의 냉기 일격 |

## 94개 전체 상태표

| No | 룬워드 | Recipe ID | 룬 조합 | 상태 |
|---:|---|---|---|---|
| 1 | Nadir | `rw_nadir` | `Nef,Tir` | 유지 |
| 2 | Steel | `rw_steel` | `Tir,El` | 유지 |
| 3 | Malice | `rw_malice` | `Ith,El,Eth` | 유지 |
| 4 | Stealth | `rw_stealth` | `Tal,Eth` | 유지 |
| 5 | Leaf | `rw_leaf` | `Tir,Ral` | 유지 |
| 6 | Ancient's Pledge | `rw_ancients_pledge` | `Ral,Ort,Tal` | 유지 |
| 7 | Holy Thunder | `rw_holy_thunder` | `Eth,Ral,Ort,Tal` | 유지 |
| 8 | Zephyr | `rw_zephyr` | `Ort,Eth` | 유지 |
| 9 | Pattern | `rw_pattern` | `Tal,Ort,Thul` | 변경 |
| 10 | King's Grace | `rw_kings_grace` | `Amn,Ral,Thul` | 유지 |
| 11 | Strength | `rw_strength` | `Amn,Tir` | 변경 |
| 12 | Edge | `rw_edge` | `Tir,Tal,Amn` | 변경 |
| 13 | Myth | `rw_myth` | `Hel,Amn,Nef` | 유지 |
| 14 | Spirit | `rw_spirit` | `Tal,Thul,Ort,Amn` | 유지 |
| 15 | Honor | `rw_honor` | `Amn,El,Ith,Tir,Sol` | 변경 |
| 16 | Lore | `rw_lore` | `Ort,Sol` | 변경 |
| 17 | Radiance | `rw_radiance` | `Nef,Sol,Ith` | 유지 |
| 18 | Insight | `rw_insight` | `Ral,Tir,Tal,Sol` | 유지 |
| 19 | Rhyme | `rw_rhyme` | `Shael,Eth` | 유지 |
| 20 | Peace | `rw_peace` | `Shael,Thul,Amn` | 유지 |
| 21 | Black | `rw_black` | `Thul,Io,Nef` | 유지 |
| 22 | Bulwark | `rw_bulwark` | `Shael,Io,Sol` | 유지 |
| 23 | Cure | `rw_cure` | `Shael,Io,Tal` | 유지 |
| 24 | Ground | `rw_ground` | `Shael,Io,Ort` | 유지 |
| 25 | Hearth | `rw_hearth` | `Shael,Io,Thul` | 유지 |
| 26 | Temper | `rw_temper` | `Shael,Io,Ral` | 유지 |
| 27 | White | `rw_white` | `Dol,Io` | 유지 |
| 28 | Memory | `rw_memory` | `Lum,Io,Sol,Eth` | 변경 |
| 29 | Smoke | `rw_smoke` | `Nef,Lum` | 유지 |
| 30 | Splendor | `rw_splendor` | `Eth,Lum` | 유지 |
| 31 | Melody | `rw_melody` | `Shael,Ko,Nef` | 변경 |
| 32 | Harmony | `rw_harmony` | `Tir,Ith,Sol,Ko` | 변경 |
| 33 | Hustle-W | `rw_hustle_w` | `Shael,Ko,Eld` | 유지 |
| 34 | Hustle-A | `rw_hustle_a` | `Shael,Ko,Eld` | 유지 |
| 35 | Lionheart | `rw_lionheart` | `Hel,Lum,Fal` | 유지 |
| 36 | Obedience | `rw_obedience` | `Hel,Ko,Thul,Eth,Fal` | 변경 |
| 37 | Unbending Will | `rw_unbending_will` | `Fal,Io,Ith,Eld,El,Hel` | 유지 |
| 38 | Wealth | `rw_wealth` | `Lem,Ko,Tir` | 변경 |
| 39 | Passion | `rw_passion` | `Dol,Ort,Eld,Lem` | 유지 |
| 40 | Lawbringer | `rw_lawbringer` | `Amn,Lem,Ko` | 변경 |
| 41 | Voice of Reason | `rw_voice_of_reason` | `Lem,Ko,El,Eld` | 유지 |
| 42 | Treachery | `rw_treachery` | `Shael,Thul,Lem` | 유지 |
| 43 | Enlightenment | `rw_enlightenment` | `Pul,Ral,Sol` | 변경 |
| 44 | Wisdom | `rw_wisdom` | `Pul,Ith,Eld` | 변경 |
| 45 | Crescent Moon | `rw_crescent_moon` | `Shael,Um,Tir` | 유지 |
| 46 | Duress | `rw_duress` | `Shael,Um,Thul` | 유지 |
| 47 | Gloom | `rw_gloom` | `Fal,Um,Pul` | 유지 |
| 48 | Stone | `rw_stone` | `Shael,Um,Pul,Lum` | 유지 |
| 49 | Bone | `rw_bone` | `Sol,Um,Um` | 유지 |
| 50 | Venom | `rw_venom` | `Tal,Dol,Mal` | 변경 |
| 51 | Prudence | `rw_prudence` | `Mal,Tir` | 유지 |
| 52 | Sanctuary | `rw_sanctuary` | `Ko,Ko,Mal` | 유지 |
| 53 | Oath | `rw_oath` | `Shael,Pul,Mal,Lum` | 유지 |
| 54 | Rain | `rw_rain` | `Ort,Mal,Ith` | 유지 |
| 55 | Call to Arms | `rw_call_to_arms` | `Amn,Ral,Mal,Ist,Ohm` | 변경 |
| 56 | Delirium | `rw_delirium` | `Lem,Ist,Io` | 유지 |
| 57 | Kingslayer | `rw_kingslayer` | `Mal,Um,Gul,Fal` | 변경 |
| 58 | Rift | `rw_rift` | `Hel,Ko,Lem,Gul` | 유지 |
| 59 | Principle | `rw_principle` | `Ral,Gul,Eld` | 변경 |
| 60 | Mosaic | `rw_mosaic` | `Mal,Gul,Amn` | 유지 |
| 61 | Silence | `rw_silence` | `Dol,Eld,Hel,Ist,Tir,Vex` | 유지 |
| 62 | Heart of the Oak | `rw_heart_of_the_oak` | `Ko,Vex,Pul,Thul` | 변경 |
| 63 | Death | `rw_death` | `Hel,El,Vex,Ort,Gul` | 유지 |
| 64 | Flickering Flame | `rw_flickering_flame` | `Nef,Pul,Vex` | 유지 |
| 65 | Chaos | `rw_chaos` | `Fal,Ohm,Um` | 유지 |
| 66 | Exile | `rw_exile` | `Vex,Ohm,Ist,Dol` | 유지 |
| 67 | Fortitude | `rw_fortitude` | `El,Sol,Dol,Lo` | 유지 |
| 68 | Grief | `rw_grief` | `Eth,Tir,Lo,Mal,Ral` | 유지 |
| 69 | Bramble | `rw_bramble` | `Ral,Ohm,Sur,Eth` | 변경 |
| 70 | Wind | `rw_wind` | `Sur,El` | 유지 |
| 71 | Dragon | `rw_dragon` | `Sur,Lo,Sol` | 유지 |
| 72 | Beast | `rw_beast` | `Ber,Tir,Um,Mal,Lum` | 유지 |
| 73 | Chains of Honor | `rw_chains_of_honor` | `Dol,Um,Ber,Ist` | 유지 |
| 74 | Eternity | `rw_eternity` | `Amn,Ber,Ist,Sol,Sur` | 변경 |
| 75 | Infinity | `rw_infinity` | `Ber,Mal,Ber,Ist` | 변경 |
| 76 | Wrath | `rw_wrath` | `Pul,Lum,Ber,Mal` | 변경 |
| 77 | Fury | `rw_fury` | `Jah,Gul,Eth` | 변경 |
| 78 | Enigma | `rw_enigma` | `Jah,Ith,Ber` | 유지 |
| 79 | Famine | `rw_famine` | `Fal,Ohm,Ort,Jah` | 유지 |
| 80 | Brand | `rw_brand` | `Jah,Lo,Mal,Gul` | 유지 |
| 81 | Destruction | `rw_destruction` | `Vex,Lo,Ber,Jah,Ko` | 유지 |
| 82 | Dream | `rw_dream` | `Io,Jah,Pul` | 변경 |
| 83 | Faith | `rw_faith` | `Ohm,Jah,Lem,Eld` | 변경 |
| 84 | Ice | `rw_ice` | `Amn,Shael,Jah,Lo` | 유지 |
| 85 | Last Wish | `rw_last_wish` | `Jah,Mal,Jah,Sur,Jah,Ber` | 변경 |
| 86 | Phoenix | `rw_phoenix` | `Vex,Vex,Lo,Jah` | 유지 |
| 87 | Doom | `rw_doom` | `Hel,Ohm,Um,Lo,Cham` | 변경 |
| 88 | Hand of Justice | `rw_hand_of_justice` | `Sur,Cham,Amn,Lo` | 유지 |
| 89 | Pride | `rw_pride` | `Cham,Sur,Io,Lo` | 변경 |
| 90 | Plague | `rw_plague` | `Cham,Shael,Um` | 유지 |
| 91 | Metamorphosis | `rw_metamorphosis` | `Io,Cham,Fal` | 유지 |
| 92 | Obsession | `rw_obsession` | `Zod,Ist,Lem,Lum,Io,Nef` | 유지 |
| 93 | Mist | `rw_mist` | `Cham,Shael,Gul,Thul,Ith` | 변경 |
| 94 | Breath of the Dying | `rw_breath_of_the_dying` | `Vex,Hel,El,Eld,Zod,Eth` | 유지 |
