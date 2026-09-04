#!/usr/bin/env python3
"""Regenerate public-facing affix documentation."""

from __future__ import annotations

import argparse
import difflib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from public_doc_metadata import PublicDocMetadata, load_public_doc_metadata

REPO = Path(__file__).resolve().parent.parent
DOCS = REPO / "docs"
AFFIXES_PATH = REPO / "affixes" / "affixes.json"
GENERATED_DOCS = (
    "PREFIX_EFFECTS.md",
    "SUFFIX_EFFECTS.md",
    "RUNEWORD_EFFECTS.md",
    "AFFIX_CATALOG.md",
)


def run_generator(script_name: str, output_path: Path) -> None:
    script_path = REPO / "tools" / script_name
    subprocess.run(
        [sys.executable, str(script_path), "--output", str(output_path)],
        check=True,
        cwd=REPO,
    )


def regenerate_affix_catalog(output_path: Path, metadata: PublicDocMetadata) -> None:
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
        f"> 업데이트: {metadata.release_date}",
        f"> 기준 버전: `v{metadata.version}`",
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
        "- [프리픽스 상세](PREFIX_EFFECTS.md): 효과별 확률·ICD와 평타 발동·중복 장착 규칙",
        "- [서픽스 상세](SUFFIX_EFFECTS.md): 패밀리별 T1~T3 수치와 장착 티어 합산 규칙",
        "- [룬워드 상세](RUNEWORD_EFFECTS.md): 룬 조합·추천 베이스·개별 효과",
        "",
        "## 현재 발동·중첩 규칙 (RC6 반영)",
        "",
        "- 곰 덫과 치명 시전에는 일반 공격 확률 발동 경로가 있으며, 효과별 확률과 ICD는 프리픽스 상세를 참조하세요.",
        "- 동일 표준 발동 어픽스는 장착 사본 최대 3개의 확률을 점감 합산하고 효과는 한 번만 실행합니다. 모든 접두 효과에 공통으로 적용되는 피해량 배수 규칙은 아닙니다.",
        "- 치명 시전은 근접 치명타·강공격에서 서로 다른 주문 최대 2개, 일반 근접 공격과 활·석궁에서 최대 1개이며 발동 묶음당 전역 ICD 0.15초를 공유합니다.",
        "- 같은 접미 패밀리는 장착 티어를 합산해 T1 + T1 → T2, T1 + T2 → T3으로 적용하며 최대 T3입니다. 결과 티어 효과 하나만 적용합니다.",
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
        "치명 시전": "치명타/강공 및 일반 공격 확률 발동 추가 주문 계열",
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
            "세부 표와 티어 합산 예시는 [서픽스 상세](SUFFIX_EFFECTS.md)를 참조하세요.",
            "",
            "## 룬워드 구성 요약",
            "",
            "- 전체 룬워드는 **94개**이며, 현재는 모두 JSON 개별 정의입니다.",
            "- 상세 효과 문서는 각 룬워드의 인게임 표시 문자열을 그대로 사용합니다.",
            "- 상세 효과는 [룬워드 상세](RUNEWORD_EFFECTS.md)를 참조하세요.",
            "",
            "## 공개 문서 기준",
            "",
            "- 플레이어가 실제로 획득/재련/적용 가능한 항목 기준으로 정리합니다.",
            "- INTERNAL companion 정의나 생성기 내부 보조 항목은 숨깁니다.",
            "- 구현 세부보다 인게임 체감과 공개 설명의 일관성을 우선합니다.",
            "",
        ]
    )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  AFFIX_CATALOG.md 재생성 완료 ({len(prefixes)}P + {len(suffixes)}S + {len(runewords)}R)")


def generate_docs(output_dir: Path, metadata: PublicDocMetadata) -> None:
    print("=== 공개 문서 재생성 ===", flush=True)
    run_generator("gen_prefix_doc.py", output_dir / "PREFIX_EFFECTS.md")
    run_generator("gen_suffix_doc.py", output_dir / "SUFFIX_EFFECTS.md")
    run_generator("gen_runeword_doc.py", output_dir / "RUNEWORD_EFFECTS.md")
    regenerate_affix_catalog(output_dir / "AFFIX_CATALOG.md", metadata)
    print("\n완료!")


def check_generated_docs(generated_dir: Path) -> bool:
    mismatches: list[str] = []
    for filename in GENERATED_DOCS:
        expected_path = DOCS / filename
        generated_path = generated_dir / filename
        expected = expected_path.read_text(encoding="utf-8") if expected_path.exists() else ""
        generated = generated_path.read_text(encoding="utf-8")
        if expected == generated:
            continue

        mismatches.append(filename)
        try:
            expected_label = str(expected_path.relative_to(REPO))
        except ValueError:
            expected_label = str(expected_path)
        diff = difflib.unified_diff(
            expected.splitlines(),
            generated.splitlines(),
            fromfile=expected_label,
            tofile=f"generated/{filename}",
            lineterm="",
        )
        for line in list(diff)[:80]:
            print(line, file=sys.stderr)

    if mismatches:
        print(
            "ERROR: generated public docs are stale: " + ", ".join(mismatches),
            file=sys.stderr,
        )
        print("Run: python3 tools/update_docs.py", file=sys.stderr)
        return False

    print(f"public docs sync OK: {len(GENERATED_DOCS)} generated files")
    return True


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="regenerate into a temporary directory and fail if checked-in docs differ",
    )
    args = parser.parse_args(argv)
    metadata = load_public_doc_metadata()

    if args.check:
        with tempfile.TemporaryDirectory(prefix="caff-public-docs-") as temp_dir:
            generated_dir = Path(temp_dir)
            generate_docs(generated_dir, metadata)
            return 0 if check_generated_docs(generated_dir) else 2

    generate_docs(DOCS, metadata)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
