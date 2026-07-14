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
