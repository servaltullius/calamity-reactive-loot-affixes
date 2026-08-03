#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class QaSkseLogcheckTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.logcheck = cls.repo_root / "tools" / "qa_skse_logcheck.py"

    def _run(self, *extra_lines: str, strict: bool = False) -> subprocess.CompletedProcess[str]:
        lines = [
            "[info] CalamityAffixes: plugin loaded, waiting for kDataLoaded.",
            "[info] CalamityAffixes: runtime config loaded (affixes=243, prefixWeapon=50).",
            "[info] CalamityAffixes: installed HandleHealthDamage vfunc hooks.",
            "[info] CalamityAffixes: PrismaUI API acquired.",
            "[info] CalamityAffixes: TrapSystem enabled.",
            *extra_lines,
        ]
        with tempfile.TemporaryDirectory(prefix="caff-logcheck-") as temp_dir:
            log_path = Path(temp_dir) / "CalamityAffixes.log"
            log_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
            command = [sys.executable, str(self.logcheck), "--log", str(log_path)]
            if strict:
                command.append("--strict")
            return subprocess.run(
                command,
                cwd=self.repo_root,
                text=True,
                capture_output=True,
                check=False,
            )

    def test_valid_startup_log_passes(self) -> None:
        result = self._run()

        self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("required_ok: 5 / 5", result.stdout)
        self.assertIn("RESULT: PASS", result.stdout)

    def test_generic_error_is_detected(self) -> None:
        result = self._run("[error] CalamityAffixes: synthetic failure")

        self.assertEqual(result.returncode, 1, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("generic_error", result.stdout)
        self.assertIn("RESULT: FAIL", result.stdout)

    def test_no_affixes_loaded_is_detected(self) -> None:
        result = self._run("[error] CalamityAffixes: no affixes loaded.")

        self.assertEqual(result.returncode, 1, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("no_affixes_loaded", result.stdout)
        self.assertIn("RESULT: FAIL", result.stdout)

    def test_warning_is_nonfatal_unless_strict(self) -> None:
        relaxed = self._run("[warn] CalamityAffixes: synthetic warning")
        strict = self._run("[warn] CalamityAffixes: synthetic warning", strict=True)

        self.assertEqual(relaxed.returncode, 0, msg=f"stdout={relaxed.stdout}\nstderr={relaxed.stderr}")
        self.assertIn("generic_warn", relaxed.stdout)
        self.assertIn("RESULT: WARN", relaxed.stdout)
        self.assertEqual(strict.returncode, 1, msg=f"stdout={strict.stdout}\nstderr={strict.stderr}")
        self.assertIn("RESULT: FAIL (strict warnings)", strict.stdout)


if __name__ == "__main__":
    unittest.main()
