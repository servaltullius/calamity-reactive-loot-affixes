#!/usr/bin/env python3
"""Verify the vendored CommonLibSSE-NG tree against its recorded pin.

extern/CommonLibSSE-NG is vendored: ~2000 upstream files are committed straight
into this repo, and the upstream .git directory is gitignored.  That makes the
dependency reproducible only by accident -- nothing recorded which upstream
commit the tree came from, and a local edit was indistinguishable from upstream
code.  CommonLibSSE-NG.pin.json now records the commit, and extern/patches/
holds the deliberate local changes.

Modes:

  (default)  Offline.  Recomputes a hash over every vendored file and compares
             it to the recorded tree hash, and checks each patched file against
             its recorded post-patch hash.  Catches drift, accidental reverts,
             and undeclared local edits without touching the network.

  --fetch    Online.  Clones upstream at the pinned commit, applies the recorded
             patches, and diffs the result against the vendored tree.  This is
             the full reproducibility proof; run it when bumping the pin.

  --update   Rewrites the hashes in the pin file from the current tree.  Use it
             only right after a deliberate, reviewed change to the vendored
             tree or the patches.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
EXTERN_DIR = REPO_ROOT / "skse" / "CalamityAffixes" / "extern"
VENDOR_DIR = EXTERN_DIR / "CommonLibSSE-NG"
PIN_FILE = EXTERN_DIR / "CommonLibSSE-NG.pin.json"


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 16), b""):
            digest.update(chunk)
    return digest.hexdigest()


def tracked_files(root: Path) -> list[Path]:
    """Git-tracked files under root, relative to root.

    Uses git rather than a filesystem walk so ignored build output, the
    upstream .git directory, and local install trees stay out of the hash.
    """
    out = subprocess.run(
        ["git", "-C", str(REPO_ROOT), "ls-files", "-z", "--", str(root.relative_to(REPO_ROOT))],
        capture_output=True,
        check=True,
        text=True,
    ).stdout
    names = [n for n in out.split("\0") if n]
    return sorted(Path(n).relative_to(root.relative_to(REPO_ROOT)) for n in names)


def tree_hash(root: Path) -> tuple[str, int]:
    """Order-independent hash over (relative path, content) of every tracked file."""
    digest = hashlib.sha256()
    files = tracked_files(root)
    for rel in files:
        digest.update(rel.as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(file_sha256(root / rel).encode("ascii"))
        digest.update(b"\0")
    return digest.hexdigest(), len(files)


def load_pin() -> dict:
    return json.loads(PIN_FILE.read_text(encoding="utf-8"))


def cmd_update(pin: dict) -> int:
    digest, count = tree_hash(VENDOR_DIR)
    pin["tree_sha256"] = digest
    pin["tracked_file_count"] = count
    for entry in pin["patches"]:
        entry["patched_sha256"] = file_sha256(VENDOR_DIR / entry["file"])
    PIN_FILE.write_text(json.dumps(pin, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"updated pin: {count} tracked files, tree_sha256={digest[:16]}...")
    return 0


def cmd_verify(pin: dict) -> int:
    problems: list[str] = []

    if "tree_sha256" not in pin:
        print("pin has no recorded hashes; run with --update first", file=sys.stderr)
        return 1

    for entry in pin["patches"]:
        target = VENDOR_DIR / entry["file"]
        patch = EXTERN_DIR / entry["patch"]
        if not target.is_file():
            problems.append(f"patched file missing: {entry['file']}")
            continue
        if not patch.is_file() or patch.stat().st_size == 0:
            problems.append(f"patch missing or empty: {entry['patch']}")
        actual = file_sha256(target)
        if actual != entry.get("patched_sha256"):
            problems.append(
                f"{entry['file']} no longer matches its recorded patched content "
                f"(expected {entry.get('patched_sha256', '<unset>')[:16]}..., got {actual[:16]}...)"
            )

    digest, count = tree_hash(VENDOR_DIR)
    if count != pin.get("tracked_file_count"):
        problems.append(
            f"tracked file count changed: expected {pin.get('tracked_file_count')}, found {count}"
        )
    if digest != pin["tree_sha256"]:
        problems.append(
            "vendored tree hash does not match the pin -- the tree was edited without "
            "updating CommonLibSSE-NG.pin.json (see extern/README.md)"
        )

    if problems:
        for problem in problems:
            print(f"ERROR: {problem}", file=sys.stderr)
        return 1

    print(
        f"CommonLibSSE-NG pin OK: {pin['tag']} ({pin['commit'][:12]}), "
        f"{count} files, {len(pin['patches'])} local patches"
    )
    return 0


def cmd_fetch(pin: dict) -> int:
    if shutil.which("git") is None:
        print("git not available", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp) / "upstream"
        print(f"cloning {pin['upstream']} @ {pin['commit'][:12]} ...")
        subprocess.run(["git", "init", "-q", str(work)], check=True)
        subprocess.run(["git", "-C", str(work), "remote", "add", "origin", pin["upstream"]], check=True)
        fetch = subprocess.run(
            ["git", "-C", str(work), "fetch", "-q", "--depth", "1", "origin", pin["commit"]],
        )
        if fetch.returncode != 0:
            print("ERROR: could not fetch the pinned commit (network or bad pin)", file=sys.stderr)
            return 1
        subprocess.run(["git", "-C", str(work), "checkout", "-q", "FETCH_HEAD"], check=True)

        for entry in pin["patches"]:
            patch = EXTERN_DIR / entry["patch"]
            applied = subprocess.run(
                ["git", "-C", str(work), "apply", str(patch)],
                capture_output=True,
                text=True,
            )
            if applied.returncode != 0:
                print(
                    f"ERROR: {entry['patch']} does not apply to upstream {pin['commit'][:12]}:\n"
                    f"{applied.stderr}",
                    file=sys.stderr,
                )
                return 1

        mismatches = []
        for rel in tracked_files(VENDOR_DIR):
            reconstructed = work / rel
            if not reconstructed.is_file():
                mismatches.append(f"{rel}: present in vendored tree, absent upstream")
            elif file_sha256(reconstructed) != file_sha256(VENDOR_DIR / rel):
                mismatches.append(f"{rel}: content differs from upstream+patches")

        if mismatches:
            print(
                f"ERROR: {len(mismatches)} file(s) differ from upstream@{pin['commit'][:12]} + patches:",
                file=sys.stderr,
            )
            for line in mismatches[:40]:
                print(f"  {line}", file=sys.stderr)
            if len(mismatches) > 40:
                print(f"  ... and {len(mismatches) - 40} more", file=sys.stderr)
            return 1

    print(f"vendored tree reproduces exactly from upstream@{pin['commit'][:12]} + {len(pin['patches'])} patches")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--update", action="store_true", help="rewrite recorded hashes from the current tree")
    group.add_argument("--fetch", action="store_true", help="reconstruct from upstream and diff (needs network)")
    args = parser.parse_args()

    if not PIN_FILE.is_file():
        print(f"missing pin file: {PIN_FILE}", file=sys.stderr)
        return 1
    pin = load_pin()

    if args.update:
        return cmd_update(pin)
    if args.fetch:
        return cmd_fetch(pin)
    return cmd_verify(pin)


if __name__ == "__main__":
    raise SystemExit(main())
