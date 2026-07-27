#!/usr/bin/env python3
"""Inventory the runtime-gate checks that assert on source text instead of behaviour.

Several gate checks open a production .cpp/.h with ifstream and assert that a
literal appears in it.  Those checks fall into two very different groups, and
only one of them is worth keeping:

  structural  A pinned declaration, type choice, or include -- "this helper must
              still exist", "_stateMutex must stay recursive".  The literal is
              the thing being guarded, so pinning the text is the point.  Cheap
              to keep true across refactors.

  brittle     A pinned call site with arguments, or a pinned statement body --
              "ProcessTrigger(Trigger::kIncomingHit, target, aggressor, hitData);".
              These block any rewording of code that is behaving correctly while
              proving nothing about the outcome, and they pass just as happily
              when the logic underneath is wrong.  Each one is a migration
              candidate: extract the decision into a pure function and assert the
              decision instead.

This script is the backlog for that migration.  It is deliberately a report
rather than a gate: run it to see what is left, and use --max-brittle in CI once
the count has been driven down, so the number can only go the right way.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = REPO_ROOT / "skse" / "CalamityAffixes" / "tests"

CHECK_RE = re.compile(r"^\s*bool (Check\w+)\(\)\s*$", re.MULTILINE)
# Literals fed to std::string::find(...) -- that is how every pin is written.
# Matched over the whole function body, not line by line: plenty of pins are
# wrapped across lines and a per-line scan silently misses those checks.
FIND_RE = re.compile(r'(?:\.|->)find\(\s*"((?:[^"\\]|\\.)*)"\s*\)', re.DOTALL)

# A pin is structural when the literal is a declaration/type/include rather than
# a fragment of executable code.
DECL_RE = re.compile(
    r"^\s*(?:#include\b"
    r"|(?:struct|class|enum|namespace|using|template)\b"
    r"|(?:static|inline|constexpr|const|virtual|explicit)\b"
    r"|[A-Za-z_][\w:]*[\w>](?:\s*[*&])?\s+[A-Za-z_]\w*\s*[({;]?\s*$"
    r")"
)


# `std::recursive_mutex _stateMutex;`, `bool foo{ false };` -- a member or type
# declaration.  It ends in ';' like a statement does, but the literal *is* the
# thing being guarded, so it stays structural.
MEMBER_DECL_RE = re.compile(
    r"^[A-Za-z_][\w:]*(?:\s*<[^;]*>)?(?:\s*[*&])?\s+[A-Za-z_]\w*\s*(?:\{[^;]*\})?\s*;$"
)

# How a check gets hold of source text.  Most open the file themselves with
# ifstream; the Prisma panel checks call a shared helper instead, because the
# view is spread over several files.  Recognising only ifstream would drop those
# checks -- and their pins -- out of the report without any error, which is the
# failure mode --max-brittle exists to prevent.
SOURCE_TEXT_MARKERS = ("ifstream", "LoadPrismaViewSource")


def classify(literal: str) -> str:
    """Return 'structural' or 'brittle' for one pinned literal."""
    text = literal.strip()
    if not text:
        return "structural"

    if DECL_RE.match(text) or MEMBER_DECL_RE.match(text):
        return "structural"

    # A statement (assignment or terminator) is a body pin.
    if text.endswith(";") or "=" in text:
        return "brittle"

    # A call site carrying arguments pins how the call is spelled.
    call = re.search(r"\w\(\s*(.*)$", text)
    if call and call.group(1).strip() not in ("", ")"):
        return "brittle"

    # Bare `Foo(` or a plain identifier: pins existence only.
    return "structural"


def scan() -> list[dict]:
    """Split each gate file into check-function bodies and classify their pins."""
    findings: list[dict] = []
    for path in sorted(TESTS_DIR.glob("runtime_gate_store_checks_*.cpp")):
        text = path.read_text(encoding="utf-8")
        headers = list(CHECK_RE.finditer(text))
        for i, header in enumerate(headers):
            end = headers[i + 1].start() if i + 1 < len(headers) else len(text)
            body = text[header.end() : end]
            if not any(marker in body for marker in SOURCE_TEXT_MARKERS):
                continue

            pins = [(literal, classify(literal)) for literal in FIND_RE.findall(body)]
            findings.append(
                {
                    "file": str(path.relative_to(REPO_ROOT)),
                    "check": header.group(1),
                    "brittle": [p for p, kind in pins if kind == "brittle"],
                    "structural": [p for p, kind in pins if kind == "structural"],
                    # A source-text check whose pins we could not parse still
                    # needs review; surfacing it beats dropping it silently.
                    "pins_parsed": bool(pins),
                }
            )
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true", help="emit machine-readable output")
    parser.add_argument(
        "--max-brittle",
        type=int,
        default=None,
        help="exit non-zero when the brittle pin count exceeds this ratchet",
    )
    args = parser.parse_args()

    findings = scan()
    brittle_total = sum(len(f["brittle"]) for f in findings)
    checks_with_brittle = [f for f in findings if f["brittle"]]

    if args.json:
        json.dump(
            {
                "checks": findings,
                "brittle_pins": brittle_total,
                "checks_with_brittle_pins": len(checks_with_brittle),
            },
            sys.stdout,
            indent=2,
            ensure_ascii=False,
        )
        sys.stdout.write("\n")
    else:
        for finding in sorted(findings, key=lambda f: -len(f["brittle"])):
            if not finding["brittle"]:
                continue
            print(f"{finding['check']}  [{finding['file']}]")
            for pin in finding["brittle"]:
                print(f"    brittle: {pin}")
        unparsed = [f["check"] for f in findings if not f["pins_parsed"]]
        if unparsed:
            print("source-text checks whose pins could not be parsed (review by hand):")
            for name in unparsed:
                print(f"    {name}")
            print()

        print(f"source-text checks:        {len(findings)}")
        print(f"  with brittle pins:       {len(checks_with_brittle)}  (migration candidates)")
        print(f"  structural pins only:    {len(findings) - len(checks_with_brittle) - len(unparsed)}  (fine as-is)")
        print(f"  pins not parsed:         {len(unparsed)}")
        print(f"brittle pins total:        {brittle_total}")

    if args.max_brittle is not None and brittle_total > args.max_brittle:
        print(
            f"\nERROR: {brittle_total} brittle pins exceeds the ratchet of {args.max_brittle}.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
