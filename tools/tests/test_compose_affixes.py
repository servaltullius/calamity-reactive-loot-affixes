#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import tempfile
import unittest
from pathlib import Path


class ComposeAffixesWorkflowTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.script_path = cls.repo_root / "tools" / "compose_affixes.py"

    def _run_compose(self, *args: str) -> subprocess.CompletedProcess[str]:
        cmd = ["python3", str(self.script_path), *args]
        return subprocess.run(
            cmd,
            cwd=self.repo_root,
            text=True,
            capture_output=True,
            check=False,
        )

    @staticmethod
    def _write_json(path: Path, payload: object) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    def _build_fixture(self, temp_root: Path) -> tuple[Path, Path]:
        modules_dir = temp_root / "modules"
        manifest_path = temp_root / "affixes.modules.json"
        output_path = temp_root / "affixes.json"

        self._write_json(
            modules_dir / "spec.root.json",
            {
                "version": 1,
                "modKey": "CalamityAffixes.esp",
                "eslFlag": True,
                "keywords": {},
            },
        )
        self._write_json(modules_dir / "keywords.tags.json", [{"editorId": "CAFF_TAG_DOT", "name": "dot"}])
        self._write_json(
            modules_dir / "keywords.affixes.json",
            [{"id": "affix_test", "editorId": "CAFF_AFFIX_TEST", "runtime": {"trigger": "Hit", "action": {"type": "DebugNotify"}}}],
        )
        self._write_json(
            modules_dir / "keywords.kidRules.json",
            [{"type": "Weapon", "strings": "NONE", "formFilters": "NONE", "traits": "-E", "chance": 100.0}],
        )
        self._write_json(
            modules_dir / "keywords.spidRules.json",
            [{"line": "DeathItem = CAFF_LItem_RunewordFragmentDrops|ActorTypeNPC|NONE|NONE|NONE|1|100"}],
        )

        self._write_json(
            manifest_path,
            {
                "root": "modules/spec.root.json",
                "keywords": {
                    "tags": ["modules/keywords.tags.json"],
                    "affixes": ["modules/keywords.affixes.json"],
                    "kidRules": ["modules/keywords.kidRules.json"],
                    "spidRules": ["modules/keywords.spidRules.json"],
                },
            },
        )
        return manifest_path, output_path

    def test_compose_writes_expected_output(self) -> None:
        with tempfile.TemporaryDirectory(prefix="compose-affixes-") as temp_dir:
            temp_root = Path(temp_dir)
            manifest_path, output_path = self._build_fixture(temp_root)

            result = self._run_compose("--manifest", str(manifest_path), "--output", str(output_path))
            self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
            self.assertIn("Wrote composed spec", result.stdout)

            payload = json.loads(output_path.read_text(encoding="utf-8"))
            keywords = payload["keywords"]
            self.assertEqual(len(keywords["tags"]), 1)
            self.assertEqual(len(keywords["affixes"]), 1)
            self.assertEqual(len(keywords["kidRules"]), 1)
            self.assertEqual(len(keywords["spidRules"]), 1)

    def test_check_detects_stale_output(self) -> None:
        with tempfile.TemporaryDirectory(prefix="compose-affixes-") as temp_dir:
            temp_root = Path(temp_dir)
            manifest_path, output_path = self._build_fixture(temp_root)

            first = self._run_compose("--manifest", str(manifest_path), "--output", str(output_path))
            self.assertEqual(first.returncode, 0, msg=f"stdout={first.stdout}\nstderr={first.stderr}")

            tags_path = temp_root / "modules" / "keywords.tags.json"
            tags_payload = json.loads(tags_path.read_text(encoding="utf-8"))
            tags_payload.append({"editorId": "CAFF_TAG_EXTRA", "name": "extra"})
            self._write_json(tags_path, tags_payload)

            stale = self._run_compose(
                "--manifest",
                str(manifest_path),
                "--output",
                str(output_path),
                "--check",
            )
            self.assertEqual(stale.returncode, 2, msg=f"stdout={stale.stdout}\nstderr={stale.stderr}")
            self.assertIn("does not match output file", stale.stderr)

            refresh = self._run_compose("--manifest", str(manifest_path), "--output", str(output_path))
            self.assertEqual(refresh.returncode, 0, msg=f"stdout={refresh.stdout}\nstderr={refresh.stderr}")

            updated = json.loads(output_path.read_text(encoding="utf-8"))
            self.assertEqual(len(updated["keywords"]["tags"]), 2)


if __name__ == "__main__":
    unittest.main()
