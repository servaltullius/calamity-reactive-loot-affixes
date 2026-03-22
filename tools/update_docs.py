#!/usr/bin/env python3
"""Regenerate public-facing affix documentation."""

from __future__ import annotations

import json
import subprocess
import sys
from datetime import date
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DOCS = REPO / "docs"
AFFIXES_PATH = REPO / "affixes" / "affixes.json"


def run_generator(script_name: str) -> None:
    script_path = REPO / "tools" / script_name
    subprocess.run([sys.executable, str(script_path)], check=True, cwd=REPO)


def regenerate_affix_catalog() -> None:
    payload = json.loads(AFFIXES_PATH.read_text(encoding="utf-8"))
    affixes = payload["keywords"]["affixes"]

    prefixes = [
        a
        for a in affixes
        if a.get("slot") == "prefix"
        and not a.get("id", "").startswith("internal_")
        and not (a.get("id", "").startswith("runeword_") and a.get("id", "").endswith("_final"))
    ]
    suffixes = [a for a in affixes if a.get("slot") == "suffix"]
    runewords = [
        a
        for a in affixes
        if a.get("id", "").startswith("runeword_") and a.get("id", "").endswith("_final")
    ]

    prefix_categories = {
        "원소/마법 타격": [
            "storm_call",
            "flame_strike",
            "frost_strike",
            "spark_strike",
            "flame_weakness",
            "frost_weakness",
            "shock_weakness",
            "elemental_bane",
            "thunder_mastery",
            "elemental_attunement",
        ],
        "소환/유틸리티": [
            "wolf_spirit",
            "flame_sprite",
            "flame_atronach",
            "frost_atronach",
            "storm_atronach",
            "dremora_pact",
            "soul_snare",
            "soul_siphon",
            "shadow_stride",
            "silent_step",
            "battle_frenzy",
        ],
        "피격 방어/생존": [
            "shadow_veil",
            "stone_ward",
            "arcane_ward",
            "healing_surge",
            "mage_armor_t1",
            "mage_armor_t2",
        ],
        "특수 메커닉": [
            "archmage_t1",
            "archmage_t2",
            "archmage_t3",
            "archmage_t4",
            "fire_infusion_50",
            "fire_infusion_100",
            "frost_infusion_50",
            "frost_infusion_100",
            "shock_infusion_50",
            "shock_infusion_100",
        ],
        "치명 시전": [
            "crit_cast_firebolt",
            "crit_cast_ice_spike",
            "crit_cast_lightning_bolt",
            "crit_cast_thunderbolt",
            "crit_cast_icy_spear",
            "crit_cast_chain_lightning",
            "crit_cast_ice_storm",
        ],
        "패시브/두루마리": [
            "scroll_mastery_t1",
            "scroll_mastery_t2",
            "scroll_mastery_t3",
            "scroll_mastery_t4",
        ],
        "트랩/폭발/제어": [
            "ember_pyre",
            "death_pyre_t1",
            "death_pyre_t2",
            "death_pyre_t3",
            "conjured_pyre_t1",
            "conjured_pyre_t2",
            "bear_trap",
            "rune_trap",
            "chaos_rune",
            "plague_spore",
            "tar_blight",
            "siphon_spore",
        ],
        "외침/학파 확장": [
            "voice_of_power",
            "death_mark",
            "ice_form",
            "disarming_shout",
            "whirlwind_sprint",
            "spell_breach",
            "stamina_drain",
            "nourishing_flame",
            "mana_knot",
        ],
    }
    prefix_ids = {a["id"] for a in prefixes}

    lines = [
        "# 어픽스 카탈로그",
        "",
        f"> 업데이트: {date.today().isoformat()}",
        "> 기준 버전: `v1.2.21`",
        "> 기준 파일: `affixes/affixes.json`",
        "> INTERNAL 항목은 공개 문서에서 숨김",
        "",
        "이 문서는 현재 플레이어가 실제로 보는 어픽스 구조를 한 번에 훑기 위한 공개용 입구 문서입니다.",
        "세부 수치와 개별 효과 설명은 각 상세 문서를 참조하세요.",
        "",
        "## 현재 규모",
        "",
        f"- 프리픽스: **{len(prefixes)}개**",
        f"- 서픽스: **{len(suffixes)}개**",
        f"- 룬워드: **{len(runewords)}개**",
        "",
        "## 문서 안내",
        "",
        "- 프리픽스 상세: `docs/PREFIX_EFFECTS.md`",
        "- 서픽스 상세: `docs/SUFFIX_EFFECTS.md`",
        "- 룬워드 상세: `docs/RUNEWORD_EFFECTS.md`",
        "",
        "## 프리픽스 구성 요약",
        "",
        "| 묶음 | 대표 효과 | 설명 |",
        "|------|-----------|------|",
    ]

    group_descriptions = {
        "원소/마법 타격": "원소 추가 피해, 저항 약화, 적응형 원소 대응",
        "소환/유틸리티": "소환체, 자원 회복, 이동/은신/속도 보조",
        "피격 방어/생존": "피격 방어 버프와 매지카 전환 계열",
        "특수 메커닉": "Archmage, ConvertDamage 같은 빌드 핵심 메커닉",
        "치명 시전": "치명타/강공 기반 추가 주문 발동 계열",
        "패시브/두루마리": "상시 적용형 보조 효과와 소비 절약 계열",
        "트랩/폭발/제어": "Bloom, Trap, Corpse Explosion, 소환체 폭발 계열",
        "외침/학파 확장": "Thu'um 테마와 마법학파 확장 효과",
    }
    representative_names = {a["id"]: a.get("nameKo", a.get("name", a["id"])) for a in prefixes}

    def preview_names(ids: list[str]) -> str:
        seen: list[str] = []
        for eid in ids:
            if eid not in representative_names:
                continue
            name = representative_names[eid].split(":")[0]
            if name not in seen:
                seen.append(name)
            if len(seen) == 3:
                break
        return ", ".join(seen)

    for group, ids in prefix_categories.items():
        visible_ids = [eid for eid in ids if eid in prefix_ids]
        preview = preview_names(visible_ids)
        lines.append(f"| {group} | {preview} | {group_descriptions[group]} |")

    uncovered = sorted(prefix_ids - {eid for ids in prefix_categories.values() for eid in ids})
    if uncovered:
        preview = preview_names(uncovered)
        lines.append(f"| 기타 | {preview} | 분류표에 없는 공개 프리픽스 잔여 항목 |")

    lines.extend(
        [
            "",
            "## 서픽스 구성 요약",
            "",
            "서픽스는 22개 패밀리, 66개 엔트리로 구성되며 대부분 T1/T2/T3 3단계 구조입니다.",
            "",
            "| 분류 | 예시 |",
            "|------|------|",
            "| 자원/생존 | vitality, endurance, brilliance, guardian, spell_ward |",
            "| 재생/유틸 | regeneration, tenacity, meditation, steed, swiftness |",
            "| 전투 전문화 | swordsman, champion, marksman, gladiator, assassin |",
            "| 방어/은신/보조 | evasion, fortitude, bulwark, shadow |",
            "| 제작/마법 | enchanter, conjurer |",
            "",
            "세부 표는 `docs/SUFFIX_EFFECTS.md`를 참조하세요.",
            "",
            "## 룬워드 구성 요약",
            "",
            "- 전체 룬워드는 **94개**이며, 현재는 모두 JSON 개별 정의입니다.",
            "- 상세 효과 문서는 각 룬워드의 인게임 표시 문자열을 그대로 사용합니다.",
            "- 상세 효과는 `docs/RUNEWORD_EFFECTS.md`를 참조하세요.",
            "",
            "## 공개 문서 기준",
            "",
            "- 플레이어가 실제로 획득/재련/적용 가능한 항목 기준으로 정리합니다.",
            "- INTERNAL companion 정의나 생성기 내부 보조 항목은 숨깁니다.",
            "- 구현 세부보다 인게임 체감과 공개 설명의 일관성을 우선합니다.",
            "",
        ]
    )

    path = DOCS / "AFFIX_CATALOG.md"
    path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  AFFIX_CATALOG.md 재생성 완료 ({len(prefixes)}P + {len(suffixes)}S + {len(runewords)}R)")


def main() -> None:
    print("=== 공개 문서 재생성 ===")
    run_generator("gen_prefix_doc.py")
    run_generator("gen_suffix_doc.py")
    run_generator("gen_runeword_doc.py")
    regenerate_affix_catalog()
    print("\n완료!")


if __name__ == "__main__":
    main()
