#!/usr/bin/env python3
"""Generate public prefix effects documentation markdown."""

import json
from datetime import date
from pathlib import Path

CORE_JSON = Path("affixes/modules/keywords.affixes.core.json")
OUTPUT = Path("docs/PREFIX_EFFECTS.md")


def categorize(entries):
    """Assign category to each entry."""
    cats = {}
    cat_map = {
        "storm_call": "원소 타격",
        "flame_strike": "원소 타격",
        "frost_strike": "원소 타격",
        "spark_strike": "원소 타격",
        "flame_weakness": "원소 취약",
        "frost_weakness": "원소 취약",
        "shock_weakness": "원소 취약",
        "wolf_spirit": "소환",
        "flame_sprite": "소환",
        "flame_atronach": "소환",
        "frost_atronach": "소환",
        "storm_atronach": "소환",
        "dremora_pact": "소환",
        "soul_snare": "소환",
        "shadow_veil": "피격 방어",
        "stone_ward": "피격 방어",
        "arcane_ward": "피격 방어",
        "healing_surge": "피격 방어",
        "mage_armor_t1": "피격 방어",
        "mage_armor_t2": "피격 방어",
        "scroll_mastery_t1": "두루마리 숙련",
        "scroll_mastery_t2": "두루마리 숙련",
        "scroll_mastery_t3": "두루마리 숙련",
        "scroll_mastery_t4": "두루마리 숙련",
        "ember_brand": "화염 DoT",
        "ember_pyre": "화염 DoT",
        "ice_shackle": "CC / 디버프",
        "mana_burn": "CC / 디버프",
        "life_drain": "CC / 디버프",
        "soul_siphon": "유틸리티",
        "shadow_stride": "유틸리티",
        "silent_step": "유틸리티",
        "battle_frenzy": "유틸리티",
        "bear_trap": "덫 / 룬",
        "rune_trap": "덫 / 룬",
        "chaos_rune": "덫 / 룬",
        "fire_infusion_50": "원소 주입 (전환)",
        "fire_infusion_100": "원소 주입 (전환)",
        "frost_infusion_50": "원소 주입 (전환)",
        "frost_infusion_100": "원소 주입 (전환)",
        "shock_infusion_50": "원소 주입 (전환)",
        "shock_infusion_100": "원소 주입 (전환)",
        "archmage_t1": "대마법사",
        "archmage_t2": "대마법사",
        "archmage_t3": "대마법사",
        "archmage_t4": "대마법사",
        "crit_cast_firebolt": "치명 시전 (Crit Cast)",
        "crit_cast_ice_spike": "치명 시전 (Crit Cast)",
        "crit_cast_lightning_bolt": "치명 시전 (Crit Cast)",
        "crit_cast_thunderbolt": "치명 시전 (Crit Cast)",
        "crit_cast_icy_spear": "치명 시전 (Crit Cast)",
        "crit_cast_chain_lightning": "치명 시전 (Crit Cast)",
        "crit_cast_ice_storm": "치명 시전 (Crit Cast)",
        "death_pyre_t1": "시체 소각 (죽음의 화장)",
        "death_pyre_t2": "시체 소각 (죽음의 화장)",
        "death_pyre_t3": "시체 소각 (죽음의 화장)",
        "conjured_pyre_t1": "소환 화장",
        "conjured_pyre_t2": "소환 화장",
        "elemental_bane": "역병 / 적응",
        "plague_spore": "역병 / 적응",
        "tar_blight": "역병 / 적응",
        "siphon_spore": "역병 / 적응",
        "thunder_mastery": "성장형",
        "elemental_attunement": "성장형",
        "voice_of_power": "Thu'um (외침) — 신규",
        "death_mark": "Thu'um (외침) — 신규",
        "ice_form": "Thu'um (외침) — 신규",
        "disarming_shout": "Thu'um (외침) — 신규",
        "whirlwind_sprint": "Thu'um (외침) — 신규",
        "spell_breach": "마법학파 — 신규",
        "stamina_drain": "마법학파 — 신규",
        "nourishing_flame": "마법학파 — 신규",
        "mana_knot": "마법학파 — 신규",
    }

    for i, e in enumerate(entries):
        eid = e["id"]
        if eid.startswith("internal_"):
            continue
        cat = cat_map.get(eid, "기타")
        cats.setdefault(cat, []).append((i, e))
    return cats

CAT_ORDER = [
    "원소 타격", "원소 취약", "소환", "피격 방어", "두루마리 숙련",
    "화염 DoT", "CC / 디버프", "유틸리티", "덫 / 룬",
    "원소 주입 (전환)", "대마법사", "치명 시전 (Crit Cast)",
    "시체 소각 (죽음의 화장)", "소환 화장",
    "역병 / 적응", "성장형",
    "Thu'um (외침) — 신규", "마법학파 — 신규",
]


def format_entry(idx, e):
    rt = e.get("runtime", {})
    action = rt.get("action", {})
    spell = action.get("spellEditorId", "")
    kid = e.get("kid", {})
    kid_type = kid.get("type", "")
    name_ko = e.get("nameKo", e.get("name", ""))
    name_en = e.get("nameEn", "")

    lines = []
    header = f"- **`{e['id']}`**"
    if kid_type:
        header += f" [{kid_type}]"
    lines.append(header)
    if name_ko:
        lines.append(f"  - 한글 표시: {name_ko}")
    if name_en and name_en != name_ko:
        lines.append(f"  - 영문 표시: {name_en}")
    if spell:
        lines.append(f"  - 대표 스펠: `{spell}`")

    return "\n".join(lines)


def main():
    with open(CORE_JSON) as f:
        entries = json.load(f)

    public_entries = [entry for entry in entries if not entry["id"].startswith("internal_")]
    cats = categorize(entries)

    out = []
    out.append("# 프리픽스 효과 정리 (공개용)")
    out.append("")
    out.append(f"> 업데이트: {date.today().isoformat()}")
    out.append("> 기준 버전: `v1.2.21`")
    out.append("> 기준 코드:")
    out.append("> - 효과 정의: `affixes/modules/keywords.affixes.core.json`")
    out.append("> - 변환 스크립트: `tools/transform_prefixes.py`")
    out.append("")
    out.append(f"- 총 공개 프리픽스: **{len(public_entries)}개**")
    out.append("- INTERNAL 항목은 공개 문서에서 숨김")
    out.append("- 스카이림 로어 기반 전면 리네이밍 (v1.2.21)")
    out.append("- 9개 freed slot → Thu'um 5종 + 마법학파 4종 신규 효과")
    out.append("- 각 효과 설명은 인게임 표시 문자열(`nameKo`/`nameEn`)을 그대로 사용")
    out.append("")

    # Category counts
    out.append("### 카테고리별 수량")
    out.append("")
    out.append("| 카테고리 | 수 |")
    out.append("|----------|---|")
    for cat in CAT_ORDER:
        if cat in cats:
            out.append(f"| {cat} | {len(cats[cat])} |")
    out.append(f"| **합계** | **{len(public_entries)}** |")
    out.append("")

    # Per-category entries
    for cat in CAT_ORDER:
        if cat not in cats:
            continue
        out.append(f"## {cat}")
        if "신규" in cat:
            out.append("")
            out.append("> v1.2.21 신규 추가 — freed slot 활용, 기존 스펠만 참조")
        out.append("")
        for idx, e in cats[cat]:
            out.append(format_entry(idx, e))
            out.append("")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"Wrote {OUTPUT} ({len(out)} lines)")


if __name__ == "__main__":
    main()
