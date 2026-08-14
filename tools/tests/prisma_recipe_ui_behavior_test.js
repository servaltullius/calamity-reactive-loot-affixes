"use strict";

const assert = require("assert");

// The view's JS lives in scripts/*.js; read it as one buffer so the slices
// below span the whole program rather than whichever file they happen to hit.
const { loadScripts } = require("../prisma_view_source.js");
const source = loadScripts();
assert(source.length > 0, "main script block missing");
assert.doesNotThrow(
  () => new Function(source),
  "main Prisma script contains invalid JavaScript"
);

function between(startMarker, endMarker) {
  const start = source.indexOf(startMarker);
  const end = source.indexOf(endMarker, start);
  assert(start >= 0 && end > start, `missing source block: ${startMarker}`);
  return source.slice(start, end);
}

const recipeLocalization = between(
  "\nfunction resolveLocalizedRecipeText(",
  "\nfunction resolveRecipeBaseBadge(item)"
);
const recipeSearch = between(
  "\nfunction resolveRecipeBaseBadge(item)",
  "\nfunction resolveRecipeListViewModel()"
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
  "\nfunction resolveRecipeListViewModel()",
  "\nfunction createRecipeButton(item)"
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
    const validRecipeMaterialFilters = new Set(["all", "ready", "missing1"]);
    const recipeSearchDocumentByToken = new Map();
    let recipeItemsState = [
      { token: "weapon", name: "Steel Fury", baseKey: "weapon", runeTokens: ["11", "11"] },
      { token: "armor", name: "Arcane Ward", baseKey: "armor", runeTokens: ["11", "22"] },
      { token: "mixed", name: "Hybrid Soul", baseKey: "mixed", runeTokens: ["22", "22", "33"], selected: true }
    ];
    let recipeSearchQuery = "";
    let recipeBaseFilter = "all";
    let recipeMaterialFilter = "all";
    let runeInventoryKnownState = true;
    let runeInventoryOwnedByToken = new Map([
      ["11", 2],
      ["22", 0],
      ["33", 0]
    ]);
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
    assert.strictEqual(resolveRecipeMaterialState(recipeItemsState[0]).key, "ready");
    assert.strictEqual(resolveRecipeMaterialState(recipeItemsState[1]).key, "missing1");
    assert.strictEqual(resolveRecipeMaterialState(recipeItemsState[2]).missingFragments, 3);

    const repeatedRune = { runeTokens: ["11", "11", "22"] };
    runeInventoryOwnedByToken.set("11", 1);
    runeInventoryOwnedByToken.set("22", 1);
    const repeatedState = resolveRecipeMaterialState(repeatedRune);
    assert.strictEqual(repeatedState.requiredFragments, 3);
    assert.strictEqual(repeatedState.coveredFragments, 2);
    assert.strictEqual(repeatedState.missingFragments, 1);
    runeInventoryOwnedByToken.set("11", 2);
    runeInventoryOwnedByToken.set("22", 0);

    assert.strictEqual(
      resolveRecipeMaterialState({ runeTokens: ["11", "44"] }).known,
      false,
      "an incomplete known inventory snapshot must fail closed per recipe"
    );
    assert.strictEqual(normalizeRecipeRuneTokens([11]), null);
    assert.deepStrictEqual(normalizeRecipeRuneTokens(["11", "11"]), ["11", "11"]);

    recipeBaseFilter = "weapon";
    view = resolveRecipeListViewModel();
    assert.deepStrictEqual(view.visibleItems.map((item) => item.token), ["weapon"]);
    assert.strictEqual(view.countText, "Showing 1/3");

    recipeBaseFilter = "armor";
    recipeSearchQuery = "ward";
    recipeMaterialFilter = "missing1";
    view = resolveRecipeListViewModel();
    assert.deepStrictEqual(view.visibleItems.map((item) => item.token), ["armor"]);

    recipeBaseFilter = "weapon";
    view = resolveRecipeListViewModel();
    assert.strictEqual(view.visibleItems.length, 0);
    assert(view.emptyState.title.includes("current filters"));
    assert.strictEqual(getSelectedRecipeItem().token, "mixed");

    recipeBaseFilter = "all";
    recipeSearchQuery = "";
    recipeMaterialFilter = "ready";
    view = resolveRecipeListViewModel();
    assert.deepStrictEqual(view.visibleItems.map((item) => item.token), ["weapon"]);
    assert.strictEqual(getSelectedRecipeItem().token, "mixed");

    uiLang = "ko";
    assert.strictEqual(resolveRecipeListViewModel().countText, "표시 1/3");

    assert.strictEqual(setRecipeBaseFilter("mixed"), true);
    assert.strictEqual(recipeBaseFilter, "mixed");
    assert.strictEqual(filterControlUpdates, 1);
    assert.strictEqual(scheduledRecipeRenders, 1);
    assert.strictEqual(setRecipeBaseFilter("mixed"), false);
    assert.strictEqual(scheduledRecipeRenders, 1);
    assert.strictEqual(setRecipeBaseFilter("invalid"), true);
    assert.strictEqual(recipeBaseFilter, "all");

    uiLang = "en";
    assert.strictEqual(setRecipeMaterialFilter("missing1"), true);
    assert.strictEqual(recipeMaterialFilter, "missing1");
    assert.strictEqual(resolveRecipeListViewModel().visibleItems[0].token, "armor");
    runeInventoryKnownState = false;
    assert.strictEqual(setRecipeMaterialFilter("ready"), true);
    assert.strictEqual(recipeMaterialFilter, "all");
    view = resolveRecipeListViewModel();
    assert.strictEqual(view.activeMaterialFilter, "all");
    assert.strictEqual(view.visibleItems.length, 3);
  `
);
runRecipeFilterBehavior(assert);

const panelLayoutModeSource = between(
  "\nfunction resolvePanelLayoutMode(width)",
  "\nfunction updatePanelUiScale()"
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
  "\nfunction triggerRunewordStateShift(element, state)",
  "\nfunction renderRunewordFlowProgress(actionState, state)"
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

const inspectorSplitSource = between(
  "\nfunction resolveRunewordInspectorTexts(",
  "\nfunction applyTooltipPlacement()"
);
const runInspectorSplitBehavior = new Function(
  "assert",
  `
    "use strict";
    const t = (en) => en;
    ${inspectorSplitSource}
    const filled = resolveRunewordInspectorTexts("Recipe detail", "Existing base affix", false);
    assert.strictEqual(filled.recipeText, "Recipe detail");
    assert.strictEqual(filled.baseText, "Existing base affix");
    assert(!filled.recipeText.includes(filled.baseText));
    const empty = resolveRunewordInspectorTexts("", "", false);
    assert(empty.recipeText.includes("Select a recipe"));
    assert(empty.baseText.includes("Select an equipped base"));
    const pending = resolveRunewordInspectorTexts("", "", true);
    assert(pending.baseText.includes("Refreshing"));
  `
);
runInspectorSplitBehavior(assert);

const maxHeightSource = between(
  "\nfunction getTooltipMaxLogicalHeight()",
  "\nfunction fitTooltipLayoutToViewport(rawLayout, rect)"
);
const fitLayoutSource = between(
  "\nfunction fitTooltipLayoutToViewport(rawLayout, rect)",
  "\nfunction measureTooltipRect()"
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
  "\nfunction setTooltip(raw)",
  "\nfunction parseControlPanelOpenState(raw)"
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
  "\nfunction resolveListboxNavigationIndex(",
  "\nfunction handleListboxKeydown(event)"
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

const workingBaseChooserLifecycleSource = between(
  "\nfunction focusWorkingBaseChooserOption()",
  "\nfunction sendCommand(command)"
);
const panelKeydownSource = between(
  "\nfunction handlePanelKeydown(event)",
  "\nfunction shouldInvalidatePreviewForCommand(command)"
);
const panelCommandDispatchSource = between(
  "\nfunction dispatchPanelCommand(button)",
  "\nfunction handleDelegatedPanelCommandClick(event)"
);
const runWorkingBaseChooserBehavior = new Function(
  "assert",
  `
    "use strict";
    let focusCount = 0;
    let expanded = "true";
    const workingBaseDetails = {
      open: true,
      contains() { return false; }
    };
    const runewordBaseChooserSummary = {
      focus() { focusCount += 1; },
      setAttribute(name, value) {
        if (name === "aria-expanded") expanded = value;
      }
    };
    const inventoryBaseList = { contains() { return false; } };
    const document = { activeElement: null };
    function requestAnimationFrame() {}
    function setTimeout() {}
    ${workingBaseChooserLifecycleSource}

    assert.strictEqual(closeWorkingBaseChooser(true), true);
    assert.strictEqual(workingBaseDetails.open, false);
    assert.strictEqual(expanded, "false");
    assert.strictEqual(focusCount, 1);
    assert.strictEqual(closeWorkingBaseChooser(true), false);

    let controlPanelOpen = true;
    const sent = [];
    function sendCommand(command) { sent.push(command); }
    ${panelKeydownSource}

    let prevented = 0;
    let stopped = 0;
    const escapeEvent = {
      key: "Escape",
      code: "Escape",
      keyCode: 27,
      preventDefault() { prevented += 1; },
      stopPropagation() { stopped += 1; }
    };
    workingBaseDetails.open = true;
    handlePanelKeydown(escapeEvent);
    assert.strictEqual(workingBaseDetails.open, false);
    assert.deepStrictEqual(sent, []);
    assert.strictEqual(focusCount, 2);

    handlePanelKeydown(escapeEvent);
    assert.deepStrictEqual(sent, ["ui.close"]);
    assert.strictEqual(prevented, 2);
    assert.strictEqual(stopped, 2);

    const panelCommandAttribute = "data-cmd";
    const panelOpenTabAttribute = "data-open-tab";
    const recipeSelectionCommandPrefix = "runeword.recipe.select:";
    const affixExpandCommandPrefix = "affix.expand:";
    let runewordResetArmedUntil = 0;
    let pendingOpenMainTab = null;
    let previewInvalidations = 0;
    let affixExpandPendingState = null;
    const begunAffixExpansions = [];
    const panelRenderSection = { runewordPanelState: "runewordPanelState" };
    function handleTooltipUiCommand() { return false; }
    function armRunewordResetConfirmation() { return false; }
    function clearRunewordResetConfirmation() {}
    function beginOptimisticRecipeSelection() {}
    function beginAffixExpandPending(command) {
      begunAffixExpansions.push(command);
      return true;
    }
    function setActionFeedback() {}
    function schedulePanelRender() {}
    function shouldInvalidatePreviewForCommand(command) {
      return command.startsWith("runeword.base.select:") ||
        command.startsWith(affixExpandCommandPrefix);
    }
    function resolvePreviewPendingStateForCommand() { return true; }
    function invalidateRunewordAffixPreview() { previewInvalidations += 1; }
    ${panelCommandDispatchSource}

    sent.length = 0;
    workingBaseDetails.open = true;
    const candidate = {
      disabled: false,
      getAttribute(name) {
        return name === panelCommandAttribute
          ? "runeword.base.select:18446744073709551615"
          : null;
      }
    };
    inventoryBaseList.contains = (button) => button === candidate;
    assert.strictEqual(dispatchPanelCommand(candidate), true);
    assert.deepStrictEqual(sent, ["runeword.base.select:18446744073709551615"]);
    assert.strictEqual(previewInvalidations, 1);
    assert.strictEqual(workingBaseDetails.open, false);
    assert.strictEqual(focusCount, 3);

    const expandCandidate = {
      getAttribute(name) {
        return name === panelCommandAttribute ? "affix.expand:42:1" : null;
      }
    };
    assert.strictEqual(dispatchPanelCommand(expandCandidate), true);
    assert.deepStrictEqual(begunAffixExpansions, ["affix.expand:42:1"]);
    assert.deepStrictEqual(sent, [
      "runeword.base.select:18446744073709551615",
      "affix.expand:42:1"
    ]);
    assert.strictEqual(previewInvalidations, 2);
  `
);
runWorkingBaseChooserBehavior(assert);

console.log("Prisma recipe UI behavior: OK");
