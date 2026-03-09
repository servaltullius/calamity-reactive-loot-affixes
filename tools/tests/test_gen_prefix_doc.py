#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


class GenPrefixDocTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        module_path = cls.repo_root / "tools" / "gen_prefix_doc.py"
        spec = importlib.util.spec_from_file_location("gen_prefix_doc", module_path)
        if spec is None or spec.loader is None:
            raise RuntimeError(f"Unable to load module: {module_path}")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        cls.gen_prefix_doc = module

    def test_categorize_skips_internal_entries(self) -> None:
        categories = self.gen_prefix_doc.categorize(
            [
                {"id": "storm_call", "nameKo": "폭풍 소환"},
                {"id": "internal_hidden", "nameKo": "INTERNAL: Hidden"},
            ]
        )

        flattened_ids = [
            entry["id"]
            for grouped in categories.values()
            for _, entry in grouped
        ]

        self.assertIn("storm_call", flattened_ids)
        self.assertNotIn("internal_hidden", flattened_ids)


if __name__ == "__main__":
    unittest.main()
