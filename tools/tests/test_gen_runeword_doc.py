#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


class GenRunewordDocTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        module_path = cls.repo_root / "tools" / "gen_runeword_doc.py"
        spec = importlib.util.spec_from_file_location("gen_runeword_doc", module_path)
        if spec is None or spec.loader is None:
            raise RuntimeError(f"Unable to load module: {module_path}")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        cls.gen_runeword_doc = module

    def test_render_entry_uses_exact_ingame_display_text(self) -> None:
        recipe = {
            "id": "rw_test",
            "name": "Test",
            "runes": ["Tal", "Eth"],
            "resultAffixId": "runeword_test_final",
            "recommendedBase": "Armor",
        }
        affix = {
            "id": "runeword_test_final",
            "nameKo": "룬워드 시험 [Tal-Eth]: 적중 시 보호막. 3초마다 발동.",
            "nameEn": "Runeword Test (Tal-Eth): On hit, grants a barrier. Triggers every 3s.",
        }

        rendered = self.gen_runeword_doc.render_entry(recipe, affix)

        self.assertIn("한글 표시: 룬워드 시험 [Tal-Eth]: 적중 시 보호막. 3초마다 발동.", rendered)
        self.assertIn("영문 표시: Runeword Test (Tal-Eth): On hit, grants a barrier. Triggers every 3s.", rendered)
        self.assertIn("룬 조합: `Tal-Eth`", rendered)
        self.assertIn("추천 베이스: Armor", rendered)


if __name__ == "__main__":
    unittest.main()
