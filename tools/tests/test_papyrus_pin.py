#!/usr/bin/env python3
"""Guards for tools/verify_papyrus_pin.py.

The pin is what lets a release be packaged without the Creation Kit's Papyrus
compiler, so every way a committed .pex can go stale has to actually fail here.
A check that silently stopped firing would hand CI a green light to ship a .pex
that no longer matches its .psc -- exactly the outcome the pin exists to
prevent. Each test therefore tampers with one input and asserts the mismatch is
reported, and the tamper is undone by using a throwaway tree.
"""

from __future__ import annotations

import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]


def _load_module():
    path = REPO_ROOT / "tools" / "verify_papyrus_pin.py"
    spec = importlib.util.spec_from_file_location("verify_papyrus_pin", path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class PapyrusPinRealRepoTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.mod = _load_module()

    def test_pin_holds_on_a_clean_checkout(self) -> None:
        """The whole point of the tool: a clean tree must verify."""
        pin = json.loads(self.mod.PIN_FILE.read_text(encoding="utf-8"))
        self.assertEqual([], self.mod.collect_mismatches(pin))

    def test_pin_covers_exactly_the_scripts_compile_papyrus_builds(self) -> None:
        """Drift between the two lists would leave an output unpinned.

        verify_papyrus_pin.py duplicates compile_papyrus.sh's target list because
        the shell script is not importable. Pin the agreement so adding a fourth
        script to one and not the other is caught here rather than by shipping an
        unverified .pex.
        """
        script = (REPO_ROOT / "tools" / "compile_papyrus.sh").read_text(encoding="utf-8")
        for source, output in self.mod.TARGETS:
            self.assertIn(Path(source).name, script)
            self.assertIn(Path(output).name, script)

        pin = json.loads(self.mod.PIN_FILE.read_text(encoding="utf-8"))
        self.assertEqual(
            [t[1] for t in self.mod.TARGETS],
            [entry["output"] for entry in pin["targets"]],
        )

    def test_pin_and_zip_verifier_expect_the_same_pex_set(self) -> None:
        """Three files name this list; only agreement makes any of them mean anything.

        verify_mo2_zip.py rejects a package whose PEX set is not exactly these
        three. If the pin ever covered a different set, one of the two would
        wave through what the other rejects, and which one bit you would depend
        on whether a compiler happened to be present.
        """
        source = (REPO_ROOT / "tools" / "verify_mo2_zip.py").read_text(encoding="utf-8")
        for _, output in self.mod.TARGETS:
            self.assertIn(f"/Scripts/{Path(output).name}", source)
        self.assertEqual(3, source.count("/Scripts/CalamityAffixes_"))

    def test_every_stub_on_the_import_path_is_recorded(self) -> None:
        pin = json.loads(self.mod.PIN_FILE.read_text(encoding="utf-8"))
        recorded = {entry["path"] for entry in pin["shared_inputs"]}
        self.assertEqual(set(self.mod.shared_input_paths()), recorded)
        self.assertIn("tools/papyrus-stubs/Calamity_Papyrus_Flags.flg", recorded)

    def test_cli_exits_zero_on_the_real_repo(self) -> None:
        result = subprocess.run(
            ["python3", str(REPO_ROOT / "tools" / "verify_papyrus_pin.py"), "--quiet"],
            capture_output=True,
        )
        self.assertEqual(0, result.returncode, msg=result.stderr.decode())
        self.assertEqual(b"", result.stdout)


class PapyrusPinTamperTests(unittest.TestCase):
    """Each check, proven non-vacuous against a synthetic tree."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.mod = _load_module()

    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)

        (self.root / "Data" / "Scripts" / "Source").mkdir(parents=True)
        (self.root / self.mod.STUB_DIR).mkdir(parents=True)

        for source, output in self.mod.TARGETS:
            (self.root / source).write_bytes(b"ScriptName Whatever\n")
            (self.root / output).write_bytes(b"\xfaW\xc0\xde compiled\n")
        for name in ("Calamity_Papyrus_Flags.flg", "SKI_ConfigBase.psc"):
            (self.root / self.mod.STUB_DIR / name).write_bytes(b"stub\n")

        self.pin = self.mod.build_pin(self.root)

    def assert_reports(self, needle: str) -> None:
        problems = self.mod.collect_mismatches(self.pin, self.root)
        self.assertTrue(
            any(needle in p for p in problems),
            msg=f"expected a problem mentioning {needle!r}, got {problems}",
        )

    def test_baseline_tree_is_clean(self) -> None:
        # Without this, every tamper test below could be passing for the wrong
        # reason -- a tree that never verified in the first place.
        self.assertEqual([], self.mod.collect_mismatches(self.pin, self.root))

    def test_edited_source_is_reported(self) -> None:
        """The headline case: .psc edited, .pex not recompiled."""
        (self.root / self.mod.TARGETS[0][0]).write_bytes(b"ScriptName Whatever\nInt x\n")
        self.assert_reports("source changed")

    def test_edited_output_is_reported(self) -> None:
        (self.root / self.mod.TARGETS[1][1]).write_bytes(b"tampered")
        self.assert_reports("output changed")

    def test_edited_stub_is_reported(self) -> None:
        """Stubs are on the import path, so editing one changes the output."""
        (self.root / self.mod.STUB_DIR / "SKI_ConfigBase.psc").write_bytes(b"stub\nchanged\n")
        self.assert_reports("import-path input changed")

    def test_edited_flags_file_is_reported(self) -> None:
        (self.root / self.mod.STUB_DIR / "Calamity_Papyrus_Flags.flg").write_bytes(b"other\n")
        self.assert_reports("Calamity_Papyrus_Flags.flg")

    def test_added_stub_is_reported(self) -> None:
        """A new import-path file can change the output even if nothing else did."""
        (self.root / self.mod.STUB_DIR / "SKI_QuestBase.psc").write_bytes(b"new\n")
        self.assert_reports("not recorded in the pin")

    def test_missing_source_is_reported(self) -> None:
        (self.root / self.mod.TARGETS[0][0]).unlink()
        self.assert_reports("missing source")

    def test_missing_output_is_reported(self) -> None:
        (self.root / self.mod.TARGETS[2][1]).unlink()
        self.assert_reports("missing output")

    def test_stray_pex_is_reported(self) -> None:
        """Packaging copies Data/ wholesale, so an unpinned .pex would ship.

        compile_papyrus.sh deletes known-legacy outputs before writing, but on
        the compiler-less path nothing runs to do that. Requiring the set to be
        exactly the pinned three is what makes the fallback safe without
        duplicating that cleanup list.
        """
        (self.root / self.mod.PEX_DIR / "CalamityAffixes_AffixManager.pex").write_bytes(b"old\n")
        self.assert_reports("unpinned .pex present")


if __name__ == "__main__":
    unittest.main()
