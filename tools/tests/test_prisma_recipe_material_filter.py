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


class PrismaRecipeMaterialFilterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.view_source = _load_view_source_module()
        cls.source = cls.view_source.load_view_source()
        cls.markup = (cls.view_source.VIEW_DIR / "index.html").read_text(encoding="utf-8")
        cls.parser = _ElementByIdParser()
        cls.parser.feed(cls.markup)

    def _between(self, start: str, end: str) -> str:
        start_index = self.source.index(start)
        end_index = self.source.index(end, start_index)
        return self.source[start_index:end_index]

    def test_material_filter_is_a_described_accessible_group(self) -> None:
        group_tag, group_attrs = self.parser.elements["recipeMaterialFilters"]
        self.assertEqual(group_tag, "div")
        self.assertEqual(group_attrs["role"], "group")
        self.assertEqual(group_attrs["aria-describedby"], "recipeMaterialFilterHint")
        self.assertEqual(group_attrs["aria-disabled"], "true")

        hint_tag, hint_attrs = self.parser.elements["recipeMaterialFilterHint"]
        self.assertEqual(hint_tag, "div")
        self.assertEqual(hint_attrs["role"], "status")
        self.assertEqual(hint_attrs["aria-live"], "polite")
        self.assertEqual(hint_attrs["aria-atomic"], "true")

        for value in ("all", "ready", "missing1"):
            self.assertIn(f'data-recipe-material-filter="{value}"', self.markup)
        self.assertIn("Any fragments / 조각 무관", self.markup)
        self.assertIn("Fragments ready / 룬 조각 준비", self.markup)
        self.assertIn("Missing 1 fragment / 룬 조각 1개 부족", self.markup)

    def test_copy_limits_status_to_current_full_recipe_inventory(self) -> None:
        for marker in (
            "Filters use only current inventory fragment counts for each full recipe.",
            "They do not include inserted progress",
            "do not indicate base compatibility",
            "whether transmutation is currently available",
            "필터는 전체 레시피 기준 현재 인벤토리의 룬 조각만 계산합니다.",
            "기존 삽입 진행을 포함하지 않으며",
            "베이스 호환성이나 현재 실제 변환 가능 여부를 뜻하지 않습니다.",
        ):
            self.assertIn(marker, self.source)

        for marker in (
            'return t("Fragments ready", "룬 조각 준비")',
            'return t("Missing 1 fragment", "룬 조각 1개 부족")',
            "fragments covered",
        ):
            self.assertIn(marker, self.source)

        material_block = self._between(
            "function resolveRecipeMaterialState(item)",
            "function setRecipeBaseFilter(nextFilter)",
        )
        self.assertNotIn("canInsert", material_block)
        self.assertNotIn("baseCompatibility", material_block)
        self.assertNotIn("insertedRunes", material_block)

    def test_rune_tokens_are_strict_decimal_strings_and_preserve_duplicates(self) -> None:
        token_normalizer = self._between(
            "function normalizePositiveUint64DecimalString(value)",
            "function normalizeRecipeRuneTokens(raw)",
        )
        self.assertIn('typeof value !== "string"', token_normalizer)
        self.assertIn("/^[1-9][0-9]*$/", token_normalizer)
        self.assertIn("maxUint64DecimalString", token_normalizer)

        recipe_normalizer = self._between(
            "function normalizeRecipeRuneTokens(raw)",
            "function normalizeRecipeCatalogItems(raw)",
        )
        self.assertIn("normalized.push(token)", recipe_normalizer)
        self.assertNotIn("new Set", recipe_normalizer)
        self.assertNotIn("sort(", recipe_normalizer)

        inventory_normalizer = self._between(
            "function normalizeRuneInventorySnapshot(data)",
            "function applyRuneInventorySnapshot(data)",
        )
        self.assertIn('typeof owned !== "number"', inventory_normalizer)
        self.assertIn("Number.isSafeInteger(owned)", inventory_normalizer)
        self.assertIn("ownedByToken.has(runeToken)", inventory_normalizer)

        material_resolver = self._between(
            "function resolveRecipeMaterialState(item)",
            "function resolveRecipeMaterialBadgeText(materialState)",
        )
        self.assertIn("requiredByToken.get(token)", material_resolver)
        self.assertIn("runeInventoryOwnedByToken.has(token)", material_resolver)
        self.assertIn("Math.min(required, owned)", material_resolver)

    def test_static_signature_includes_validated_rune_tokens(self) -> None:
        signature = self._between(
            "function buildRecipeCatalogSignature(items)",
            "function resolveConfirmedRecipeToken(items)",
        )
        self.assertIn("item?.runeTokens", signature)
        self.assertNotIn("selected", signature)

        setter = self._between(
            "function setRecipeItems(raw)",
            "function setRunewordPanelState(raw)",
        )
        self.assertIn("normalizeRecipeCatalogItems", setter)
        self.assertIn("buildRecipeCatalogSignature(nextItems)", setter)

    def test_dynamic_inventory_preserves_static_dom_and_search_caches(self) -> None:
        dynamic_apply = self._between(
            "function applyRuneInventorySnapshot(data)",
            "function setInventoryItems(raw)",
        )
        self.assertIn("schedulePanelRender(panelRenderSection.recipeItems)", dynamic_apply)
        self.assertNotIn("invalidateRecipePresentationCaches", dynamic_apply)
        self.assertNotIn("recipeCatalogDomDirty", dynamic_apply)
        self.assertNotIn("recipeSearchDocumentByToken.clear", dynamic_apply)
        self.assertNotIn("recipeNodeByToken.clear", dynamic_apply)

        render = self._between(
            "function renderRecipeItems()",
            "function resolveRunewordPanelActionState(state)",
        )
        self.assertIn("if (recipeCatalogDomDirty)", render)
        self.assertIn("refreshRecipeMaterialViews()", render)
        self.assertIn("recipeNodeByToken.get(token)", render)

    def test_unknown_inventory_disables_controls_and_forces_all(self) -> None:
        controls = self._between(
            "function updateRecipeFilterControls()",
            "function initUiText()",
        )
        self.assertIn('recipeMaterialFilters.setAttribute(', controls)
        self.assertIn('"aria-disabled"', controls)
        self.assertIn("button.disabled = !runeInventoryKnownState", controls)

        setter = self._between(
            "function setRecipeMaterialFilter(nextFilter)",
            "function handleRecipeFilterClick(event)",
        )
        self.assertIn('const normalized = runeInventoryKnownState ? requested : "all"', setter)

        dynamic_apply = self._between(
            "function applyRuneInventorySnapshot(data)",
            "function setInventoryItems(raw)",
        )
        self.assertIn('recipeMaterialFilter = "all"', dynamic_apply)


if __name__ == "__main__":
    unittest.main()
