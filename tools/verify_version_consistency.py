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
# SemVer pre-release (`-rc1`) and build metadata (`+build5`) are not part of the
# version CMake compiles into the DLL. See normalize_release_tag.
PRERELEASE_OR_BUILD_RE = re.compile(r"[-+].*$", re.DOTALL)


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


def normalize_release_tag(raw: str) -> str:
    """Reduce a git release tag to the release version it declares.

    This repository ships pre-releases as `-rc` tags -- there are 23 of them,
    e.g. v1.2.20-rc18. The plugin version never carries that suffix: an rc build
    of 1.7.3 still reports 1.7.3, because CMake only knows MAJOR.MINOR.PATCH.
    Comparing the raw tag would therefore reject every pre-release, which is the
    one case where the tag and the declared version are *supposed* to differ.

    SemVer puts the pre-release after `-` and build metadata after `+`, so
    everything from the first of those onwards is dropped.

        v1.7.3       -> 1.7.3
        v1.7.3-rc1   -> 1.7.3
        1.7.3+build5 -> 1.7.3

    A wrong base version is still caught: v1.8.0-rc1 against a declared 1.7.3
    normalizes to 1.8.0 and fails.
    """
    return PRERELEASE_OR_BUILD_RE.sub("", raw.strip().removeprefix("v"))


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
            "match what the plugin actually reports cannot be released). "
            "A leading 'v' and any SemVer pre-release/build suffix are stripped, "
            "so v1.7.3-rc1 satisfies a declared 1.7.3."
        ),
    )
    args = parser.parse_args()

    versions = collect()
    if args.expect:
        normalized = normalize_release_tag(args.expect)
        # Show the raw tag alongside the compared value when they differ, so a
        # failure on an rc tag is diagnosable without re-deriving the rule.
        label = (
            "--expect (release tag)"
            if normalized == args.expect
            else f"--expect (release tag {args.expect})"
        )
        versions[label] = normalized

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
