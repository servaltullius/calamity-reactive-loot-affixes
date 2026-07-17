#!/usr/bin/env python3
from __future__ import annotations

import math
import re
import shutil
import subprocess
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
            'const validRecipeBaseFilters = new Set(["all", "weapon", "armor", "mixed"]);',
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

        filter_state = self._between(
            "function setRecipeBaseFilter(nextFilter)",
            "function resolveRecipeListViewModel()",
        )
        self.assertIn("updateRecipeFilterControls()", filter_state)
        self.assertIn("schedulePanelRender(panelRenderSection.recipeItems)", filter_state)
        self.assertNotIn("invalidateRecipePresentationCaches()", filter_state)

        view_model = self._between(
            "function resolveRecipeListViewModel()",
            "function createRecipeButton(item)",
        )
        self.assertIn("resolveRecipeBaseBadge(item).className === activeFilter", view_model)
        self.assertIn("resolveRecipeSearchDocument(item).includes(query)", view_model)
        self.assertIn("hasActiveConstraint", view_model)

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

    def test_wheel_scroll_is_responsive_and_yields_to_native_input(self) -> None:
        for marker in (
            'data-wheel-scroll-mode="smooth"',
            "(function initFramePacedScroll()",
            "const SCROLL_LINE_HEIGHT_PX = 28;",
            "const LEGACY_WHEEL_NOTCH_DELTA = 120;",
            "const MOUSE_WHEEL_NOTCH_PX = 64;",
            "const MAX_WHEEL_DELTA_PX = 160;",
            "const SMOOTH_SCROLL_TIME_CONSTANT_MS = 48;",
            "const SCROLL_STOP_DISTANCE_PX = 0.75;",
            "const SCROLL_MIN_EFFECTIVE_STEP_PX = 0.5;",
            'window.matchMedia("(prefers-reduced-motion: reduce)")',
            "function shouldReduceMotion()",
            "lastProgrammaticScrollTop: null",
            "function cancelScrollAnimation(element)",
            "cancelAnimationFrame(scrollState.raf);",
            "function normalizeWheelDeltaPixels(event, scroller)",
            "Math.abs(legacyWheelDelta) >= LEGACY_WHEEL_NOTCH_DELTA",
            "MOUSE_WHEEL_NOTCH_PX *",
            "element.clientHeight > 0",
            "isControlledScroller(element)",
            "if (!isScrollable(element))",
            "const expectedScrollTop =",
            "-deltaTime / SMOOTH_SCROLL_TIME_CONSTANT_MS",
            "scrollState.targetScrollTop = clamp(",
            "const nextScrollTop = current + remaining * blend;",
            "SCROLL_MIN_EFFECTIVE_STEP_PX",
            'document.addEventListener("pointerdown", cancelScrollForPointer, true);',
            'document.addEventListener("mousedown", cancelScrollForPointer, true);',
            'document.addEventListener("scroll", syncExternalScroll, true);',
            "const targetUnchanged =",
            "const currentAtTarget =",
            "if (targetUnchanged && currentAtTarget)",
            "requestAnimationFrame((timestamp) =>",
            "if (shouldReduceMotion())",
        ):
            self.assertIn(marker, self.source)

        animation = self._between(
            "function animateScroll(element, timestamp)",
            "function cancelScrollForPointer(event)",
        )
        self.assertIn("if (!isScrollable(element))", animation)
        self.assertLess(
            animation.index("scrollState.targetScrollTop = clamp("),
            animation.index("const remaining ="),
        )
        self.assertIn("cancelScrollAnimation(element)", animation)
        self.assertLess(
            animation.index("cancelScrollAnimation(element)"),
            animation.index("const remaining ="),
        )
        self.assertLess(
            animation.index("SCROLL_MIN_EFFECTIVE_STEP_PX"),
            animation.index("requestAnimationFrame((nextTimestamp)"),
        )

        native_input = self._between(
            "function cancelScrollForPointer(event)",
            'document.addEventListener("wheel"',
        )
        self.assertIn("cancelScrollAnimation(scroller)", native_input)
        self.assertIn("lastProgrammaticScrollTop", native_input)
        self.assertIn("actualScrollTop", native_input)

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

    def test_integer_scroll_quantization_has_a_bounded_stop_path(self) -> None:
        def constant(name: str) -> float:
            match = re.search(
                rf"const {name} = ([0-9]+(?:\.[0-9]+)?);",
                self.source,
            )
            self.assertIsNotNone(match, name)
            return float(match.group(1))

        target = constant("MOUSE_WHEEL_NOTCH_PX")
        time_constant = constant("SMOOTH_SCROLL_TIME_CONSTANT_MS")
        stop_distance = constant("SCROLL_STOP_DISTANCE_PX")
        minimum_step = constant("SCROLL_MIN_EFFECTIVE_STEP_PX")

        current = 0.0
        frame_ms = 16.67
        for frame in range(1, 121):
            remaining = target - current
            if abs(remaining) <= stop_distance:
                current = target
                break
            blend = 1.0 - math.exp(-frame_ms / time_constant)
            next_position = current + remaining * blend
            if abs(next_position - current) < minimum_step:
                current = target
                break
            current = float(round(next_position))
        else:
            self.fail("quantized scroll animation did not terminate")

        self.assertEqual(current, target)
        self.assertLess(frame, 30)

    def test_scroll_controller_behavior_in_node(self) -> None:
        node = shutil.which("node")
        if node is None:
            self.skipTest("Node.js is unavailable")

        script = self.repo_root / "tools" / "tests" / "prisma_scroll_behavior_test.js"
        result = subprocess.run(
            [node, str(script)],
            cwd=self.repo_root,
            capture_output=True,
            text=True,
            timeout=15,
            check=False,
        )
        self.assertEqual(
            result.returncode,
            0,
            msg=f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}",
        )
        self.assertIn("Prisma scroll behavior: OK", result.stdout)

    def test_recipe_content_and_tooltip_layout_behavior_in_node(self) -> None:
        node = shutil.which("node")
        if node is None:
            self.skipTest("Node.js is unavailable")

        script = self.repo_root / "tools" / "tests" / "prisma_recipe_ui_behavior_test.js"
        result = subprocess.run(
            [node, str(script)],
            cwd=self.repo_root,
            capture_output=True,
            text=True,
            timeout=15,
            check=False,
        )
        self.assertEqual(
            result.returncode,
            0,
            msg=f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}",
        )
        self.assertIn("Prisma recipe UI behavior: OK", result.stdout)

    def test_recipe_localization_detail_and_selection_semantics_are_connected(self) -> None:
        for marker in (
            'role="listbox"',
            'aria-label="Equipped runeword bases / 착용 룬워드 베이스"',
            'aria-label="Runeword recipes / 룬워드 레시피"',
            'button.setAttribute("role", "option")',
            'button.setAttribute("aria-selected", selected ? "true" : "false")',
            "button.tabIndex = -1;",
            "function resolveListboxNavigationIndex(",
            "function handleListboxKeydown(event)",
            'controlPanel.addEventListener("keydown", handleListboxKeydown);',
            "function resolveLocalizedRecipeText(",
            'resolveLocalizedRecipeText(\n          item,\n          "summary"',
            'resolveLocalizedRecipeText(\n          item,\n          "detail"',
            "const detailText = resolveRecipeDetailText(item);",
            "const detail = resolveRecipeDetailText(item).toLowerCase();",
            "buildRecipePreviewTooltipText(\n          getSelectedRecipeItem()",
            "function buildSelectedRecipeInspectorText(",
            "schedulePanelRender(panelRenderSection.tooltipPlacement);",
            "if (value !== tooltipTextState)",
            "tooltipPanel.scrollTop = 0;",
        ):
            self.assertIn(marker, self.source)

        for marker in (
            'id="recipeBaseFilters"',
            'data-recipe-filter="all"',
            'data-recipe-filter="weapon"',
            'data-recipe-filter="armor"',
            'data-recipe-filter="mixed"',
            'button.setAttribute("aria-pressed", filter === recipeBaseFilter ? "true" : "false")',
        ):
            self.assertIn(marker, self.source)

        signature = self._between(
            "function buildRecipeCatalogSignature(items)",
            "function resolveConfirmedRecipeToken(items)",
        )
        for field in ("summaryEn", "summaryKo", "detailEn", "detailKo"):
            self.assertIn(field, signature)

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
            "function setRunewordStepCardState(element, state)",
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
        self.assertIn("updatePanelUiScale();", open_state)

        layout_mode = self._between(
            "function resolvePanelLayoutMode(width)",
            "function updatePanelUiScale()",
        )
        self.assertIn('if (width >= 1100) return "wide";', layout_mode)
        self.assertIn('if (width >= 900) return "medium";', layout_mode)
        self.assertIn('return "narrow";', layout_mode)
        self.assertIn("controlPanel.dataset.layout === nextMode", layout_mode)
        self.assertIn("return false;", layout_mode)

        update_scale = self._between(
            "function updatePanelUiScale()",
            "function setPanelAnchoredPosition(left, top)",
        )
        self.assertIn("applyPanelLayoutMode(rect.width)", update_scale)

        for marker in (
            '#controlPanel[data-layout="medium"] .rwWorkbench',
            '#controlPanel[data-layout="narrow"] .rwWorkbench',
            'data-layout="wide"',
        ):
            self.assertIn(marker, self.source)

        self.assertNotIn('id="runewordFlowProgress"', self.source)
        self.assertIn('element.setAttribute("aria-current", "step")', self.source)
        self.assertIn('element.removeAttribute("aria-current")', self.source)


if __name__ == "__main__":
    unittest.main()
