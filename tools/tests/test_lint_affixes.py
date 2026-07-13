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

    def _lint(self, affixes: list[dict]) -> tuple[list[str], list[str]]:
        errors: list[str] = []
        warnings: list[str] = []
        supported_triggers, supported_action_types = self.lint_affixes._load_validation_contract()
        self.lint_affixes._lint_spec(
            {"keywords": {"affixes": affixes}},
            errors=errors,
            warnings=warnings,
            supported_triggers=supported_triggers,
            supported_action_types=supported_action_types,
        )
        return errors, warnings

    @staticmethod
    def _debug_runtime() -> dict:
        return {
            "trigger": "Hit",
            "procChancePercent": 100.0,
            "action": {"type": "DebugNotify"},
        }

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

    def test_magic_effect_semantics_accept_valid_singular_and_array_records(self) -> None:
        affixes = [
            {
                "id": "valid_singular_hostile",
                "editorId": "CAFF_VALID_SINGULAR_HOSTILE",
                "records": {
                    "magicEffect": {
                        "editorId": "CAFF_MGEF_VALID_SLOW",
                        "actorValue": "SpeedMult",
                        "hostile": True,
                        "recover": True,
                    },
                    "spell": {
                        "editorId": "CAFF_SPEL_VALID_SLOW",
                        "castType": "FireAndForget",
                        "effect": {
                            "magicEffectEditorId": "CAFF_MGEF_VALID_SLOW",
                            "magnitude": 35,
                            "duration": 2,
                        },
                    },
                },
                "runtime": self._debug_runtime(),
            },
            {
                "id": "valid_array_semantics",
                "editorId": "CAFF_VALID_ARRAY_SEMANTICS",
                "records": {
                    "magicEffects": [
                        {
                            "editorId": "CAFF_MGEF_VALID_PARALYSIS",
                            "actorValue": "Paralysis",
                            "archetype": "Paralysis",
                            "hostile": True,
                            "recover": True,
                        },
                        {
                            "editorId": "CAFF_MGEF_VALID_HEAL",
                            "actorValue": "Health",
                            "hostile": False,
                            "recover": False,
                        },
                        {
                            "editorId": "CAFF_MGEF_VALID_CONSTANT_MAGICKA",
                            "actorValue": "Magicka",
                            "hostile": False,
                            "recover": True,
                        },
                    ],
                    "spells": [
                        {
                            "editorId": "CAFF_SPEL_VALID_ARRAY",
                            "castType": "FireAndForget",
                            "effects": [
                                {
                                    "magicEffectEditorId": "CAFF_MGEF_VALID_PARALYSIS",
                                    "magnitude": 1,
                                    "duration": 2,
                                },
                                {
                                    "magicEffectEditorId": "CAFF_MGEF_VALID_HEAL",
                                    "magnitude": 10,
                                    "duration": 0,
                                },
                            ],
                        },
                        {
                            "editorId": "CAFF_SPEL_VALID_CONSTANT",
                            "castType": "ConstantEffect",
                            "effect": {
                                "magicEffectEditorId": "CAFF_MGEF_VALID_CONSTANT_MAGICKA",
                                "magnitude": 25,
                                "duration": 0,
                            },
                        },
                    ],
                },
                "runtime": self._debug_runtime(),
            },
            {
                "id": "valid_regeneration_suffix",
                "editorId": "CAFF_VALID_REGEN_SUFFIX",
                "slot": "suffix",
                "family": "regeneration",
                "records": {
                    "magicEffect": {
                        "editorId": "CAFF_MGEF_VALID_REGEN_SUFFIX",
                        "actorValue": "HealRateMult",
                        "hostile": False,
                        "recover": True,
                    }
                },
                "runtime": self._debug_runtime(),
            },
            {
                "id": "valid_tenacity_suffix",
                "editorId": "CAFF_VALID_TENACITY_SUFFIX",
                "slot": "suffix",
                "family": "tenacity",
                "records": {
                    "magicEffect": {
                        "editorId": "CAFF_MGEF_VALID_TENACITY_SUFFIX",
                        "actorValue": "StaminaRateMult",
                        "hostile": False,
                        "recover": True,
                    }
                },
                "runtime": self._debug_runtime(),
            },
            {
                "id": "valid_meditation_suffix",
                "editorId": "CAFF_VALID_MEDITATION_SUFFIX",
                "slot": "suffix",
                "family": "meditation",
                "records": {
                    "magicEffect": {
                        "editorId": "CAFF_MGEF_VALID_MEDITATION_SUFFIX",
                        "actorValue": "MagickaRateMult",
                        "hostile": False,
                        "recover": True,
                    }
                },
                "runtime": self._debug_runtime(),
            },
        ]

        errors, warnings = self._lint(affixes)

        self.assertEqual([], errors)
        self.assertEqual([], warnings)

    def test_magic_effect_semantics_reject_negative_hostile_magnitude_in_all_shapes(self) -> None:
        affixes = [
            {
                "id": "invalid_singular_hostile",
                "editorId": "CAFF_INVALID_SINGULAR_HOSTILE",
                "records": {
                    "magicEffect": {
                        "editorId": "CAFF_MGEF_INVALID_SINGULAR_HOSTILE",
                        "actorValue": "SpeedMult",
                        "hostile": True,
                        "recover": True,
                    },
                    "spell": {
                        "editorId": "CAFF_SPEL_INVALID_SINGULAR_HOSTILE",
                        "effect": {
                            "magicEffectEditorId": "CAFF_MGEF_INVALID_SINGULAR_HOSTILE",
                            "magnitude": -35,
                            "duration": 2,
                        },
                    },
                },
                "runtime": self._debug_runtime(),
            },
            {
                "id": "invalid_array_hostile",
                "editorId": "CAFF_INVALID_ARRAY_HOSTILE",
                "records": {
                    "magicEffects": [
                        {
                            "editorId": "CAFF_MGEF_INVALID_ARRAY_HOSTILE",
                            "actorValue": "ResistMagic",
                            "hostile": True,
                            "recover": True,
                        }
                    ],
                    "spells": [
                        {
                            "editorId": "CAFF_SPEL_INVALID_ARRAY_HOSTILE",
                            "effects": [
                                {
                                    "magicEffectEditorId": "CAFF_MGEF_INVALID_ARRAY_HOSTILE",
                                    "magnitude": -20,
                                    "duration": 6,
                                }
                            ],
                        }
                    ],
                },
                "runtime": self._debug_runtime(),
            },
        ]

        errors, _ = self._lint(affixes)
        hostile_errors = [
            error
            for error in errors
            if "Detrimental effects require a non-negative magnitude" in error
        ]

        self.assertEqual(2, len(hostile_errors), msg=f"errors={errors}")
        self.assertTrue(any("invalid_singular_hostile" in error for error in hostile_errors))
        self.assertTrue(any("invalid_array_hostile" in error for error in hostile_errors))

    def test_magic_effect_semantics_require_paralysis_recover(self) -> None:
        affixes = [
            {
                "id": "invalid_paralysis",
                "editorId": "CAFF_INVALID_PARALYSIS",
                "records": {
                    "magicEffects": [
                        {
                            "editorId": "CAFF_MGEF_INVALID_PARALYSIS",
                            "actorValue": "Paralysis",
                            "archetype": "Paralysis",
                            "hostile": True,
                            "recover": False,
                        }
                    ]
                },
                "runtime": self._debug_runtime(),
            }
        ]

        errors, _ = self._lint(affixes)

        self.assertTrue(
            any(
                "invalid_paralysis" in error
                and "archetype=Paralysis requires recover=true" in error
                for error in errors
            ),
            msg=f"errors={errors}",
        )

    def test_instant_resource_restore_rejects_recover_but_allows_constant_effect(self) -> None:
        affixes = [
            {
                "id": "invalid_instant_restore",
                "editorId": "CAFF_INVALID_INSTANT_RESTORE",
                "records": {
                    "magicEffect": {
                        "editorId": "CAFF_MGEF_INVALID_INSTANT_RESTORE",
                        "actorValue": "Health",
                        "hostile": False,
                        "recover": True,
                    },
                    "spell": {
                        "editorId": "CAFF_SPEL_INVALID_INSTANT_RESTORE",
                        "castType": "FireAndForget",
                        "effect": {
                            "magicEffectEditorId": "CAFF_MGEF_INVALID_INSTANT_RESTORE",
                            "magnitude": 10,
                            "duration": 0,
                        },
                    },
                },
                "runtime": self._debug_runtime(),
            },
            {
                "id": "valid_constant_resource_buff",
                "editorId": "CAFF_VALID_CONSTANT_RESOURCE_BUFF",
                "records": {
                    "magicEffect": {
                        "editorId": "CAFF_MGEF_VALID_CONSTANT_RESOURCE_BUFF",
                        "actorValue": "Magicka",
                        "hostile": False,
                        "recover": True,
                    },
                    "spell": {
                        "editorId": "CAFF_SPEL_VALID_CONSTANT_RESOURCE_BUFF",
                        "castType": "ConstantEffect",
                        "effect": {
                            "magicEffectEditorId": "CAFF_MGEF_VALID_CONSTANT_RESOURCE_BUFF",
                            "magnitude": 25,
                            "duration": 0,
                        },
                    },
                },
                "runtime": self._debug_runtime(),
            },
        ]

        errors, _ = self._lint(affixes)

        self.assertTrue(
            any(
                "invalid_instant_restore" in error
                and "non-ConstantEffect restores require recover=false" in error
                for error in errors
            ),
            msg=f"errors={errors}",
        )
        self.assertFalse(
            any("valid_constant_resource_buff" in error for error in errors),
            msg=f"errors={errors}",
        )

    def test_suffix_regen_contract_and_array_actor_value_whitelist(self) -> None:
        affixes = [
            {
                "id": "invalid_regeneration_suffix",
                "editorId": "CAFF_INVALID_REGEN_SUFFIX",
                "slot": "suffix",
                "family": "regeneration",
                "records": {
                    "magicEffects": [
                        {
                            "editorId": "CAFF_MGEF_INVALID_REGEN_SUFFIX",
                            "actorValue": "HealRate",
                            "hostile": False,
                            "recover": True,
                        }
                    ]
                },
                "runtime": self._debug_runtime(),
            },
            {
                "id": "invalid_array_actor_value",
                "editorId": "CAFF_INVALID_ARRAY_ACTOR_VALUE",
                "records": {
                    "magicEffects": [
                        {
                            "editorId": "CAFF_MGEF_INVALID_ARRAY_ACTOR_VALUE",
                            "actorValue": "DefinitelyNotAnActorValue",
                            "hostile": False,
                            "recover": True,
                        }
                    ]
                },
                "runtime": self._debug_runtime(),
            },
        ]

        errors, _ = self._lint(affixes)

        self.assertTrue(
            any(
                "invalid_regeneration_suffix" in error
                and "suffix family 'regeneration'" in error
                and "HealRateMult" in error
                for error in errors
            ),
            msg=f"errors={errors}",
        )
        self.assertTrue(
            any(
                "invalid_array_actor_value" in error
                and "unknown actorValue 'DefinitelyNotAnActorValue'" in error
                and "records.magicEffects[0].actorValue" in error
                for error in errors
            ),
            msg=f"errors={errors}",
        )

    def test_corpse_explosion_percent_point_contract(self) -> None:
        affixes = [
            {
                "id": "invalid_fractional_corpse_pct",
                "editorId": "CAFF_INVALID_FRACTIONAL_CORPSE_PCT",
                "runtime": {
                    "trigger": "Kill",
                    "procChancePercent": 100.0,
                    "action": {
                        "type": "CorpseExplosion",
                        "flatDamage": 14.0,
                        "pctOfCorpseMaxHealth": 0.08,
                    },
                },
            },
            {
                "id": "valid_percent_point_corpse_pct",
                "editorId": "CAFF_VALID_PERCENT_POINT_CORPSE_PCT",
                "runtime": {
                    "trigger": "Kill",
                    "procChancePercent": 100.0,
                    "action": {
                        "type": "CorpseExplosion",
                        "flatDamage": 14.0,
                        "pctOfCorpseMaxHealth": 8.0,
                    },
                },
            },
            {
                "id": "valid_flat_only_summon_explosion",
                "editorId": "CAFF_VALID_FLAT_ONLY_SUMMON_EXPLOSION",
                "runtime": {
                    "trigger": "Kill",
                    "procChancePercent": 100.0,
                    "action": {
                        "type": "SummonCorpseExplosion",
                        "flatDamage": 20.0,
                        "pctOfCorpseMaxHealth": 0,
                    },
                },
            },
        ]

        errors, warnings = self._lint(affixes)

        self.assertTrue(
            any(
                "invalid_fractional_corpse_pct" in error
                and "looks like a fraction" in error
                and "8.0 for 8%" in error
                for error in errors
            ),
            msg=f"errors={errors}",
        )
        self.assertFalse(
            any("valid_percent_point_corpse_pct" in error for error in errors),
            msg=f"errors={errors}",
        )
        self.assertFalse(
            any("valid_flat_only_summon_explosion" in error for error in errors),
            msg=f"errors={errors}",
        )
        self.assertEqual([], warnings)


    def test_corpse_explosion_selection_priority_contract(self) -> None:
        affixes = [
            {
                "id": "invalid_non_integer_selection_priority",
                "editorId": "CAFF_INVALID_NON_INTEGER_SELECTION_PRIORITY",
                "runtime": {
                    "trigger": "Kill",
                    "procChancePercent": 100.0,
                    "action": {
                        "type": "CorpseExplosion",
                        "selectionPriority": 10.5,
                        "flatDamage": 14.0,
                        "pctOfCorpseMaxHealth": 8.0,
                    },
                },
            },
            {
                "id": "invalid_out_of_range_selection_priority",
                "editorId": "CAFF_INVALID_OUT_OF_RANGE_SELECTION_PRIORITY",
                "runtime": {
                    "trigger": "Kill",
                    "procChancePercent": 100.0,
                    "action": {
                        "type": "CorpseExplosion",
                        "selectionPriority": 101,
                        "flatDamage": 14.0,
                        "pctOfCorpseMaxHealth": 8.0,
                    },
                },
            },
            {
                "id": "valid_signature_selection_priority",
                "editorId": "CAFF_VALID_SIGNATURE_SELECTION_PRIORITY",
                "runtime": {
                    "trigger": "Kill",
                    "procChancePercent": 100.0,
                    "action": {
                        "type": "CorpseExplosion",
                        "selectionPriority": 20,
                        "flatDamage": 14.0,
                        "pctOfCorpseMaxHealth": 8.0,
                    },
                },
            },
            {
                "id": "valid_default_selection_priority",
                "editorId": "CAFF_VALID_DEFAULT_SELECTION_PRIORITY",
                "runtime": {
                    "trigger": "Kill",
                    "procChancePercent": 100.0,
                    "action": {
                        "type": "CorpseExplosion",
                        "flatDamage": 14.0,
                        "pctOfCorpseMaxHealth": 8.0,
                    },
                },
            },
        ]

        errors, _ = self._lint(affixes)

        self.assertTrue(
            any(
                "invalid_non_integer_selection_priority" in error
                and "must be an integer" in error
                for error in errors
            ),
            msg=f"errors={errors}",
        )
        self.assertTrue(
            any(
                "invalid_out_of_range_selection_priority" in error
                and "out of range" in error
                for error in errors
            ),
            msg=f"errors={errors}",
        )
        self.assertFalse(
            any("valid_signature_selection_priority" in error for error in errors),
            msg=f"errors={errors}",
        )
        self.assertFalse(
            any("valid_default_selection_priority" in error for error in errors),
            msg=f"errors={errors}",
        )


if __name__ == "__main__":
    unittest.main()
