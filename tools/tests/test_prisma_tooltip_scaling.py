#!/usr/bin/env python3
from __future__ import annotations

import math
import re
import unittest
from pathlib import Path


class PrismaTooltipScalingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.view_path = cls.repo_root / "Data" / "PrismaUI" / "views" / "CalamityAffixes" / "index.html"
        cls.source = cls.view_path.read_text(encoding="utf-8")

    def _desktop_reference(self) -> tuple[int, int, int, int, float, float]:
        match = re.search(
            r"const tooltipReferenceDesktop = Object\.freeze\(\{\s*"
            r"width:\s*(\d+),\s*"
            r"height:\s*(\d+),\s*"
            r"right:\s*(\d+),\s*"
            r"top:\s*(\d+),\s*"
            r"scale:\s*([0-9.]+),\s*"
            r"fontScale:\s*([0-9.]+)\s*"
            r"\}\);",
            self.source,
            re.DOTALL,
        )
        self.assertIsNotNone(match, "expected one 4K desktop tooltip reference object")
        assert match is not None
        width, height, right, top = (int(match.group(index)) for index in range(1, 5))
        return width, height, right, top, float(match.group(5)), float(match.group(6))

    def _scale_for(self, width: int, height: int) -> float:
        ref_width, ref_height, _, _, ref_scale, _ = self._desktop_reference()
        ratio = min(width / ref_width, height / ref_height)
        return max(1.0, min(ref_scale, ratio * ref_scale))

    def test_common_resolutions_preserve_the_current_4k_proportions(self) -> None:
        self.assertIn(
            "getTooltipViewportRatio() * tooltipReferenceDesktop.scale",
            self.source,
        )
        self.assertIn(
            "return Math.min(w / tooltipReferenceDesktop.width, h / tooltipReferenceDesktop.height);",
            self.source,
        )
        self.assertIn("max-width: 720px", self.source)
        self.assertIn("transform: scale(var(--scale))", self.source)
        self.assertIn("font-size: calc(18px * var(--tooltip-font-scale))", self.source)

        cases = (
            (3840, 2160, 2.5, 1800.0, 45.0),
            (2560, 1440, 5.0 / 3.0, 1200.0, 30.0),
            (1920, 1080, 1.25, 900.0, 22.5),
            (3440, 1440, 5.0 / 3.0, 1200.0, 30.0),
        )
        for width, height, expected_scale, expected_width, expected_font in cases:
            with self.subTest(resolution=f"{width}x{height}"):
                scale = self._scale_for(width, height)
                self.assertAlmostEqual(expected_scale, scale, places=3)
                self.assertAlmostEqual(expected_width, 720.0 * scale, places=3)
                self.assertAlmostEqual(expected_font, 18.0 * scale, places=3)

        for width, height in ((3840, 2160), (2560, 1440), (1920, 1080)):
            with self.subTest(relative_width=f"{width}x{height}"):
                self.assertAlmostEqual(0.46875, 720.0 * self._scale_for(width, height) / width, places=5)

    def test_desktop_default_position_scales_from_the_4k_reference(self) -> None:
        self.assertIn("tooltipReferenceDesktop.right * ratio", self.source)
        self.assertIn("tooltipReferenceDesktop.top * ratio", self.source)

        ref_width, ref_height, ref_right, ref_top, _, _ = self._desktop_reference()
        cases = (
            (3840, 2160, 70, 255),
            (2560, 1440, 47, 170),
            (1920, 1080, 35, 128),
            (3440, 1440, 47, 170),
        )
        for width, height, expected_right, expected_top in cases:
            with self.subTest(resolution=f"{width}x{height}"):
                ratio = min(width / ref_width, height / ref_height)
                self.assertEqual(expected_right, math.floor(ref_right * ratio + 0.5))
                self.assertEqual(expected_top, math.floor(ref_top * ratio + 0.5))

    def test_resize_recomputes_only_the_unsaved_default_layout(self) -> None:
        self.assertRegex(
            self.source,
            re.compile(
                r'window\.addEventListener\("resize",\s*\(\)\s*=>\s*\{\s*'
                r'requestAnimationFrame\(\(\)\s*=>\s*\{\s*'
                r'applyAutoScale\(\);\s*'
                r'if\s*\(!tooltipLayoutLoaded\)\s*\{\s*'
                r'tooltipLayout\s*=\s*getDefaultTooltipLayout\(\);',
                re.DOTALL,
            ),
        )
        self.assertRegex(
            self.source,
            re.compile(
                r'function\s+setTooltipLayout\(raw\)\s*\{.*?'
                r'tooltipLayout\s*=\s*normalizeTooltipLayout\(data\);\s*'
                r'tooltipLayoutLoaded\s*=\s*true;',
                re.DOTALL,
            ),
        )

    def test_mobile_defaults_and_manual_font_override_remain_available(self) -> None:
        self.assertIn("tooltipDefaultsMobile = Object.freeze({ right: 16, top: 180, fontScale: 1 })", self.source)
        self.assertIn("if (window.innerWidth <= 900)", self.source)
        self.assertIn("return { ...tooltipDefaultsMobile };", self.source)
        self.assertIn("fontScale: clamp(fontScale, 0.7, 1.8)", self.source)
        self.assertIn("const permille = Number(source.fontPermille);", self.source)
        self.assertIn("fontScale = permille / 1000;", self.source)
        self.assertIn('data-cmd="ui.tooltip.font:-0.08"', self.source)
        self.assertIn('data-cmd="ui.tooltip.font:0.08"', self.source)
        self.assertIn("ui.tooltip.save:${right},${top},${fontPermille}", self.source)

    def test_rendered_rect_is_clamped_and_long_720p_tooltips_scroll(self) -> None:
        for marker in (
            "overflow-y: auto;",
            "overscroll-behavior: contain;",
            "pointer-events: auto;",
            'data-wheel-scroll-mode="smooth"',
            "function getTooltipMaxLogicalHeight()",
            "availableVisualHeight / getTooltipAutoScale()",
            "function fitTooltipLayoutToViewport(rawLayout, rect)",
            "Math.floor(window.innerWidth - margin - rectWidth)",
            "Math.floor(window.innerHeight - margin - rectHeight)",
            "function measureTooltipRect()",
            "tooltipPanel.getBoundingClientRect()",
            "tooltipPanel.style.maxHeight = `${getTooltipMaxLogicalHeight()}px`;",
        ):
            self.assertIn(marker, self.source)

        placement = self.source.index("tooltipText.textContent = tooltipTextState;")
        reclamp = self.source.index("applyTooltipLayout();", placement)
        quick_launch = self.source.index("applyQuickLaunchVisibility();", placement)
        self.assertLess(placement, reclamp)
        self.assertLess(reclamp, quick_launch)

        # At 720p auto-scale bottoms out at 1.0. The 560 logical-pixel cap
        # remains in force even at 180% text, with overflow scrolling the rest.
        self.assertEqual(min(560, (720 - 16) // 1), 560)


if __name__ == "__main__":
    unittest.main()
