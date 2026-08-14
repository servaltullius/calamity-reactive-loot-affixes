#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import unittest
from html.parser import HTMLParser
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def _load_view_source_module():
    path = REPO_ROOT / "tools" / "prisma_view_source.py"
    spec = importlib.util.spec_from_file_location("prisma_view_source", path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class _ElementByIdParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.elements: dict[str, tuple[str, dict[str, str]]] = {}

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        attributes = {name: value or "" for name, value in attrs}
        element_id = attributes.get("id")
        if element_id:
            self.elements[element_id] = (tag, attributes)


class PrismaBuildSummaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.view_source = _load_view_source_module()
        cls.source = cls.view_source.load_view_source()
        cls.parser = _ElementByIdParser()
        cls.parser.feed(cls.source)

    def test_summary_is_a_read_only_semantic_region(self) -> None:
        section_tag, section_attrs = self.parser.elements["equippedBuildSection"]
        self.assertEqual(section_tag, "section")
        self.assertEqual(section_attrs["aria-labelledby"], "equippedBuildTitle")
        self.assertIn("equippedBuildChanceHint", section_attrs["aria-describedby"])

        status_tag, status_attrs = self.parser.elements["equippedBuildStatus"]
        self.assertEqual(status_tag, "div")
        self.assertEqual(status_attrs["role"], "status")
        self.assertEqual(status_attrs["aria-live"], "polite")
        self.assertEqual(status_attrs["aria-atomic"], "true")

        groups_tag, groups_attrs = self.parser.elements["equippedBuildGroups"]
        self.assertEqual(groups_tag, "div")
        self.assertEqual(groups_attrs["aria-busy"], "true")
        self.assertIn("hidden", groups_attrs)

        for group in ("Offense", "Defense", "Kill", "Passive"):
            list_tag, _ = self.parser.elements[f"equippedBuild{group}List"]
            self.assertEqual(list_tag, "ul")

        pane_tag, pane_attrs = self.parser.elements["mainAffixPane"]
        self.assertEqual(pane_tag, "div")
        self.assertNotIn("scrollY", pane_attrs["class"].split())

    def test_probability_copy_keeps_independent_gates_explicit(self) -> None:
        self.assertIn(
            "Shown chance is a condition-qualified roll after current modifiers.",
            self.source,
        )
        self.assertIn(
            "ICDs, proc budgets, action preconditions, and Lucky Hit still apply separately.",
            self.source,
        )
        self.assertIn("Conditional proc roll", self.source)
        self.assertIn("Lucky Hit gate", self.source)
        self.assertIn("equipped-copy count, not a guaranteed stack multiplier", self.source)
        self.assertIn("duplicate proc entries share one roll", self.source)
        self.assertIn("tiered suffix families apply only the highest tier", self.source)
        self.assertIn(
            "Hybrid effects may appear in both their trigger group and Passives",
            self.source,
        )

    def test_passive_facets_distinguish_selection_from_runtime_activity(self) -> None:
        self.assertIn("Passive also active", self.source)
        self.assertIn("Passive active", self.source)
        self.assertIn("Stat passive active · spell off", self.source)
        self.assertIn("Passive spell disabled by runtime setting", self.source)
        self.assertIn("Other passive contribution active", self.source)
        self.assertIn("Highest tier selected", self.source)
        self.assertNotIn("Highest tier active", self.source)
        self.assertNotIn("Passive disabled in MCM", self.source)

    def test_split_assets_follow_the_classic_load_order(self) -> None:
        script_names = [path.name for path in self.view_source.script_paths()]
        style_names = [path.name for path in self.view_source.stylesheet_paths()]

        self.assertLess(script_names.index("build-summary.js"), script_names.index("interop.js"))
        self.assertLess(style_names.index("panel.css"), style_names.index("build-summary.css"))
        self.assertLess(style_names.index("build-summary.css"), style_names.index("responsive.css"))


if __name__ == "__main__":
    unittest.main()
