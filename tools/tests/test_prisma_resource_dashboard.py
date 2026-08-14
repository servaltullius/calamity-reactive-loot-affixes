#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


def _load_view_source_module():
    path = Path(__file__).resolve().parents[2] / "tools" / "prisma_view_source.py"
    spec = importlib.util.spec_from_file_location("prisma_view_source", path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class PrismaResourceDashboardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.view_dir = (
            cls.repo_root / "Data" / "PrismaUI" / "views" / "CalamityAffixes"
        )
        cls.index = (cls.view_dir / "index.html").read_text(encoding="utf-8")
        loader = _load_view_source_module()
        cls.source = loader.load_view_source()
        cls.dashboard = (
            cls.view_dir / "scripts" / "resource-dashboard.js"
        ).read_text(encoding="utf-8")

    def _between(self, start: str, end: str) -> str:
        start_index = self.source.index(start)
        end_index = self.source.index(end, start_index)
        return self.source[start_index:end_index]

    def test_dashboard_is_independent_of_base_selection(self) -> None:
        dashboard_index = self.index.index('id="resourceDashboardSection"')
        workbench_index = self.index.index('<div class="rwWorkbench">')
        self.assertLess(dashboard_index, workbench_index)

        normalizer = self._between(
            "function normalizeResourceDashboardSnapshot(data)",
            "function applyResourceDashboardSnapshot(data)",
        )
        for marker in (
            "runeInventoryKnown",
            "runeInventoryExpectedCount",
            "runeInventory",
            "runeName",
            "reforgeOrbsKnown",
            "reforgeOrbsOwned",
            "pityKnown",
            "runewordFragmentFailStreak",
            "runewordFragmentFailStreakThreshold",
            "reforgeOrbFailStreak",
            "reforgeOrbFailStreakThreshold",
        ):
            self.assertIn(marker, normalizer)
        self.assertNotIn("hasBase", normalizer)
        self.assertNotIn("hasRecipe", normalizer)

    def test_unknown_data_fails_closed_and_received_is_distinct_from_syncing(self) -> None:
        for marker in (
            "received: false",
            "received: true",
            'resourceDashboardSection.setAttribute("aria-busy", state.received ? "false" : "true")',
            't("Unavailable", "사용 불가")',
            'valueNode.textContent = "— / —"',
            'progress.removeAttribute("value")',
            'typeof owned !== "number"',
            "!Number.isSafeInteger(owned)",
            "fragmentStreak <= fragmentThreshold",
            "orbStreak <= orbThreshold",
        ):
            self.assertIn(marker, self.source)

    def test_resource_changes_have_an_isolated_render_section(self) -> None:
        apply_snapshot = self._between(
            "function applyResourceDashboardSnapshot(data)",
            "function setResourceProgress(",
        )
        self.assertIn("resourceDashboardSignatureState", apply_snapshot)
        self.assertIn(
            "schedulePanelRender(panelRenderSection.resourceDashboard)",
            apply_snapshot,
        )
        self.assertNotIn("recipeItems", apply_snapshot)
        self.assertNotIn("runewordPanelState", apply_snapshot)
        self.assertNotIn("invalidateRecipePresentationCaches", apply_snapshot)

    def test_native_disclosure_list_and_progress_are_accessible(self) -> None:
        for marker in (
            '<details id="resourceRuneDetails"',
            '<summary id="resourceRuneDetailsSummary"',
            '<ul\n                    id="resourceRuneList"',
            'id="resourceDashboardSync"',
            'role="status"',
            'aria-live="polite"',
            'aria-describedby="resourceDashboardLead resourcePityHint"',
        ):
            self.assertIn(marker, self.index)
        self.assertEqual(2, self.index.count('class="rdProgress"'))
        self.assertIn("progress.setAttribute(\"aria-valuetext\"", self.dashboard)
        self.assertIn("left.runeName.localeCompare(right.runeName)", self.dashboard)

    def test_copy_describes_eligible_rolls_not_kill_countdowns(self) -> None:
        self.assertIn("next eligible ordinary drop roll is guaranteed", self.dashboard)
        self.assertIn("다음 적격 일반 드랍 판정에서 확정 지급", self.dashboard)
        for forbidden in (
            "kills remaining",
            "kills left",
            "킬 남음",
            "남은 처치",
        ):
            self.assertNotIn(forbidden, self.dashboard.lower())

    def test_new_assets_are_loaded_as_classic_resources_in_order(self) -> None:
        self.assertIn(
            '<link rel="stylesheet" href="styles/resource-dashboard.css" />',
            self.index,
        )
        build_summary = self.index.index(
            '<script src="scripts/build-summary.js"></script>'
        )
        dashboard = self.index.index(
            '<script src="scripts/resource-dashboard.js"></script>'
        )
        interop = self.index.index('<script src="scripts/interop.js"></script>')
        self.assertLess(build_summary, dashboard)
        self.assertLess(dashboard, interop)
        self.assertNotIn("type=\"module\"", self.index)


if __name__ == "__main__":
    unittest.main()
