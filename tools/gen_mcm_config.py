#!/usr/bin/env python3
"""Generate Data/MCM/Config/CalamityAffixes/config.json from tools/mcm_config.source.json.

Localized fields ({"en", "ko", "both"}) become three copies of the control, one per
Panel Language setting (iUiLanguage: 0 English, 1 Korean, 2 both). Each page starts
with three hidden toggles bound to CalamityAffixes_MCMConfig properties; the script
sets exactly one of them from iUiLanguage and refreshes the menu when it changes.
The copies use groupBehavior "skip", so only the matching one is ever created.

Usage: python3 tools/gen_mcm_config.py [--check]
"""

from __future__ import annotations

import argparse
import copy
import json
import sys
from pathlib import Path
from typing import Any, Dict, List

REPO = Path(__file__).resolve().parents[1]
SOURCE = REPO / "tools" / "mcm_config.source.json"
OUTPUT = REPO / "Data" / "MCM" / "Config" / "CalamityAffixes" / "config.json"

LANGUAGES = (
    # (key, groupControl id, script property)
    ("en", 1, "McmShowEnglish"),
    ("ko", 2, "McmShowKorean"),
    ("both", 3, "McmShowBoth"),
)

# The MCM info box shows about two lines; a bilingual help longer than this gets cut.
MAX_BOTH_HELP_CHARS = 110
# Every copy's help must also fit, counting Hangul as two columns.
MAX_HELP_WIDTH = 150


def help_width(text: str) -> int:
    return sum(2 if "\uac00" <= ch <= "\ud7a3" else 1 for ch in text)


def _is_localized(value: Any) -> bool:
    return isinstance(value, dict) and "en" in value and "ko" in value


def _pick(value: Any, language: str, *, field: str, where: str) -> Any:
    if not _is_localized(value):
        return value
    if language == "both":
        if "both" in value:
            return value["both"]
        if field == "help":
            raise ValueError(f"{where}: help needs a short 'both' text")
        return f"{value['en']} / {value['ko']}"
    return value[language]


def _localized_fields(control: Dict[str, Any]) -> bool:
    if _is_localized(control.get("text")) or _is_localized(control.get("help")):
        return True
    options = control.get("valueOptions")
    return isinstance(options, dict) and _is_localized(options.get("value"))


def _hidden_toggles() -> List[Dict[str, Any]]:
    return [
        {
            "type": "hiddenToggle",
            "text": f"Language group ({key})",
            "groupControl": group,
            "valueOptions": {"sourceType": "PropertyValueBool", "propertyName": prop},
        }
        for key, group, prop in LANGUAGES
    ]


def build(source: Dict[str, Any]) -> Dict[str, Any]:
    out = {k: copy.deepcopy(v) for k, v in source.items() if not k.startswith("_")}
    for page in out["pages"]:
        content: List[Dict[str, Any]] = _hidden_toggles()
        for index, control in enumerate(page["content"]):
            where = f"{page['pageDisplayName']}[{index}] {control.get('id') or control['type']}"
            if not _localized_fields(control):
                content.append(control)
                continue
            for key, group, _ in LANGUAGES:
                variant = copy.deepcopy(control)
                for field in ("text", "help"):
                    if field in variant:
                        variant[field] = _pick(variant[field], key, field=field, where=where)
                options = variant.get("valueOptions")
                if isinstance(options, dict) and "value" in options:
                    options["value"] = _pick(options["value"], key, field="value", where=where)
                if help_width(variant.get("help", "")) > MAX_HELP_WIDTH:
                    raise ValueError(
                        f"{where}: {key} help is {help_width(variant['help'])} columns wide (max {MAX_HELP_WIDTH})"
                    )
                if key == "both" and len(variant.get("help", "")) > MAX_BOTH_HELP_CHARS:
                    raise ValueError(
                        f"{where}: bilingual help is {len(variant['help'])} chars (max {MAX_BOTH_HELP_CHARS})"
                    )
                variant["groupCondition"] = group
                variant["groupBehavior"] = "skip"
                content.append(variant)
        page["content"] = content
    return out


def render(config: Dict[str, Any]) -> str:
    return json.dumps(config, ensure_ascii=False, indent=2) + "\n"


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the committed config.json is stale")
    args = parser.parse_args(argv)
    source = json.loads(SOURCE.read_text(encoding="utf-8"))
    text = render(build(source))
    if args.check:
        current = OUTPUT.read_text(encoding="utf-8") if OUTPUT.exists() else ""
        if current != text:
            print(f"{OUTPUT.relative_to(REPO)} is stale; run python3 tools/gen_mcm_config.py", file=sys.stderr)
            return 1
        print("MCM config OK")
        return 0
    OUTPUT.write_text(text, encoding="utf-8")
    print(f"Wrote {OUTPUT.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
