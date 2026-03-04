#!/usr/bin/env python3
"""서픽스 효과 목록 마크다운 문서 생성."""
from __future__ import annotations

import json
import sys
from pathlib import Path

SUFFIXES_PATH = Path("affixes/modules/keywords.affixes.suffixes.json")
OUTPUT_PATH = Path("docs/SUFFIX_EFFECTS.md")


def main() -> int:
    if not SUFFIXES_PATH.exists():
        print(f"[ERROR] {SUFFIXES_PATH} not found", file=sys.stderr)
        return 1

    with SUFFIXES_PATH.open("r", encoding="utf-8") as f:
        data = json.load(f)

    # Group by family
    families: dict[str, list[dict]] = {}
    for entry in data:
        fam = entry["family"]
        families.setdefault(fam, []).append(entry)

    lines = [
        f"# Calamity Suffix Effects ({len(families)} Families, {len(data)} Entries)",
        "",
        f"Auto-generated from `{SUFFIXES_PATH}`.",
        "",
        "| # | Family | ActorValue | Minor (T1) | Standard (T2) | Grand (T3) | Build |",
        "|---|--------|-----------|-----------|--------------|-----------|-------|",
    ]

    BUILD_MAP = {
        "vitality": "범용",
        "endurance": "근접/궁수",
        "brilliance": "마법사",
        "regeneration": "범용",
        "tenacity": "근접",
        "meditation": "마법사",
        "guardian": "물리 방어",
        "spell_ward": "마법 저항",
        "shadow": "스텔스/도적",
        "gladiator": "공격 속도",
        "assassin": "치명타",
        "swiftness": "범용 이동",
        "swordsman": "한손 전사",
        "champion": "양손 전사",
        "marksman": "궁수",
        "evasion": "경장/도적",
        "fortitude": "중장/탱커",
        "bulwark": "방패 탱커",
        "eagle_eye": "궁수",
        "steed": "범용 유틸",
        "enchanter": "부여 마법사",
        "conjurer": "소환 마법사",
    }

    for idx, (fam, entries) in enumerate(sorted(families.items()), 1):
        entries.sort(key=lambda e: e["id"])
        av = entries[0]["records"]["magicEffect"]["actorValue"]
        mags = []
        for e in entries:
            mag = e["records"]["spell"]["effect"]["magnitude"]
            mags.append(f"+{mag}")
        build = BUILD_MAP.get(fam, "")
        lines.append(
            f"| {idx} | **{fam}** | {av} | {mags[0]} | {mags[1]} | {mags[2]} | {build} |"
        )

    lines.append("")
    lines.append("## Detail by Family")
    lines.append("")

    for fam, entries in sorted(families.items()):
        entries.sort(key=lambda e: e["id"])
        lines.append(f"### {fam}")
        lines.append("")
        lines.append("| Tier | ID | Name (Ko) | Name (En) | Magnitude | EditorID (KYWD) |")
        lines.append("|------|----|-----------|-----------|-----------:|----------------|")
        for e in entries:
            tier = e["id"].split("_")[-1].upper()
            mag = e["records"]["spell"]["effect"]["magnitude"]
            lines.append(
                f"| {tier} | `{e['id']}` | {e['nameKo']} | {e['nameEn']} | {mag} | `{e['editorId']}` |"
            )
        lines.append("")

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[OK] Wrote {OUTPUT_PATH} ({len(data)} entries, {len(families)} families)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
