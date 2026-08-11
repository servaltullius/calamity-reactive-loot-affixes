#!/usr/bin/env python3
from __future__ import annotations

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_DIR = REPO_ROOT / "tools"
sys.path.insert(0, str(TOOLS_DIR))

import public_doc_metadata  # noqa: E402
import update_docs  # noqa: E402


class PublicDocMetadataTests(unittest.TestCase):
    def test_repo_metadata_uses_cmake_version_and_matching_changelog_date(self) -> None:
        metadata = public_doc_metadata.load_public_doc_metadata()

        self.assertEqual(public_doc_metadata.read_cmake_version(), metadata.version)
        self.assertEqual(
            public_doc_metadata.read_changelog_release_date(metadata.version),
            metadata.release_date,
        )

    def test_metadata_requires_an_explicit_date_for_the_authoritative_version(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-doc-metadata-") as temp_dir:
            root = Path(temp_dir)
            cmake = root / "CMakeLists.txt"
            changelog = root / "CHANGELOG.md"
            cmake.write_text(
                "project(CalamityAffixes VERSION 9.8.7 LANGUAGES CXX)\n",
                encoding="utf-8",
            )
            changelog.write_text("## [9.8.6] - 2026-08-11\n", encoding="utf-8")

            with self.assertRaisesRegex(ValueError, r"\[9\.8\.7\].*YYYY-MM-DD"):
                public_doc_metadata.load_public_doc_metadata(cmake, changelog)


class UpdateDocsCheckTests(unittest.TestCase):
    def test_check_compares_without_rewriting_checked_in_docs(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-doc-check-") as temp_dir:
            root = Path(temp_dir)
            checked_dir = root / "checked"
            generated_dir = root / "generated"
            checked_dir.mkdir()
            generated_dir.mkdir()
            for filename in update_docs.GENERATED_DOCS:
                (checked_dir / filename).write_text("checked\n", encoding="utf-8")
                (generated_dir / filename).write_text("generated\n", encoding="utf-8")

            original_docs = update_docs.DOCS
            update_docs.DOCS = checked_dir
            try:
                stdout = io.StringIO()
                stderr = io.StringIO()
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    self.assertFalse(update_docs.check_generated_docs(generated_dir))
                self.assertIn("generated public docs are stale", stderr.getvalue())
            finally:
                update_docs.DOCS = original_docs

            for filename in update_docs.GENERATED_DOCS:
                self.assertEqual(
                    "checked\n",
                    (checked_dir / filename).read_text(encoding="utf-8"),
                )


if __name__ == "__main__":
    unittest.main()
