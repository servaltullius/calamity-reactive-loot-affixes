#!/usr/bin/env python3
from __future__ import annotations

import re
import unittest
from pathlib import Path


class PrismaPanelPerformanceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.view_path = (
            cls.repo_root
            / "Data"
            / "PrismaUI"
            / "views"
            / "CalamityAffixes"
            / "index.html"
        )
        cls.source = cls.view_path.read_text(encoding="utf-8")

    def _between(self, start: str, end: str) -> str:
        start_index = self.source.index(start)
        end_index = self.source.index(end, start_index)
        return self.source[start_index:end_index]

    def test_recipe_catalog_reuses_keyed_nodes_and_cached_search_text(self) -> None:
        for marker in (
            "const recipeNodeByToken = new Map();",
            "const recipeSearchDocumentByToken = new Map();",
            "function rebuildRecipeCatalogDom()",
            "const fragment = document.createDocumentFragment();",
            "recipeList.replaceChildren(fragment);",
        ):
            self.assertIn(marker, self.source)

        signature = self._between(
            "function buildRecipeCatalogSignature(items)",
            "function resolveConfirmedRecipeToken(items)",
        )
        self.assertNotIn("selected", signature)

        render = self._between(
            "function renderRecipeItems()",
            "function resolveRunewordPanelActionState(state)",
        )
        self.assertIn("if (recipeCatalogDomDirty)", render)
        self.assertIn("recipeNodeByToken.get(token)", render)
        self.assertNotIn("clearChildren(recipeList)", render)

        setter = self._between(
            "function setRecipeItems(raw)",
            "function setRunewordPanelState(raw)",
        )
        self.assertIn("const catalogChanged =", setter)
        self.assertIn("if (catalogChanged)", setter)
        self.assertIn("applyConfirmedRecipeSelection(nextConfirmedToken)", setter)

    def test_recipe_selection_is_optimistic_and_confirmed_by_small_state_payload(self) -> None:
        optimistic = self._between(
            "function beginOptimisticRecipeSelection(nextToken)",
            "function invalidateRecipePresentationCaches()",
        )
        self.assertIn("updateRecipeSelectionDom(previousToken, nextToken)", optimistic)
        self.assertIn("optimisticRecipeSelectionTimeoutMs", optimistic)
        self.assertIn("confirmedRecipeTokenState", optimistic)

        dispatch = self._between(
            "function dispatchPanelCommand(button)",
            "function handleDelegatedPanelCommandClick(event)",
        )
        self.assertIn("command.startsWith(recipeSelectionCommandPrefix)", dispatch)
        self.assertIn("beginOptimisticRecipeSelection(", dispatch)
        self.assertLess(
            dispatch.index("beginOptimisticRecipeSelection("),
            dispatch.index("sendCommand(command)"),
        )

        panel_state = self._between(
            "function setRunewordPanelState(raw)",
            "function setRunewordAffixPreview(raw)",
        )
        self.assertIn(
            'Object.prototype.hasOwnProperty.call(data, "recipeToken")',
            panel_state,
        )
        self.assertIn("applyConfirmedRecipeSelection(recipeToken)", panel_state)

    def test_wheel_scroll_is_clamped_and_frame_paced(self) -> None:
        for marker in (
            'data-wheel-scroll-mode="smooth"',
            "(function initFramePacedScroll()",
            "const SCROLL_LINE_HEIGHT_PX = 28;",
            "const MAX_WHEEL_DELTA_PX = 160;",
            "const SMOOTH_SCROLL_TIME_CONSTANT_MS = 72;",
            "function normalizeWheelDeltaPixels(event, scroller)",
            "element.clientHeight > 0",
            "if (!isScrollable(element))",
            "-deltaTime / SMOOTH_SCROLL_TIME_CONSTANT_MS",
            "scrollState.targetScrollTop = clamp(",
            "const targetUnchanged =",
            "const currentAtTarget =",
            "if (targetUnchanged && currentAtTarget)",
            "requestAnimationFrame((timestamp) =>",
        ):
            self.assertIn(marker, self.source)

        animation = self._between(
            "function animateScroll(element, timestamp)",
            'document.addEventListener("wheel"',
        )
        self.assertIn("if (!isScrollable(element))", animation)
        self.assertLess(
            animation.index("scrollState.targetScrollTop = clamp("),
            animation.index("const remaining ="),
        )

        wheel_handler = self._between(
            'document.addEventListener("wheel"',
            "}, { passive: false, capture: true });",
        )
        self.assertLess(
            wheel_handler.index("if (targetUnchanged && currentAtTarget)"),
            wheel_handler.index("event.preventDefault()"),
        )

        self.assertNotIn('data-wheel-scroll-mode="amplified"', self.source)
        self.assertNotIn("RECIPE_SCROLL_MIN_STEP", self.source)

    def test_motion_avoids_forced_reflow_and_infinite_paint(self) -> None:
        self.assertNotIn("void element.offsetWidth", self.source)
        self.assertNotIn("@keyframes rwActivePulse", self.source)
        self.assertNotIn("@keyframes rwPrimaryPulse", self.source)
        self.assertNotRegex(
            self.source,
            re.compile(r"animation\s*:[^;{}]*\binfinite\b", re.IGNORECASE),
        )

        for marker in (
            "transition: opacity 140ms ease-out;",
            "animation: cpPaneEnter 140ms ease-out;",
            "animation: rwStateShift 160ms ease-out;",
            "transform: translate3d(0, 2px, 0);",
            "@media (prefers-reduced-motion: reduce)",
        ):
            self.assertIn(marker, self.source)

        shift = self._between(
            "function triggerRunewordStateShift(element, state)",
            "function setRunewordFlowState(element, state)",
        )
        self.assertIn("element.dataset.shiftNonce", shift)
        self.assertIn("window.setTimeout(", shift)
        self.assertNotIn("offsetWidth", shift)

    def test_panel_open_and_resize_skip_redundant_layout_work(self) -> None:
        resize = self._between(
            "function applyPendingPanelResize()",
            "function keepPanelInViewport()",
        )
        self.assertIn("panelResizeState.framePending", resize)
        self.assertIn("requestAnimationFrame(applyPendingPanelResize)", resize)

        pointer_move = self._between(
            "function movePanel(event)",
            "function endPanelDrag(event)",
        )
        self.assertIn("panelResizeState.nextWidth = nextWidth", pointer_move)
        self.assertIn("panelResizeState.nextHeight = nextHeight", pointer_move)
        self.assertIn("schedulePanelResizeFrame()", pointer_move)
        resize_branch = pointer_move[
            pointer_move.index("if (panelResizeState)") :
            pointer_move.index("if (!panelDragState)")
        ]
        self.assertNotIn("controlPanel.style.width", resize_branch)
        self.assertNotIn("controlPanel.style.height", resize_branch)

        open_state = self._between(
            "function applyControlPanelOpenState(nextOpen)",
            "function setControlPanel(raw)",
        )
        self.assertIn("controlPanelOpen === nextOpen", open_state)
        self.assertIn("controlPanel.style.display === expectedDisplay", open_state)
        self.assertIn('controlPanel.classList.add("is-visible")', open_state)
        self.assertNotIn("updatePanelUiScale();", open_state)


if __name__ == "__main__":
    unittest.main()
