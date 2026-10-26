#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path


class GenSuffixDocTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        module_path = cls.repo_root / "tools" / "gen_suffix_doc.py"
        sys.path.insert(0, str(module_path.parent))
        spec = importlib.util.spec_from_file_location("gen_suffix_doc", module_path)
        if spec is None or spec.loader is None:
            raise RuntimeError(f"Unable to load module: {module_path}")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        cls.gen_suffix_doc = module

    @staticmethod
    def _entries(actor_value: str, archetype: str | None = None) -> list[dict]:
        mgef = {"actorValue": actor_value}
        if archetype:
            mgef["archetype"] = archetype
        return [{"records": {"magicEffect": mgef}}]

    def test_carrier_family_shows_runtime_label_instead_of_none(self) -> None:
        label = self.gen_suffix_doc.display_stat("eagle_eye", self._entries("None", "Script"))
        self.assertEqual("활 당기기 속도 (런타임 적용)", label)

    def test_unlabelled_carrier_family_stops_generation(self) -> None:
        with self.assertRaises(SystemExit):
            self.gen_suffix_doc.display_stat("new_carrier", self._entries("None", "Script"))

    def test_armor_modifier_families_show_damage_reduction_label(self) -> None:
        self.assertEqual(
            "받는 피해 감소(경갑, %)",
            self.gen_suffix_doc.display_stat("evasion", self._entries("LightArmorModifier")),
        )
        self.assertEqual(
            "받는 피해 감소(중갑, %)",
            self.gen_suffix_doc.display_stat("fortitude", self._entries("HeavyArmorModifier")),
        )

    def test_regular_family_keeps_actor_value(self) -> None:
        self.assertEqual("HealRateMult", self.gen_suffix_doc.display_stat("regeneration", self._entries("HealRateMult")))


if __name__ == "__main__":
    unittest.main()
