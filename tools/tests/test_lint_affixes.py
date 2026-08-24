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
        cls.schema = json.loads(
            (cls.repo_root / "affixes" / "affixes.schema.json").read_text(encoding="utf-8")
        )
        cls.repo_spec = json.loads(
            (cls.repo_root / "affixes" / "affixes.json").read_text(encoding="utf-8")
        )

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

    def _lint_payload(self, payload: dict) -> tuple[list[str], list[str]]:
        errors: list[str] = []
        warnings: list[str] = []
        supported_triggers, supported_action_types = self.lint_affixes._load_validation_contract()
        self.lint_affixes._lint_spec(
            payload,
            errors=errors,
            warnings=warnings,
            supported_triggers=supported_triggers,
            supported_action_types=supported_action_types,
        )
        return errors, warnings

    def test_repo_bear_trap_accepts_append_only_movable_static_world_marker(self) -> None:
        errors, warnings = self._lint_payload(self.repo_spec)
        self.assertEqual([], errors)
        self.assertEqual([], warnings)

        bear = next(
            affix for affix in self.repo_spec["keywords"]["affixes"] if affix["id"] == "bear_trap"
        )
        feedback = bear["runtime"]["action"]["trapFeedback"]
        self.assertEqual(
            {
                "initialEvent": "StartOpen",
                "triggerEvent": "Trigger01",
                "rearmEvent": "Reset01",
                "openGateMilliseconds": 900,
            },
            feedback["worldMarkerAnimation"],
        )
        self.assertEqual("Skyrim.esm|0x0001828A", feedback["placed"]["soundForm"])
        self.assertNotIn("armed", feedback)
        self.assertNotIn("soundForm", feedback["triggered"])

    def test_source_bear_movable_static_explicitly_enables_animation_updates(self) -> None:
        root_module = json.loads(
            (self.repo_root / "affixes" / "modules" / "spec.root.json").read_text(encoding="utf-8")
        )
        bear_marker = next(
            record["movableStatic"]
            for record in root_module["keywords"]["appendedRecords"]
            if record.get("type") == "MovableStatic"
            and record.get("movableStatic", {}).get("editorId") == "CAFF_MSTT_TRAP_BEAR_VISUAL"
        )

        self.assertIs(bear_marker.get("mustUpdateAnimations"), True)

    def test_movable_static_animation_update_flag_defaults_off_and_requires_boolean(self) -> None:
        payload = json.loads(json.dumps(self.repo_spec))
        bear_marker = next(
            record["movableStatic"]
            for record in payload["keywords"]["appendedRecords"]
            if record.get("type") == "MovableStatic"
            and record.get("movableStatic", {}).get("editorId") == "CAFF_MSTT_TRAP_BEAR_VISUAL"
        )
        bear_marker.pop("mustUpdateAnimations", None)

        errors, warnings = self._lint_payload(payload)
        self.assertEqual([], errors)
        self.assertEqual([], warnings)

        bear_marker["mustUpdateAnimations"] = False
        errors, warnings = self._lint_payload(payload)
        self.assertEqual([], errors)
        self.assertEqual([], warnings)

        bear_marker["mustUpdateAnimations"] = "true"
        errors, _ = self._lint_payload(payload)
        self.assertTrue(
            any("movableStatic.mustUpdateAnimations must be a boolean" in error for error in errors),
            msg=f"expected boolean contract error, got {errors}",
        )

    def test_trap_feedback_rejects_both_marker_authorities(self) -> None:
        payload = json.loads(json.dumps(self.repo_spec))
        bear = next(
            affix for affix in payload["keywords"]["affixes"] if affix["id"] == "bear_trap"
        )
        bear["runtime"]["action"]["trapFeedback"]["markerArtObjectEditorId"] = (
            "CAFF_ARTO_VFX_TRAP_BEAR_MARKER"
        )

        errors, _ = self._lint_payload(payload)
        self.assertTrue(
            any("requires exactly one of markerArtObjectEditorId or markerWorldObjectEditorId" in error for error in errors),
            msg=f"expected exclusive marker authority error, got {errors}",
        )

    def test_trap_feedback_rejects_missing_movable_static_world_marker(self) -> None:
        payload = json.loads(json.dumps(self.repo_spec))
        payload["keywords"]["appendedRecords"] = [
            record
            for record in payload["keywords"]["appendedRecords"]
            if not (
                record.get("type") == "MovableStatic"
                and record.get("movableStatic", {}).get("editorId") == "CAFF_MSTT_TRAP_BEAR_VISUAL"
            )
        ]

        errors, _ = self._lint_payload(payload)
        self.assertTrue(
            any("references missing MovableStatic 'CAFF_MSTT_TRAP_BEAR_VISUAL'" in error for error in errors),
            msg=f"expected missing movable static marker error, got {errors}",
        )

    def test_trap_feedback_rejects_form_spec_for_movable_static_world_marker(self) -> None:
        payload = json.loads(json.dumps(self.repo_spec))
        bear = next(
            affix for affix in payload["keywords"]["affixes"] if affix["id"] == "bear_trap"
        )
        bear["runtime"]["action"]["trapFeedback"]["markerWorldObjectEditorId"] = (
            "CalamityAffixes.esp|0x000B00"
        )

        errors, _ = self._lint_payload(payload)
        self.assertTrue(
            any("markerWorldObjectEditorId must be an EditorID" in error for error in errors),
            msg=f"expected EditorID-only marker lookup error, got {errors}",
        )

    def test_world_marker_animation_requires_world_object_marker(self) -> None:
        payload = json.loads(json.dumps(self.repo_spec))
        bear = next(
            affix for affix in payload["keywords"]["affixes"] if affix["id"] == "bear_trap"
        )
        feedback = bear["runtime"]["action"]["trapFeedback"]
        feedback["markerArtObjectEditorId"] = "CAFF_ARTO_VFX_TRAP_BEAR_MARKER"
        del feedback["markerWorldObjectEditorId"]

        errors, _ = self._lint_payload(payload)
        self.assertTrue(
            any("worldMarkerAnimation is supported only with markerWorldObjectEditorId" in error for error in errors),
            msg=f"expected world marker animation authority error, got {errors}",
        )

    def test_world_marker_animation_requires_complete_valid_payload(self) -> None:
        payload = json.loads(json.dumps(self.repo_spec))
        bear = next(
            affix for affix in payload["keywords"]["affixes"] if affix["id"] == "bear_trap"
        )
        animation = bear["runtime"]["action"]["trapFeedback"]["worldMarkerAnimation"]
        animation.pop("rearmEvent")
        animation["triggerEvent"] = "  "
        animation["openGateMilliseconds"] = 5001

        errors, _ = self._lint_payload(payload)
        self.assertTrue(
            any("triggerEvent must be a non-empty string" in error for error in errors),
            msg=f"expected empty trigger event error, got {errors}",
        )
        self.assertTrue(
            any("rearmEvent must be a non-empty string" in error for error in errors),
            msg=f"expected missing rearm event error, got {errors}",
        )
        self.assertTrue(
            any("openGateMilliseconds must be an integer in range 0..5000" in error for error in errors),
            msg=f"expected open gate range error, got {errors}",
        )

    def test_world_marker_animation_rejects_fractional_open_gate(self) -> None:
        payload = json.loads(json.dumps(self.repo_spec))
        bear = next(
            affix for affix in payload["keywords"]["affixes"] if affix["id"] == "bear_trap"
        )
        animation = bear["runtime"]["action"]["trapFeedback"]["worldMarkerAnimation"]
        animation["openGateMilliseconds"] = 900.5

        errors, _ = self._lint_payload(payload)
        self.assertTrue(
            any("openGateMilliseconds must be an integer in range 0..5000" in error for error in errors),
            msg=f"expected fractional open gate error, got {errors}",
        )

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

    def test_generated_sync_detects_deleted_field_still_present(self) -> None:
        spec_payload = {
            "keywords": {
                "affixes": [
                    {
                        "id": "affix_test",
                        "editorId": "CAFF_AFFIX_TEST",
                        "runtime": {
                            "trigger": "Hit",
                            "action": {"type": "DebugNotify"},
                        },
                    }
                ]
            }
        }
        generated_payload = json.loads(json.dumps(spec_payload))
        generated_payload["keywords"]["affixes"][0]["runtime"]["procChancePercent"] = 35.0

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
                "$.keywords.affixes[0].runtime.procChancePercent (unexpected key in generated)"
                in error
                for error in errors
            ),
            msg=f"expected deleted-field mismatch, got errors={errors} warnings={warnings}",
        )

    def test_generated_sync_normalizes_generator_defaults(self) -> None:
        spec_payload = {
            "version": 1,
            "modKey": "CalamityAffixes.esp",
            "eslFlag": True,
            "loot": {},
            "keywords": {
                "tags": [],
                "affixes": [
                    {
                        "id": "affix_test",
                        "editorId": "CAFF_AFFIX_TEST",
                        "records": {
                            "magicEffect": {
                                "editorId": "CAFF_MGEF_TEST",
                                "actorValue": "Health",
                            }
                        },
                        "runtime": {
                            "trigger": "Hit",
                            "action": {"type": "DebugNotify"},
                        },
                    }
                ],
                "kidRules": [],
                "spidRules": [],
            },
        }
        generated_payload = json.loads(json.dumps(spec_payload))
        generated_payload["loot"].update(
            {
                "chancePercent": 0.0,
                "runewordFragmentChancePercent": 8.0,
                "reforgeOrbChancePercent": 12.0,
                "uniqueActorGuaranteedRunewordChancePercent": 40.0,
                "currencyDropMode": "hybrid",
                "lootSourceChanceMultCorpse": 1.0,
                "lootSourceChanceMultContainer": 1.0,
                "lootSourceChanceMultBossContainer": 1.15,
                "lootSourceChanceMultWorld": 1.0,
                "renameItem": False,
                "sharedPool": False,
                "debugLog": False,
                "dotTagSafetyAutoDisable": False,
                "dotTagSafetyUniqueEffectThreshold": 96,
                "trapGlobalMaxActive": 64,
                "trapCastBudgetPerTick": 8,
                "triggerProcBudgetPerWindow": 12,
                "triggerProcBudgetWindowMs": 100,
                "cleanupInvalidLegacyAffixes": True,
                "stripTrackedSuffixSlots": True,
            }
        )
        generated_payload["keywords"]["appendedMagicEffects"] = []
        generated_payload["keywords"]["appendedRecords"] = []
        generated_payload["keywords"]["affixes"][0]["records"]["magicEffect"].update(
            {"hostile": False, "recover": False}
        )

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

        self.assertEqual([], errors)
        self.assertEqual([], warnings)

    def test_schema_rejects_runtime_and_action_typos(self) -> None:
        mutations = {
            "runtime typo": ("runtime", "procChnacePercent", 100.0),
            "action typo": ("action", "magnitudeScalling", {}),
        }
        for label, (target, key, value) in mutations.items():
            with self.subTest(label=label):
                payload = json.loads(json.dumps(self.repo_spec))
                runtime = payload["keywords"]["affixes"][0]["runtime"]
                destination = runtime if target == "runtime" else runtime["action"]
                destination[key] = value
                errors: list[str] = []
                self.lint_affixes._validate_schema(
                    instance=payload,
                    schema=self.schema,
                    label="test",
                    errors=errors,
                )
                self.assertTrue(
                    any("Additional properties are not allowed" in error and key in error for error in errors),
                    msg=f"expected schema rejection for {key}, got errors={errors}",
                )

    def test_schema_rejects_wrong_runtime_type_and_missing_action_type(self) -> None:
        wrong_type = json.loads(json.dumps(self.repo_spec))
        wrong_type["keywords"]["affixes"][0]["runtime"]["procChancePercent"] = "100"
        wrong_action_type = json.loads(json.dumps(self.repo_spec))
        wrong_action_type["keywords"]["affixes"][0]["runtime"]["action"]["magnitudeScaling"] = "scaled"
        missing_type = json.loads(json.dumps(self.repo_spec))
        del missing_type["keywords"]["affixes"][0]["runtime"]["action"]["type"]
        missing_cast_spell = json.loads(json.dumps(self.repo_spec))
        del missing_cast_spell["keywords"]["affixes"][0]["runtime"]["action"]["spellEditorId"]

        for label, payload, expected_fragment in (
            ("wrong type", wrong_type, "is not of type 'number'"),
            ("wrong action type", wrong_action_type, "is not of type 'object'"),
            ("missing type", missing_type, "'type' is a required property"),
            ("missing CastSpell spell", missing_cast_spell, "is not valid under any of the given schemas"),
        ):
            with self.subTest(label=label):
                errors: list[str] = []
                self.lint_affixes._validate_schema(
                    instance=payload,
                    schema=self.schema,
                    label="test",
                    errors=errors,
                )
                self.assertTrue(
                    any(expected_fragment in error for error in errors),
                    msg=f"expected {label} schema rejection, got errors={errors}",
                )

    def test_schema_restricts_normal_weapon_hit_chance_to_runtime_consumers(self) -> None:
        payloads: list[tuple[str, dict]] = []

        debug_payload = json.loads(json.dumps(self.repo_spec))
        debug_runtime = next(
            affix["runtime"]
            for affix in debug_payload["keywords"]["affixes"]
            if affix.get("runtime", {}).get("action", {}).get("type") == "DebugNotify"
        )
        debug_runtime["normalWeaponHitProcChancePercent"] = 5.0
        payloads.append(("DebugNotify", debug_payload))

        trap_payload = json.loads(json.dumps(self.repo_spec))
        trap_runtime = next(
            affix["runtime"]
            for affix in trap_payload["keywords"]["affixes"]
            if affix.get("runtime", {}).get("action", {}).get("type") == "SpawnTrap"
            and not affix["runtime"]["action"].get("requireCritOrPowerAttack", False)
        )
        trap_runtime["normalWeaponHitProcChancePercent"] = 5.0
        payloads.append(("non-crit-gated SpawnTrap", trap_payload))

        for label, payload in payloads:
            with self.subTest(label=label):
                errors: list[str] = []
                self.lint_affixes._validate_schema(
                    instance=payload,
                    schema=self.schema,
                    label="test",
                    errors=errors,
                )
                self.assertTrue(
                    any("is not valid under any of the given schemas" in error for error in errors),
                    msg=f"expected schema rejection for {label}, got errors={errors}",
                )

    def test_schema_rejects_non_boolean_movable_static_animation_update_flag(self) -> None:
        payload = json.loads(json.dumps(self.repo_spec))
        bear_marker = next(
            record["movableStatic"]
            for record in payload["keywords"]["appendedRecords"]
            if record.get("type") == "MovableStatic"
            and record.get("movableStatic", {}).get("editorId") == "CAFF_MSTT_TRAP_BEAR_VISUAL"
        )
        bear_marker["mustUpdateAnimations"] = "true"

        errors: list[str] = []
        self.lint_affixes._validate_schema(
            instance=payload,
            schema=self.schema,
            label="test",
            errors=errors,
        )

        self.assertTrue(
            any(
                "mustUpdateAnimations" in error
                and ("is not of type 'boolean'" in error or "is not valid under any" in error)
                for error in errors
            ),
            msg=f"expected schema boolean rejection, got errors={errors}",
        )

    def test_schema_accepts_corpse_explosion_runtime_default_max_targets(self) -> None:
        payload = json.loads(json.dumps(self.repo_spec))
        corpse_action = next(
            affix["runtime"]["action"]
            for affix in payload["keywords"]["affixes"]
            if affix.get("runtime", {}).get("action", {}).get("type")
            in {"CorpseExplosion", "SummonCorpseExplosion"}
        )
        corpse_action.pop("maxTargets", None)

        errors: list[str] = []
        self.lint_affixes._validate_schema(
            instance=payload,
            schema=self.schema,
            label="test",
            errors=errors,
        )

        self.assertEqual([], errors)

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
