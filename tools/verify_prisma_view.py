#!/usr/bin/env python3
"""Check that the split Prisma panel view still loads everything it needs.

The view is `index.html` plus `styles/*.css` and `scripts/*.js`, wired together
by `<link>` and `<script src>` tags. Nothing in the C++ plugin knows about that
structure -- it passes one path to `CreateView` and the browser does the rest --
so the failure modes here are invisible to every other check in the repository:

  - a referenced file that does not exist: the browser skips it and the panel
    comes up unstyled or half-dead, while the log still records a successful
    `CreateView`
  - a file nobody references: dead weight that reads as live code, and a rename
    that silently drops a whole module
  - markup drifting back into one file: the split undone by accident
  - `type="module"`: changes top-level scope, so extraction stops being the
    order-preserving no-op the split depends on

`--reconstruct` prints index.html with every reference inlined, which is what
the byte-identity proof in the split commits compares against.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
VIEW_DIR = REPO_ROOT / "Data" / "PrismaUI" / "views" / "CalamityAffixes"

LINK_RE = re.compile(r"""<link\b[^>]*\bhref\s*=\s*["']([^"']+)["'][^>]*>""", re.IGNORECASE)
SCRIPT_SRC_RE = re.compile(r"""<script\b[^>]*\bsrc\s*=\s*["']([^"']+)["'][^>]*>""", re.IGNORECASE)
INLINE_STYLE_RE = re.compile(r"<style\b[^>]*>(.*?)</style\s*>", re.DOTALL | re.IGNORECASE)
INLINE_SCRIPT_RE = re.compile(
    r"<script(?![^>]*\bsrc\b)[^>]*>(.*?)</script\s*>", re.DOTALL | re.IGNORECASE
)
MODULE_RE = re.compile(r"""<script\b[^>]*\btype\s*=\s*["']module["']""", re.IGNORECASE)


def references(source: str) -> list[str]:
    return LINK_RE.findall(source) + SCRIPT_SRC_RE.findall(source)


def collect_problems(view_dir: Path) -> list[str]:
    index_path = view_dir / "index.html"
    if not index_path.is_file():
        return [f"missing view entry point: {index_path}"]

    source = index_path.read_text(encoding="utf-8")
    problems: list[str] = []

    referenced: set[Path] = set()
    for relative in references(source):
        if ":" in relative or relative.startswith("/"):
            problems.append(f"reference is not a local relative path: {relative}")
            continue
        target = view_dir / relative
        if not target.is_file():
            problems.append(f"referenced file does not exist: {relative}")
            continue
        referenced.add(target.resolve())

    for asset in sorted(view_dir.rglob("*")):
        if not asset.is_file() or asset.name == "index.html":
            continue
        if asset.resolve() not in referenced:
            problems.append(f"file is never loaded by index.html: {asset.relative_to(view_dir)}")

    for kind, pattern in (("style", INLINE_STYLE_RE), ("script", INLINE_SCRIPT_RE)):
        for body in pattern.findall(source):
            if body.strip():
                problems.append(
                    f"index.html still holds an inline <{kind}> body "
                    f"({len(body.splitlines())} lines); it belongs in a separate file"
                )

    if MODULE_RE.search(source):
        problems.append(
            'type="module" changes top-level scope, so the scripts would no longer '
            "share one lexical environment; the split relies on classic scripts"
        )

    return problems


def reconstruct(view_dir: Path) -> str:
    """index.html with each referenced file inlined where it is referenced.

    Delegates to the loader the tests read the view through. A second inlining
    implementation here could disagree with that one, and then this command
    would attest to a reconstruction nothing else ever sees.
    """
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from prisma_view_source import load_view_source

    return load_view_source(view_dir)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--reconstruct",
        action="store_true",
        help="print index.html with every referenced file inlined, and exit",
    )
    args = parser.parse_args()

    if args.reconstruct:
        sys.stdout.write(reconstruct(VIEW_DIR))
        return 0

    problems = collect_problems(VIEW_DIR)
    if problems:
        print("Prisma view structure is broken:", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        return 1

    loaded = references((VIEW_DIR / "index.html").read_text(encoding="utf-8"))
    print(f"Prisma view OK: index.html loads {len(loaded)} files, all present and all used.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
