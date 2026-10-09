#!/usr/bin/env python3
"""Generate public prefix effects documentation markdown."""

import argparse
import json
from pathlib import Path

from public_doc_metadata import load_public_doc_metadata

CORE_JSON = Path("affixes/modules/keywords.affixes.core.json")
# v2.3.0 shared-status prefixes live in their own module.
STATUS_JSON = Path("affixes/modules/keywords.affixes.status.json")


def load_prefix_entries():
    entries = json.loads(CORE_JSON.read_text(encoding="utf-8"))
    if STATUS_JSON.exists():
        entries += json.loads(STATUS_JSON.read_text(encoding="utf-8"))
    return entries
OUTPUT = Path("docs/PREFIX_EFFECTS.md")


def categorize(entries):
    """Assign category to each entry."""
    cats = {}
    cat_map = {
        "storm_call": "원소 타격",
        "flame_strike": "원소 타격",
        "frost_strike": "원소 타격",
        "spark_strike": "원소 타격",
        "flame_weakness": "원소 취약",
        "frost_weakness": "원소 취약",
        "shock_weakness": "원소 취약",
        "wolf_spirit": "소환",
        "flame_sprite": "소환",
        "flame_atronach": "소환",
        "frost_atronach": "소환",
        "storm_atronach": "소환",
        "dremora_pact": "소환",
        "soul_snare": "소환",
        "shadow_veil": "피격 방어",
        "stone_ward": "피격 방어",
        "arcane_ward": "피격 방어",
        "healing_surge": "피격 방어",
        "mage_armor_t1": "피격 방어",
        "mage_armor_t2": "피격 방어",
        "scroll_mastery_t1": "두루마리 숙련",
        "scroll_mastery_t2": "두루마리 숙련",
        "scroll_mastery_t3": "두루마리 숙련",
        "scroll_mastery_t4": "두루마리 숙련",
        "ember_brand": "화염 DoT",
        "ember_pyre": "화염 DoT",
        "ice_shackle": "CC / 디버프",
        "mana_burn": "CC / 디버프",
        "life_drain": "CC / 디버프",
        "soul_siphon": "유틸리티",
        "shadow_stride": "유틸리티",
        "silent_step": "유틸리티",
        "battle_frenzy": "유틸리티",
        "bear_trap": "덫 / 룬",
        "rune_trap": "덫 / 룬",
        "chaos_rune": "덫 / 룬",
        "fire_infusion_50": "원소 주입 (전환)",
        "fire_infusion_100": "원소 주입 (전환)",
        "frost_infusion_50": "원소 주입 (전환)",
        "frost_infusion_100": "원소 주입 (전환)",
        "shock_infusion_50": "원소 주입 (전환)",
        "shock_infusion_100": "원소 주입 (전환)",
        "archmage_t1": "대마법사",
        "archmage_t2": "대마법사",
        "archmage_t3": "대마법사",
        "archmage_t4": "대마법사",
        "crit_cast_firebolt": "치명 시전 (Crit Cast)",
        "crit_cast_ice_spike": "치명 시전 (Crit Cast)",
        "crit_cast_lightning_bolt": "치명 시전 (Crit Cast)",
        "crit_cast_thunderbolt": "치명 시전 (Crit Cast)",
        "crit_cast_icy_spear": "치명 시전 (Crit Cast)",
        "crit_cast_chain_lightning": "치명 시전 (Crit Cast)",
        "crit_cast_ice_storm": "치명 시전 (Crit Cast)",
        "death_pyre_t1": "시체 소각 (죽음의 화장)",
        "death_pyre_t2": "시체 소각 (죽음의 화장)",
        "death_pyre_t3": "시체 소각 (죽음의 화장)",
        "conjured_pyre_t1": "소환 화장",
        "conjured_pyre_t2": "소환 화장",
        "elemental_bane": "역병 / 적응",
        "plague_spore": "역병 / 적응",
        "tar_blight": "역병 / 적응",
        "siphon_spore": "역병 / 적응",
        "thunder_mastery": "성장형",
        "elemental_attunement": "성장형",
        "voice_of_power": "Thu'um (외침) — 신규",
        "death_mark": "Thu'um (외침) — 신규",
        "ice_form": "Thu'um (외침) — 신규",
        "disarming_shout": "Thu'um (외침) — 신규",
        "whirlwind_sprint": "Thu'um (외침) — 신규",
        "spell_breach": "마법학파 — 신규",
        "stamina_drain": "마법학파 — 신규",
        "nourishing_flame": "마법학파 — 신규",
        "mana_knot": "마법학파 — 신규",
        "doom_brand": "공용 상태 (v2.3.0)",
        "exploit_weakness": "공용 상태 (v2.3.0)",
        "contagion": "공용 상태 (v2.3.0)",
    }

    for i, e in enumerate(entries):
        eid = e["id"]
        if eid.startswith("internal_"):
            continue
        cat = cat_map.get(eid, "기타")
        cats.setdefault(cat, []).append((i, e))
    return cats

CAT_ORDER = [
    "원소 타격", "원소 취약", "소환", "피격 방어", "두루마리 숙련",
    "화염 DoT", "CC / 디버프", "유틸리티", "덫 / 룬",
    "원소 주입 (전환)", "대마법사", "치명 시전 (Crit Cast)",
    "시체 소각 (죽음의 화장)", "소환 화장",
    "역병 / 적응", "성장형",
    "Thu'um (외침) — 신규", "마법학파 — 신규", "공용 상태 (v2.3.0)",
]


def format_entry(idx, e):
    rt = e.get("runtime", {})
    action = rt.get("action", {})
    spell = action.get("spellEditorId", "")
    kid = e.get("kid", {})
    kid_type = kid.get("type", "")
    name_ko = e.get("nameKo", e.get("name", ""))
    name_en = e.get("nameEn", "")

    lines = []
    header = f"- **`{e['id']}`**"
    if kid_type:
        header += f" [{kid_type}]"
    lines.append(header)
    if name_ko:
        lines.append(f"  - 한글 표시: {name_ko}")
    if name_en and name_en != name_ko:
        lines.append(f"  - 영문 표시: {name_en}")
    if spell:
        lines.append(f"  - 대표 스펠: `{spell}`")

    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args(argv)
    metadata = load_public_doc_metadata()

    entries = load_prefix_entries()

    public_entries = [entry for entry in entries if not entry["id"].startswith("internal_")]
    cats = categorize(entries)

    out = []
    out.append("# 프리픽스 효과 정리 (공개용)")
    out.append("")
    out.append(f"> 업데이트: {metadata.release_date}")
    out.append(f"> 기준 버전: `v{metadata.version}`")
    out.append("> 기준 코드:")
    out.append("> - 효과 정의: `affixes/modules/keywords.affixes.core.json`, `affixes/modules/keywords.affixes.status.json`")
    out.append("> - 변환 스크립트: `tools/transform_prefixes.py`")
    out.append("")
    out.append(f"- 총 공개 프리픽스: **{len(public_entries)}개**")
    out.append("- INTERNAL 항목은 공개 문서에서 숨김")
    out.append("- 스카이림 로어 기반 전면 리네이밍 (v1.2.21)")
    out.append("- 9개 freed slot → Thu'um 5종 + 마법학파 4종 신규 효과")
    out.append("- 각 효과 설명은 인게임 표시 문자열(`nameKo`/`nameEn`)을 그대로 사용")
    out.append("- 티어형 프리픽스는 같은 패밀리가 한 아이템에 중복되지 않으며, 각 `kid.chance`를 실제 획득 가중치로 사용")
    out.append("- 같은 원소의 50%/100% 피해 주입도 하나의 배타적 패밀리로 취급")
    out.append("")
    out.append("## 발동과 중복 장착 규칙 (RC6 반영)")
    out.append("")
    out.append("- 곰 덫은 일반 무기 적중에서도 기본 40%, 치명 시전은 일반 근접 적중에서도 주문별 기본 35% 또는 45%로 판정. 개별 항목의 확률·ICD를 함께 확인")
    out.append("- 치명타·강공격에는 기존 판정만 사용하며 평타 확률을 추가로 이중 판정하지 않음. 주문·배시·폭발은 평타 발동 대상에서 제외")
    out.append("- 서로 다른 장비에 장착한 동일 표준 발동 어픽스는 장비별 확률 페널티를 반영한 강한 사본부터 최대 3개를 합산. 일반 주문 발동·적응형 원소 주문·덫에 적용하며, 치명 시전·피해 전환 등 별도 처리 효과에 일괄 적용하지 않음")
    out.append("- 합산 확률은 `1 - (1-p1)×(1-p2)×(1-p3)` 방식. 각 사본이 40%이고 추가 보정이 없으면 1/2/3개 장착 시 40% → 64% → 78.4%. 동일 효과의 피해량을 2배·3배로 만들거나 여러 번 실행하는 것은 아니며 발동 예산·ICD도 한 번만 소비")
    out.append("- 치명 시전은 한 번의 공격(치명타·강공격·일반 근접·활·석궁)에 주문 하나만 시전. 여러 개를 착용하면 차례로 돌아가며 발동하고, 발동 묶음당 전역 ICD 0.15초를 공유함")
    out.append("- 표기된 확률은 기본값이며 장착 구성·런타임 보정·발동 조건·ICD에 따라 실제 발동 빈도가 달라짐. 접미 슬롯 자체는 표준 발동의 다중 어픽스 확률 페널티를 늘리지 않음")
    out.append("- 접미의 티어 합산은 별도 규칙: [서픽스 효과 정리](SUFFIX_EFFECTS.md) 참조")
    out.append("- 같은 종류의 자기 버프는 새것이 옛것을 대체함(예: 공격력 버프 두 개가 겹쳐 쌓이지 않음). 상시 효과와 다른 모드 버프는 건드리지 않음")
    out.append("")
    out.append("## 공용 상태 (v2.3.0)")
    out.append("")
    out.append("상태를 거는 어픽스는 설명 끝에 상태 이름을 적어 둠. 다른 장비에서 건 상태끼리도 함께 작동함")
    out.append("")
    out.append("- **빙결·출혈·감전 축적**: 발동할 때마다 대상의 미터가 참. 100이 되면 미터가 비면서 효과가 터짐. 보스·드래곤·고유 적은 150에서 시작하고 터질 때마다 50씩 올라감(최대 300). 마지막으로 쌓인 뒤 3초가 지나면 초당 15씩 줄어듦")
    out.append("  - **빙결**: 3초 동안 이동속도 -80%, 주는 피해 -30%")
    out.append("  - **출혈**: 대상 최대 체력의 8% 피해(강한 적은 3%, 최소 10·최대 400)")
    out.append("  - **감전**: 대상과 반경 300 안의 적 최대 2명에게 번개 피해 40")
    out.append("- **화상**: 1중첩당 초당 화염 피해 4, 최대 5중첩. 새 중첩이 붙으면 4초 지속이 다시 시작됨")
    out.append("- **노출**: 저항 감소. 같은 저항에는 가장 강한 칼라미티 노출 하나만 적용")
    out.append("- **파멸**: 표식 1.5초 뒤 폭발. 다시 걸면 시간만 갱신하고 더 큰 폭발을 유지")
    out.append("- 상태는 **약점 포착**(서로 다른 상태 2개 이상)과 **전염**(처치 시 주변으로 옮김)에 쓰임")
    out.append("")

    # Category counts
    out.append("### 카테고리별 수량")
    out.append("")
    out.append("| 카테고리 | 수 |")
    out.append("|----------|---|")
    for cat in CAT_ORDER:
        if cat in cats:
            out.append(f"| {cat} | {len(cats[cat])} |")
    out.append(f"| **합계** | **{len(public_entries)}** |")
    out.append("")

    # Per-category entries
    for cat in CAT_ORDER:
        if cat not in cats:
            continue
        out.append(f"## {cat}")
        if "신규" in cat:
            out.append("")
            out.append("> v1.2.21 신규 추가 — freed slot 활용")
        out.append("")
        for idx, e in cats[cat]:
            out.append(format_entry(idx, e))
            out.append("")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(out).rstrip() + "\n", encoding="utf-8")
    print(f"Wrote {args.output} ({len(out)} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
