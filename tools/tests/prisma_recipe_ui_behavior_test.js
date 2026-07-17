"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");

const viewPath = path.resolve(
  __dirname,
  "../../Data/PrismaUI/views/CalamityAffixes/index.html"
);
const source = fs.readFileSync(viewPath, "utf8");
const scriptOpen = source.indexOf("<script>");
const scriptClose = source.indexOf("</script>", scriptOpen);
assert(scriptOpen >= 0 && scriptClose > scriptOpen, "main script block missing");
assert.doesNotThrow(
  () => new Function(source.slice(scriptOpen + "<script>".length, scriptClose)),
  "main Prisma script contains invalid JavaScript"
);

function between(startMarker, endMarker) {
  const start = source.indexOf(startMarker);
  const end = source.indexOf(endMarker, start);
  assert(start >= 0 && end > start, `missing source block: ${startMarker}`);
  return source.slice(start, end);
}

const recipeLocalization = between(
  "      function resolveLocalizedRecipeText(",
  "      function resolveRecipeBaseBadge(item)"
);
const recipeSearch = between(
  "      function resolveRecipeBaseBadge(item)",
  "      function resolveRecipeListViewModel()"
);

const runRecipeBehavior = new Function(
  "assert",
  `
    "use strict";
    let uiLang = "en";
    function t(en, ko) {
      if (uiLang === "en") return en;
      if (uiLang === "ko") return ko;
      return en + " / " + ko;
    }
    const recipeSearchDocumentByToken = new Map();
    let recipeItemsState = [];
    let runewordPanelState = {};
    let confirmedRecipeTokenState = "";
    let optimisticRecipeTokenState = "";
    ${recipeLocalization}
    ${recipeSearch}

    const item = {
      token: "42",
      name: "Test Word",
      runes: "El-Eld",
      summaryKey: "behavior_test",
      summaryEn: "On hit 25% chance to cast Test Spell",
      summaryKo: "적중 시 25% 확률로 테스트 주문 시전",
      detailEn: "Internal cooldown (ICD): 4.0s",
      detailKo: "내부 쿨다운 (ICD): 4.0초",
      baseKey: "weapon"
    };

    recipeItemsState = [
      { token: "old", name: "Old Word", selected: true },
      item
    ];
    confirmedRecipeTokenState = "old";
    optimisticRecipeTokenState = item.token;
    assert.strictEqual(getSelectedRecipeItem(), item);
    optimisticRecipeTokenState = "";

    assert.strictEqual(resolveRecipeNumericSummaryText(item), item.summaryEn);
    assert.strictEqual(resolveRecipeDetailText(item), item.detailEn);
    const englishTooltip = buildRecipePreviewTooltipText(item);
    assert(englishTooltip.includes(item.summaryEn));
    assert(englishTooltip.includes(item.detailEn));
    assert(!englishTooltip.includes(item.summaryKo));
    assert(buildRecipeSearchDocument(item).includes(item.detailEn.toLowerCase()));

    uiLang = "ko";
    assert.strictEqual(resolveRecipeNumericSummaryText(item), item.summaryKo);
    assert.strictEqual(resolveRecipeDetailText(item), item.detailKo);
    const koreanTooltip = buildRecipePreviewTooltipText(item);
    assert(koreanTooltip.includes(item.summaryKo));
    assert(koreanTooltip.includes(item.detailKo));
    assert(!koreanTooltip.includes(item.summaryEn));

    uiLang = "both";
    const bilingualTooltip = buildRecipePreviewTooltipText(item);
    assert(bilingualTooltip.includes(item.summaryEn));
    assert(bilingualTooltip.includes(item.summaryKo));
    assert(bilingualTooltip.includes(item.detailEn));
    assert(bilingualTooltip.includes(item.detailKo));

    uiLang = "en";
    assert.strictEqual(
      resolveRecipeNumericSummaryText({
        summaryKey: "adaptive_strike",
        summary: "구형 한국어 native 요약"
      }),
      "Adaptive elemental strike"
    );
  `
);
runRecipeBehavior(assert);

const recipeListViewModelSource = between(
  "      function resolveRecipeListViewModel()",
  "      function createRecipeButton(item)"
);
const runRecipeFilterBehavior = new Function(
  "assert",
  `
    "use strict";
    let uiLang = "en";
    function t(en, ko) {
      if (uiLang === "en") return en;
      if (uiLang === "ko") return ko;
      return en + " / " + ko;
    }
    const validRecipeBaseFilters = new Set(["all", "weapon", "armor", "mixed"]);
    const recipeSearchDocumentByToken = new Map();
    let recipeItemsState = [
      { token: "weapon", name: "Steel Fury", baseKey: "weapon" },
      { token: "armor", name: "Arcane Ward", baseKey: "armor" },
      { token: "mixed", name: "Hybrid Soul", baseKey: "mixed", selected: true }
    ];
    let recipeSearchQuery = "";
    let recipeBaseFilter = "all";
    let runewordPanelState = {};
    let confirmedRecipeTokenState = "mixed";
    let optimisticRecipeTokenState = "";
    const panelRenderSection = { recipeItems: "recipeItems" };
    let filterControlUpdates = 0;
    let scheduledRecipeRenders = 0;
    function updateRecipeFilterControls() {
      filterControlUpdates += 1;
    }
    function schedulePanelRender(section) {
      assert.strictEqual(section, panelRenderSection.recipeItems);
      scheduledRecipeRenders += 1;
    }
    ${recipeLocalization}
    ${recipeSearch}
    ${recipeListViewModelSource}

    let view = resolveRecipeListViewModel();
    assert.strictEqual(view.visibleItems.length, 3);
    assert.strictEqual(view.countText, "Total 3");

    recipeBaseFilter = "weapon";
    view = resolveRecipeListViewModel();
    assert.deepStrictEqual(view.visibleItems.map((item) => item.token), ["weapon"]);
    assert.strictEqual(view.countText, "Showing 1/3");

    recipeBaseFilter = "armor";
    recipeSearchQuery = "ward";
    view = resolveRecipeListViewModel();
    assert.deepStrictEqual(view.visibleItems.map((item) => item.token), ["armor"]);

    recipeBaseFilter = "weapon";
    view = resolveRecipeListViewModel();
    assert.strictEqual(view.visibleItems.length, 0);
    assert(view.emptyState.title.includes("current filters"));
    assert.strictEqual(getSelectedRecipeItem().token, "mixed");

    uiLang = "ko";
    assert.strictEqual(resolveRecipeListViewModel().countText, "표시 0/3");

    assert.strictEqual(setRecipeBaseFilter("mixed"), true);
    assert.strictEqual(recipeBaseFilter, "mixed");
    assert.strictEqual(filterControlUpdates, 1);
    assert.strictEqual(scheduledRecipeRenders, 1);
    assert.strictEqual(setRecipeBaseFilter("mixed"), false);
    assert.strictEqual(scheduledRecipeRenders, 1);
    assert.strictEqual(setRecipeBaseFilter("invalid"), true);
    assert.strictEqual(recipeBaseFilter, "all");
  `
);
runRecipeFilterBehavior(assert);

const panelLayoutModeSource = between(
  "      function resolvePanelLayoutMode(width)",
  "      function updatePanelUiScale()"
);
const runPanelLayoutModeBehavior = new Function(
  "assert",
  `
    "use strict";
    const controlPanel = { dataset: { layout: "wide" } };
    ${panelLayoutModeSource}
    assert.strictEqual(resolvePanelLayoutMode(1100), "wide");
    assert.strictEqual(resolvePanelLayoutMode(1099), "medium");
    assert.strictEqual(resolvePanelLayoutMode(900), "medium");
    assert.strictEqual(resolvePanelLayoutMode(899), "narrow");
    assert.strictEqual(applyPanelLayoutMode(1320), false);
    assert.strictEqual(applyPanelLayoutMode(1050), true);
    assert.strictEqual(controlPanel.dataset.layout, "medium");
    assert.strictEqual(applyPanelLayoutMode(1000), false);
    assert.strictEqual(applyPanelLayoutMode(850), true);
    assert.strictEqual(controlPanel.dataset.layout, "narrow");
  `
);
runPanelLayoutModeBehavior(assert);

const runewordStepStateSource = between(
  "      function triggerRunewordStateShift(element, state)",
  "      function renderRunewordFlowProgress(actionState, state)"
);
const runRunewordStepStateBehavior = new Function(
  "assert",
  `
    "use strict";
    const window = { setTimeout: (callback) => callback() };
    const classes = new Set();
    const attributes = new Map();
    const element = {
      dataset: {},
      classList: {
        add: (name) => classes.add(name),
        remove: (name) => classes.delete(name),
        toggle: (name, enabled) => enabled ? classes.add(name) : classes.delete(name)
      },
      setAttribute: (name, value) => attributes.set(name, value),
      removeAttribute: (name) => attributes.delete(name)
    };
    ${runewordStepStateSource}
    setRunewordStepCardState(element, "active");
    assert.strictEqual(attributes.get("aria-current"), "step");
    assert(classes.has("active"));
    setRunewordStepCardState(element, "complete");
    assert(!attributes.has("aria-current"));
    assert(classes.has("complete"));
    assert(!classes.has("active"));
  `
);
runRunewordStepStateBehavior(assert);

const inspectorMergeSource = between(
  "      function buildSelectedRecipeInspectorText(",
  "      function applyTooltipPlacement()"
);
const runInspectorMergeBehavior = new Function(
  "assert",
  `
    "use strict";
    const t = (en) => en;
    ${inspectorMergeSource}
    const recipe = "Recipe summary\\nNative recipe detail";
    const base = "Existing base affix";
    const merged = buildSelectedRecipeInspectorText(recipe, base);
    assert(merged.startsWith(recipe));
    assert(merged.includes("Selected base affixes\\n" + base));
  `
);
runInspectorMergeBehavior(assert);

const maxHeightSource = between(
  "      function getTooltipMaxLogicalHeight()",
  "      function fitTooltipLayoutToViewport(rawLayout, rect)"
);
const fitLayoutSource = between(
  "      function fitTooltipLayoutToViewport(rawLayout, rect)",
  "      function measureTooltipRect()"
);

const runTooltipLayoutBehavior = new Function(
  "assert",
  `
    "use strict";
    const window = { innerWidth: 1280, innerHeight: 720 };
    const clamp = (value, minimum, maximum) =>
      Math.min(maximum, Math.max(minimum, value));
    const normalizeTooltipLayout = (layout) => ({ ...layout });
    let autoScale = 1;
    const getTooltipAutoScale = () => autoScale;
    ${maxHeightSource}
    ${fitLayoutSource}

    assert.strictEqual(getTooltipMaxLogicalHeight(), 560);
    autoScale = 2;
    assert.strictEqual(getTooltipMaxLogicalHeight(), 352);

    const fitted = fitTooltipLayoutToViewport(
      { right: 1100, top: 600, fontScale: 1.8 },
      { width: 720, height: 560 }
    );
    assert.deepStrictEqual(fitted, {
      right: 552,
      top: 152,
      fontScale: 1.8
    });
  `
);
runTooltipLayoutBehavior(assert);

const tooltipSetterSource = between(
  "      function setTooltip(raw)",
  "      function parseControlPanelOpenState(raw)"
);
const runTooltipSetterBehavior = new Function(
  "assert",
  `
    "use strict";
    const tooltipPanel = { scrollTop: 240 };
    const panelRenderSection = { tooltipPlacement: "tooltipPlacement" };
    let tooltipTextState = "old";
    let scheduled = 0;
    function schedulePanelRender(section) {
      assert.strictEqual(section, panelRenderSection.tooltipPlacement);
      scheduled += 1;
    }
    ${tooltipSetterSource}

    setTooltip("new");
    assert.strictEqual(tooltipPanel.scrollTop, 0);
    tooltipPanel.scrollTop = 180;
    setTooltip("new");
    assert.strictEqual(tooltipPanel.scrollTop, 180);
    assert.strictEqual(scheduled, 2);
  `
);
runTooltipSetterBehavior(assert);

const listboxNavigationSource = between(
  "      function resolveListboxNavigationIndex(",
  "      function handleListboxKeydown(event)"
);
const runListboxNavigationBehavior = new Function(
  "assert",
  `
    "use strict";
    ${listboxNavigationSource}
    assert.strictEqual(resolveListboxNavigationIndex(1, "ArrowDown", 4), 2);
    assert.strictEqual(resolveListboxNavigationIndex(1, "ArrowRight", 4), 2);
    assert.strictEqual(resolveListboxNavigationIndex(1, "ArrowUp", 4), 0);
    assert.strictEqual(resolveListboxNavigationIndex(1, "ArrowLeft", 4), 0);
    assert.strictEqual(resolveListboxNavigationIndex(2, "Home", 4), 0);
    assert.strictEqual(resolveListboxNavigationIndex(1, "End", 4), 3);
    assert.strictEqual(resolveListboxNavigationIndex(0, "ArrowUp", 4), 0);
    assert.strictEqual(resolveListboxNavigationIndex(3, "ArrowDown", 4), 3);
    assert.strictEqual(resolveListboxNavigationIndex(1, "Enter", 4), -1);
    assert.strictEqual(resolveListboxNavigationIndex(-1, "Home", 4), -1);
  `
);
runListboxNavigationBehavior(assert);

console.log("Prisma recipe UI behavior: OK");
