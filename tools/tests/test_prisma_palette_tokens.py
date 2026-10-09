#!/usr/bin/env python3
"""The panel's colours live in one palette block (styles/base.css :root).

v2.3.0 replaced about 300 colour literals with rgba(var(--<role>-rgb), alpha) so
the panel could match the player's UI theme by editing one place. A literal that
creeps back in stays the old colour whenever the palette changes, which is how
the neon green survived in corners before. Shadow black is the one exception.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
VIEW_DIR = REPO_ROOT / "Data" / "PrismaUI" / "views" / "CalamityAffixes"
COLOUR_LITERAL = re.compile(r"rgba?\(\s*\d+\s*,\s*\d+\s*,\s*\d+[^)]*\)|#[0-9a-fA-F]{3,8}\b")
SHADOW_BLACK = re.compile(r"rgba?\(\s*0\s*,\s*0\s*,\s*0\s*(,\s*[\d.]+\s*)?\)")
ROLE_REFERENCE = re.compile(r"var\(--([a-z-]+)-rgb\)")


def _palette_block(text: str) -> str:
    start = text.index(":root {")
    return text[start:text.index("}", start)]


class PrismaPaletteTokenTests(unittest.TestCase):
    def test_colours_outside_the_palette_use_tokens(self) -> None:
        offenders = []
        for path in sorted((VIEW_DIR / "styles").glob("*.css")):
            text = path.read_text(encoding="utf-8")
            if path.name == "base.css":
                text = text.replace(_palette_block(text), "")
            for match in COLOUR_LITERAL.finditer(text):
                if not SHADOW_BLACK.fullmatch(match.group(0)):
                    line = text.count("\n", 0, match.start()) + 1
                    offenders.append(f"{path.name}:{line}: {match.group(0)}")
        for path in [VIEW_DIR / "index.html", *sorted((VIEW_DIR / "scripts").glob("*.js"))]:
            text = path.read_text(encoding="utf-8")
            for match in COLOUR_LITERAL.finditer(text):
                if match.group(0).startswith("#") and not re.fullmatch(r"#[0-9a-fA-F]{6}", match.group(0)):
                    continue  # "#1" style ids and anchors are not colours
                if not SHADOW_BLACK.fullmatch(match.group(0)):
                    offenders.append(f"{path.name}: {match.group(0)}")
        self.assertEqual(offenders, [], "use rgba(var(--<role>-rgb), alpha) from styles/base.css")

    def test_every_referenced_role_is_defined(self) -> None:
        palette = _palette_block((VIEW_DIR / "styles" / "base.css").read_text(encoding="utf-8"))
        defined = set(re.findall(r"--([a-z-]+)-rgb\s*:", palette))
        used = set()
        for path in (VIEW_DIR / "styles").glob("*.css"):
            used |= set(ROLE_REFERENCE.findall(path.read_text(encoding="utf-8")))
        self.assertTrue(used, "the view should reference palette roles")
        self.assertEqual(sorted(used - defined), [], "a role used in the view is missing from the palette")


if __name__ == "__main__":
    unittest.main()
