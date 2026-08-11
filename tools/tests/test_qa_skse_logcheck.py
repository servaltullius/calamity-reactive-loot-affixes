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
            "[debug] CalamityAffixes: trap world marker spawn (base=CAFF_MSTT_TRAP_BEAR_VISUAL, baseForm=0xFE401950, ref=0xFF001234, handle=0x1234, pos=(0.0, 0.0, 0.0), forcePersist=false, handleAllocated=true, resolved=true, strongOwnerRetained=true, deferredCleanupQueued=false, retainedForCleanup=true, activationBlockIssued=true, collisionDisableIssued=true, spawned=true).",
            "[info] CalamityAffixes: trap world marker probe observation (affixId=bear_trap, marker=CAFF_MSTT_TRAP_BEAR_VISUAL, configured=true, handleAllocated=true, resolved=true, animationConfigured=true).",
            "[info] CalamityAffixes: trap world marker probe observation (affixId=rune_trap, marker=CAFF_MSTT_TRAP_RUNE_VISUAL, configured=true, handleAllocated=true, resolved=true, animationConfigured=true).",
            "[info] CalamityAffixes: trap world marker probe observation (affixId=plague_spore, marker=CAFF_MSTT_TRAP_PLAGUE_VISUAL, configured=true, handleAllocated=true, resolved=true, animationConfigured=false).",
            "[info] CalamityAffixes: trap world marker probe observation (affixId=tar_blight, marker=CAFF_MSTT_TRAP_TAR_VISUAL, configured=true, handleAllocated=true, resolved=true, animationConfigured=true).",
            "[info] CalamityAffixes: trap world marker probe observation (affixId=siphon_spore, marker=CAFF_MSTT_TRAP_SIPHON_VISUAL, configured=true, handleAllocated=true, resolved=true, animationConfigured=false).",
            "[info] CalamityAffixes: trap world marker probe observation (affixId=chaos_rune, marker=CAFF_MSTT_TRAP_CHAOS_VISUAL, configured=true, handleAllocated=true, resolved=true, animationConfigured=true).",
            "[debug] CalamityAffixes: trap world marker animation (phase=initial, event=StartOpen, ref=0xFF001234, cellAttached=true, has3D=true, graphReady=true, accepted=true, attempt=2 / 8, retryScheduled=false).",
            "[debug] CalamityAffixes: trap world marker cleanup (reason=expired, handle=0x1234, resolved=true, strongOwnerRetained=true, ref=0xFF001234, cellAttached=true, disableIssued=true, deleteIssued=true, deferredQueued=false).",
            "[debug] CalamityAffixes: trap world marker deferred cleanup resolved (reason=deferred-resolved, handle=0x1235, resolved=true, ref=0xFF001235, cellAttached=true, activationBlockIssued=true, collisionDisableIssued=true, disableIssued=true, deleteIssued=true).",
            "[debug] CalamityAffixes: pre-save trap cleanup complete (activeTraps=0, unresolvedDeferred=0).",
            "[debug] CalamityAffixes: serialization save trap state (activeTraps=0, unresolvedDeferred=0).",
        )

        self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("proc_dispatch: OBSERVED (1)", result.stdout)
        self.assertIn("magic_effect_apply: OBSERVED (1)", result.stdout)
        self.assertIn("feedback_art_accepted: OBSERVED (1)", result.stdout)
        self.assertIn("feedback_sound_accepted: OBSERVED (1)", result.stdout)
        self.assertIn("trap_world_marker_spawn: OBSERVED (1)", result.stdout)
        self.assertIn("trap_world_marker_probe: OBSERVED (6)", result.stdout)
        self.assertIn("trap world marker probe contracts: OBSERVED 6/6", result.stdout)
        self.assertIn("trap_world_marker_animation_accepted: OBSERVED (1)", result.stdout)
        self.assertIn("trap_world_marker_cleanup: OBSERVED (1)", result.stdout)
        self.assertIn("trap_world_marker_deferred_cleanup: OBSERVED (1)", result.stdout)
        self.assertIn("trap_pre_save_state_clean: OBSERVED (1)", result.stdout)
        self.assertIn("trap_serialization_state_clean: OBSERVED (1)", result.stdout)
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

    def test_trap_world_reference_failures_are_surfaced_without_visual_claims(self) -> None:
        result = self._run(
            "[debug] CalamityAffixes: trap world marker spawn (base=CAFF_MSTT_TRAP_BEAR_VISUAL, handleAllocated=true, resolved=false, spawned=false).",
            "[info] CalamityAffixes: trap world marker probe observation (affixId=bear_trap, configured=true, handleAllocated=true, resolved=false, animationConfigured=true).",
            "[warn] CalamityAffixes: trap world marker probe skipped (reason=logical-trap-headroom, active=47, requested=6, cap=48).",
            "[warn] CalamityAffixes: trap world marker animation abandoned (phase=initial, event=StartOpen, ref=0x0, attempt=8, reason=reference-unusable); gameplay continues fail-open.",
            "[debug] CalamityAffixes: trap world marker cleanup (reason=pre-save, handle=0x1234, resolved=false, strongOwnerRetained=false, ref=0x0, cellAttached=false, disableIssued=false, deleteIssued=false, deferredQueued=true).",
            "[debug] CalamityAffixes: pre-save trap cleanup complete (activeTraps=0, unresolvedDeferred=1).",
            "[debug] CalamityAffixes: serialization save trap state (activeTraps=0, unresolvedDeferred=1).",
        )

        self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("trap_world_marker_spawn_rejected: 1 sample(s)", result.stdout)
        self.assertIn("trap_world_marker_probe_unresolved: 1 sample(s)", result.stdout)
        self.assertIn("trap_world_marker_probe_skipped: 1 sample(s)", result.stdout)
        self.assertIn("trap world marker probe contracts: OBSERVED 0/6", result.stdout)
        self.assertIn("trap_world_marker_animation_abandoned: 1 sample(s)", result.stdout)
        self.assertIn("trap_world_marker_cleanup_unresolved: 1 sample(s)", result.stdout)
        self.assertIn("trap_pre_save_state_not_clean: 1 sample(s)", result.stdout)
        self.assertIn("trap_serialization_state_not_clean: 1 sample(s)", result.stdout)
        self.assertIn("engine-path only; NOT an on-screen visual or audible-audio verdict", result.stdout)
        self.assertIn("RESULT: WARN", result.stdout)

    def test_legacy_particle_probe_does_not_count_as_world_reference_observation(self) -> None:
        result = self._run(
            "[info] CalamityAffixes: trap marker probe (variant=bear:runefrost, model=Magic\\RuneFrostProjectile01.nif, spawned=true).",
        )

        self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("trap_world_marker_probe: NOT OBSERVED", result.stdout)
        self.assertNotIn("trap_world_marker_probe: OBSERVED", result.stdout)
        self.assertIn("trap world marker probe contracts: OBSERVED 0/6", result.stdout)
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
