#!/usr/bin/env python3
"""Generate public runeword effects documentation markdown."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from public_doc_metadata import load_public_doc_metadata

CONTRACT_PATH = Path("affixes/runeword.contract.json")
RUNEWORDS_PATH = Path("affixes/modules/keywords.affixes.runewords.json")
OUTPUT_PATH = Path("docs/RUNEWORD_EFFECTS.md")


def load_payload() -> tuple[list[dict], dict[str, dict]]:
    contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    runewords = json.loads(RUNEWORDS_PATH.read_text(encoding="utf-8"))
    affix_by_id = {entry["id"]: entry for entry in runewords}
    return contract["runewordCatalog"], affix_by_id


def render_entry(recipe: dict, affix: dict) -> str:
    rune_sequence = "-".join(recipe["runes"])
    lines = [
        f"### {recipe['name']}",
        "",
        f"- 룬 조합: `{rune_sequence}`",
        f"- 추천 베이스: {recipe['recommendedBase']}",
        f"- 한글 표시: {affix.get('nameKo', affix.get('name', ''))}",
    ]
    name_en = affix.get("nameEn", "")
    if name_en and name_en != affix.get("nameKo", ""):
        lines.append(f"- 영문 표시: {name_en}")
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=OUTPUT_PATH)
    args = parser.parse_args(argv)
    metadata = load_public_doc_metadata()
    recipes, affix_by_id = load_payload()

    lines = [
        "# 룬워드 효과 정리 (공개용)",
        "",
        f"> 업데이트: {metadata.release_date}",
        f"> 기준 버전: `v{metadata.version}`",
        f"> 기준 코드: `{RUNEWORDS_PATH}`",
        f"> 룬 조합 기준: `{CONTRACT_PATH}`",
        "",
        f"- 총 룬워드: **{len(recipes)}개**",
        "- 각 효과 설명은 인게임 표시 문자열(`nameKo`/`nameEn`)을 그대로 사용",
        "",
    ]

    for recipe in recipes:
        affix = affix_by_id[recipe["resultAffixId"]]
        lines.append(render_entry(recipe, affix))
        lines.append("")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    print(f"[OK] Wrote {args.output} ({len(recipes)} runewords)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
