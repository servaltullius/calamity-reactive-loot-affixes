#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_DIR = REPO_ROOT / "tools"
sys.path.insert(0, str(TOOLS_DIR))

import gen_effects_en_doc  # noqa: E402


def load(relative: str):
    return json.loads((REPO_ROOT / relative).read_text(encoding="utf-8"))


class GenEffectsEnDocTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-effects-en-") as temp_dir:
            output = Path(temp_dir) / "EFFECTS_EN.md"
            subprocess.run(
                [sys.executable, str(TOOLS_DIR / "gen_effects_en_doc.py"), "--output", str(output)],
                check=True,
                cwd=REPO_ROOT,
                capture_output=True,
            )
            cls.text = output.read_text(encoding="utf-8")

    def test_the_english_list_contains_no_korean(self) -> None:
        self.assertEqual([], re.findall(r"[가-힣]+", self.text))

    def test_every_public_prefix_suffix_and_runeword_is_listed(self) -> None:
        prefixes = [e for e in load(gen_effects_en_doc.CORE_JSON) if not e["id"].startswith("internal_")]
        for entry in prefixes:
            self.assertIn(f"- {gen_effects_en_doc.bold_name(entry['nameEn'])}\n", self.text, entry["id"])
        for entry in load(gen_effects_en_doc.SUFFIXES_PATH):
            self.assertIn(entry["nameEn"].split(": ", 1)[1], self.text, entry["id"])
        for recipe in load(gen_effects_en_doc.CONTRACT_PATH)["runewordCatalog"]:
            self.assertIn(f"| **{recipe['name']}** | {'-'.join(recipe['runes'])} |", self.text, recipe["id"])

    def test_runeword_rows_use_the_recipe_name_and_keep_only_the_effect(self) -> None:
        # Some in-game names keep only the first word ("Runeword Holy" for Holy Thunder).
        lines = gen_effects_en_doc.render_runewords(
            [{"name": "Holy Thunder", "runes": ["Eth", "Ral"], "resultAffixId": "rw", "recommendedBase": None}],
            {"rw": {"nameEn": "Runeword Holy (Eth-Ral): 22% on hit / ICD 7s - Holy Thunder (Shock Damage 30)"}},
            [{"rune": "Eth", "weight": 4}],
        )
        self.assertIn("| **Holy Thunder** | Eth-Ral | Any | 22% on hit / ICD 7s - Holy Thunder (Shock Damage 30) |", lines)

    def test_an_uncategorized_prefix_is_listed_instead_of_dropped(self) -> None:
        lines = gen_effects_en_doc.render_prefixes(
            [{"id": "brand_new_prefix", "nameEn": "Brand New (10% on hit): Something"}]
        )
        self.assertIn("### Other", lines)
        self.assertIn("- **Brand New** (10% on hit): Something", lines)

    def test_a_suffix_family_without_an_english_label_fails_loudly(self) -> None:
        with self.assertRaises(SystemExit):
            gen_effects_en_doc.render_suffixes(
                [{"id": "suffix_new_t1", "family": "brand_new", "nameEn": "of Minor New: Thing +1"}]
            )


if __name__ == "__main__":
    unittest.main()
