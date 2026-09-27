#!/usr/bin/env python3
"""Generate the English, player-facing effect list (docs/EFFECTS_EN.md).

The Korean effect docs are written for Korean readers and carry developer notes.
This one keeps only what an English-speaking player needs: every prefix, suffix,
and runeword in its in-game English text, plus the stacking rules that change
what a build actually does. It reuses the prefix categories of gen_prefix_doc.py
so both lists always group the same way.
"""

from __future__ import annotations

import argparse
import json
from itertools import groupby
from pathlib import Path

from gen_prefix_doc import CAT_ORDER, CORE_JSON, categorize
from public_doc_metadata import load_public_doc_metadata

SUFFIXES_PATH = Path("affixes/modules/keywords.affixes.suffixes.json")
RUNEWORDS_PATH = Path("affixes/modules/keywords.affixes.runewords.json")
CONTRACT_PATH = Path("affixes/runeword.contract.json")
OUTPUT_PATH = Path("docs/EFFECTS_EN.md")
REPO_URL = "https://github.com/servaltullius/calamity-reactive-loot-affixes"

# English headings for the categories gen_prefix_doc.py assigns. "기타" collects
# prefixes nobody categorized yet, so a new prefix is listed instead of dropped.
PREFIX_CATEGORY_EN = {
    "원소 타격": "Elemental Strikes",
    "원소 취약": "Elemental Weakness",
    "소환": "Summons",
    "피격 방어": "Defense When Hit",
    "두루마리 숙련": "Scroll Mastery",
    "화염 DoT": "Burning",
    "CC / 디버프": "Control and Debuffs",
    "유틸리티": "Utility",
    "덫 / 룬": "Traps and Runes",
    "원소 주입 (전환)": "Elemental Infusion (Damage Conversion)",
    "대마법사": "Archmage",
    "치명 시전 (Crit Cast)": "Crit Cast",
    "시체 소각 (죽음의 화장)": "Death Pyre (Corpse Explosions)",
    "소환 화장": "Conjured Pyre",
    "역병 / 적응": "Plague and Adaptive",
    "성장형": "Evolving",
    "Thu'um (외침) — 신규": "Thu'um (Shouts)",
    "마법학파 — 신규": "Schools of Magic",
    "기타": "Other",
}

SUFFIX_GOOD_FOR = {
    "assassin": "Critical hits",
    "brilliance": "Mages",
    "bulwark": "Shield tanks",
    "champion": "Two-handed fighters",
    "conjurer": "Conjurers",
    "eagle_eye": "Archers",
    "enchanter": "Enchanters",
    "endurance": "Melee and archers",
    "evasion": "Light armor",
    "fortitude": "Heavy armor",
    "gladiator": "Attack speed",
    "guardian": "Physical defense",
    "marksman": "Archers",
    "meditation": "Mages",
    "regeneration": "Anyone",
    "shadow": "Stealth",
    "spell_ward": "Magic resistance",
    "steed": "Carry weight",
    "swiftness": "Movement",
    "swordsman": "One-handed fighters",
    "tenacity": "Melee",
    "vitality": "Anyone",
}

BASE_EN = {"Weapon": "Weapon", "Armor": "Armor", None: "Any"}
RUNE_RARITY_EN = {4: "Common", 3: "Uncommon", 2: "Rare", 1: "Very rare"}


def cell(text: str) -> str:
    return text.replace("|", "\\|")


def bold_name(name_en: str) -> str:
    """Bold the effect name: the text before its first " (" or ": "."""
    cuts = [i for i in (name_en.find(" ("), name_en.find(": ")) if i > 0]
    if not cuts:
        return f"**{name_en}**"
    cut = min(cuts)
    return f"**{name_en[:cut]}**{name_en[cut:]}"


def render_prefixes(entries: list[dict]) -> list[str]:
    public = [e for e in entries if not e["id"].startswith("internal_")]
    categories = categorize(entries)
    order = [c for c in CAT_ORDER if c in categories] + [c for c in categories if c not in CAT_ORDER]
    missing = [c for c in order if c not in PREFIX_CATEGORY_EN]
    if missing:
        raise SystemExit(f"gen_effects_en_doc.py: add an English heading for prefix categories {missing}")

    lines = [
        f"## Prefixes ({len(public)})",
        "",
        "Prefixes are triggered effects. An item holds at most one prefix.",
        "",
    ]
    listed = 0
    for category in order:
        items = categories[category]
        lines += [f"### {PREFIX_CATEGORY_EN[category]}", ""]
        for _, entry in items:
            lines.append(f"- {bold_name(entry['nameEn'])}")
        lines.append("")
        listed += len(items)
    if listed != len(public):
        raise SystemExit(f"gen_effects_en_doc.py: listed {listed} of {len(public)} prefixes")
    return lines


def render_suffixes(entries: list[dict]) -> list[str]:
    families: dict[str, list[dict]] = {}
    for entry in entries:
        families.setdefault(entry["family"], []).append(entry)
    missing = sorted(set(families) - set(SUFFIX_GOOD_FOR))
    if missing:
        raise SystemExit(f"gen_effects_en_doc.py: add a 'Good for' label for suffix families {missing}")

    lines = [
        f"## Suffixes ({len(entries)} in {len(families)} families)",
        "",
        "Suffixes are passive bonuses. An item holds up to two suffixes, each from a different family.",
        "Tier names read *of Minor X* (tier 1), *of X* or *of the X* (tier 2), and *of Grand X* (tier 3).",
        "",
        "| Family | Tier 1 | Tier 2 | Tier 3 | Good for |",
        "| --- | --- | --- | --- | --- |",
    ]
    for family, tiers in sorted(families.items()):
        tiers.sort(key=lambda e: e["id"])
        top_name = tiers[-1]["nameEn"].split(": ", 1)[0]
        display = top_name.removeprefix("of Grand ").removeprefix("of the ").removeprefix("of ")
        effects = [cell(t["nameEn"].split(": ", 1)[-1]) for t in tiers]
        lines.append(f"| **{cell(display)}** | " + " | ".join(effects) + f" | {SUFFIX_GOOD_FOR[family]} |")
    lines.append("")
    return lines


def render_runewords(recipes: list[dict], affix_by_id: dict[str, dict], weights: list[dict]) -> list[str]:
    lines = [
        f"## Runewords ({len(recipes)})",
        "",
        "Collect the listed rune fragments, pick the recipe in the panel's Runeword tab, and transmute it onto an equipped weapon or armor.",
        "The suggested base is only a hint: the panel names a more specific one, and no base ever blocks the transmute.",
        "",
        "### Runes",
        "",
        "| Rarity | Runes |",
        "| --- | --- |",
    ]
    for weight, group in groupby(weights, key=lambda w: w["weight"]):
        lines.append(f"| {RUNE_RARITY_EN.get(weight, f'Weight {weight}')} | {', '.join(w['rune'] for w in group)} |")
    lines.append("")

    by_size: dict[int, list[dict]] = {}
    for recipe in recipes:
        by_size.setdefault(len(recipe["runes"]), []).append(recipe)
    for size in sorted(by_size):
        group = sorted(by_size[size], key=lambda r: r["name"])
        lines += [
            f"### {size} runes ({len(group)})",
            "",
            "| Runeword | Runes | Base | Effect |",
            "| --- | --- | --- | --- |",
        ]
        for recipe in group:
            runes = "-".join(recipe["runes"])
            name_en = affix_by_id[recipe["resultAffixId"]].get("nameEn", "")
            # In-game text reads "Runeword <name> (<runes>): <effect>"; keep only the effect.
            marker = f" ({runes}): "
            effect = name_en.split(marker, 1)[1] if marker in name_en else name_en
            base = BASE_EN.get(recipe.get("recommendedBase"), str(recipe.get("recommendedBase")))
            lines.append(f"| **{cell(recipe['name'])}** | {runes} | {base} | {cell(effect)} |")
        lines.append("")
    return lines


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=OUTPUT_PATH)
    args = parser.parse_args(argv)
    metadata = load_public_doc_metadata()

    prefixes = json.loads(CORE_JSON.read_text(encoding="utf-8"))
    suffixes = json.loads(SUFFIXES_PATH.read_text(encoding="utf-8"))
    contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    runewords = {e["id"]: e for e in json.loads(RUNEWORDS_PATH.read_text(encoding="utf-8"))}

    lines = [
        "# Calamity Effect List",
        "",
        f"> Version `v{metadata.version}` ({metadata.release_date}). Generated from the mod data; every line is the in-game English text.",
        f"> Korean lists: [prefixes](PREFIX_EFFECTS.md), [suffixes](SUFFIX_EFFECTS.md), [runewords](RUNEWORD_EFFECTS.md). Project: [GitHub]({REPO_URL})",
        "",
        "Each item holds **1 runeword** and up to **3 regular affixes** (1 prefix + 2 suffixes).",
        "",
        "## Reading an entry",
        "",
        "- **18% on hit**: the chance each time you hit. **on hit taken**: when you are hit. **on kill**: when you kill.",
        "- **Lucky Hit 35%**: the hit has a 35% chance to trigger the effect.",
        "- **ICD 8s**: internal cooldown; the effect cannot fire again for 8 seconds.",
        "- Listed chances are base values. Your gear and the rules below change how often an effect really fires.",
        "",
        "## Stacking rules",
        "",
        "- **A runeword and a prefix that both trigger on the same item** each fire at 80% of their listed chance; one triggered affix alone fires at 100%. Passive suffixes never lower it. (Chaos at 16% next to a Crit Cast prefix shows 12.8%.)",
        "- **The same triggered prefix on different items** (spells, adaptive spells, and traps) combines up to 3 copies: `1 - (1-p1)(1-p2)(1-p3)`, so 40% copies fire 40% → 64% → 78.4% of the time with 1, 2, or 3 equipped. It fires more often; it does not hit harder or fire twice.",
        "- **Crit Cast** spells: melee critical and power attacks can cast up to 2 different spells, normal melee hits and bow or crossbow hits up to 1, and all share a 0.15s cooldown.",
        "- **Suffixes of the same family** add their tiers and cap at tier 3: tier 1 + tier 1 = tier 2, tier 1 + tier 2 = tier 3. Only the resulting tier applies (Guardian 1 + 2 gives +80 Armor, not +75).",
        "",
    ]
    lines += render_prefixes(prefixes)
    lines += render_suffixes(suffixes)
    lines += render_runewords(contract["runewordCatalog"], runewords, contract["runewordRuneWeights"])

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    print(f"[OK] Wrote {args.output} (English effect list)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
