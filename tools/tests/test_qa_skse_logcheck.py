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

    def test_observational_groups_report_counts_without_gating_result(self) -> None:
        result = self._run(
            "[debug] CalamityAffixes: proc (affixId=runeword_dream_final, trigger=0, chancePct=30, target=Troll, hasHitData=true)",
            "[debug] CalamityAffixes: CastSpellImmediate (affix=runeword_dream_final, spell=Shock Strike, magnitudeOverride=13.5, target=Troll).",
            "[debug] CalamityAffixes: magic effect apply observed (mgef=0xFE401915, caster=Prisoner (0x00000014), target=Troll (0x00107CAD)).",
            "[debug] CalamityAffixes: action feedback art (art=0x5B1BC, recipient=0x107CAD, instantiated=true).",
            "[debug] CalamityAffixes: action feedback sound (sound=0x3F206, spatial=true, built=true, played=true).",
        )

        self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("proc_dispatch: OBSERVED (1)", result.stdout)
        self.assertIn("magic_effect_apply: OBSERVED (1)", result.stdout)
        self.assertIn("feedback_art_accepted: OBSERVED (1)", result.stdout)
        self.assertIn("feedback_sound_accepted: OBSERVED (1)", result.stdout)
        # Engine-path observation must never be presented as a visual/audio verdict.
        self.assertIn("engine-path only; NOT an on-screen visual or audible-audio verdict", result.stdout)
        self.assertIn("RESULT: PASS", result.stdout)

    def test_missing_observational_groups_do_not_fail_the_run(self) -> None:
        result = self._run()

        self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("proc_dispatch: NOT OBSERVED", result.stdout)
        self.assertIn("no proc activity in scanned tail", result.stdout)
        self.assertIn("RESULT: PASS", result.stdout)

    def test_feedback_engine_rejections_are_surfaced_but_nonfatal(self) -> None:
        result = self._run(
            "[debug] CalamityAffixes: action feedback sound (sound=0x3F206, spatial=true, built=false).",
            "[debug] CalamityAffixes: action feedback art skipped (art=0x5B1BC, recipient=0x107CAD, is3DLoaded=false).",
        )

        self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("feedback_sound_rejected: 1 sample(s)", result.stdout)
        self.assertIn("feedback_art_skipped_no3d: 1 sample(s)", result.stdout)
        self.assertIn("RESULT: PASS", result.stdout)

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
