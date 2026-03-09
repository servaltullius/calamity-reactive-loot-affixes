#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


class LintAffixesGeneratedSyncTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        module_path = cls.repo_root / "tools" / "lint_affixes.py"
        spec = importlib.util.spec_from_file_location("lint_affixes", module_path)
        if spec is None or spec.loader is None:
            raise RuntimeError(f"Unable to load module: {module_path}")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        cls.lint_affixes = module

    def test_generated_sync_detects_runtime_content_mismatch(self) -> None:
        spec_payload = {
            "$schema": "https://example.invalid/schema.json",
            "keywords": {
                "affixes": [
                    {
                        "id": "affix_test",
                        "editorId": "CAFF_AFFIX_TEST",
                        "runtime": {
                            "trigger": "Hit",
                            "procChancePercent": 35.0,
                            "action": {"type": "DebugNotify"},
                        },
                    }
                ]
            },
        }
        generated_payload = {
            "keywords": {
                "affixes": [
                    {
                        "id": "affix_test",
                        "editorId": "CAFF_AFFIX_TEST",
                        "runtime": {
                            "trigger": "Hit",
                            "procChancePercent": 15.0,
                            "action": {"type": "DebugNotify"},
                        },
                    }
                ]
            },
        }

        errors: list[str] = []
        warnings: list[str] = []
        self.lint_affixes._check_generated_sync(
            spec_payload,
            generated_payload,
            spec_path=Path("missing-spec.json"),
            generated_path=Path("missing-generated.json"),
            errors=errors,
            warnings=warnings,
        )

        self.assertTrue(
            any(
                "keywords.affixes[0].runtime.procChancePercent" in error
                for error in errors
            ),
            msg=f"expected runtime mismatch error, got errors={errors} warnings={warnings}",
        )

    def test_special_actions_require_positive_proc_chance(self) -> None:
        spec_payload = {
            "$schema": "https://example.invalid/schema.json",
            "keywords": {
                "affixes": [
                    {
                        "id": "affix_special",
                        "editorId": "CAFF_AFFIX_SPECIAL",
                        "runtime": {
                            "trigger": "Hit",
                            "procChancePercent": 0.0,
                            "action": {"type": "ConvertDamage"},
                        },
                    }
                ]
            },
        }

        errors: list[str] = []
        warnings: list[str] = []
        self.lint_affixes._lint_spec(
            spec_payload,
            errors=errors,
            warnings=warnings,
            supported_triggers={"Hit", "IncomingHit", "DotApply", "Kill", "LowHealth"},
            supported_action_types={
                "DebugNotify",
                "CastSpell",
                "CastSpellAdaptiveElement",
                "CastOnCrit",
                "ConvertDamage",
                "MindOverMatter",
                "Archmage",
                "CorpseExplosion",
                "SummonCorpseExplosion",
                "SpawnTrap",
            },
        )

        self.assertTrue(
            any("special actions require runtime.procChancePercent > 0" in error for error in errors),
            msg=f"expected special-action proc chance error, got errors={errors} warnings={warnings}",
        )


if __name__ == "__main__":
    unittest.main()
