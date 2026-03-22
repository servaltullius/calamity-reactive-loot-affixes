#!/usr/bin/env python3
"""공개용 서픽스 효과 문서 생성."""
from __future__ import annotations

import json
import sys
from datetime import date
from pathlib import Path

SUFFIXES_PATH = Path("affixes/modules/keywords.affixes.suffixes.json")
OUTPUT_PATH = Path("docs/SUFFIX_EFFECTS.md")


def display_magnitude(actor_value: str, magnitude: float | int) -> str:
    if actor_value == "WeaponSpeedMult":
        return str(int(round(float(magnitude) * 100)))
    if isinstance(magnitude, float) and magnitude.is_integer():
        return str(int(magnitude))
    return str(magnitude)


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
        "# 서픽스 효과 정리 (공개용)",
        "",
        f"> 업데이트: {date.today().isoformat()}",
        "> 기준 버전: `v1.2.21`",
        f"> 기준 코드: `{SUFFIXES_PATH}`",
        "",
        f"- 총 서픽스 패밀리: **{len(families)}개**",
        f"- 총 서픽스 엔트리: **{len(data)}개**",
        "- 각 티어 이름은 인게임 표시 문자열(`nameKo`/`nameEn`)을 그대로 사용",
        "- 공개용 문서 기준으로 패밀리/핵심 수치/추천 빌드를 중심으로 정리",
        "",
        "## 패밀리 요약",
        "",
        "| # | 패밀리 | 영향 능력치 | T1 | T2 | T3 | 추천 빌드 |",
        "|---|--------|-------------|----|----|----|-----------|",
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
        av = str(av)
        mags = []
        for e in entries:
            mag = e["records"]["spell"]["effect"]["magnitude"]
            mags.append(f"+{display_magnitude(av, mag)}")
        build = BUILD_MAP.get(fam, "")
        lines.append(
            f"| {idx} | **{fam}** | {av} | {mags[0]} | {mags[1]} | {mags[2]} | {build} |"
        )

    lines.append("")
    lines.append("## 패밀리별 상세")
    lines.append("")

    for fam, entries in sorted(families.items()):
        entries.sort(key=lambda e: e["id"])
        lines.append(f"### {fam}")
        lines.append("")
        lines.append(f"- 영향 능력치: `{entries[0]['records']['magicEffect']['actorValue']}`")
        if BUILD_MAP.get(fam):
            lines.append(f"- 추천 빌드: {BUILD_MAP[fam]}")
        lines.append("")
        lines.append("| 티어 | ID | 한국어 이름 | 영어 이름 | 수치 |")
        lines.append("|------|----|-------------|-----------|-----:|")
        for e in entries:
            tier = e["id"].split("_")[-1].upper()
            mag = e["records"]["spell"]["effect"]["magnitude"]
            lines.append(
                f"| {tier} | `{e['id']}` | {e['nameKo']} | {e['nameEn']} | {display_magnitude(entries[0]['records']['magicEffect']['actorValue'], mag)} |"
            )
        lines.append("")

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[OK] Wrote {OUTPUT_PATH} ({len(data)} entries, {len(families)} families)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
