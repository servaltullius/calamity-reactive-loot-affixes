#!/usr/bin/env python3
"""Verify MO2 ZIP structure for CalamityAffixes.

Checks that the required files exist at the correct paths inside the ZIP,
and that no common packaging mistakes are present (e.g., double Data/ nesting).

Usage:
    python3 tools/verify_mo2_zip.py path/to/CalamityAffixes_MO2_vX.Y.Z.zip
    python3 tools/verify_mo2_zip.py --latest          # pick newest zip in dist/
"""

from __future__ import annotations

import argparse
import pathlib
import sys
import zipfile

# Root prefix inside the zip (build_mo2_zip.sh packages under CalamityAffixes/).
MOD_ROOT = "CalamityAffixes"

# Required files (relative to MOD_ROOT inside the zip).
REQUIRED_FILES: list[str] = [
    "CalamityAffixes.esp",
    "CalamityAffixes_KID.ini",
    "CalamityAffixes_DISTR.ini",
    "SKSE/Plugins/CalamityAffixes.dll",
    "SKSE/Plugins/CalamityAffixes/affixes.json",
    "SKSE/Plugins/InventoryInjector/CalamityAffixes.json",
    "MCM/Config/CalamityAffixes/settings.ini",
    "MCM/Config/CalamityAffixes/config.json",
    "MCM/Config/CalamityAffixes/keybinds.json",
    "PrismaUI/views/CalamityAffixes/index.html",
]

# Patterns that indicate packaging errors.
BAD_PATTERNS: list[tuple[str, str]] = [
    ("Data/Data/", "double Data/ nesting detected"),
    ("Data/SKSE/", "Data/ prefix should not appear inside mod root"),
    ("Data/MCM/", "Data/ prefix should not appear inside mod root"),
]


def find_latest_zip(repo_root: pathlib.Path) -> pathlib.Path | None:
    dist = repo_root / "dist"
    if not dist.is_dir():
        return None
    zips = sorted(dist.glob("CalamityAffixes_MO2_*.zip"), key=lambda p: p.stat().st_mtime)
    return zips[-1] if zips else None


def verify(zip_path: pathlib.Path) -> list[str]:
    errors: list[str] = []

    if not zip_path.is_file():
        return [f"File not found: {zip_path}"]

    with zipfile.ZipFile(zip_path, "r") as zf:
        names = set(zf.namelist())

        # Check required files.
        for rel in REQUIRED_FILES:
            full = f"{MOD_ROOT}/{rel}"
            if full not in names:
                errors.append(f"MISSING: {full}")

        # Check bad patterns.
        for name in names:
            for pattern, msg in BAD_PATTERNS:
                if pattern in name:
                    errors.append(f"BAD PATH ({msg}): {name}")
                    break

        # Check that DLL is not at zip root.
        if "CalamityAffixes.dll" in names:
            errors.append("BAD PATH: CalamityAffixes.dll at zip root (should be under SKSE/Plugins/)")

        # Check for at least one .pex script.
        pex_files = [n for n in names if n.endswith(".pex")]
        if not pex_files:
            errors.append("WARNING: no .pex files found (Papyrus scripts missing?)")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify MO2 ZIP structure")
    parser.add_argument("zip_path", nargs="?", help="Path to the MO2 ZIP file")
    parser.add_argument("--latest", action="store_true", help="Pick newest zip in dist/")
    args = parser.parse_args()

    repo_root = pathlib.Path(__file__).resolve().parent.parent

    if args.latest:
        zip_path = find_latest_zip(repo_root)
        if zip_path is None:
            print("No CalamityAffixes_MO2_*.zip found in dist/", file=sys.stderr)
            return 1
    elif args.zip_path:
        zip_path = pathlib.Path(args.zip_path)
    else:
        parser.print_help()
        return 2

    print(f"Verifying: {zip_path}")
    errors = verify(zip_path)

    if errors:
        print(f"\n{len(errors)} issue(s) found:")
        for e in errors:
            print(f"  - {e}")
        return 1
    else:
        print("OK — all required files present, no packaging errors.")
        return 0


if __name__ == "__main__":
    sys.exit(main())
