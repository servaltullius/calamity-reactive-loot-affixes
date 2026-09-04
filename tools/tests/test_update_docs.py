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


class GeneratedRuntimeRulesTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temp_dir = tempfile.TemporaryDirectory(prefix="caff-doc-rules-")
        cls.addClassCleanup(cls.temp_dir.cleanup)
        output_dir = Path(cls.temp_dir.name)
        update_docs.generate_docs(output_dir, public_doc_metadata.load_public_doc_metadata())
        cls.documents = {
            name: (output_dir / name).read_text(encoding="utf-8")
            for name in update_docs.GENERATED_DOCS
        }

    def test_suffix_docs_explain_rank_sum_cap_and_single_effect(self) -> None:
        rendered = self.documents["SUFFIX_EFFECTS.md"]
        for expected in (
            "티어를 합산", "최대 T3", "T1 + T1 → T2", "T1 + T2 → T3",
            "T2 + T2 → T3", "능력치 숫자를 단순 합산하는 방식은 아님",
            "치명타 피해 보너스도 합산 결과 티어의 값 한 번만 적용",
        ):
            with self.subTest(expected=expected):
                self.assertTrue(expected in rendered, f"Missing suffix rule: {expected}")
        self.assertNotIn("가장 높은 티어 하나만 적용", rendered)
        self.assertNotIn("추가 중첩되지 않음", rendered)

    def test_prefix_docs_distinguish_duplicate_chance_and_crit_cast_limits(self) -> None:
        rendered = self.documents["PREFIX_EFFECTS.md"]
        for expected in (
            "최대 3개", "40% → 64% → 78.4%", "피해량을 2배·3배",
            "근접 치명타·강공격은 최대 2개", "일반 근접 공격과 활·석궁은 최대 1개",
            "0.15초", "이중 판정하지 않음", "근접 일반 공격 시 45%",
        ):
            with self.subTest(expected=expected):
                self.assertTrue(expected in rendered, f"Missing prefix rule: {expected}")

    def test_catalog_links_details_and_mentions_current_proc_and_suffix_rules(self) -> None:
        rendered = self.documents["AFFIX_CATALOG.md"]
        for expected in (
            "[프리픽스 상세](PREFIX_EFFECTS.md)", "[서픽스 상세](SUFFIX_EFFECTS.md)",
            "[룬워드 상세](RUNEWORD_EFFECTS.md)", "일반 공격 확률 발동",
            "T1 + T1 → T2", "최대 T3",
        ):
            with self.subTest(expected=expected):
                self.assertTrue(expected in rendered, f"Missing catalog rule: {expected}")
        self.assertNotIn("치명타/강공 기반 추가 주문 발동 계열", rendered)


if __name__ == "__main__":
    unittest.main()
