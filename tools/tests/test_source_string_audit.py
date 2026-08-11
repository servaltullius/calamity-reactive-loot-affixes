#!/usr/bin/env python3
"""The brittle-pin ratchet must see every check that pins source text.

`audit_source_string_tests.py` finds those checks by looking for how they read
the source. When the Prisma panel checks stopped calling ifstream directly and
started calling a shared loader, the auditor stopped recognising them: 80 pins
left the report, the then-current ratchet passed with room to spare, and nothing went
red. A ratchet that quietly loosens is worse than no ratchet, so the recognition
rule gets its own test.
"""

from __future__ import annotations

import importlib.util
import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
TESTS_DIR = REPO_ROOT / "skse" / "CalamityAffixes" / "tests"
MAX_BRITTLE_PINS = 316


def _load_audit_module():
    path = REPO_ROOT / "tools" / "audit_source_string_tests.py"
    spec = importlib.util.spec_from_file_location("audit_source_string_tests", path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _check_body(name: str) -> str:
    """The body of one gate check, sliced the way the auditor slices it."""
    for path in sorted(TESTS_DIR.glob("runtime_gate_store_checks_*.cpp")):
        text = path.read_text(encoding="utf-8")
        headers = list(re.finditer(r"^\s*bool (Check\w+)\(\)\s*$", text, re.MULTILINE))
        for index, header in enumerate(headers):
            if header.group(1) != name:
                continue
            end = headers[index + 1].start() if index + 1 < len(headers) else len(text)
            return text[header.end() : end]
    raise AssertionError(f"no gate check named {name}")


class SourceStringAuditTests(unittest.TestCase):
    # Reads the Prisma view through the shared loader, and opens nothing itself.
    LOADER_CHECK = "CheckPrismaPanelUxFlowPolicy"

    def test_the_sample_check_really_does_not_use_ifstream(self) -> None:
        """Guards the guard: if this check went back to ifstream, the test below
        would pass for the wrong reason and prove nothing."""
        body = _check_body(self.LOADER_CHECK)
        self.assertNotIn("ifstream", body)
        self.assertIn("LoadPrismaViewSource", body)

    def test_checks_that_read_the_view_through_the_loader_are_counted(self) -> None:
        findings = _load_audit_module().scan()
        counted = {finding["check"] for finding in findings}
        self.assertIn(self.LOADER_CHECK, counted)

        pins = next(f for f in findings if f["check"] == self.LOADER_CHECK)
        self.assertTrue(
            pins["brittle"] or pins["structural"],
            "the check was recognised but none of its pins were collected",
        )

    def test_brittle_pin_count_does_not_increase(self) -> None:
        findings = _load_audit_module().scan()
        brittle_total = sum(len(finding["brittle"]) for finding in findings)
        self.assertLessEqual(
            brittle_total,
            MAX_BRITTLE_PINS,
            "replace source-statement pins with behavior or structural checks; "
            "do not raise the ratchet",
        )

    def test_every_marker_is_a_way_a_check_actually_reads_source(self) -> None:
        """A marker that matches nothing is dead weight that hides a rename."""
        module = _load_audit_module()
        bodies = "".join(
            path.read_text(encoding="utf-8")
            for path in sorted(TESTS_DIR.glob("runtime_gate_store_checks_*.cpp"))
        )
        for marker in module.SOURCE_TEXT_MARKERS:
            with self.subTest(marker=marker):
                self.assertIn(marker, bodies)


if __name__ == "__main__":
    unittest.main()
