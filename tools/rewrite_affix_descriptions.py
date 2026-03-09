#!/usr/bin/env python3
"""Rewrite affix nameKo/name to user-friendly 서술형 Korean descriptions.

Reads runtime config values and rewrites descriptions using plain Korean,
replacing jargon (Lucky Hit, ICD) with accessible language.

Usage:
    python3 tools/rewrite_affix_descriptions.py                # preview only
    python3 tools/rewrite_affix_descriptions.py --apply        # write to JSON
"""

import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
AFFIXES_PATH = REPO_ROOT / "affixes" / "affixes.json"


def format_icd(seconds: float) -> str:
    if seconds <= 0:
        return ""
    if seconds == int(seconds):
        return f"{int(seconds)}초마다 발동."
    return f"{seconds}초마다 발동."


def format_per_target_icd(seconds: float) -> str:
    if seconds <= 0:
        return ""
    if seconds == int(seconds):
        return f"같은 대상 {int(seconds)}초."
    return f"같은 대상 {seconds}초."


def format_pct(val: float) -> str:
    if val == int(val):
        return str(int(val))
    return f"{val}"


def extract_affix_name(nameKo: str) -> str:
    """Extract the display name (before parenthetical conditions or colon)."""
    # Runeword: "룬워드 XXX (Rune-Rune): ..."
    m = re.match(r'^(룬워드\s+\S+)\s*\(([^)]+)\):\s*(.+)$', nameKo)
    if m:
        return m.group(1), m.group(2), m.group(3)

    # Standard prefix: "이름 (conditions): effect"
    m = re.match(r'^(.+?)\s*\(([^)]+)\):\s*(.+)$', nameKo)
    if m:
        return m.group(1), m.group(2), m.group(3)

    # Passive/simple: "이름: effect"
    m = re.match(r'^(.+?):\s*(.+)$', nameKo)
    if m:
        return m.group(1), "", m.group(2)

    return nameKo, "", ""


def build_trigger_text(rt: dict) -> str:
    """Build trigger condition text from runtime config."""
    trigger = rt.get("trigger", "Hit")
    proc_pct = rt.get("procChancePercent", 100.0)
    lucky_hit = rt.get("luckyHitChancePercent", 0)
    require_recently_hit = rt.get("requireRecentlyHitSeconds", 0)
    require_recently_kill = rt.get("requireRecentlyKillSeconds", 0)
    require_not_hit = rt.get("requireNotHitRecentlySeconds", 0)
    low_health_pct = rt.get("lowHealthThresholdPercent", 0)

    parts = []

    # Low health trigger
    if trigger == "LowHealth" and low_health_pct > 0:
        parts.append(f"체력 {format_pct(low_health_pct)}% 이하일 때")
        return " ".join(parts)

    # Recently conditions
    if require_recently_kill > 0 and trigger == "Hit":
        kill_sec = int(require_recently_kill) if require_recently_kill == int(require_recently_kill) else require_recently_kill
        if lucky_hit > 0:
            parts.append(f"최근 {kill_sec}초 내 처치 후 {format_pct(lucky_hit)}% 확률로")
        elif proc_pct < 100:
            parts.append(f"최근 {kill_sec}초 내 처치 후 {format_pct(proc_pct)}% 확률로")
        else:
            parts.append(f"최근 {kill_sec}초 내 처치 후")
        return " ".join(parts)

    if require_recently_hit > 0 and trigger == "Hit":
        hit_sec = int(require_recently_hit) if require_recently_hit == int(require_recently_hit) else require_recently_hit
        if lucky_hit > 0:
            parts.append(f"최근 {hit_sec}초 내 적중 유지 시 {format_pct(lucky_hit)}% 확률로")
        elif proc_pct < 100:
            parts.append(f"최근 {hit_sec}초 내 적중 유지 시 {format_pct(proc_pct)}% 확률로")
        else:
            parts.append(f"최근 {hit_sec}초 내 적중 유지 시")
        return " ".join(parts)

    # Lucky hit
    if lucky_hit > 0:
        parts.append(f"{format_pct(lucky_hit)}% 확률로")
        return " ".join(parts)

    # Standard triggers
    if trigger == "Hit":
        if proc_pct >= 100:
            parts.append("매 적중 시")
        else:
            parts.append(f"적중 시 {format_pct(proc_pct)}% 확률로")
    elif trigger == "IncomingHit":
        if proc_pct >= 100:
            parts.append("피격 시")
        else:
            parts.append(f"피격 시 {format_pct(proc_pct)}% 확률로")
    elif trigger == "Kill":
        if proc_pct >= 100:
            parts.append("처치 시")
        else:
            parts.append(f"처치 시 {format_pct(proc_pct)}% 확률로")
    elif trigger == "DotApply":
        if proc_pct >= 100:
            parts.append("지속 피해 적용 시")
        else:
            parts.append(f"지속 피해 적용 시 {format_pct(proc_pct)}% 확률로")
    elif trigger == "LowHealth":
        parts.append(f"체력 {format_pct(low_health_pct)}% 이하일 때")

    return " ".join(parts)


def clean_effect_text(effect: str) -> str:
    """Clean up effect text: translate remaining English, simplify."""
    # Common English → Korean spell name translations
    replacements = {
        "Firebolt": "파이어볼트",
        "Ice Spike": "아이스 스파이크",
        "Lightning Bolt": "라이트닝 볼트",
        "Thunderbolt": "썬더볼트",
        "Icy Spear": "아이시 스피어",
        "Chain Lightning": "체인 라이트닝",
        "Ice Storm": "아이스 스톰",
        "Turn Undead": "언데드 퇴치",
        "Fear": "공포",
        "Calm": "진정",
        "Frenzy": "광란",
        "Ethereal": "에테리얼",
        "HealRate": "회복력",
        "Magicka Regen": "매지카 재생",
        "Weapon Speed": "무기 공속",
        "Magic Resist": "마법 저항",
        "Target Frost Resist": "적 냉기저항",
        "Target Fire Resist": "적 화염저항",
    }
    for eng, ko in replacements.items():
        effect = effect.replace(eng, ko)

    # "적중 피해의" → "적중의"
    effect = effect.replace("적중 피해의", "적중의")

    # "파워어택" → "강공"
    effect = effect.replace("파워어택", "강공")

    return effect.strip()


def rewrite_runeword(affix_id: str, nameKo: str, rt: dict) -> str:
    """Rewrite runeword descriptions."""
    name, runes, effect = extract_affix_name(nameKo)

    if not runes or not effect:
        return nameKo  # Can't parse, skip

    # Parse runeword condition: "적중 20% / ICD 8초 - 효과"
    # or "피격시 / ICD 90초 - 효과"
    # or "적중 100% / ICD 0.5초 / 대상별 6초 - 효과"
    # Split on " - " to separate condition from effect
    cond_part = ""
    effect_part = effect
    if " - " in effect:
        cond_part, effect_part = effect.split(" - ", 1)

    icd = rt.get("icdSeconds", 0)
    pt_icd = rt.get("perTargetICDSeconds", 0)
    proc_pct = rt.get("procChancePercent", 100.0)
    trigger = rt.get("trigger", "Hit")
    low_health_pct = rt.get("lowHealthThresholdPercent", 0)
    passive_spell = rt.get("passiveSpellEditorId", "")

    # Build trigger text
    trigger_text = ""
    if trigger == "LowHealth":
        trigger_text = f"체력 {format_pct(low_health_pct)}% 이하일 때"
    elif trigger == "IncomingHit":
        if proc_pct >= 100:
            trigger_text = "피격 시"
        else:
            trigger_text = f"피격 시 {format_pct(proc_pct)}% 확률로"
    elif trigger == "Kill":
        if proc_pct >= 100:
            trigger_text = "처치 시"
        else:
            trigger_text = f"처치 시 {format_pct(proc_pct)}% 확률로"
    elif trigger == "Hit":
        if proc_pct >= 100:
            trigger_text = "매 적중 시"
        else:
            trigger_text = f"적중 시 {format_pct(proc_pct)}% 확률로"

    effect_part = clean_effect_text(effect_part)

    # Extract passive bonus from cond_part if present (e.g., "+ 이동속도 +10%")
    # These are in the effect_part after the main proc effect
    # Check if effect has "+" separated passive bonuses
    passive_bonus = ""
    # Look for patterns like "+ 공격속도 +10%" or "+ 체력+50 마나+30"
    # These are already in the cond_part or effect_part
    # For runewords, passive bonuses are after the proc effect, separated by "+"
    # Example: "질풍 사격(활속도 +30%, 6초)" - no passive
    # Example: "차원 위상 (이속+45%, 8초) + 이동속도 +10%" - has passive
    if " + " in effect_part:
        # Check if the last part looks like a passive (stat bonus with no parenthetical duration)
        parts = effect_part.rsplit(" + ", 1)
        last_part = parts[1].strip()
        # If the last part doesn't have parentheses (no duration), it's likely a passive
        if "(" not in last_part and ("+" in last_part or "-" in last_part):
            effect_part = parts[0].strip()
            passive_bonus = last_part

    # Build result
    result_parts = [f"{name} [{runes}]:"]

    if trigger_text:
        result_parts.append(f"{trigger_text} {effect_part}.")
    else:
        result_parts.append(f"{effect_part}.")

    # Add passive bonus
    if passive_bonus:
        result_parts.append(f"{passive_bonus}.")

    # Add cooldowns
    cooldowns = []
    if icd > 0:
        cooldowns.append(format_icd(icd))
    if pt_icd > 0:
        cooldowns.append(format_per_target_icd(pt_icd))
    if cooldowns:
        result_parts.append(" ".join(cooldowns))

    return " ".join(result_parts)


def rewrite_prefix(affix_id: str, nameKo: str, rt: dict) -> str:
    """Rewrite prefix descriptions."""
    act = rt.get("action", {})
    act_type = act.get("type", "")
    icd = rt.get("icdSeconds", 0)
    pt_icd = rt.get("perTargetICDSeconds", 0)

    name, conds, effect = extract_affix_name(nameKo)

    # Skip INTERNAL affixes
    if nameKo.startswith("INTERNAL"):
        return nameKo

    # Passives (no conditions): keep as is
    if not conds and act_type in ("DebugNotify", ""):
        return nameKo

    # ConvertDamage: already clean
    if act_type == "ConvertDamage":
        return nameKo

    # MindOverMatter: already clean
    if act_type == "MindOverMatter":
        return nameKo

    effect = clean_effect_text(effect)
    trigger_text = build_trigger_text(rt)

    # CastOnCrit: special handling
    if act_type == "CastOnCrit":
        # Parse CastOnCrit name pattern: "치명 시전: SPELLNAME"
        m = re.match(r'^치명 시전:\s*(.+)$', name)
        spell_name = m.group(1) if m else name

        # Extract magnitude info from effect
        # "Firebolt + 적중 피해의 30%" → "적중의 30%"
        effect = clean_effect_text(effect)
        # Remove redundant spell name prefix from effect (e.g., "파이어볼트 + 적중의 30%" → "적중의 30%")
        effect = re.sub(r'^[A-Za-z\s]+\+\s*', '', effect)  # English name
        effect = re.sub(r'^' + re.escape(spell_name) + r'\s*\+\s*', '', effect)  # Korean name

        # CastOnCrit always has a global 0.15s ICD in C++ code
        coc_icd = icd if icd > 0 else 0.15
        result = f"치명 시전: {spell_name}: 치명타/강공 시 {spell_name} 시전 ({effect}). {format_icd(coc_icd)}"
        return result

    # Archmage: special
    if act_type == "Archmage":
        # JSON key: "damagePctOfMaxMagicka" (not "archmage"-prefixed)
        dmg_pct = act.get("damagePctOfMaxMagicka", 0)
        cost_pct = act.get("costPctOfMaxMagicka", 0)
        return f"{name}: 주문 적중 시 최대 매지카의 {format_pct(dmg_pct)}%를 번개 피해로 가하고, {format_pct(cost_pct)}%를 추가 소모한다."

    # CorpseExplosion: special
    if act_type in ("CorpseExplosion", "SummonCorpseExplosion"):
        # Keep the effect mostly as is but clean trigger
        trigger = rt.get("trigger", "Kill")
        proc = rt.get("procChancePercent", 100)

        if act_type == "SummonCorpseExplosion":
            trigger_str = "소환물 사망 시"
        elif trigger == "Kill":
            if proc >= 100:
                trigger_str = "처치 시"
            else:
                trigger_str = f"처치 시 {format_pct(proc)}% 확률로"
        else:
            trigger_str = trigger_text

        # Clean up chain description
        effect = effect.replace("연쇄(", "연쇄 폭발(위력 ")
        effect = re.sub(r'(\d+)초 (\d+)회 제한', r'초당 \2회 제한', effect)

        result = f"{name}: {trigger_str} {effect}."
        if icd > 0:
            result += f" {format_icd(icd)}"
        return result

    # SpawnTrap: keep complex description, just clean trigger
    if act_type == "SpawnTrap":
        require_crit = act.get("requireCritOrPowerAttack", False)
        trap_pt_icd = rt.get("perTargetICDSeconds", 0)

        # Add crit/power attack condition if required
        if require_crit:
            trigger_text = trigger_text.replace("매 적중 시", "치명타/강공 시")
            trigger_text = trigger_text.replace("적중 시", "치명타/강공 시")

        result = f"{name}: {trigger_text} {effect}."
        cooldowns = []
        if icd > 0:
            cooldowns.append(format_icd(icd))
        if trap_pt_icd > 0:
            cooldowns.append(format_per_target_icd(trap_pt_icd))
        if cooldowns:
            result += " " + " ".join(cooldowns)
        return result

    # CastSpellAdaptiveElement
    if act_type == "CastSpellAdaptiveElement":
        result = f"{name}: {trigger_text} {effect}."
        cooldowns = []
        if icd > 0:
            cooldowns.append(format_icd(icd))
        if pt_icd > 0:
            cooldowns.append(format_per_target_icd(pt_icd))
        if cooldowns:
            result += " " + " ".join(cooldowns)
        return result

    # Standard CastSpell
    result = f"{name}: {trigger_text} {effect}."
    cooldowns = []
    if icd > 0:
        cooldowns.append(format_icd(icd))
    if pt_icd > 0:
        cooldowns.append(format_per_target_icd(pt_icd))
    if cooldowns:
        result += " " + " ".join(cooldowns)
    return result


def rewrite_suffix(affix_id: str, nameKo: str, rt: dict) -> str:
    """Suffix descriptions are already clean, minimal changes."""
    return nameKo


def rewrite_description(affix: dict) -> str:
    """Main dispatcher for rewriting descriptions."""
    affix_id = affix.get("id", "")
    nameKo = affix.get("nameKo", "")
    rt = affix.get("runtime", {})
    slot = affix.get("slot", "")

    # Skip INTERNAL
    if nameKo.startswith("INTERNAL"):
        return nameKo

    if slot == "suffix":
        return rewrite_suffix(affix_id, nameKo, rt)

    # Runewords
    if affix_id.startswith("runeword_") and affix_id.endswith("_final"):
        return rewrite_runeword(affix_id, nameKo, rt)

    # Prefixes (including non-runeword)
    return rewrite_prefix(affix_id, nameKo, rt)


def main():
    apply = "--apply" in sys.argv

    with open(AFFIXES_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    affixes = data["keywords"]["affixes"]
    changes = []

    for affix in affixes:
        old_ko = affix.get("nameKo", "")
        new_ko = rewrite_description(affix)

        if new_ko != old_ko:
            changes.append({
                "id": affix["id"],
                "old": old_ko,
                "new": new_ko,
            })

    # Preview
    print(f"=== 변경 대상: {len(changes)}건 ===\n")
    for c in changes:
        print(f"[{c['id']}]")
        print(f"  Before: {c['old']}")
        print(f"  After:  {c['new']}")
        print()

    if apply:
        # Apply changes
        change_map = {c["id"]: c["new"] for c in changes}
        for affix in affixes:
            if affix["id"] in change_map:
                new_val = change_map[affix["id"]]
                affix["nameKo"] = new_val
                affix["name"] = new_val

        with open(AFFIXES_PATH, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")

        print(f"\n=== {len(changes)}건 적용 완료 ===")
    else:
        print(f"\n=== 미리보기 모드입니다. --apply 옵션으로 적용하세요. ===")


if __name__ == "__main__":
    main()
