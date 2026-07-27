#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]


def _load_module():
    path = REPO_ROOT / "tools" / "verify_version_consistency.py"
    spec = importlib.util.spec_from_file_location("verify_version_consistency", path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class VersionConsistencyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.mod = _load_module()

    # --- the checker's readers, against the real repo files -----------------

    def test_reads_all_three_declarations_from_the_repo(self) -> None:
        versions = self.mod.collect()
        for name, value in versions.items():
            self.assertIsNotNone(value, msg=f"no version parsed from {name}")

    def test_repo_versions_agree(self) -> None:
        """The whole point of the tool: this must hold on a clean checkout."""
        versions = self.mod.collect()
        self.assertEqual(
            len(set(versions.values())),
            1,
            msg=f"declared versions disagree: {versions}",
        )

    def test_cmake_is_the_version_compiled_into_the_dll(self) -> None:
        # Version.h.in interpolates PROJECT_VERSION_*, so the CMake project
        # version is what SKSE ends up reporting. Guard that link.
        template = (
            REPO_ROOT / "skse" / "CalamityAffixes" / "include" / "CalamityAffixes" / "Version.h.in"
        ).read_text(encoding="utf-8")
        for part in ("MAJOR", "MINOR", "PATCH"):
            self.assertIn(f"@PROJECT_VERSION_{part}@", template)

    # --- the readers, against synthetic inputs ------------------------------

    def test_cmake_reader_parses_the_project_call(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "CMakeLists.txt"
            path.write_text(
                "cmake_minimum_required(VERSION 3.21)\n"
                "project(CalamityAffixes VERSION 9.8.7 LANGUAGES CXX)\n",
                encoding="utf-8",
            )
            self.assertEqual(self.mod.read_cmake_version(path), "9.8.7")

    def test_cmake_reader_returns_none_when_absent(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "CMakeLists.txt"
            path.write_text("project(SomethingElse VERSION 1.0.0)\n", encoding="utf-8")
            self.assertIsNone(self.mod.read_cmake_version(path))

    def test_vcpkg_reader_accepts_each_manifest_version_key(self) -> None:
        for key in ("version", "version-semver", "version-string"):
            with self.subTest(key=key), tempfile.TemporaryDirectory() as tmp:
                path = Path(tmp) / "vcpkg.json"
                path.write_text(json.dumps({"name": "x", key: "2.3.4"}), encoding="utf-8")
                self.assertEqual(self.mod.read_vcpkg_version(path), "2.3.4")

    def test_changelog_reader_takes_the_newest_release_not_unreleased(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "CHANGELOG.md"
            path.write_text(
                "# Changelog\n\n"
                "## [Unreleased]\n\n"
                "## [5.4.3] - 2026-01-01\n\n"
                "## [5.4.2] - 2025-12-01\n",
                encoding="utf-8",
            )
            self.assertEqual(self.mod.read_changelog_version(path), "5.4.3")


if __name__ == "__main__":
    unittest.main()
