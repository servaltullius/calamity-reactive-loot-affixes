#!/usr/bin/env python3
"""`verify_prisma_view.py` is the only check that sees the split panel view.

Nothing else can: the plugin passes one path to `CreateView` and the browser
resolves the rest, so a broken `<link>` or `<script src>` produces a successful
log line and a panel that is unstyled or empty in game. These tests exist to
keep that checker honest -- each one breaks the view in a specific way and
requires the checker to say so.
"""

from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def _load_module():
    path = REPO_ROOT / "tools" / "verify_prisma_view.py"
    spec = importlib.util.spec_from_file_location("verify_prisma_view", path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


INDEX = """<!doctype html>
<html lang="en">
  <head>
    <link rel="stylesheet" href="styles/base.css" />
  </head>
  <body>
    <script src="scripts/dom.js"></script>
  </body>
</html>
"""


class PrismaViewStructureTests(unittest.TestCase):
    def setUp(self) -> None:
        self.module = _load_module()
        self._temp = tempfile.TemporaryDirectory(prefix="caff-prisma-view-")
        self.addCleanup(self._temp.cleanup)
        self.view = Path(self._temp.name)
        (self.view / "styles").mkdir()
        (self.view / "scripts").mkdir()
        (self.view / "index.html").write_text(INDEX, encoding="utf-8")
        (self.view / "styles" / "base.css").write_text(":root {}\n", encoding="utf-8")
        (self.view / "scripts" / "dom.js").write_text('"use strict";\n', encoding="utf-8")

    def problems(self) -> list[str]:
        return self.module.collect_problems(self.view)

    def test_a_well_formed_view_reports_nothing(self) -> None:
        """Guards the guard: without this, every test below could be passing
        because the fixture is broken in some unrelated way."""
        self.assertEqual([], self.problems())

    def test_a_reference_with_no_file_is_reported(self) -> None:
        (self.view / "scripts" / "dom.js").unlink()
        self.assertIn("referenced file does not exist: scripts/dom.js", self.problems())

    def test_a_file_with_no_reference_is_reported(self) -> None:
        (self.view / "scripts" / "orphan.js").write_text("//\n", encoding="utf-8")
        self.assertIn("file is never loaded by index.html: scripts/orphan.js", self.problems())

    def test_markup_drifting_back_into_index_html_is_reported(self) -> None:
        (self.view / "index.html").write_text(
            INDEX.replace("  </body>", "    <style>\n      .x { color: red; }\n    </style>\n  </body>"),
            encoding="utf-8",
        )
        self.assertTrue(
            any("inline <style> body" in problem for problem in self.problems()),
            self.problems(),
        )

    def test_a_module_script_is_reported(self) -> None:
        """`type="module"` gives each file its own top-level scope, so the
        scripts stop sharing one lexical environment and the split is no longer
        the order-preserving extraction everything else assumes."""
        (self.view / "index.html").write_text(
            INDEX.replace('<script src=', '<script type="module" src='), encoding="utf-8"
        )
        self.assertTrue(
            any('type="module"' in problem for problem in self.problems()),
            self.problems(),
        )

    def test_a_remote_reference_is_reported(self) -> None:
        """A strict-CSP view cannot fetch anything off-host, and a URL is not
        part of the view's own source in any case."""
        (self.view / "index.html").write_text(
            INDEX.replace('href="styles/base.css"', 'href="https://cdn.example.com/base.css"'),
            encoding="utf-8",
        )
        problems = self.problems()
        self.assertTrue(
            any("not a local relative path" in problem for problem in problems), problems
        )

    def test_the_real_view_passes(self) -> None:
        self.assertEqual([], self.module.collect_problems(self.module.VIEW_DIR))


if __name__ == "__main__":
    unittest.main()
