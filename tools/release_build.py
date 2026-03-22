#!/usr/bin/env python3
"""End-to-end release build pipeline for CalamityAffixes.

Steps:
  1. C++ DLL build  (cmake)
  2. C++ tests      (ctest)
  3. C# Generator tests (dotnet test)
  4. MO2 ZIP packaging  (build_mo2_zip.sh)
  5. ZIP structure verification (verify_mo2_zip.py)
  6. SHA256 checksum

Usage:
    python3 tools/release_build.py
    python3 tools/release_build.py --skip-build   # reuse existing DLL
    python3 tools/release_build.py --version 1.2.20
"""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import subprocess
import sys
import time

from repo_paths import repo_run_root

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
SKSE_DIR = REPO_ROOT / "skse" / "CalamityAffixes"
BUILD_DIR = SKSE_DIR / "build.linux-clangcl-rel"


def run(cmd: list[str], *, cwd: pathlib.Path | None = None, env_extra: dict[str, str] | None = None) -> None:
    import os
    env = os.environ.copy()
    if env_extra:
        env.update(env_extra)
    print(f"  $ {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd, env=env)
    if result.returncode != 0:
        print(f"\nFAILED (exit {result.returncode}): {' '.join(cmd)}", file=sys.stderr)
        sys.exit(result.returncode)


def sha256_file(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def find_latest_zip() -> pathlib.Path | None:
    dist = REPO_ROOT / "dist"
    if not dist.is_dir():
        return None
    zips = sorted(dist.glob("CalamityAffixes_MO2_*.zip"), key=lambda p: p.stat().st_mtime)
    return zips[-1] if zips else None


def step(msg: str) -> None:
    print(f"\n{'='*60}")
    print(f"  {msg}")
    print(f"{'='*60}")


def main() -> int:
    parser = argparse.ArgumentParser(description="CalamityAffixes release build pipeline")
    parser.add_argument("--skip-build", action="store_true", help="Skip C++ DLL build")
    parser.add_argument("--skip-tests", action="store_true", help="Skip all tests")
    parser.add_argument("--version", type=str, help="Override package version")
    args = parser.parse_args()

    t0 = time.monotonic()

    run(["python3", str(REPO_ROOT / "tools" / "ensure_skse_build.py"), "--lane", "plugin", "--lane", "runtime-gate"])

    # 1. C++ build
    if args.skip_build:
        step("1/6 C++ build — SKIPPED")
    else:
        step("1/6 C++ DLL build")
        if not BUILD_DIR.is_dir():
            print(f"Build directory not found: {BUILD_DIR}", file=sys.stderr)
            print("Run cmake configure first.", file=sys.stderr)
            return 1
        run(["cmake", "--build", str(BUILD_DIR), "--config", "Release"])

    # 2. C++ tests
    if args.skip_tests:
        step("2/6 C++ tests — SKIPPED")
    else:
        step("2/6 C++ tests")
        run(["ctest", "--output-on-failure"], cwd=BUILD_DIR)

    # 3. C# Generator tests
    if args.skip_tests:
        step("3/6 C# Generator tests — SKIPPED")
    else:
        step("3/6 C# Generator tests")
        with repo_run_root(REPO_ROOT) as run_root:
            run(
                ["dotnet", "test", "tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj", "-c", "Release"],
                cwd=run_root,
            )

    # 4. MO2 ZIP packaging
    step("4/6 MO2 ZIP packaging")
    env_extra = {}
    if args.version:
        env_extra["CAFF_PACKAGE_VERSION"] = args.version
    run(["bash", str(REPO_ROOT / "tools" / "build_mo2_zip.sh")], env_extra=env_extra or None)

    # 5. ZIP verification
    step("5/6 ZIP structure verification")
    run(["python3", str(REPO_ROOT / "tools" / "verify_mo2_zip.py"), "--latest"])

    # 6. SHA256
    step("6/6 SHA256 checksum")
    zip_path = find_latest_zip()
    if zip_path:
        digest = sha256_file(zip_path)
        checksum_file = zip_path.with_suffix(".zip.sha256")
        checksum_file.write_text(f"{digest}  {zip_path.name}\n")
        print(f"  {digest}  {zip_path.name}")
        print(f"  Written: {checksum_file}")
    else:
        print("  WARNING: no zip found for checksum")

    elapsed = time.monotonic() - t0
    print(f"\nRelease build completed in {elapsed:.1f}s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
