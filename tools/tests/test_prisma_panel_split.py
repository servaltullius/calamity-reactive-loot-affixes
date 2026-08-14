#!/usr/bin/env python3
from __future__ import annotations

import unittest
from collections import Counter
from html.parser import HTMLParser
from pathlib import Path
import re


REPO_ROOT = Path(__file__).resolve().parents[2]
INDEX_PATH = (
    REPO_ROOT / "Data" / "PrismaUI" / "views" / "CalamityAffixes" / "index.html"
)


class _HierarchyParser(HTMLParser):
    _VOID_TAGS = {
        "area",
        "base",
        "br",
        "col",
        "embed",
        "hr",
        "img",
        "input",
        "link",
        "meta",
        "param",
        "source",
        "track",
        "wbr",
    }

    def __init__(self) -> None:
        super().__init__()
        self.stack: list[tuple[str, str]] = []
        self.counts: Counter[str] = Counter()
        self.attrs: dict[str, dict[str, str]] = {}
        self.ancestors: dict[str, tuple[str, ...]] = {}
        self.tags: dict[str, str] = {}

    def handle_starttag(
        self, tag: str, attrs: list[tuple[str, str | None]]
    ) -> None:
        attributes = {name: value or "" for name, value in attrs}
        element_id = attributes.get("id", "")
        if element_id:
            self.counts[element_id] += 1
            self.tags[element_id] = tag
            self.attrs[element_id] = attributes
            self.ancestors[element_id] = tuple(
                ancestor_id for _, ancestor_id in self.stack if ancestor_id
            )
        if tag not in self._VOID_TAGS:
            self.stack.append((tag, element_id))

    def handle_startendtag(
        self, tag: str, attrs: list[tuple[str, str | None]]
    ) -> None:
        self.handle_starttag(tag, attrs)
        if tag not in self._VOID_TAGS:
            self.stack.pop()

    def handle_endtag(self, tag: str) -> None:
        while self.stack:
            open_tag, _ = self.stack.pop()
            if open_tag == tag:
                break


class PrismaPanelSplitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.index = INDEX_PATH.read_text(encoding="utf-8")
        cls.parser = _HierarchyParser()
        cls.parser.feed(cls.index)

    def assert_descendant(self, child: str, ancestor: str) -> None:
        self.assertIn(
            ancestor,
            self.parser.ancestors[child],
            f"{child} must be inside {ancestor}",
        )

    def assert_rule_contains(
        self, css: str, selector: str, *fragments: str
    ) -> None:
        match = re.search(rf"{re.escape(selector)}\s*\{{([^}}]*)\}}", css)
        self.assertIsNotNone(match, f"missing CSS rule: {selector}")
        body = match.group(1)
        for fragment in fragments:
            self.assertIn(fragment, body, f"{selector} must contain {fragment}")

    def test_working_base_is_one_shared_authoritative_picker(self) -> None:
        for element_id in (
            "workingBaseDetails",
            "workingBaseName",
            "workingBaseMeta",
            "inventoryBaseList",
        ):
            self.assertEqual(self.parser.counts[element_id], 1)
        self.assert_descendant("inventoryBaseList", "workingBaseDetails")
        self.assert_descendant("workingBaseName", "runewordBaseStep")
        self.assert_descendant("workingBaseDetails", "runewordBaseStep")
        self.assertLess(
            self.index.index('id="workingBaseDetails"'),
            self.index.index('id="mainTabContent"'),
        )
        self.assertEqual(self.parser.tags["workingBaseDetails"], "details")
        self.assertEqual(self.parser.tags["runewordBaseChooserSummary"], "summary")
        summary = self.parser.attrs["runewordBaseChooserSummary"]
        self.assertEqual(summary["aria-haspopup"], "listbox")
        self.assertEqual(summary["aria-controls"], "inventoryBaseList")
        self.assertEqual(summary["aria-expanded"], "false")
        self.assertNotIn(
            "data-wheel-scroll-mode",
            self.parser.attrs["inventoryBaseList"],
        )

    def test_working_base_picker_is_readable_at_each_layout_size(self) -> None:
        runeword_css = (
            INDEX_PATH.parent / "styles" / "runeword.css"
        ).read_text(encoding="utf-8")
        responsive_css = (
            INDEX_PATH.parent / "styles" / "responsive.css"
        ).read_text(encoding="utf-8")

        self.assert_rule_contains(
            runeword_css,
            ".wbChooserOverlay",
            "calc(820px * var(--panel-ui-scale))",
            "max-height: none",
            "calc(14px * var(--panel-ui-scale))",
            "overflow: visible",
        )
        self.assert_rule_contains(
            runeword_css,
            ".wbBaseList",
            "grid-template-columns: repeat(3, minmax(0, 1fr))",
            "max-height: none",
            "height: auto",
            "overflow: visible",
        )
        self.assert_rule_contains(
            runeword_css,
            ".wbBaseList .cpListItem",
            "max(14px, calc(15px * var(--panel-ui-scale)))",
            "max(52px, calc(60px * var(--panel-ui-scale)))",
            "-webkit-line-clamp: unset",
            "overflow-wrap: anywhere",
        )
        self.assert_rule_contains(
            responsive_css,
            '#controlPanel[data-layout="medium"] .wbChooserOverlay',
            "calc(680px * var(--panel-ui-scale))",
            "max-height: none",
        )
        self.assert_rule_contains(
            responsive_css,
            '#controlPanel[data-layout="medium"] .wbBaseList',
            "grid-template-columns: repeat(2, minmax(0, 1fr))",
            "max-height: none",
        )
        self.assert_rule_contains(
            responsive_css,
            '#controlPanel[data-layout="narrow"] .wbChooserOverlay',
            "left: calc(8px * var(--panel-ui-scale))",
            "right: calc(8px * var(--panel-ui-scale))",
            "width: auto",
            "max-height: none",
        )
        self.assert_rule_contains(
            responsive_css,
            '#controlPanel[data-layout="narrow"] .wbBaseList',
            "grid-template-columns: 1fr",
            "max-height: none",
        )

    def test_runeword_and_affix_mutations_have_separate_panes(self) -> None:
        for element_id in (
            "resourceDashboardSection",
            "runewordRecipeStep",
            "runewordActionDetails",
            "runewordInsertButton",
            "runewordCubeDetails",
        ):
            self.assert_descendant(element_id, "mainRunewordPane")

        for element_id in (
            "resourceOrbDashboardSection",
            "equippedBuildSection",
            "runewordBaseAffixDetails",
            "runewordReforgeDetails",
            "runewordReforgeButton",
        ):
            self.assert_descendant(element_id, "mainAffixPane")

        self.assert_descendant("runewordRecoveryDetails", "mainAdvancedPane")
        self.assert_descendant("runewordResetButton", "mainAdvancedPane")

        self.assertEqual(
            self.parser.attrs["runewordInsertButton"]["data-cmd"],
            "runeword.insert",
        )
        self.assertEqual(
            self.parser.attrs["runewordReforgeButton"]["data-cmd"],
            "runeword.reforge",
        )
        self.assertEqual(
            self.parser.attrs["runewordResetButton"]["data-cmd"],
            "runeword.reset",
        )

    def test_wide_work_panes_do_not_request_whole_pane_scrolling(self) -> None:
        for pane_id in ("mainRunewordPane", "mainAffixPane"):
            self.assertNotIn(
                "scrollY", self.parser.attrs[pane_id]["class"].split()
            )
        self.assertIn(
            "scrollY", self.parser.attrs["mainAdvancedPane"]["class"].split()
        )
        responsive_css = (
            INDEX_PATH.parent / "styles" / "responsive.css"
        ).read_text(encoding="utf-8")
        self.assertIn(
            '#controlPanel[data-layout="medium"] #mainAffixPane',
            responsive_css,
        )
        self.assertIn("overflow-y: auto", responsive_css)

    def test_tab_relationships_remain_bidirectional(self) -> None:
        for stem in ("Runeword", "Affix", "Advanced"):
            tab_id = f"main{stem}Tab"
            pane_id = f"main{stem}Pane"
            self.assertEqual(self.parser.attrs[tab_id]["aria-controls"], pane_id)
            self.assertEqual(self.parser.attrs[pane_id]["aria-labelledby"], tab_id)


if __name__ == "__main__":
    unittest.main()
