#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

from repo_paths import stable_repo_run_root


@dataclass(frozen=True)
class LaneSpec:
    name: str
    source_dir: Path
    build_dir: Path
    configure_args: tuple[str, ...]


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.is_file() else ""


def _cache_has_current_paths(source_dir: Path, build_dir: Path) -> bool:
    cache_text = _read_text(build_dir / "CMakeCache.txt")
    dart_text = _read_text(build_dir / "DartConfiguration.tcl")
    if not cache_text or not dart_text:
        return False

    expected_source = str(source_dir)
    expected_build = str(build_dir)
    return (
        f"For build in directory: {expected_build}" in cache_text
        and f"CMAKE_HOME_DIRECTORY:INTERNAL={expected_source}" in cache_text
        and f"SourceDirectory: {expected_source}" in dart_text
        and f"BuildDirectory: {expected_build}" in dart_text
        and f'ConfigureCommand: "/usr/bin/cmake" "{expected_source}"' in dart_text
    )


def _required_tool(name: str) -> str:
    resolved = shutil.which(name)
    if not resolved:
        raise FileNotFoundError(f"required tool not found on PATH: {name}")
    return resolved


def _clear_stale_cmake_state(build_dir: Path) -> None:
    removable_paths = (
        build_dir / "CMakeCache.txt",
        build_dir / "CMakeFiles",
        build_dir / "CTestTestfile.cmake",
        build_dir / "DartConfiguration.tcl",
        build_dir / "cmake_install.cmake",
        build_dir / "build.ninja",
        build_dir / ".ninja_deps",
        build_dir / ".ninja_log",
    )
    for path in removable_paths:
        if path.is_dir():
            shutil.rmtree(path)
        elif path.exists():
            path.unlink()


def _latest_child_dir(root: Path) -> Path | None:
    if not root.is_dir():
        return None
    candidates = sorted(path for path in root.iterdir() if path.is_dir())
    return candidates[-1] if candidates else None


def _xwin_linker_flags(xwin_root: Path) -> str:
    linker_flags = ["/machine:x64"]

    msvc_version_dir = _latest_child_dir(xwin_root / "VC" / "Tools" / "MSVC")
    sdk_version_dir = _latest_child_dir(xwin_root / "Windows Kits" / "10" / "Lib")
    if not msvc_version_dir or not sdk_version_dir:
        return " ".join(linker_flags)

    candidate_libs = (
        msvc_version_dir / "lib" / "x86_64",
        sdk_version_dir / "um" / "x86_64",
        sdk_version_dir / "ucrt" / "x86_64",
    )
    for lib_dir in candidate_libs:
        if lib_dir.is_dir():
            linker_flags.append(f'/libpath:"{lib_dir}"')
    return " ".join(linker_flags)


def _plugin_lane(repo_root: Path, repo_run_root_path: Path) -> LaneSpec:
    skse_root = repo_root / "skse" / "CalamityAffixes"
    run_skse_root = repo_run_root_path / "skse" / "CalamityAffixes"
    winsysroot = run_skse_root / ".xwin"
    prefix_path = run_skse_root / "extern" / "CommonLibSSE-NG" / "install.linux-clangcl-rel-se"
    vendor_cmake_root = run_skse_root / "extern" / "vendor" / "lib" / "cmake"
    linker_flags = _xwin_linker_flags(winsysroot)

    return LaneSpec(
        name="plugin",
        source_dir=skse_root,
        build_dir=skse_root / "build.linux-clangcl-rel",
        configure_args=(
            "-G",
            "Ninja",
            "-DBUILD_TESTING=ON",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_SYSTEM_NAME=Windows",
            f"-DCMAKE_C_COMPILER={_required_tool('clang-cl')}",
            f"-DCMAKE_CXX_COMPILER={_required_tool('clang-cl')}",
            f"-DCMAKE_LINKER={_required_tool('lld-link')}",
            f"-DCMAKE_AR={_required_tool('llvm-lib')}",
            f"-DCMAKE_MT={_required_tool('llvm-mt')}",
            f"-DCMAKE_RC_COMPILER={_required_tool('llvm-rc')}",
            f"-DCMAKE_PREFIX_PATH={prefix_path}",
            f"-Dfmt_DIR={vendor_cmake_root / 'fmt'}",
            f"-Dspdlog_DIR={vendor_cmake_root / 'spdlog'}",
            f"-Dnlohmann_json_DIR={vendor_cmake_root / 'nlohmann_json'}",
            f"-DCMAKE_C_FLAGS=/winsysroot {winsysroot}",
            f"-DCMAKE_CXX_FLAGS=/winsysroot {winsysroot}",
            f"-DCMAKE_EXE_LINKER_FLAGS={linker_flags}",
            f"-DCMAKE_MODULE_LINKER_FLAGS={linker_flags}",
            f"-DCMAKE_SHARED_LINKER_FLAGS={linker_flags}",
            f"-DCMAKE_STATIC_LINKER_FLAGS={linker_flags}",
            "-DCMAKE_TRY_COMPILE_CONFIGURATION=Release",
        ),
    )


def _runtime_gate_lane(repo_root: Path) -> LaneSpec:
    tests_root = repo_root / "skse" / "CalamityAffixes" / "tests"
    return LaneSpec(
        name="runtime-gate",
        source_dir=tests_root,
        build_dir=tests_root / "build",
        configure_args=(
            "-DCMAKE_BUILD_TYPE=Release",
        ),
    )


def ensure_lane(lane: LaneSpec) -> bool:
    lane.build_dir.mkdir(parents=True, exist_ok=True)
    if _cache_has_current_paths(lane.source_dir, lane.build_dir):
        return False

    _clear_stale_cmake_state(lane.build_dir)

    cmd = [
        "cmake",
        "-S",
        str(lane.source_dir),
        "-B",
        str(lane.build_dir),
        *lane.configure_args,
    ]
    subprocess.run(cmd, check=True)
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Ensure SKSE CMake build trees are configured for the current repo path.")
    parser.add_argument(
        "--lane",
        action="append",
        choices=("plugin", "runtime-gate"),
        required=True,
        help="Lane(s) to verify/configure.",
    )
    args = parser.parse_args()

    repo_root = _repo_root()
    run_root = stable_repo_run_root(repo_root)
    available = {
        "plugin": _plugin_lane(repo_root, run_root),
        "runtime-gate": _runtime_gate_lane(repo_root),
    }

    for lane_name in args.lane:
        lane = available[lane_name]
        try:
            changed = ensure_lane(lane)
        except (FileNotFoundError, subprocess.CalledProcessError) as exc:
            print(f"ensure_skse_build: failed to configure {lane.name}: {exc}", file=sys.stderr)
            return 1

        state = "configured" if changed else "up-to-date"
        print(f"ensure_skse_build: {lane.name} {state} ({lane.build_dir})")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
