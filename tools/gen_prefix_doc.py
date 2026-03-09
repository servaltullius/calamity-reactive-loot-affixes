#!/usr/bin/env python3
"""Generate public prefix effects documentation markdown."""

import json, subprocess, sys
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


TRIGGER_KO = {
    "Hit": "적중", "IncomingHit": "피격", "Kill": "처치",
    "DotApply": "DoT 적용", "LowHealth": "체력 위급",
}

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
    trigger = rt.get("trigger", "")
    proc = rt.get("procChancePercent", 0)
    lucky = rt.get("luckyHitChancePercent", 0)
    icd = rt.get("icdSeconds", 0)
    pt_icd = rt.get("perTargetICDSeconds", 0)
    req_kill = rt.get("requireRecentlyKillSeconds", 0)
    atype = action.get("type", "")
    apply_to = action.get("applyTo", "")
    spell = action.get("spellEditorId", "")
    kid = e.get("kid", {})
    kid_type = kid.get("type", "")
    chance = kid.get("chance", 0)
    lw = rt.get("lootWeight", 0)

    trig_ko = TRIGGER_KO.get(trigger, trigger)

    # Extract display label
    name_ko = e.get("nameKo", e.get("name", ""))
    label = name_ko.split(" (")[0] if " (" in name_ko else name_ko

    parts = []
    if lucky > 0:
        parts.append(f"Lucky Hit {lucky}%")
    elif proc > 0:
        parts.append(f"{trig_ko} {proc}%")
    else:
        parts.append(trig_ko)
    if icd > 0:
        parts.append(f"ICD {icd}초")
    if pt_icd > 0:
        parts.append(f"표적별 ICD {pt_icd}초")
    if req_kill > 0:
        parts.append(f"최근 처치 {req_kill}초")
    trigger_str = " / ".join(parts)

    target_str = "자신" if apply_to == "Self" else "대상" if apply_to == "Target" else apply_to

    lines = []
    header = f"- **{label}** (`{e['id']}`)"
    if kid_type:
        header += f" [{kid_type}]"
    lines.append(header)
    lines.append(f"  - {trigger_str} → {target_str} | {atype}")

    if spell:
        lines.append(f"  - 스펠: `{spell}`")

    # Evolution
    evo = action.get("evolution")
    if evo:
        lines.append(f"  - 성장형: 단계 {evo['thresholds']}, 배수 {evo['multipliers']}")

    # ModeCycle
    mc = action.get("modeCycle")
    if mc:
        lines.append(f"  - 모드 순환: {mc['labels']}")

    # MagnitudeScaling
    ms = action.get("magnitudeScaling")
    if ms:
        s = f"  - 스케일: {ms['source']} ×{ms['mult']}"
        if ms.get("add", 0):
            s += f" +{ms['add']}"
        if ms.get("max", 0):
            s += f" (max {ms['max']})"
        lines.append(s)

    # Special action fields
    if atype == "ConvertDamage":
        lines.append(f"  - 전환: {action['element']} {action['percent']}%")
    elif atype == "CorpseExplosion":
        lines.append(
            f"  - 반경 {action['radius']}, {action['flatDamage']}+{action['pctOfCorpseMaxHealth']}% 시체HP, "
            f"연쇄 {action['maxChainDepth']}회"
        )
    elif atype == "SummonCorpseExplosion":
        lines.append(
            f"  - 반경 {action['radius']}, {action['flatDamage']}+{action['pctOfCorpseMaxHealth']}% 소환물HP, "
            f"연쇄 {action['maxChainDepth']}회"
        )
    elif atype == "SpawnTrap":
        lines.append(f"  - 덫 설치: {action.get('delaySeconds', 0)}초 후, {action.get('lifetimeSeconds', 0)}초 유지")
    elif atype == "MindOverMatter":
        lines.append(f"  - 물리 피해 {action.get('percent', 0)}% → 매지카 전환")
    elif atype == "ScrollEchoPreservation":
        lines.append(f"  - 스크롤 비소모 +{action.get('chancePercent', 0)}%")
    elif atype == "Archmage":
        lines.append(f"  - 대마법사: 티어 {action.get('tier', '?')}")

    if lw == 0:
        lines.append("  - 드롭 가중치: 0 (하위 전용)")
    elif lw != 1.0:
        lines.append(f"  - 드롭 가중치: {lw}")

    return "\n".join(lines)


def get_old_ids():
    """Get old IDs from git HEAD."""
    r = subprocess.run(
        ["git", "show", "HEAD:affixes/modules/keywords.affixes.core.json"],
        capture_output=True, text=True,
    )
    if r.returncode != 0:
        return None
    return [e["id"] for e in json.loads(r.stdout)]


def main():
    with open(CORE_JSON) as f:
        entries = json.load(f)

    public_entries = [entry for entry in entries if not entry["id"].startswith("internal_")]
    cats = categorize(entries)

    out = []
    out.append("# 프리픽스 효과 정리 (공개용)")
    out.append("")
    out.append("> 업데이트: 2026-03-04")
    out.append("> 기준 코드:")
    out.append("> - 효과 정의: `affixes/modules/keywords.affixes.core.json`")
    out.append("> - 변환 스크립트: `tools/transform_prefixes.py`")
    out.append("")
    out.append(f"- 총 공개 프리픽스: **{len(public_entries)}개**")
    out.append("- INTERNAL 항목은 공개 문서에서 숨김")
    out.append("- 스카이림 로어 기반 전면 리네이밍 (v1.2.21)")
    out.append("- 9개 freed slot → Thu'um 5종 + 마법학파 4종 신규 효과")
    out.append("- EditorID 100% 보존 (FormID 무변동)")
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

    # ID mapping table
    out.append("---")
    out.append("")
    out.append("## 전체 ID 매핑 (이전 → 현재)")
    out.append("")

    freed_indices = {36, 38, 40, 42, 44, 46, 62, 63, 66}
    old_ids = get_old_ids()
    if old_ids:
        out.append("| # | 이전 ID | 현재 ID | 유형 |")
        out.append("|---|---------|---------|------|")
        for i in range(min(len(old_ids), len(entries))):
            old_id = old_ids[i]
            new_id = entries[i]["id"]
            if old_id.startswith("internal_") or new_id.startswith("internal_"):
                continue
            if old_id == new_id:
                typ = "유지"
            elif i in freed_indices:
                typ = "**신규 효과**"
            else:
                typ = "리네이밍"
            out.append(f"| {i} | `{old_id}` | `{new_id}` | {typ} |")
    else:
        out.append("(git HEAD에서 이전 ID를 가져올 수 없음)")

    out.append("")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"Wrote {OUTPUT} ({len(out)} lines)")


if __name__ == "__main__":
    main()
