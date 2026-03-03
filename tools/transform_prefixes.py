#!/usr/bin/env python3
"""
Transform 83 prefix affixes: Skyrim lore-based rework.
74 rename-only + 9 new effects on freed slots.
EditorIDs and shared records are preserved.
"""

import json, sys
from pathlib import Path

CORE_JSON = Path("affixes/modules/keywords.affixes.core.json")

# ── 60 standard renames ──────────────────────────────────────────────
# Keys: new_id, new_en, new_ko
# Optional: full_name_en/full_name_ko (complete name replacement)
# Optional: rec_old_en/rec_new_en/rec_old_ko/rec_new_ko (records.*.name)
RENAME_MAP = {
    # ── Element Strikes (4) ──
    "arc_lightning": {
        "new_id": "storm_call",
        "new_en": "Storm Call", "new_ko": "폭풍 소환",
        "rec_old_en": "Arc Lightning", "rec_new_en": "Storm Call",
        "rec_old_ko": "아크 번개", "rec_new_ko": "폭풍 소환",
    },
    "hit_fire_damage": {
        "new_id": "flame_strike",
        "new_en": "Flame Strike", "new_ko": "화염 일격",
        "rec_old_en": "Fire Damage", "rec_new_en": "Flame Strike",
        "rec_old_ko": "화염 피해", "rec_new_ko": "화염 일격",
    },
    "hit_frost_damage": {
        "new_id": "frost_strike",
        "new_en": "Frost Strike", "new_ko": "냉기 일격",
        "rec_old_en": "Frost Damage", "rec_new_en": "Frost Strike",
        "rec_old_ko": "냉기 피해", "rec_new_ko": "냉기 일격",
    },
    "hit_shock_damage": {
        "new_id": "spark_strike",
        "new_en": "Spark Strike", "new_ko": "전격 일격",
        "rec_old_en": "Shock Damage", "rec_new_en": "Spark Strike",
        "rec_old_ko": "번개 피해", "rec_new_ko": "전격 일격",
    },
    # ── Element Weakness (3) ──
    "hit_fire_shred": {
        "new_id": "flame_weakness",
        "new_en": "Flame Weakness", "new_ko": "화염 취약",
        "rec_old_en": "Fire Exposure", "rec_new_en": "Flame Weakness",
        "rec_old_ko": "화염 노출", "rec_new_ko": "화염 취약",
    },
    "hit_frost_shred": {
        "new_id": "frost_weakness",
        "new_en": "Frost Weakness", "new_ko": "냉기 취약",
        "rec_old_en": "Frost Exposure", "rec_new_en": "Frost Weakness",
        "rec_old_ko": "냉기 노출", "rec_new_ko": "냉기 취약",
    },
    "hit_shock_shred": {
        "new_id": "shock_weakness",
        "new_en": "Shock Weakness", "new_ko": "전격 취약",
        "rec_old_en": "Shock Exposure", "rec_new_en": "Shock Weakness",
        "rec_old_ko": "번개 노출", "rec_new_ko": "전격 취약",
    },
    # ── Summon (7) ──
    "hit_conjure_familiar": {
        "new_id": "wolf_spirit",
        "new_en": "Wolf Spirit", "new_ko": "늑대 혼령",
    },
    "hit_flaming_familiar": {
        "new_id": "flame_sprite",
        "new_en": "Flame Sprite", "new_ko": "화염 정령",
    },
    "hit_conjure_flame_atronach": {
        "new_id": "flame_atronach",
        "new_en": "Flame Atronach", "new_ko": "화염 아트로나크",
    },
    "hit_conjure_frost_atronach": {
        "new_id": "frost_atronach",
        "new_en": "Frost Atronach", "new_ko": "냉기 아트로나크",
    },
    "hit_conjure_storm_atronach": {
        "new_id": "storm_atronach",
        "new_en": "Storm Atronach", "new_ko": "폭풍 아트로나크",
    },
    "hit_conjure_dremora_lord": {
        "new_id": "dremora_pact",
        "new_en": "Dremora Pact", "new_ko": "드레모라 서약",
    },
    "hit_soul_trap": {  # no records in source (uses vanilla spell)
        "new_id": "soul_snare",
        "new_en": "Soul Snare", "new_ko": "영혼 올가미",
    },
    # ── Defense (6) ──
    "incoming_invisibility": {
        "new_id": "shadow_veil",
        "new_en": "Shadow Veil", "new_ko": "그림자 장막",
        "rec_old_en": "Invisibility", "rec_new_en": "Shadow Veil",
        "rec_old_ko": "투명화", "rec_new_ko": "그림자 장막",
    },
    "incoming_warden_shell": {
        "new_id": "stone_ward",
        "new_en": "Stone Ward", "new_ko": "돌의 수호",
        "rec_old_en": "Warden Shell", "rec_new_en": "Stone Ward",
        "rec_old_ko": "수호 껍질", "rec_new_ko": "돌의 수호",
    },
    "incoming_resolve_barrier": {
        "new_id": "arcane_ward",
        "new_en": "Arcane Ward", "new_ko": "마법 방벽",
        "rec_old_en": "Resolve Barrier", "rec_new_en": "Arcane Ward",
        "rec_old_ko": "결의 장막", "rec_new_ko": "마법 방벽",
    },
    "incoming_vital_rebound": {
        "new_id": "healing_surge",
        "new_en": "Healing Surge", "new_ko": "치유의 파도",
        "rec_old_en": "Vital Rebound", "rec_new_en": "Healing Surge",
        "rec_old_ko": "재기 반동", "rec_new_ko": "치유의 파도",
    },
    "incoming_mind_over_matter_t1": {
        "new_id": "mage_armor_t1",
        "new_en": "Mage Armor I", "new_ko": "마법사의 갑옷 I",
        "full_name_ko": "마법사의 갑옷 I: 물리 피해 10%를 매지카로 전환",
        "full_name_en": "Mage Armor I: Redirect 10% of physical hit damage to Magicka",
    },
    "incoming_mind_over_matter_t2": {
        "new_id": "mage_armor_t2",
        "new_en": "Mage Armor II", "new_ko": "마법사의 갑옷 II",
        "full_name_ko": "마법사의 갑옷 II: 물리 피해 15%를 매지카로 전환",
        "full_name_en": "Mage Armor II: Redirect 15% of physical hit damage to Magicka",
    },
    # ── Scroll (4) ──
    "scroll_echo_preservation_t1": {
        "new_id": "scroll_mastery_t1",
        "new_en": "Scroll Mastery I", "new_ko": "두루마리 숙련 I",
        "full_name_ko": "두루마리 숙련 I: 스크롤 비소모 +20%",
        "full_name_en": "Scroll Mastery I: +20% chance to not consume scrolls",
    },
    "scroll_echo_preservation_t2": {
        "new_id": "scroll_mastery_t2",
        "new_en": "Scroll Mastery II", "new_ko": "두루마리 숙련 II",
        "full_name_ko": "두루마리 숙련 II: 스크롤 비소모 +25%",
        "full_name_en": "Scroll Mastery II: +25% chance to not consume scrolls",
    },
    "scroll_echo_preservation": {
        "new_id": "scroll_mastery_t3",
        "new_en": "Scroll Mastery III", "new_ko": "두루마리 숙련 III",
        "full_name_ko": "두루마리 숙련 III: 스크롤 비소모 +30%",
        "full_name_en": "Scroll Mastery III: +30% chance to not consume scrolls",
    },
    "scroll_echo_preservation_t4": {
        "new_id": "scroll_mastery_t4",
        "new_en": "Scroll Mastery IV", "new_ko": "두루마리 숙련 IV",
        "full_name_ko": "두루마리 숙련 IV: 스크롤 비소모 +35%",
        "full_name_en": "Scroll Mastery IV: +35% chance to not consume scrolls",
    },
    # ── Fire DoT (2) ──
    "hit_emberbrand_ignite": {
        "new_id": "ember_brand",
        "new_en": "Ember Brand", "new_ko": "잔불 낙인",
        "rec_old_en": "Emberbrand Ignite", "rec_new_en": "Ember Brand",
        "rec_old_ko": "불씨 각인 점화", "rec_new_ko": "잔불 낙인",
    },
    "kill_emberbrand_burst": {
        "new_id": "ember_pyre",
        "new_en": "Ember Pyre", "new_ko": "잔불 장작",
    },
    # ── CC / Debuff (3) ──
    "hit_frostlock_snare": {
        "new_id": "ice_shackle",
        "new_en": "Ice Shackle", "new_ko": "얼음 족쇄",
        "rec_old_en": "Frostlock Snare", "rec_new_en": "Ice Shackle",
        "rec_old_ko": "서리 족쇄", "rec_new_ko": "얼음 족쇄",
    },
    "hit_shock_overload": {
        "new_id": "mana_burn",
        "new_en": "Mana Burn", "new_ko": "마나 소진",
        "rec_old_en": "Shock Overload", "rec_new_en": "Mana Burn",
        "rec_old_ko": "과전류", "rec_new_ko": "마나 소진",
    },
    "hit_blood_siphon": {
        "new_id": "life_drain",
        "new_en": "Life Drain", "new_ko": "생명 흡수",
        "rec_old_en": "Blood Siphon", "rec_new_en": "Life Drain",
        "rec_old_ko": "혈류 흡수", "rec_new_ko": "생명 흡수",
    },
    # ── Utility (4) ──
    "kill_soul_surge": {
        "new_id": "soul_siphon",
        "new_en": "Soul Siphon", "new_ko": "영혼 착취",
        "rec_old_en": "Soul Surge", "rec_new_en": "Soul Siphon",
        "rec_old_ko": "영혼 쇄도", "rec_new_ko": "영혼 착취",
    },
    "hit_shadowstep_burst": {
        "new_id": "shadow_stride",
        "new_en": "Shadow Stride", "new_ko": "그림자 질주",
        "rec_old_en": "Shadowstep Burst", "rec_new_en": "Shadow Stride",
        "rec_old_ko": "그림자 질주", "rec_new_ko": "그림자 질주",
    },
    "hit_muffle": {
        "new_id": "silent_step",
        "new_en": "Silent Step", "new_ko": "무음 보행",
    },
    "hit_swap_jackpot": {
        "new_id": "battle_frenzy",
        "new_en": "Battle Frenzy", "new_ko": "전투 광란",
        "rec_old_en": "Swap Jackpot", "rec_new_en": "Battle Frenzy",
        "rec_old_ko": "스왑 잭팟", "rec_new_ko": "전투 광란",
    },
    # ── Traps (3) ──
    "trap_ironjaw": {
        "new_id": "bear_trap",
        "new_en": "Bear Trap", "new_ko": "곰 덫",
        "rec_old_en": "Ironjaw Trap", "rec_new_en": "Bear Trap",
        "rec_old_ko": "철턱 덫", "rec_new_ko": "곰 덫",
    },
    "hit_minefield": {
        "new_id": "rune_trap",
        "new_en": "Rune Trap", "new_ko": "룬 함정",
        "rec_old_en": "Minefield", "rec_new_en": "Rune Trap",
        "rec_old_ko": "지뢰 밭", "rec_new_ko": "룬 함정",
    },
    "hit_chaos_mine": {
        "new_id": "chaos_rune",
        "new_en": "Chaos Rune", "new_ko": "혼돈의 룬",
        "rec_old_en": "Chaos Mine", "rec_new_en": "Chaos Rune",
        "rec_old_ko": "카오스 지뢰", "rec_new_ko": "혼돈의 룬",
    },
    # ── Convert keep (6) ──
    "convert_phys_to_fire_50": {
        "new_id": "fire_infusion_50",
        "new_en": "Fire Infusion", "new_ko": "화염 주입",
        "full_name_ko": "화염 주입 (50%)",
        "full_name_en": "Fire Infusion (50%)",
    },
    "convert_phys_to_fire_100": {
        "new_id": "fire_infusion_100",
        "new_en": "Fire Infusion", "new_ko": "화염 주입",
        "full_name_ko": "화염 주입 (100%)",
        "full_name_en": "Fire Infusion (100%)",
    },
    "convert_phys_to_frost_50": {
        "new_id": "frost_infusion_50",
        "new_en": "Frost Infusion", "new_ko": "냉기 주입",
        "full_name_ko": "냉기 주입 (50%)",
        "full_name_en": "Frost Infusion (50%)",
    },
    "convert_phys_to_frost_100": {
        "new_id": "frost_infusion_100",
        "new_en": "Frost Infusion", "new_ko": "냉기 주입",
        "full_name_ko": "냉기 주입 (100%)",
        "full_name_en": "Frost Infusion (100%)",
    },
    "convert_phys_to_shock_50": {
        "new_id": "shock_infusion_50",
        "new_en": "Shock Infusion", "new_ko": "전격 주입",
        "full_name_ko": "전격 주입 (50%)",
        "full_name_en": "Shock Infusion (50%)",
    },
    "convert_phys_to_shock_100": {
        "new_id": "shock_infusion_100",
        "new_en": "Shock Infusion", "new_ko": "전격 주입",
        "full_name_ko": "전격 주입 (100%)",
        "full_name_en": "Shock Infusion (100%)",
    },
    # ── CoC → Crit Cast (7) ──
    "coc_firebolt": {
        "new_id": "crit_cast_firebolt",
        "new_en": "Crit Cast: Firebolt", "new_ko": "치명 시전: 파이어볼트",
    },
    "coc_ice_spike": {
        "new_id": "crit_cast_ice_spike",
        "new_en": "Crit Cast: Ice Spike", "new_ko": "치명 시전: 아이스 스파이크",
    },
    "coc_lightning_bolt": {
        "new_id": "crit_cast_lightning_bolt",
        "new_en": "Crit Cast: Lightning Bolt", "new_ko": "치명 시전: 라이트닝 볼트",
    },
    "coc_thunderbolt": {
        "new_id": "crit_cast_thunderbolt",
        "new_en": "Crit Cast: Thunderbolt", "new_ko": "치명 시전: 썬더볼트",
    },
    "coc_icy_spear": {
        "new_id": "crit_cast_icy_spear",
        "new_en": "Crit Cast: Icy Spear", "new_ko": "치명 시전: 아이시 스피어",
    },
    "coc_chain_lightning": {
        "new_id": "crit_cast_chain_lightning",
        "new_en": "Crit Cast: Chain Lightning", "new_ko": "치명 시전: 체인 라이트닝",
    },
    "coc_ice_storm": {
        "new_id": "crit_cast_ice_storm",
        "new_en": "Crit Cast: Ice Storm", "new_ko": "치명 시전: 아이스 스톰",
    },
    # ── Corpse → Death Pyre (3) ──
    "corpse_explosion_t1": {
        "new_id": "death_pyre_t1",
        "new_en": "Death Pyre T1", "new_ko": "죽음의 화장 T1",
    },
    "corpse_explosion_t2": {
        "new_id": "death_pyre_t2",
        "new_en": "Death Pyre T2", "new_ko": "죽음의 화장 T2",
    },
    "corpse_explosion_t3": {
        "new_id": "death_pyre_t3",
        "new_en": "Death Pyre T3", "new_ko": "죽음의 화장 T3",
    },
    # ── Summon Corpse → Conjured Pyre (2) ──
    "summon_corpse_explosion_t1": {
        "new_id": "conjured_pyre_t1",
        "new_en": "Conjured Pyre T1", "new_ko": "소환 화장 T1",
    },
    "summon_corpse_explosion_t2": {
        "new_id": "conjured_pyre_t2",
        "new_en": "Conjured Pyre T2", "new_ko": "소환 화장 T2",
    },
    # ── Plague / Adaptation (4) ──
    "hit_adaptive_exposure": {
        "new_id": "elemental_bane",
        "new_en": "Elemental Bane", "new_ko": "원소 저주",
    },
    "dot_bloom": {
        "new_id": "plague_spore",
        "new_en": "Plague Spore", "new_ko": "역병 포자",
        "rec_old_en": "DoT Bloom", "rec_new_en": "Plague Spore",
        "rec_old_ko": "DoT 블룸", "rec_new_ko": "역병 포자",
    },
    "dot_bloom_tar": {
        "new_id": "tar_blight",
        "new_en": "Tar Blight", "new_ko": "역청 황폐",
        "rec_old_en": "DoT Tar Bloom", "rec_new_en": "Tar Blight",
        "rec_old_ko": "DoT 타르 블룸", "rec_new_ko": "역청 황폐",
    },
    "dot_bloom_siphon": {
        "new_id": "siphon_spore",
        "new_en": "Siphon Spore", "new_ko": "흡수 포자",
        "rec_old_en": "DoT Siphon Bloom", "rec_new_en": "Siphon Spore",
        "rec_old_ko": "DoT 흡수 블룸", "rec_new_ko": "흡수 포자",
    },
    # ── Evolution (2) ──
    "hit_evolving_lightning": {
        "new_id": "thunder_mastery",
        "new_en": "Thunder Mastery", "new_ko": "뇌격 숙련",
    },
    "hit_mode_cycle_elements": {
        "new_id": "elemental_attunement",
        "new_en": "Elemental Attunement", "new_ko": "원소 조율",
    },
}

# ── 10 INTERNAL renames ──────────────────────────────────────────────
INTERNAL_MAP = {
    "internal_spell_trap_dragonteeth_poison": {
        "new_id": "internal_spell_bear_trap_poison",
        "name_old": "Trap Dragon's Teeth", "name_new": "Bear Trap",
        "rec_old": "Dragon's Teeth", "rec_new": "Bear Trap",
    },
    "internal_spell_trap_dragonteeth_drain_magicka": {
        "new_id": "internal_spell_bear_trap_drain_magicka",
        "name_old": "Trap Dragon's Teeth", "name_new": "Bear Trap",
        "rec_old": "Dragon's Teeth", "rec_new": "Bear Trap",
    },
    "internal_spell_trap_dragonteeth_drain_stamina": {
        "new_id": "internal_spell_bear_trap_drain_stamina",
        "name_old": "Trap Dragon's Teeth", "name_new": "Bear Trap",
        "rec_old": "Dragon's Teeth", "rec_new": "Bear Trap",
    },
    "internal_spell_dot_bloom_tar_sunder": {
        "new_id": "internal_spell_tar_blight_sunder",
        "name_old": "DoT Tar Bloom", "name_new": "Tar Blight",
        "rec_old": "DoT Tar Bloom", "rec_new": "Tar Blight",
    },
    "internal_spell_dot_bloom_siphon_sta": {
        "new_id": "internal_spell_siphon_spore_sta",
        "name_old": "DoT Siphon Bloom", "name_new": "Siphon Spore",
        "rec_old": "DoT Siphon Bloom", "rec_new": "Siphon Spore",
    },
    "internal_spell_chaos_curse_sunder": {
        "new_id": "internal_spell_chaos_rune_sunder",
        "name_old": "Chaos Curse", "name_new": "Chaos Rune",
        "rec_old": "Chaos Curse", "rec_new": "Chaos Rune",
    },
    "internal_spell_chaos_curse_drain_magregen": {
        "new_id": "internal_spell_chaos_rune_drain_magregen",
        "name_old": "Chaos Curse", "name_new": "Chaos Rune",
        "rec_old": "Chaos Curse", "rec_new": "Chaos Rune",
    },
    "internal_spell_chaos_curse_drain_stamregen": {
        "new_id": "internal_spell_chaos_rune_drain_stamregen",
        "name_old": "Chaos Curse", "name_new": "Chaos Rune",
        "rec_old": "Chaos Curse", "rec_new": "Chaos Rune",
    },
    "internal_spell_chaos_curse_slow_attack": {
        "new_id": "internal_spell_chaos_rune_slow_attack",
        "name_old": "Chaos Curse", "name_new": "Chaos Rune",
        "rec_old": "Chaos Curse", "rec_new": "Chaos Rune",
    },
    "internal_spell_chaos_curse_fragile": {
        "new_id": "internal_spell_chaos_rune_fragile",
        "name_old": "Chaos Curse", "name_new": "Chaos Rune",
        "rec_old": "Chaos Curse", "rec_new": "Chaos Rune",
    },
}

# ── 4 no-change entries ──────────────────────────────────────────────
NO_CHANGE_IDS = {"archmage_t1", "archmage_t2", "archmage_t3", "archmage_t4"}

# ── 9 freed slots: full replacement ──────────────────────────────────
# records are PRESERVED from original entry (shared dependency).
# Only id, name, nameEn, nameKo, kid, runtime are replaced.
FREED_SLOTS = {
    # ── Thu'um (5) + Spell Breach (1) ──
    "convert_phys_to_fire_25": {  # idx 36 → voice_of_power (HAS records)
        "new_id": "voice_of_power",
        "name_ko": "힘의 외침 (피격 25% / ICD 6초): 가속 부여",
        "name_en": "Voice of Power (25% on hit taken / ICD 6s): Haste Buff",
        "kid": {
            "type": "Armor",
            "strings": "NONE",
            "formFilters": "NONE",
            "traits": "-E",
            "chance": 0.5,
        },
        "runtime": {
            "trigger": "IncomingHit",
            "procChancePercent": 25.0,
            "icdSeconds": 6.0,
            "action": {
                "type": "CastSpell",
                "spellEditorId": "CAFF_SPEL_SWAP_JACKPOT_HASTE",
                "applyTo": "Self",
                "noHitEffectArt": True,
            },
            "lootWeight": 1.0,
        },
    },
    "convert_phys_to_fire_75": {  # idx 38 → death_mark (NO records)
        "new_id": "death_mark",
        "name_ko": "죽음의 표식 (Lucky Hit 15% / 표적별 ICD 25초): 방어력 파쇄",
        "name_en": "Death's Mark (Lucky Hit 15% / per-target ICD 25s): Armor Shred",
        "kid": {
            "type": "Weapon",
            "strings": "NONE",
            "formFilters": "NONE",
            "traits": "-E",
            "chance": 0.4,
        },
        "runtime": {
            "trigger": "Hit",
            "procChancePercent": 100.0,
            "luckyHitChancePercent": 15.0,
            "luckyHitProcCoefficient": 1.0,
            "icdSeconds": 1.0,
            "perTargetICDSeconds": 25.0,
            "action": {
                "type": "CastSpell",
                "spellEditorId": "CAFF_SPEL_CHAOS_CURSE_SUNDER",
                "applyTo": "Target",
            },
            "lootWeight": 1.0,
        },
    },
    "convert_phys_to_frost_25": {  # idx 40 → ice_form (HAS records)
        "new_id": "ice_form",
        "name_ko": "얼음 형상 (Lucky Hit 8% / ICD 15초): 극한 동결",
        "name_en": "Ice Form (Lucky Hit 8% / ICD 15s): Deep Freeze",
        "kid": {
            "type": "Weapon",
            "strings": "NONE",
            "formFilters": "NONE",
            "traits": "-E",
            "chance": 0.3,
        },
        "runtime": {
            "trigger": "Hit",
            "procChancePercent": 100.0,
            "luckyHitChancePercent": 8.0,
            "luckyHitProcCoefficient": 1.0,
            "icdSeconds": 15.0,
            "action": {
                "type": "CastSpell",
                "spellEditorId": "CAFF_SPEL_TRAP_IRONJAW_SNARE",
                "applyTo": "Target",
            },
            "lootWeight": 1.0,
        },
    },
    "convert_phys_to_frost_75": {  # idx 42 → disarming_shout (NO records)
        "new_id": "disarming_shout",
        "name_ko": "무장 해제 (Lucky Hit 10% / ICD 10초): 공격속도 감소",
        "name_en": "Disarming Shout (Lucky Hit 10% / ICD 10s): Attack Speed Reduction",
        "kid": {
            "type": "Weapon",
            "strings": "NONE",
            "formFilters": "NONE",
            "traits": "-E",
            "chance": 0.4,
        },
        "runtime": {
            "trigger": "Hit",
            "procChancePercent": 100.0,
            "luckyHitChancePercent": 10.0,
            "luckyHitProcCoefficient": 1.0,
            "icdSeconds": 10.0,
            "action": {
                "type": "CastSpell",
                "spellEditorId": "CAFF_SPEL_CHAOS_CURSE_SLOW_ATTACK",
                "applyTo": "Target",
            },
            "lootWeight": 1.0,
        },
    },
    "convert_phys_to_shock_25": {  # idx 44 → whirlwind_sprint (HAS records)
        "new_id": "whirlwind_sprint",
        "name_ko": "질풍 외침 (처치 100% / ICD 3초): 이동속도 버프 (성장형)",
        "name_en": "Whirlwind Sprint (100% on kill / ICD 3s): Move Speed Buff (Evolution)",
        "kid": {
            "type": "Weapon",
            "strings": "NONE",
            "formFilters": "NONE",
            "traits": "-E",
            "chance": 0.35,
        },
        "runtime": {
            "trigger": "Kill",
            "procChancePercent": 100.0,
            "icdSeconds": 3.0,
            "action": {
                "type": "CastSpell",
                "spellEditorId": "CAFF_SPEL_HIT_SHADOWSTEP_BURST",
                "applyTo": "Self",
                "noHitEffectArt": True,
                "evolution": {
                    "xpPerProc": 1,
                    "thresholds": [0, 8, 24, 50],
                    "multipliers": [1.0, 1.25, 1.55, 2.0],
                },
            },
            "lootWeight": 1.0,
        },
    },
    "convert_phys_to_shock_75": {  # idx 46 → spell_breach (NO records)
        "new_id": "spell_breach",
        "name_ko": "마법 약화 (Lucky Hit 12% / ICD 10초): 마법 저항 감소",
        "name_en": "Spell Breach (Lucky Hit 12% / ICD 10s): Magic Resist Reduction",
        "kid": {
            "type": "Weapon",
            "strings": "NONE",
            "formFilters": "NONE",
            "traits": "-E",
            "chance": 0.35,
        },
        "runtime": {
            "trigger": "Hit",
            "procChancePercent": 100.0,
            "luckyHitChancePercent": 12.0,
            "luckyHitProcCoefficient": 1.0,
            "icdSeconds": 10.0,
            "action": {
                "type": "CastSpell",
                "spellEditorId": "CAFF_SPEL_CHAOS_CURSE_FRAGILE",
                "applyTo": "Target",
            },
            "lootWeight": 1.0,
        },
    },
    # ── Magic School (3) ──
    "corpse_explosion_t4": {  # idx 62 → stamina_drain (NO records)
        "new_id": "stamina_drain",
        "name_ko": "기력 고갈 (Lucky Hit 20% / ICD 4초): 스태미나 드레인",
        "name_en": "Stamina Drain (Lucky Hit 20% / ICD 4s): Stamina Drain",
        "kid": {
            "type": "Weapon",
            "strings": "NONE",
            "formFilters": "NONE",
            "traits": "-E",
            "chance": 0.35,
        },
        "runtime": {
            "trigger": "Hit",
            "procChancePercent": 100.0,
            "luckyHitChancePercent": 20.0,
            "luckyHitProcCoefficient": 1.0,
            "icdSeconds": 4.0,
            "action": {
                "type": "CastSpell",
                "spellEditorId": "CAFF_SPEL_TRAP_DRAGONTEETH_DRAIN_STAMINA",
                "applyTo": "Target",
            },
            "lootWeight": 1.0,
        },
    },
    "corpse_explosion_t5": {  # idx 63 → nourishing_flame (NO records)
        "new_id": "nourishing_flame",
        "name_ko": "자양의 불꽃 (적중 15% / ICD 1.5초 / 최근 처치 필요): 자가 치유 (적중 피해 스케일)",
        "name_en": "Nourishing Flame (15% on hit / ICD 1.5s / requires recent kill): Self Heal (Hit Damage Scaling)",
        "kid": {
            "type": "Weapon",
            "strings": "NONE",
            "formFilters": "NONE",
            "traits": "-E",
            "chance": 0.3,
        },
        "runtime": {
            "trigger": "Hit",
            "procChancePercent": 15.0,
            "requireRecentlyKillSeconds": 5.0,
            "icdSeconds": 1.5,
            "action": {
                "type": "CastSpell",
                "spellEditorId": "CAFF_SPEL_HIT_BLOOD_SIPHON",
                "applyTo": "Self",
                "noHitEffectArt": True,
                "magnitudeScaling": {
                    "source": "HitPhysicalDealt",
                    "mult": 0.06,
                    "add": 3.0,
                    "min": 3.0,
                    "max": 20.0,
                    "spellBaseAsMin": True,
                },
            },
            "lootWeight": 1.0,
        },
    },
    "summon_corpse_explosion_t3": {  # idx 66 → mana_knot (NO records)
        "new_id": "mana_knot",
        "name_ko": "마나 매듭 (Lucky Hit 15% / ICD 8초): 매지카 재생 감소",
        "name_en": "Mana Knot (Lucky Hit 15% / ICD 8s): Magicka Regen Reduction",
        "kid": {
            "type": "Weapon",
            "strings": "NONE",
            "formFilters": "NONE",
            "traits": "-E",
            "chance": 0.3,
        },
        "runtime": {
            "trigger": "Hit",
            "procChancePercent": 100.0,
            "luckyHitChancePercent": 15.0,
            "luckyHitProcCoefficient": 1.0,
            "icdSeconds": 8.0,
            "action": {
                "type": "CastSpell",
                "spellEditorId": "CAFF_SPEL_CHAOS_CURSE_DRAIN_MAGREGEN",
                "applyTo": "Target",
            },
            "lootWeight": 1.0,
        },
    },
}


def extract_label(name: str) -> str:
    """Extract label portion before first ' (' in name."""
    if " (" in name:
        return name.split(" (")[0]
    return name


def transform_records(entry: dict, cfg: dict) -> None:
    """Transform records.*.name fields using old/new English/Korean."""
    if "records" not in entry:
        return
    old_en = cfg.get("rec_old_en")
    if not old_en:
        return
    new_en = cfg["rec_new_en"]
    old_ko = cfg.get("rec_old_ko", "")
    new_ko = cfg.get("rec_new_ko", "")

    for key in ("magicEffect", "spell"):
        if key in entry["records"] and "name" in entry["records"][key]:
            n = entry["records"][key]["name"]
            n = n.replace(old_en, new_en, 1)
            if old_ko and new_ko:
                n = n.replace(old_ko, new_ko, 1)
            entry["records"][key]["name"] = n


def main():
    with open(CORE_JSON) as f:
        entries = json.load(f)

    assert len(entries) == 83, f"Expected 83 entries, got {len(entries)}"

    # Snapshot editorIds for validation
    original_editor_ids = [e["editorId"] for e in entries]

    seen_ids = set()
    changed = 0

    for i, entry in enumerate(entries):
        old_id = entry["id"]

        # ── No change ──
        if old_id in NO_CHANGE_IDS:
            seen_ids.add(old_id)
            continue

        # ── Freed slots (9) ──
        if old_id in FREED_SLOTS:
            cfg = FREED_SLOTS[old_id]
            # Preserve immutable fields
            saved_editor_id = entry["editorId"]
            saved_records = entry.get("records")
            saved_slot = entry["slot"]

            # Build new entry preserving key order
            new_entry = {
                "id": cfg["new_id"],
                "editorId": saved_editor_id,
                "name": cfg["name_ko"],
                "nameEn": cfg["name_en"],
                "nameKo": cfg["name_ko"],
            }
            if saved_records:
                new_entry["records"] = saved_records
            new_entry["kid"] = cfg["kid"]
            new_entry["runtime"] = cfg["runtime"]
            new_entry["slot"] = saved_slot

            entries[i] = new_entry
            seen_ids.add(cfg["new_id"])
            changed += 1
            continue

        # ── Internal renames (10) ──
        if old_id in INTERNAL_MAP:
            cfg = INTERNAL_MAP[old_id]
            entry["id"] = cfg["new_id"]
            for field in ("name", "nameEn", "nameKo"):
                entry[field] = entry[field].replace(cfg["name_old"], cfg["name_new"], 1)
            if "records" in entry:
                for key in ("magicEffect", "spell"):
                    if key in entry["records"] and "name" in entry["records"][key]:
                        entry["records"][key]["name"] = entry["records"][key]["name"].replace(
                            cfg["rec_old"], cfg["rec_new"], 1
                        )
            seen_ids.add(cfg["new_id"])
            changed += 1
            continue

        # ── Standard renames (60) ──
        if old_id in RENAME_MAP:
            cfg = RENAME_MAP[old_id]
            entry["id"] = cfg["new_id"]

            if "full_name_ko" in cfg:
                entry["name"] = cfg["full_name_ko"]
                entry["nameEn"] = cfg["full_name_en"]
                entry["nameKo"] = cfg["full_name_ko"]
            else:
                old_ko_label = extract_label(entry["name"])
                old_en_label = extract_label(entry["nameEn"])
                entry["name"] = entry["name"].replace(old_ko_label, cfg["new_ko"], 1)
                entry["nameEn"] = entry["nameEn"].replace(old_en_label, cfg["new_en"], 1)
                entry["nameKo"] = entry["nameKo"].replace(old_ko_label, cfg["new_ko"], 1)

            transform_records(entry, cfg)
            seen_ids.add(cfg["new_id"])
            changed += 1
            continue

        print(f"WARNING: unhandled entry [{i}]: {old_id}", file=sys.stderr)
        seen_ids.add(old_id)

    # ── Validation ──
    # 1. All 83 entries processed
    assert len(seen_ids) == 83, f"Expected 83 unique IDs, got {len(seen_ids)}: duplicates or missing"

    # 2. 79 changed (83 - 4 archmage no-change)
    assert changed == 79, f"Expected 79 changes, got {changed}"

    # 3. EditorIDs preserved
    for i, entry in enumerate(entries):
        assert entry["editorId"] == original_editor_ids[i], (
            f"[{i}] EditorID mismatch: {entry['editorId']} != {original_editor_ids[i]}"
        )

    # 4. ID uniqueness
    all_ids = [e["id"] for e in entries]
    dupes = [x for x in all_ids if all_ids.count(x) > 1]
    assert not dupes, f"Duplicate IDs: {set(dupes)}"

    # 5. INTERNAL prefix preserved
    for entry in entries:
        if entry["id"].startswith("internal_"):
            assert entry["name"].startswith("INTERNAL:"), (
                f"{entry['id']}: name should start with INTERNAL:"
            )

    # 6. Shared records not modified (idx 36, 40, 44)
    shared_checks = {
        36: ("CAFF_MGEF_DMG_FIRE_DYNAMIC", "CAFF_SPEL_DMG_FIRE_DYNAMIC"),
        40: ("CAFF_MGEF_DMG_FROST_DYNAMIC", "CAFF_SPEL_DMG_FROST_DYNAMIC"),
        44: ("CAFF_MGEF_DMG_SHOCK_DYNAMIC", "CAFF_SPEL_DMG_SHOCK_DYNAMIC"),
    }
    for idx, (mgef_eid, spel_eid) in shared_checks.items():
        rec = entries[idx].get("records", {})
        assert rec.get("magicEffect", {}).get("editorId") == mgef_eid, (
            f"[{idx}] records.magicEffect.editorId changed!"
        )
        assert rec.get("spell", {}).get("editorId") == spel_eid, (
            f"[{idx}] records.spell.editorId changed!"
        )

    # 7. All 9 new effects have lootWeight > 0
    for old_id, cfg in FREED_SLOTS.items():
        new_id = cfg["new_id"]
        entry = next(e for e in entries if e["id"] == new_id)
        assert entry["runtime"]["lootWeight"] > 0, f"{new_id}: lootWeight must be > 0"

    # 8. kid.type changes verified
    assert entries[36]["kid"]["type"] == "Armor", "voice_of_power should be Armor"
    assert entries[66]["kid"]["type"] == "Weapon", "mana_knot should be Weapon"

    # ── Write output ──
    with open(CORE_JSON, "w") as f:
        json.dump(entries, f, indent=2, ensure_ascii=False)
        f.write("\n")

    print(f"✓ Transformed {changed}/83 entries in {CORE_JSON}")
    print(f"  - 60 standard renames")
    print(f"  - 10 internal renames")
    print(f"  - 9 freed slot replacements")
    print(f"  - 4 archmage unchanged")
    print(f"  - All {len(entries)} editorIds preserved")
    print(f"  - All {len(seen_ids)} IDs unique")


if __name__ == "__main__":
    main()
