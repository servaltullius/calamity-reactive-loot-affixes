#!/usr/bin/env python3
"""Verify MO2 ZIP structure and release-critical identity checks.

Checks required files, exact Papyrus outputs, archive integrity, package version,
and (when supplied) that the packaged DLL byte-for-byte matches the fresh build.

Usage:
    python3 tools/verify_mo2_zip.py path/to/CalamityAffixes_MO2_vX.Y.Z.zip
    python3 tools/verify_mo2_zip.py --latest
    python3 tools/verify_mo2_zip.py path.zip --expected-dll build/CalamityAffixes.dll --expected-version 1.2.3
"""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import sys
import zipfile

MOD_ROOT = "CalamityAffixes"
DLL_RELATIVE_PATH = "SKSE/Plugins/CalamityAffixes.dll"

REQUIRED_FILES: tuple[str, ...] = (
    "CalamityAffixes.esp",
    "CalamityAffixes_KID.ini",
    "CalamityAffixes_DISTR.ini",
    DLL_RELATIVE_PATH,
    "SKSE/Plugins/CalamityAffixes/affixes.json",
    "SKSE/Plugins/CalamityAffixes/runtime_contract.json",
    "SKSE/Plugins/InventoryInjector/CalamityAffixes.json",
    "MCM/Config/CalamityAffixes/settings.ini",
    "MCM/Config/CalamityAffixes/config.json",
    "MCM/Config/CalamityAffixes/keybinds.json",
    "PrismaUI/views/CalamityAffixes/index.html",
)

EXPECTED_PEX_FILES: frozenset[str] = frozenset(
    {
        f"{MOD_ROOT}/Scripts/CalamityAffixes_ModeControl.pex",
        f"{MOD_ROOT}/Scripts/CalamityAffixes_ModEventEmitter.pex",
        f"{MOD_ROOT}/Scripts/CalamityAffixes_MCMConfig.pex",
    }
)

BAD_PATTERNS: tuple[tuple[str, str], ...] = (
    ("Data/Data/", "double Data/ nesting detected"),
    ("Data/SKSE/", "Data/ prefix should not appear inside mod root"),
    ("Data/MCM/", "Data/ prefix should not appear inside mod root"),
)


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def find_latest_zip(repo_root: pathlib.Path) -> pathlib.Path | None:
    dist = repo_root / "dist"
    if not dist.is_dir():
        return None
    zips = sorted(dist.glob("CalamityAffixes_MO2_*.zip"), key=lambda p: p.stat().st_mtime)
    return zips[-1] if zips else None


def verify(
    zip_path: pathlib.Path,
    *,
    expected_dll: pathlib.Path | None = None,
    expected_version: str | None = None,
) -> list[str]:
    errors: list[str] = []

    if not zip_path.is_file():
        return [f"File not found: {zip_path}"]

    if expected_version:
        normalized_version = expected_version.removeprefix("v")
        expected_name = re.compile(
            rf"^CalamityAffixes_MO2_v{re.escape(normalized_version)}_\d{{4}}-\d{{2}}-\d{{2}}\.zip$"
        )
        if expected_name.fullmatch(zip_path.name) is None:
            errors.append(
                f"VERSION MISMATCH: expected v{normalized_version} package name, got {zip_path.name}"
            )

    if expected_dll is not None and not expected_dll.is_file():
        errors.append(f"EXPECTED DLL MISSING: {expected_dll}")

    try:
        with zipfile.ZipFile(zip_path, "r") as archive:
            raw_names = archive.namelist()
            names = set(raw_names)

            duplicate_names = sorted({name for name in raw_names if raw_names.count(name) > 1})
            for name in duplicate_names:
                errors.append(f"DUPLICATE ENTRY: {name}")

            bad_member = archive.testzip()
            if bad_member is not None:
                errors.append(f"CRC FAILURE: {bad_member}")

            for relative_path in REQUIRED_FILES:
                full_path = f"{MOD_ROOT}/{relative_path}"
                if full_path not in names:
                    errors.append(f"MISSING: {full_path}")

            for name in names:
                for pattern, message in BAD_PATTERNS:
                    if pattern in name:
                        errors.append(f"BAD PATH ({message}): {name}")
                        break

            if "CalamityAffixes.dll" in names:
                errors.append(
                    "BAD PATH: CalamityAffixes.dll at zip root (should be under SKSE/Plugins/)"
                )

            packaged_pex_files = {
                name
                for name in names
                if name.startswith(f"{MOD_ROOT}/") and name.casefold().endswith(".pex")
            }
            for missing_pex in sorted(EXPECTED_PEX_FILES - packaged_pex_files):
                errors.append(f"MISSING PEX: {missing_pex}")
            for unexpected_pex in sorted(packaged_pex_files - EXPECTED_PEX_FILES):
                errors.append(f"UNEXPECTED PEX: {unexpected_pex}")
            for expected_pex in sorted(EXPECTED_PEX_FILES & packaged_pex_files):
                if archive.getinfo(expected_pex).file_size == 0:
                    errors.append(f"EMPTY PEX: {expected_pex}")

            packaged_dll_path = f"{MOD_ROOT}/{DLL_RELATIVE_PATH}"
            if packaged_dll_path in names:
                packaged_dll_info = archive.getinfo(packaged_dll_path)
                if packaged_dll_info.file_size == 0:
                    errors.append(f"EMPTY DLL: {packaged_dll_path}")
                if expected_dll is not None and expected_dll.is_file():
                    expected_digest = sha256_file(expected_dll)
                    packaged_digest = hashlib.sha256(archive.read(packaged_dll_path)).hexdigest()
                    if packaged_digest != expected_digest:
                        errors.append(
                            "DLL HASH MISMATCH: "
                            f"package={packaged_digest} build={expected_digest}"
                        )
    except (OSError, zipfile.BadZipFile) as error:
        errors.append(f"INVALID ZIP: {error}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify MO2 ZIP release structure and identity")
    parser.add_argument("zip_path", nargs="?", help="Path to the MO2 ZIP file")
    parser.add_argument("--latest", action="store_true", help="Pick newest zip in dist/")
    parser.add_argument(
        "--expected-dll",
        type=pathlib.Path,
        help="Freshly built DLL whose SHA256 must match the packaged DLL",
    )
    parser.add_argument(
        "--expected-version",
        help="Expected package version (with or without a leading v)",
    )
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
    errors = verify(
        zip_path,
        expected_dll=args.expected_dll,
        expected_version=args.expected_version,
    )

    if errors:
        print(f"\n{len(errors)} issue(s) found:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print("OK - required files, exact PEX set, and archive integrity verified.")
    if args.expected_version:
        print(f"Package version: v{args.expected_version.removeprefix('v')}")
    if args.expected_dll:
        print(f"DLL SHA256 match: {sha256_file(args.expected_dll)}")
    print(f"ZIP SHA256: {sha256_file(zip_path)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
