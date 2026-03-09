#!/usr/bin/env python3
"""Update affix documentation markdown files to match new Korean descriptions.

1. PREFIX_EFFECTS.md — Replace English jargon (Lucky Hit, ICD) with Korean
2. RUNEWORD_EFFECTS.md — Replace ICD with Korean
3. AFFIX_CATALOG.md — Regenerate from current affixes.json
"""

import json
import re
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
AFFIXES_PATH = REPO / "affixes" / "affixes.json"
DOCS = REPO / "docs"


def fix_prefix_effects():
    """Replace English jargon in PREFIX_EFFECTS.md."""
    path = DOCS / "PREFIX_EFFECTS.md"
    text = path.read_text(encoding="utf-8")

    # "Lucky Hit 38.0%" → "38% 확률"
    text = re.sub(r'Lucky Hit (\d+(?:\.\d+)?)%', lambda m: f"{_fmt(m.group(1))}% 확률", text)

    # "ICD X초" → "쿨타임 X초"
    text = re.sub(r'ICD (\d+(?:\.\d+)?)초', lambda m: f"쿨타임 {_fmt(m.group(1))}초", text)

    # "파워어택" → "강공"
    text = text.replace("파워어택", "강공")

    # "(Crit Cast)" → "(치명 시전)"
    text = text.replace("(Crit Cast)", "(치명 시전)")

    path.write_text(text, encoding="utf-8")
    print(f"  PREFIX_EFFECTS.md 업데이트 완료")


def fix_runeword_effects():
    """Replace English jargon in RUNEWORD_EFFECTS.md."""
    path = DOCS / "RUNEWORD_EFFECTS.md"
    text = path.read_text(encoding="utf-8")

    # "ICD X초" → "쿨타임 X초"
    text = re.sub(r'ICD (\d+(?:\.\d+)?)초', lambda m: f"쿨타임 {_fmt(m.group(1))}초", text)

    # "파워어택" → "강공"
    text = text.replace("파워어택", "강공")

    path.write_text(text, encoding="utf-8")
    print(f"  RUNEWORD_EFFECTS.md 업데이트 완료")


def regenerate_affix_catalog():
    """Regenerate AFFIX_CATALOG.md from current affixes.json."""
    with open(AFFIXES_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    affixes = data["keywords"]["affixes"]

    # Group by slot
    prefixes = [a for a in affixes if a.get("slot") == "prefix"
                and not a.get("id", "").startswith("internal_")
                and not (a.get("id", "").startswith("runeword_") and a.get("id", "").endswith("_final"))]
    suffixes = [a for a in affixes if a.get("slot") == "suffix"]
    runewords = [a for a in affixes if a.get("id", "").startswith("runeword_") and a.get("id", "").endswith("_final")]

    lines = []
    lines.append("# 어픽스 카탈로그 — 공개 한국어 설명")
    lines.append("")
    lines.append(f"> 기준: `affixes/affixes.json` (자동 생성)")
    lines.append(f"> 업데이트: 2026-03-04")
    lines.append(f"> 프리픽스 {len(prefixes)}개 + 서픽스 {len(suffixes)}개 + 룬워드 {len(runewords)}개")
    lines.append("> INTERNAL 항목은 공개 카탈로그에서 숨김")
    lines.append("")

    # --- Prefixes ---
    lines.append("## 프리픽스 (Prefix)")
    lines.append("")

    # Sub-group prefixes by action type
    prefix_groups = {}
    for a in prefixes:
        rt = a.get("runtime", {})
        act = rt.get("action", {})
        act_type = act.get("type", "Passive")
        if act_type not in prefix_groups:
            prefix_groups[act_type] = []
        prefix_groups[act_type].append(a)

    # Order: CastSpell, CastOnCrit, Archmage, ConvertDamage, MindOverMatter, CorpseExplosion, SummonCorpseExplosion, SpawnTrap, CastSpellAdaptiveElement, DebugNotify
    group_order = ["CastSpell", "CastOnCrit", "Archmage", "ConvertDamage", "MindOverMatter",
                   "CorpseExplosion", "SummonCorpseExplosion", "SpawnTrap", "CastSpellAdaptiveElement", "DebugNotify"]
    group_labels = {
        "CastSpell": "스펠 시전",
        "CastOnCrit": "치명 시전 (CastOnCrit)",
        "Archmage": "대마법사 (Archmage)",
        "ConvertDamage": "속성 전환 (ConvertDamage)",
        "MindOverMatter": "마법사의 갑옷 (MindOverMatter)",
        "CorpseExplosion": "시체 폭발 (CorpseExplosion)",
        "SummonCorpseExplosion": "소환 화장 (SummonCorpseExplosion)",
        "SpawnTrap": "덫/룬 (SpawnTrap)",
        "CastSpellAdaptiveElement": "적응형 원소 (AdaptiveElement)",
        "DebugNotify": "패시브 (Passive)",
    }

    for group_key in group_order:
        if group_key not in prefix_groups:
            continue
        group = prefix_groups[group_key]
        label = group_labels.get(group_key, group_key)
        lines.append(f"### {label}")
        lines.append("")
        for a in group:
            _write_affix_entry(lines, a)
        lines.append("")

    # Remaining groups
    for group_key, group in prefix_groups.items():
        if group_key in group_order:
            continue
        lines.append(f"### {group_key}")
        lines.append("")
        for a in group:
            _write_affix_entry(lines, a)
        lines.append("")

    # --- Suffixes ---
    lines.append("## 서픽스 (Suffix)")
    lines.append("")

    # Group suffixes by family
    suffix_families = {}
    for a in suffixes:
        family = a.get("suffixFamily", "기타")
        if family not in suffix_families:
            suffix_families[family] = []
        suffix_families[family].append(a)

    for family, group in sorted(suffix_families.items()):
        lines.append(f"### {family}")
        lines.append("")
        for a in group:
            _write_affix_entry(lines, a)
        lines.append("")

    # --- Runewords ---
    lines.append("## 룬워드 (Runeword)")
    lines.append("")
    lines.append("> 94개 전부 JSON 개별 정의. 자세한 효과는 `docs/RUNEWORD_EFFECTS.md` 참조.")
    lines.append("")
    for a in runewords:
        _write_affix_entry(lines, a)
    lines.append("")

    path = DOCS / "AFFIX_CATALOG.md"
    path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  AFFIX_CATALOG.md 재생성 완료 ({len(prefixes)}P + {len(suffixes)}S + {len(runewords)}R)")


def _write_affix_entry(lines, a):
    aid = a.get("id", "")
    name_ko = a.get("nameKo", a.get("name", aid))
    slot = a.get("slot", "")
    item_type = a.get("itemType", "")
    lines.append(f"- **{name_ko}** (`{aid}`) [{item_type or slot}]")


def _fmt(val_str):
    """Format numeric string: remove trailing .0"""
    f = float(val_str)
    if f == int(f):
        return str(int(f))
    return val_str


def main():
    print("=== 문서 업데이트 ===")
    fix_prefix_effects()
    fix_runeword_effects()
    regenerate_affix_catalog()
    print("\n완료!")


if __name__ == "__main__":
    main()
