#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path


class LintAffixesGeneratedSyncTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        tools_dir = cls.repo_root / "tools"
        sys.path.insert(0, str(tools_dir))
        module_path = tools_dir / "lint_affixes.py"
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

    def test_generated_sync_ignores_file_timestamps_when_content_matches(self) -> None:
        spec_payload = {
            "keywords": {
                "affixes": [
                    {
                        "id": "affix_test",
                        "editorId": "CAFF_AFFIX_TEST",
                        "nameKo": "시험",
                    }
                ]
            }
        }
        generated_payload = json.loads(json.dumps(spec_payload))

        with tempfile.TemporaryDirectory(prefix="caff-lint-generated-") as temp_dir:
            root = Path(temp_dir)
            spec_path = root / "affixes.json"
            generated_path = root / "generated.json"
            spec_path.write_text("{}\n", encoding="utf-8")
            generated_path.write_text("{}\n", encoding="utf-8")
            os.utime(generated_path, ns=(1_000_000_000, 1_000_000_000))
            os.utime(spec_path, ns=(2_000_000_000, 2_000_000_000))

            errors: list[str] = []
            warnings: list[str] = []
            self.lint_affixes._check_generated_sync(
                spec_payload,
                generated_payload,
                spec_path=spec_path,
                generated_path=generated_path,
                errors=errors,
                warnings=warnings,
            )

        self.assertEqual([], errors)
        self.assertEqual([], warnings)

    def test_generated_sync_detects_authored_content_outside_runtime_fields(self) -> None:
        spec_payload = {
            "keywords": {
                "affixes": [
                    {
                        "id": "affix_test",
                        "editorId": "CAFF_AFFIX_TEST",
                        "nameKo": "원본 이름",
                    }
                ]
            }
        }
        generated_payload = {
            "keywords": {
                "affixes": [
                    {
                        "id": "affix_test",
                        "editorId": "CAFF_AFFIX_TEST",
                        "nameKo": "오래된 이름",
                    }
                ]
            }
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
            any("keywords.affixes[0].nameKo" in error for error in errors),
            msg=f"expected authored content mismatch, got errors={errors} warnings={warnings}",
        )

    def test_module_sync_uses_composed_content_instead_of_timestamps(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-lint-modules-") as temp_dir:
            root = Path(temp_dir)
            manifest_path = self._write_module_fixture(root)
            spec_path = root / "affixes.json"
            composed = self.lint_affixes.compose_spec(manifest_path)
            spec_path.write_text(json.dumps(composed), encoding="utf-8")

            os.utime(spec_path, ns=(1_000_000_000, 1_000_000_000))
            for module_path in root.glob("*.module.json"):
                os.utime(module_path, ns=(2_000_000_000, 2_000_000_000))

            errors: list[str] = []
            warnings: list[str] = []
            self.lint_affixes._check_module_manifest_sync(
                spec_path=spec_path,
                manifest_path=manifest_path,
                errors=errors,
                warnings=warnings,
            )

        self.assertEqual([], errors)
        self.assertEqual([], warnings)

    def test_module_sync_detects_composed_content_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-lint-modules-stale-") as temp_dir:
            root = Path(temp_dir)
            manifest_path = self._write_module_fixture(root)
            spec_path = root / "affixes.json"
            composed = self.lint_affixes.compose_spec(manifest_path)
            composed["version"] = "stale"
            spec_path.write_text(json.dumps(composed), encoding="utf-8")

            errors: list[str] = []
            warnings: list[str] = []
            self.lint_affixes._check_module_manifest_sync(
                spec_path=spec_path,
                manifest_path=manifest_path,
                errors=errors,
                warnings=warnings,
            )

        self.assertTrue(
            any("does not match modular sources" in error for error in errors),
            msg=f"expected composed spec mismatch, got errors={errors} warnings={warnings}",
        )

    @staticmethod
    def _write_module_fixture(root: Path) -> Path:
        files = {
            "root.module.json": {"version": "1.0.0", "keywords": {}},
            "tags.module.json": [],
            "affixes.module.json": [],
            "kid.module.json": [],
            "spid.module.json": [],
        }
        for name, payload in files.items():
            (root / name).write_text(json.dumps(payload), encoding="utf-8")

        manifest = {
            "root": "root.module.json",
            "keywords": {
                "tags": ["tags.module.json"],
                "affixes": ["affixes.module.json"],
                "kidRules": ["kid.module.json"],
                "spidRules": ["spid.module.json"],
            },
        }
        manifest_path = root / "affixes.modules.json"
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        return manifest_path

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
