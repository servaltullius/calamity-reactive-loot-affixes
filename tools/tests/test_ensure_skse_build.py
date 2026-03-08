from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import ensure_skse_build


class EnsureSkseBuildTests(unittest.TestCase):
    def test_cache_check_requires_matching_current_paths(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-cmake-cache-") as temp_dir:
            temp_root = Path(temp_dir)
            source_dir = temp_root / "src"
            build_dir = temp_root / "build"
            prefix_dir = temp_root / "prefix"
            source_dir.mkdir()
            build_dir.mkdir()
            prefix_dir.mkdir()

            (build_dir / "CMakeCache.txt").write_text(
                (
                    "# This is the CMakeCache file.\n"
                    f"# For build in directory: {build_dir}\n"
                    f"CMAKE_HOME_DIRECTORY:INTERNAL={source_dir}\n"
                    f"CMAKE_PREFIX_PATH:UNINITIALIZED={prefix_dir}\n"
                ),
                encoding="utf-8",
            )
            (build_dir / "DartConfiguration.tcl").write_text(
                "SourceDirectory: {source}\nBuildDirectory: {build}\nConfigureCommand: \"/usr/bin/cmake\" \"{source}\"\n".format(
                    source=source_dir,
                    build=build_dir,
                ),
                encoding="utf-8",
            )

            self.assertTrue(
                ensure_skse_build._cache_has_current_paths(
                    source_dir,
                    build_dir,
                    (f"CMAKE_PREFIX_PATH:UNINITIALIZED={prefix_dir}",),
                )
            )

    def test_cache_check_rejects_stale_source_path(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-cmake-cache-stale-") as temp_dir:
            temp_root = Path(temp_dir)
            source_dir = temp_root / "src"
            build_dir = temp_root / "build"
            prefix_dir = temp_root / "prefix"
            source_dir.mkdir()
            build_dir.mkdir()
            prefix_dir.mkdir()

            stale_source = temp_root / "old-src"
            (build_dir / "CMakeCache.txt").write_text(
                (
                    "# This is the CMakeCache file.\n"
                    f"# For build in directory: {build_dir}\n"
                    f"CMAKE_HOME_DIRECTORY:INTERNAL={stale_source}\n"
                    f"CMAKE_PREFIX_PATH:UNINITIALIZED={prefix_dir}\n"
                ),
                encoding="utf-8",
            )
            (build_dir / "DartConfiguration.tcl").write_text(
                "SourceDirectory: {source}\nBuildDirectory: {build}\nConfigureCommand: \"/usr/bin/cmake\" \"{source}\"\n".format(
                    source=stale_source,
                    build=build_dir,
                ),
                encoding="utf-8",
            )

            self.assertFalse(
                ensure_skse_build._cache_has_current_paths(
                    source_dir,
                    build_dir,
                    (f"CMAKE_PREFIX_PATH:UNINITIALIZED={prefix_dir}",),
                )
            )

    def test_cache_check_rejects_stale_prefix_path(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-cmake-cache-prefix-") as temp_dir:
            temp_root = Path(temp_dir)
            source_dir = temp_root / "src"
            build_dir = temp_root / "build"
            stale_prefix = temp_root / "old-prefix"
            expected_prefix = temp_root / "new-prefix"
            source_dir.mkdir()
            build_dir.mkdir()
            stale_prefix.mkdir()
            expected_prefix.mkdir()

            (build_dir / "CMakeCache.txt").write_text(
                (
                    "# This is the CMakeCache file.\n"
                    f"# For build in directory: {build_dir}\n"
                    f"CMAKE_HOME_DIRECTORY:INTERNAL={source_dir}\n"
                    f"CMAKE_PREFIX_PATH:UNINITIALIZED={stale_prefix}\n"
                ),
                encoding="utf-8",
            )
            (build_dir / "DartConfiguration.tcl").write_text(
                "SourceDirectory: {source}\nBuildDirectory: {build}\nConfigureCommand: \"/usr/bin/cmake\" \"{source}\"\n".format(
                    source=source_dir,
                    build=build_dir,
                ),
                encoding="utf-8",
            )

            self.assertFalse(
                ensure_skse_build._cache_has_current_paths(
                    source_dir,
                    build_dir,
                    (f"CMAKE_PREFIX_PATH:UNINITIALIZED={expected_prefix}",),
                )
            )


if __name__ == "__main__":
    unittest.main()
