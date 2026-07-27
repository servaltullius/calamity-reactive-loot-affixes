#!/usr/bin/env python3
"""Read the Prisma panel view as one text buffer, whatever it is split across.

The view used to be a single `index.html` holding markup, a `<style>` block and
a `<script>` block, and every check that inspects it grew up reading that one
file. Splitting it into `styles/*.css` and `scripts/*.js` would break those
checks in two ways -- one loud, one silent:

  - positive assertions stop finding text that moved out of index.html
  - **negative** assertions (`assertNotIn`, `assertNotRegex`) start passing for
    the wrong reason, because the text they forbid is no longer in the buffer
    they search. Coverage disappears without a single test failing.

This module removes the difference: it returns index.html with each referenced
stylesheet and script inlined at the point of reference, so a check sees the
same text before and after the split. It exists to make the second failure mode
impossible, not merely to save edits.

Load order comes from the `<link>` and `<script src>` tags themselves. That is
the same order the browser applies, and it keeps the HTML the single source of
truth -- a separate list here could drift from what actually loads.
"""

from __future__ import annotations

import re
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
VIEW_DIR = REPO_ROOT / "Data" / "PrismaUI" / "views" / "CalamityAffixes"
INDEX_PATH = VIEW_DIR / "index.html"

# Deliberately narrow: only local relative references are inlined. An absolute
# or protocol URL is not part of the view's own source and must not be fetched.
LINK_RE = re.compile(r"""<link\b[^>]*\bhref\s*=\s*["']([^"':]+?)["'][^>]*>""", re.IGNORECASE)
SCRIPT_SRC_RE = re.compile(
    r"""<script\b[^>]*\bsrc\s*=\s*["']([^"':]+?)["'][^>]*>\s*</script\s*>""", re.IGNORECASE
)


def _read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def stylesheet_paths(view_dir: Path = VIEW_DIR) -> list[Path]:
    """Referenced stylesheets, in document order."""
    return [view_dir / rel for rel in LINK_RE.findall(_read(view_dir / "index.html"))]


def script_paths(view_dir: Path = VIEW_DIR) -> list[Path]:
    """Referenced scripts, in document order -- which is execution order."""
    return [view_dir / rel for rel in SCRIPT_SRC_RE.findall(_read(view_dir / "index.html"))]


def load_styles(view_dir: Path = VIEW_DIR) -> str:
    """Every CSS rule the view applies, in cascade order.

    Includes any inline `<style>` body, so this keeps working while the split is
    in progress and after it is finished.
    """
    source = _read(view_dir / "index.html")
    inline = "\n".join(re.findall(r"<style\b[^>]*>(.*?)</style\s*>", source, re.DOTALL | re.IGNORECASE))
    external = "\n".join(_read(path) for path in stylesheet_paths(view_dir))
    return "\n".join(part for part in (inline, external) if part)


def load_scripts(view_dir: Path = VIEW_DIR) -> str:
    """Every line of JS the view runs, in execution order."""
    source = _read(view_dir / "index.html")
    inline = "\n".join(
        re.findall(r"<script(?![^>]*\bsrc\b)[^>]*>(.*?)</script\s*>", source, re.DOTALL | re.IGNORECASE)
    )
    external = "\n".join(_read(path) for path in script_paths(view_dir))
    return "\n".join(part for part in (inline, external) if part)


def load_view_source(view_dir: Path = VIEW_DIR) -> str:
    """index.html with every referenced stylesheet and script inlined in place.

    Substituting at the reference site rather than appending keeps document
    order intact, so slicing helpers that read "from marker A to marker B" get
    the same span they got from the unsplit file.
    """
    source = _read(view_dir / "index.html")

    def inline_link(match: re.Match[str]) -> str:
        return "<style>\n" + _read(view_dir / match.group(1)) + "\n</style>"

    def inline_script(match: re.Match[str]) -> str:
        return "<script>\n" + _read(view_dir / match.group(1)) + "\n</script>"

    source = LINK_RE.sub(inline_link, source)
    return SCRIPT_SRC_RE.sub(inline_script, source)


if __name__ == "__main__":
    print(load_view_source(), end="")
