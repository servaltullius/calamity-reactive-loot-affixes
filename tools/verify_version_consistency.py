#!/usr/bin/env python3
"""Check that every declared plugin version agrees.

The authoritative version is `project(CalamityAffixes VERSION ...)` in
skse/CalamityAffixes/CMakeLists.txt: CMake configures Version.h.in from it, and
that is what ends up compiled into the DLL and reported to SKSE.

Two other places restate it and can drift silently, because nothing reads them
at build time:

  skse/CalamityAffixes/vcpkg.json   "version-string"
  CHANGELOG.md                      the newest "## [x.y.z]" heading

vcpkg.json sat at 1.4.0 while the plugin shipped 1.7.2 -- eleven releases of
drift that no build step could notice. This check is what makes that visible.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
CMAKE_FILE = REPO_ROOT / "skse" / "CalamityAffixes" / "CMakeLists.txt"
VCPKG_FILE = REPO_ROOT / "skse" / "CalamityAffixes" / "vcpkg.json"
CHANGELOG_FILE = REPO_ROOT / "CHANGELOG.md"

CMAKE_VERSION_RE = re.compile(r"project\(\s*CalamityAffixes\s+VERSION\s+([0-9]+(?:\.[0-9]+)*)")
CHANGELOG_VERSION_RE = re.compile(r"^##\s*\[([0-9]+\.[0-9]+\.[0-9]+)\]", re.MULTILINE)


def read_cmake_version(path: Path = CMAKE_FILE) -> str | None:
    match = CMAKE_VERSION_RE.search(path.read_text(encoding="utf-8"))
    return match.group(1) if match else None


def read_vcpkg_version(path: Path = VCPKG_FILE) -> str | None:
    data = json.loads(path.read_text(encoding="utf-8"))
    # vcpkg manifests may use any of these; whichever is present must agree.
    for key in ("version", "version-semver", "version-string"):
        if key in data:
            return data[key]
    return None


def read_changelog_version(path: Path = CHANGELOG_FILE) -> str | None:
    # The newest released heading; "## [Unreleased]" is skipped by the regex
    # because it does not parse as a version.
    match = CHANGELOG_VERSION_RE.search(path.read_text(encoding="utf-8"))
    return match.group(1) if match else None


def collect() -> dict[str, str | None]:
    return {
        "CMakeLists.txt (authoritative)": read_cmake_version(),
        "vcpkg.json": read_vcpkg_version(),
        "CHANGELOG.md (newest entry)": read_changelog_version(),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quiet", action="store_true", help="print nothing on success")
    parser.add_argument(
        "--expect",
        metavar="VERSION",
        help=(
            "additionally require the declared version to equal this "
            "(the release workflow passes the git tag, so a tag that does not "
            "match what the plugin actually reports cannot be released)"
        ),
    )
    args = parser.parse_args()

    versions = collect()
    if args.expect:
        versions[f"--expect (release tag)"] = args.expect.lstrip("v")

    missing = [name for name, value in versions.items() if value is None]
    if missing:
        for name in missing:
            print(f"ERROR: could not read a version from {name}", file=sys.stderr)
        return 1

    distinct = set(versions.values())
    if len(distinct) != 1:
        print("ERROR: declared versions disagree:", file=sys.stderr)
        width = max(len(name) for name in versions)
        for name, value in versions.items():
            print(f"  {name.ljust(width)}  {value}", file=sys.stderr)
        print(
            "\nCMakeLists.txt is authoritative (it configures Version.h.in). "
            "Bring the others in line with it.",
            file=sys.stderr,
        )
        return 1

    if not args.quiet:
        print(f"version consistency OK: {distinct.pop()} across {len(versions)} declarations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
