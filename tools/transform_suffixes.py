#!/usr/bin/env python3
"""Suffix 8개 패밀리(24개 엔트리) 교체 변환 스크립트.

EditorID(KYWD/MGEF/SPEL) 100% 보존, id/family/name/actorValue/magnitude만 교체.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

SUFFIXES_PATH = Path("affixes/modules/keywords.affixes.suffixes.json")

# ──────────────────────────────────────────────────
# 교체 매핑: old_id → new config
# ──────────────────────────────────────────────────

TIER_PREFIX_KO = {1: "약간의 ", 2: "", 3: "위대한 "}
TIER_PREFIX_EN = {1: "of Minor ", 2: "of ", 3: "of Grand "}


def _make_entries(
    old_family: str,
    new_family: str,
    new_family_ko: str,
    new_family_en: str,
    actor_value: str,
    effect_en: str,
    effect_ko: str,
    magnitudes: tuple[int, int, int],
    unit: str = "%",
) -> dict:
    """3개 티어 엔트리 매핑 생성."""
    result = {}
    for tier in (1, 2, 3):
        old_id = f"suffix_{old_family}_t{tier}"
        mag = magnitudes[tier - 1]
        unit_str = unit

        result[old_id] = {
            "id": f"suffix_{new_family}_t{tier}",
            "family": new_family,
            "nameKo": f"{TIER_PREFIX_KO[tier]}{new_family_ko}: {effect_ko} +{mag}{unit_str}",
            "nameEn": f"{TIER_PREFIX_EN[tier]}{new_family_en}: {effect_en} +{mag}{unit_str}",
            "actorValue": actor_value,
            "magnitude": mag,
            "recNameEn": f"{effect_en} +{mag}{unit_str}",
            "recNameKo": f"{effect_ko} +{mag}{unit_str}",
        }
    return result


REPLACE_MAP: dict[str, dict] = {}

# 1. swordsman (검술) — was flame_ward
REPLACE_MAP.update(_make_entries(
    "flame_ward", "swordsman", "검술", "Swordsmanship",
    "OneHandedModifier", "One-Handed", "한손 무기", (5, 10, 15),
))

# 2. champion (투사) — was frost_ward
REPLACE_MAP.update(_make_entries(
    "frost_ward", "champion", "투사", "Champion",
    "TwoHandedModifier", "Two-Handed", "양손 무기", (5, 10, 15),
))

# 3. marksman (명사수) — was storm_ward
REPLACE_MAP.update(_make_entries(
    "storm_ward", "marksman", "명사수", "Marksman",
    "MarksmanModifier", "Archery", "활/석궁", (5, 10, 15),
))

# 4. evasion (회피) — was serpent
REPLACE_MAP.update(_make_entries(
    "serpent", "evasion", "회피", "Evasion",
    "LightArmorModifier", "Light Armor", "경갑", (5, 10, 15),
))

# 5. fortitude (견고) — was destruction
REPLACE_MAP.update(_make_entries(
    "destruction", "fortitude", "견고", "Fortitude",
    "HeavyArmorModifier", "Heavy Armor", "중갑", (5, 10, 15),
))

# 6. bulwark (방벽) — was alteration
REPLACE_MAP.update(_make_entries(
    "alteration", "bulwark", "방벽", "Bulwark",
    "BlockModifier", "Block", "방어", (5, 10, 15),
))

# 7. eagle_eye (독수리눈) — was dovahkiin
REPLACE_MAP.update(_make_entries(
    "dovahkiin", "eagle_eye", "독수리눈", "Eagle Eye",
    "BowSpeedBonus", "Bow Speed", "활 속도", (5, 10, 15),
))

# 8. steed (준마) — was conjuration  (절대값, % 없음)
REPLACE_MAP.update(_make_entries(
    "conjuration", "steed", "준마", "Steed",
    "CarryWeight", "Carry Weight", "소지 무게", (15, 30, 50),
    unit="",
))


def transform(data: list[dict]) -> list[dict]:
    """교체 대상 24개 엔트리만 변환, 나머지 36개 무변경."""
    transformed = 0
    for entry in data:
        old_id = entry["id"]
        if old_id not in REPLACE_MAP:
            continue

        cfg = REPLACE_MAP[old_id]

        # EditorID 보존 검증 (변경 전 기록)
        kywd_eid = entry["editorId"]
        mgef_eid = entry["records"]["magicEffect"]["editorId"]
        spel_eid = entry["records"]["spell"]["editorId"]
        mgef_ref = entry["records"]["spell"]["effect"]["magicEffectEditorId"]

        # 교체
        entry["id"] = cfg["id"]
        entry["family"] = cfg["family"]
        entry["name"] = cfg["nameKo"]
        entry["nameEn"] = cfg["nameEn"]
        entry["nameKo"] = cfg["nameKo"]

        rec_name = f"Calamity Suffix: {cfg['recNameEn']} / {cfg['recNameKo']}"
        entry["records"]["magicEffect"]["actorValue"] = cfg["actorValue"]
        entry["records"]["magicEffect"]["name"] = rec_name
        entry["records"]["spell"]["name"] = rec_name
        entry["records"]["spell"]["effect"]["magnitude"] = cfg["magnitude"]

        # passiveSpellEditorId는 SPEL editorId와 동일 — 변경 불필요 (이미 보존)

        # EditorID 보존 확인
        assert entry["editorId"] == kywd_eid, f"KYWD editorId changed: {kywd_eid}"
        assert entry["records"]["magicEffect"]["editorId"] == mgef_eid, f"MGEF editorId changed: {mgef_eid}"
        assert entry["records"]["spell"]["editorId"] == spel_eid, f"SPEL editorId changed: {spel_eid}"
        assert entry["records"]["spell"]["effect"]["magicEffectEditorId"] == mgef_ref, f"MGEF ref changed: {mgef_ref}"

        transformed += 1

    return data, transformed


def validate(data: list[dict]) -> list[str]:
    """변환 후 유효성 검증."""
    errors = []

    # 1. 60개 id 유일성
    ids = [e["id"] for e in data]
    if len(ids) != len(set(ids)):
        dupes = [x for x in ids if ids.count(x) > 1]
        errors.append(f"Duplicate IDs: {set(dupes)}")

    # 2. 20개 family 유일성
    families = set(e["family"] for e in data)
    if len(families) != 20:
        errors.append(f"Expected 20 families, got {len(families)}: {sorted(families)}")

    # 3. magnitude > 0
    for e in data:
        mag = e["records"]["spell"]["effect"]["magnitude"]
        if mag <= 0:
            errors.append(f"{e['id']}: magnitude={mag} <= 0")

    # 4. 8개 신규 actorValue 존재
    expected_avs = {
        "OneHandedModifier", "TwoHandedModifier", "MarksmanModifier",
        "LightArmorModifier", "HeavyArmorModifier", "BlockModifier",
        "BowSpeedBonus", "CarryWeight",
    }
    actual_avs = set(e["records"]["magicEffect"]["actorValue"] for e in data)
    missing = expected_avs - actual_avs
    if missing:
        errors.append(f"Missing actorValues: {missing}")

    # 5. 총 60개
    if len(data) != 60:
        errors.append(f"Expected 60 entries, got {len(data)}")

    return errors


def main() -> int:
    if not SUFFIXES_PATH.exists():
        print(f"[ERROR] File not found: {SUFFIXES_PATH}", file=sys.stderr)
        return 1

    with SUFFIXES_PATH.open("r", encoding="utf-8") as f:
        data = json.load(f)

    print(f"Loaded {len(data)} suffix entries")

    data, count = transform(data)
    print(f"Transformed {count}/24 entries")

    if count != 24:
        print(f"[ERROR] Expected 24 transformations, got {count}", file=sys.stderr)
        return 1

    errors = validate(data)
    if errors:
        for err in errors:
            print(f"[ERROR] {err}", file=sys.stderr)
        return 1

    with SUFFIXES_PATH.open("w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
        f.write("\n")

    print(f"[OK] Wrote {SUFFIXES_PATH}")
    print(f"[OK] 24 entries replaced, 36 preserved, 60 total")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
