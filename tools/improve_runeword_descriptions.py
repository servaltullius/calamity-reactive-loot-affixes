#!/usr/bin/env python3
"""Improve vague runeword descriptions in keywords.affixes.runewords.json.

Replaces generic terms like "Buff", "IAS", "Life Steal", "Exposure/노출"
with concrete effect descriptions.
"""

import json
import sys
from pathlib import Path

RUNEWORD_JSON = Path(__file__).resolve().parent.parent / "affixes" / "modules" / "keywords.affixes.runewords.json"

# Map of id → (old_nameKo_fragment, new_nameKo_fragment, old_nameEn_fragment, new_nameEn_fragment)
# Each entry is a list of (old, new) pairs for Ko and En respectively.
IMPROVEMENTS: dict[str, list[tuple[str, str, str, str]]] = {
    # --- "Buff" → concrete effect ---
    "runeword_fortitude_final": [
        ("방어 버프", "대미지 저항 강화", "Defense buff", "Damage Resist boost"),
    ],
    "runeword_insight_final": [
        ("마나 버프", "마나 회복 강화", "Mana buff", "Magicka Recovery boost"),
    ],
    "runeword_spirit_final": [
        ("마나 버프", "마나 회복 강화", "Mana buff", "Magicka Recovery boost"),
    ],
    "runeword_call_to_arms_final": [
        ("전투 버프", "전투력 강화 (공격+방어)", "Buff", "Battle Cry (Offense+Defense)"),
    ],
    "runeword_chains_of_honor_final": [
        ("방어 버프", "페이즈 (이동속도+회피 강화)", "Buff", "Phase (Speed+Evasion boost)"),
    ],
    "runeword_faith_final": [
        ("돌격 버프", "광신 강화 (공격력+공속)", "Surge buff", "Fanaticism (Damage+Speed)"),
    ],
    "runeword_last_wish_final": [
        ("방어+저항 대폭 버프", "방어도+저항 대폭 강화", "Armor+Resist buff", "Armor+Resist boost"),
    ],
    "runeword_honor_final": [
        ("강화 버프", "활력 강화 (공격력+방어)", "Buff", "Vigor (Damage+Defense)"),
    ],
    "runeword_hustle_w_final": [
        ("돌진 버프", "돌진 (이동+공속 강화)", "Rush buff", "Rush (Move+Attack Speed)"),
        # IAS fix
        ("", "", "IAS", "Attack Speed"),
    ],
    "runeword_rhyme_final": [
        ("방어 버프", "보호막 (방어도 강화)", "Buff", "Shield Ward (Armor boost)"),
    ],
    "runeword_hustle_a_final": [
        ("방어 버프", "방어 강화 (방어도+회피)", "Buff", "Defense boost (Armor+Evasion)"),
    ],
    "runeword_treachery_final": [
        ("회피 버프", "회피 강화 (회피율+이동)", "Evasion buff", "Evasion boost (Dodge+Speed)"),
        # IAS fix
        ("", "", "IAS", "Attack Speed"),
    ],
    # --- "Exposure/노출" → "저항 감소" ---
    "runeword_infinity_final": [
        ("노출(적 저항 감소)", "저항 감소", "Exposure", "Resist Reduction"),
    ],
    "runeword_heart_of_the_oak_final": [
        ("노출)", "저항 감소)", "exposure)", "Resist Reduction)"),
    ],
    # --- "Life Steal" + "IAS" ---
    "runeword_grief_final": [
        ("", "", "Life Steal (8%)", "Heal 8% of Hit Damage"),
        ("", "", "IAS", "Attack Speed"),
    ],
}


def apply_improvements(data: list[dict]) -> int:
    changed = 0
    for entry in data:
        eid = entry.get("id", "")
        if eid not in IMPROVEMENTS:
            continue

        name_ko = entry.get("nameKo", "")
        name_en = entry.get("nameEn", "")
        original_ko = name_ko
        original_en = name_en

        for old_ko, new_ko, old_en, new_en in IMPROVEMENTS[eid]:
            if old_ko and old_ko in name_ko:
                name_ko = name_ko.replace(old_ko, new_ko, 1)
            if old_en and old_en in name_en:
                name_en = name_en.replace(old_en, new_en, 1)

        if name_ko != original_ko or name_en != original_en:
            entry["nameKo"] = name_ko
            entry["nameEn"] = name_en
            changed += 1
            print(f"  [{eid}]")
            if name_ko != original_ko:
                print(f"    Ko: {original_ko}")
                print(f"     → {name_ko}")
            if name_en != original_en:
                print(f"    En: {original_en}")
                print(f"     → {name_en}")

    return changed


def main() -> None:
    if not RUNEWORD_JSON.exists():
        print(f"ERROR: {RUNEWORD_JSON} not found", file=sys.stderr)
        sys.exit(1)

    with open(RUNEWORD_JSON, "r", encoding="utf-8") as f:
        data = json.load(f)

    print(f"Loaded {len(data)} runeword entries from {RUNEWORD_JSON.name}")
    print()

    changed = apply_improvements(data)

    if changed == 0:
        print("No changes needed.")
        return

    with open(RUNEWORD_JSON, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")

    print(f"\nUpdated {changed} entries.")


if __name__ == "__main__":
    main()
