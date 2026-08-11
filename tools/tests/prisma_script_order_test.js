"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const {
  VIEW_DIR,
  scriptPaths
} = require("../prisma_view_source.js");

class FakeClassList {
  constructor() { this.values = new Set(); }
  add(...names) { for (const name of names) this.values.add(name); }
  remove(...names) { for (const name of names) this.values.delete(name); }
  toggle(name, enabled) {
    const next = enabled === undefined ? !this.values.has(name) : Boolean(enabled);
    if (next) this.values.add(name);
    else this.values.delete(name);
    return next;
  }
  contains(name) { return this.values.has(name); }
}

class FakeElement {
  constructor(tagName = "div") {
    this.tagName = tagName.toUpperCase();
    this.classList = new FakeClassList();
    this.className = "";
    this.children = [];
    this.attributes = new Map();
    this.dataset = {};
    this.hidden = false;
    this.style = { setProperty() {} };
    this.scrollTop = 0;
    this.scrollHeight = 0;
    this.clientHeight = 0;
    this.textContent = "";
    this.title = "";
  }

  addEventListener() {}
  appendChild(child) { this.children.push(child); return child; }
  contains() { return false; }
  focus() {}
  get firstChild() { return this.children[0] || null; }
  getAttribute(name) { return this.attributes.has(name) ? this.attributes.get(name) : null; }
  getBoundingClientRect() {
    return { left: 0, top: 0, right: 0, bottom: 0, width: 0, height: 0 };
  }
  removeAttribute(name) { this.attributes.delete(name); }
  removeChild(child) {
    const index = this.children.indexOf(child);
    if (index >= 0) this.children.splice(index, 1);
    return child;
  }
  replaceChildren(...children) { this.children = [...children]; }
  setAttribute(name, value) { this.attributes.set(name, String(value)); }
}

const elements = new Map();
function element(id) {
  if (!elements.has(id)) {
    elements.set(id, new FakeElement());
  }
  return elements.get(id);
}

const document = {
  activeElement: null,
  body: element("body"),
  documentElement: element("documentElement"),
  addEventListener() {},
  createDocumentFragment: () => new FakeElement("fragment"),
  createElement: (tagName) => new FakeElement(tagName),
  getElementById: element,
  querySelectorAll: () => []
};

let nextAnimationFrame = 1;
const sandbox = {
  URL,
  Element: FakeElement,
  cancelAnimationFrame() {},
  clearTimeout() {},
  console,
  document,
  getComputedStyle: () => ({ display: "block", overflowY: "auto" }),
  innerHeight: 1080,
  innerWidth: 1920,
  location: { href: "file:///CalamityAffixes/index.html" },
  matchMedia: () => ({ matches: false }),
  addEventListener() {},
  requestAnimationFrame() {
    const id = nextAnimationFrame;
    nextAnimationFrame += 1;
    return id;
  },
  setTimeout() { return 1; }
};
sandbox.window = sandbox;

const context = vm.createContext(sandbox);
const paths = scriptPaths();
assert(paths.length > 0, "Prisma view has no external scripts");

for (const scriptPath of paths) {
  const relative = path.relative(VIEW_DIR, scriptPath).replaceAll("\\", "/");
  const source = fs.readFileSync(scriptPath, "utf8");
  try {
    new vm.Script(source, { filename: relative }).runInContext(context);
  } catch (error) {
    assert.fail(`${relative} failed in HTML script order: ${error.stack || error}`);
  }
}

assert.strictEqual(
  typeof sandbox.setControlPanel,
  "function",
  "bootstrap did not register the control-panel fallback"
);
assert.strictEqual(
  typeof sandbox.setRunewordPanelState,
  "function",
  "bootstrap did not register the runeword-state fallback"
);

const compatibilityPayload = {
  hasBase: true,
  hasRecipe: true,
  isComplete: true,
  baseCompatibilityWarning: true,
  baseCompatibilityMessageEn: "Recommended base: Armor. Selected base: Weapon.",
  baseCompatibilityMessageKo: "권장 베이스: 방어구. 선택 베이스: 무기."
};
sandbox.setRunewordPanelState(JSON.stringify(compatibilityPayload));

const storedRunewordState = new vm.Script(
  "runewordPanelState",
  { filename: "runeword-state-contract-test.js" }
).runInContext(context);
assert.strictEqual(storedRunewordState.baseCompatibilityWarning, true);
assert.strictEqual(
  storedRunewordState.baseCompatibilityMessageEn,
  compatibilityPayload.baseCompatibilityMessageEn
);
assert.strictEqual(
  storedRunewordState.baseCompatibilityMessageKo,
  compatibilityPayload.baseCompatibilityMessageKo
);

const buildSummaryPayload = {
  ready: true,
  runtimeEnabled: true,
  equippedAffixSlots: 8,
  entries: [
    {
      token: "18446744073709551613",
      displayNameEn: "Storm Brand",
      displayNameKo: "폭풍 낙인",
      group: "offense",
      triggerKey: "hit",
      slotKind: "prefix",
      suffixState: "none",
      equippedCount: 2,
      hasPassiveContribution: true,
      passiveContributionActive: true,
      passiveSpellDisabled: false,
      hasProcRoll: true,
      procRollChancePct: 24.5,
      hasLuckyHitGate: true,
      luckyHitGateChancePct: 30
    },
    {
      token: "18446744073709551612",
      displayNameEn: "Last Shelter",
      displayNameKo: "마지막 피난처",
      group: "defense",
      triggerKey: "lowHealth",
      slotKind: "runeword",
      suffixState: "none",
      equippedCount: 1,
      hasProcRoll: false,
      procRollChancePct: 0,
      hasLuckyHitGate: false,
      luckyHitGateChancePct: 0
    },
    {
      token: "18446744073709551611",
      displayNameEn: "Grave Spark",
      displayNameKo: "무덤 불꽃",
      group: "kill",
      triggerKey: "kill",
      slotKind: "prefix",
      suffixState: "none",
      equippedCount: 1,
      hasProcRoll: true,
      procRollChancePct: 120,
      hasLuckyHitGate: false,
      luckyHitGateChancePct: 0
    },
    {
      token: "18446744073709551610",
      displayNameEn: "Vitality III",
      displayNameKo: "활력 III",
      group: "passive",
      triggerKey: "passive",
      slotKind: "suffix",
      suffixState: "highest",
      equippedCount: 1,
      hasPassiveContribution: true,
      passiveContributionActive: true,
      passiveSpellDisabled: true,
      hasProcRoll: false,
      procRollChancePct: 0,
      hasLuckyHitGate: false,
      luckyHitGateChancePct: 0
    },
    {
      token: "18446744073709551609",
      displayNameEn: "Vitality I",
      displayNameKo: "활력 I",
      group: "passive",
      triggerKey: "passive",
      slotKind: "suffix",
      suffixState: "suppressed",
      equippedCount: 1,
      hasPassiveContribution: true,
      passiveContributionActive: false,
      passiveSpellDisabled: true,
      hasProcRoll: false,
      procRollChancePct: 0,
      hasLuckyHitGate: false,
      luckyHitGateChancePct: 0
    },
    {
      token: "18446744073709551608",
      displayNameEn: "Traveler",
      displayNameKo: "여행자",
      group: "passive",
      triggerKey: "passive",
      slotKind: "suffix",
      suffixState: "stacking",
      equippedCount: 1,
      hasPassiveContribution: true,
      passiveContributionActive: false,
      passiveSpellDisabled: true,
      hasProcRoll: false,
      procRollChancePct: 0,
      hasLuckyHitGate: false,
      luckyHitGateChancePct: 0
    },
    {
      token: "18446744073709551607",
      displayNameEn: "Scrollkeeper I",
      displayNameKo: "두루마리 수호자 I",
      group: "passive",
      triggerKey: "passive",
      slotKind: "suffix",
      suffixState: "suppressed",
      equippedCount: 1,
      hasPassiveContribution: true,
      passiveContributionActive: true,
      passiveSpellDisabled: true,
      hasProcRoll: false,
      procRollChancePct: 0,
      hasLuckyHitGate: false,
      luckyHitGateChancePct: 0
    },
    // Numeric 64-bit tokens are unsafe and must not enter the summary state.
    {
      token: 18446744073709551606,
      displayNameEn: "Unsafe number",
      displayNameKo: "안전하지 않은 숫자",
      group: "offense",
      triggerKey: "hit",
      slotKind: "prefix",
      suffixState: "none",
      equippedCount: 1,
      hasProcRoll: true,
      procRollChancePct: 10,
      hasLuckyHitGate: false,
      luckyHitGateChancePct: 0
    }
  ]
};

sandbox.setRunewordPanelState(JSON.stringify({
  ...compatibilityPayload,
  equippedBuild: buildSummaryPayload
}));
const storedEquippedBuildState = new vm.Script(
  "equippedBuildState",
  { filename: "equipped-build-state-contract-test.js" }
).runInContext(context);
assert.strictEqual(storedEquippedBuildState.received, true);
assert.strictEqual(storedEquippedBuildState.ready, true);
assert.strictEqual(storedEquippedBuildState.entries.length, 7);
assert.strictEqual(storedEquippedBuildState.entries[0].token, "18446744073709551613");
assert.strictEqual(storedEquippedBuildState.entries[0].hasPassiveContribution, true);
assert.strictEqual(storedEquippedBuildState.entries[0].passiveContributionActive, true);
assert.strictEqual(storedEquippedBuildState.entries[0].passiveSpellDisabled, false);
assert.strictEqual(storedEquippedBuildState.entries[1].hasPassiveContribution, false);
assert.strictEqual(storedEquippedBuildState.entries[1].passiveContributionActive, false);
assert.strictEqual(storedEquippedBuildState.entries[1].passiveSpellDisabled, false);
assert.strictEqual(storedEquippedBuildState.entries[2].procRollChancePct, 100);
assert.strictEqual(sandbox.resolveEquippedBuildViewState(storedEquippedBuildState), "ready");

sandbox.renderEquippedBuildSummary();
assert.strictEqual(element("equippedBuildGroups").hidden, false);
assert.strictEqual(element("equippedBuildChanceHint").hidden, false);
assert.strictEqual(element("equippedBuildGroups").getAttribute("aria-busy"), "false");
assert.strictEqual(element("equippedBuildOffenseCount").textContent, "2");
assert.strictEqual(element("equippedBuildDefenseCount").textContent, "1");
assert.strictEqual(element("equippedBuildKillCount").textContent, "1");
assert.strictEqual(element("equippedBuildPassiveCount").textContent, "6");
assert(
  element("equippedBuildPassiveCount").getAttribute("aria-label").includes(
    "6 effect copies shown in this group"
  )
);
assert.strictEqual(element("equippedBuildOffenseList").children.length, 1);
assert.strictEqual(element("equippedBuildPassiveList").children.length, 5);
assert.strictEqual(element("equippedBuildSlotCount").textContent, "8 slots / 슬롯 8개");

function collectFakeElementText(node) {
  return [node.textContent, ...node.children.map(collectFakeElementText)].join(" ");
}

const offenseText = collectFakeElementText(element("equippedBuildOffenseList"));
assert(offenseText.includes("On hit"));
assert(offenseText.includes("Conditional proc roll 24.5%"));
assert(offenseText.includes("Shared single roll"));
assert(offenseText.includes("Lucky Hit gate 30%"));
assert(offenseText.includes("Passive also active"));
const passiveText = collectFakeElementText(element("equippedBuildPassiveList"));
assert(passiveText.includes("Storm Brand"));
assert(passiveText.includes("Highest tier selected"));
assert(passiveText.includes("Suppressed by higher tier"));
assert(passiveText.includes("Independent suffix"));
assert(passiveText.includes("Passive active"));
assert(passiveText.includes("Stat passive active · spell off"));
assert(passiveText.includes("Passive spell disabled by runtime setting"));
assert(passiveText.includes("Other passive contribution active"));
const suppressedPassiveEntry = element("equippedBuildPassiveList").children.find(
  (child) => child.children[0]?.children[0]?.textContent.startsWith("Vitality I /")
);
assert(suppressedPassiveEntry);
assert.strictEqual(suppressedPassiveEntry.className, "ebEntry suppressed");
assert(
  !collectFakeElementText(suppressedPassiveEntry).includes(
    "Passive spell disabled by runtime setting"
  )
);
const partiallySuppressedPassiveEntry = element("equippedBuildPassiveList").children.find(
  (child) => child.children[0]?.children[0]?.textContent.startsWith("Scrollkeeper I /")
);
assert(partiallySuppressedPassiveEntry);
assert.strictEqual(partiallySuppressedPassiveEntry.className, "ebEntry suppressed-partial");
assert(
  collectFakeElementText(partiallySuppressedPassiveEntry).includes(
    "Other passive contribution active"
  )
);
assert(
  element("equippedBuildChanceHint").textContent.includes(
    "Hybrid effects may appear in both their trigger group and Passives"
  )
);

sandbox.setRunewordPanelState(JSON.stringify({
  equippedBuild: { ready: true, runtimeEnabled: false, equippedAffixSlots: 0, entries: [] }
}));
sandbox.renderEquippedBuildSummary();
assert.strictEqual(sandbox.resolveEquippedBuildViewState(), "runtime-disabled");
assert.strictEqual(element("equippedBuildGroups").hidden, true);
assert.strictEqual(element("equippedBuildChanceHint").hidden, true);
assert(collectFakeElementText(element("equippedBuildStatus")).includes("Calamity effects are disabled"));

sandbox.setRunewordPanelState(JSON.stringify({
  equippedBuild: { ready: false, runtimeEnabled: false, equippedAffixSlots: 0, entries: [] }
}));
sandbox.renderEquippedBuildSummary();
assert.strictEqual(
  sandbox.resolveEquippedBuildViewState(),
  "syncing",
  "startup/cache-not-ready state must not be presented as an explicit runtime disable"
);

sandbox.setRunewordPanelState(JSON.stringify({
  equippedBuild: { ready: false, runtimeEnabled: true, equippedAffixSlots: 0, entries: [] }
}));
sandbox.renderEquippedBuildSummary();
assert.strictEqual(sandbox.resolveEquippedBuildViewState(), "syncing");
assert.strictEqual(element("equippedBuildGroups").getAttribute("aria-busy"), "true");
assert(collectFakeElementText(element("equippedBuildStatus")).includes("Synchronizing equipped effects"));

sandbox.setRunewordPanelState(JSON.stringify({
  equippedBuild: { ready: true, runtimeEnabled: true, equippedAffixSlots: 0, entries: [] }
}));
sandbox.renderEquippedBuildSummary();
assert.strictEqual(sandbox.resolveEquippedBuildViewState(), "empty");
assert(collectFakeElementText(element("equippedBuildStatus")).includes("No Calamity affixes equipped"));

// Restore the original fixture before the existing runeword action assertions.
sandbox.setRunewordPanelState(JSON.stringify(compatibilityPayload));

const completedRunewordActionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState)",
  { filename: "runeword-reforge-contract-test.js" }
).runInContext(context);
assert(
  completedRunewordActionState.baseCompatibilityMessage.includes(
    compatibilityPayload.baseCompatibilityMessageEn
  ),
  "base compatibility warning was not preserved through the action-state resolver"
);
assert(
  completedRunewordActionState.reforgeHint.includes(
    "reroll only the regular affixes"
  ),
  "completed-runeword Reforge hint must describe the regular-affix-only runtime contract"
);
assert(
  completedRunewordActionState.reforgeHint.includes(
    "완성된 룬워드는 유지됩니다"
  ),
  "completed-runeword Reforge hint must say the runeword is preserved"
);
assert(
  !completedRunewordActionState.reforgeHint.includes("runeword effect + affixes"),
  "completed-runeword Reforge hint still promises to reroll the runeword"
);

const reforgeBaseA = "4294967297";
const reforgeBaseB = "4294967298";
const lockedReforgeCommandPrefix = "runeword.reforge:";
const lockedPrefixToken = "18446744073709551614";
const lockedSuffixToken = "9223372036854775807";
const buildReforgePayload = (overrides = {}) => ({
  hasBase: true,
  hasRecipe: false,
  regularAffixCount: 3,
  reforgeOrbsOwned: 1,
  standardReforgeCost: 1,
  lockedReforgeCost: 2,
  reforgeLockCandidates: [
    {
      affixToken: lockedPrefixToken,
      displayNameEn: "Stormbound",
      displayNameKo: "폭풍결속",
      slotKind: "prefix"
    },
    {
      affixToken: lockedSuffixToken,
      displayNameEn: "of Shelter",
      displayNameKo: "피난의",
      slotKind: "suffix"
    },
    // Numeric 64-bit tokens are unsafe in JS and must not enter UI state.
    {
      affixToken: 18446744073709551614,
      displayNameEn: "Unsafe Number",
      displayNameKo: "안전하지 않은 숫자",
      slotKind: "prefix"
    }
  ],
  ...overrides
});

sandbox.setInventoryItems(JSON.stringify([
  { key: reforgeBaseA, name: "Base A", selected: true }
]));
sandbox.setRunewordPanelState(JSON.stringify(buildReforgePayload()));

let reforgeActionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState)",
  { filename: "reforge-standard-cost-contract-test.js" }
).runInContext(context);
assert.strictEqual(reforgeActionState.reforgeCommand, "runeword.reforge");
assert.strictEqual(reforgeActionState.reforgeCost, 1);
assert.strictEqual(reforgeActionState.reforgeEnabled, true);
assert.strictEqual(reforgeActionState.reforgeLockCandidates.length, 2);
assert.strictEqual(
  reforgeActionState.reforgeLockCandidates[0].affixToken,
  lockedPrefixToken,
  "64-bit reforge token did not remain a decimal string"
);
assert.strictEqual(
  sandbox.shouldInvalidatePreviewForCommand(`${lockedReforgeCommandPrefix}${reforgeBaseA}:${lockedPrefixToken}`),
  true,
  "locked reforge command must invalidate the selected-base affix preview"
);

assert.strictEqual(
  sandbox.selectReforgeLockCandidate(lockedPrefixToken, false),
  true,
  "eligible prefix lock candidate could not be selected"
);
reforgeActionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState)",
  { filename: "reforge-locked-cost-contract-test.js" }
).runInContext(context);
assert.strictEqual(
  reforgeActionState.reforgeCommand,
  `${lockedReforgeCommandPrefix}${reforgeBaseA}:${lockedPrefixToken}`
);
assert.strictEqual(reforgeActionState.reforgeCost, 2);
assert.strictEqual(reforgeActionState.reforgeEnabled, false);
assert(
  reforgeActionState.reforgeCostSummary.includes("Need 1 more"),
  "locked reforge insufficient-orb summary is missing"
);

sandbox.setRunewordPanelState(JSON.stringify(buildReforgePayload({ reforgeOrbsOwned: 2 })));
reforgeActionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState)",
  { filename: "reforge-lock-retention-contract-test.js" }
).runInContext(context);
assert.strictEqual(
  reforgeActionState.lockedReforgeCandidate.affixToken,
  lockedPrefixToken,
  "valid lock selection was not retained across a panel refresh"
);
assert.strictEqual(reforgeActionState.reforgeEnabled, true);

sandbox.setInventoryItems(JSON.stringify([
  { key: reforgeBaseB, name: "Base B", selected: true }
]));
assert.strictEqual(
  new vm.Script("reforgeLockTokenState", { filename: "reforge-base-reset-test.js" }).runInContext(context),
  "",
  "changing selected base did not clear the volatile reforge lock"
);

sandbox.setInventoryItems(JSON.stringify([
  { key: reforgeBaseA, name: "Base A", selected: true }
]));
sandbox.setRunewordPanelState(JSON.stringify(buildReforgePayload({ reforgeOrbsOwned: 2 })));
assert.strictEqual(sandbox.selectReforgeLockCandidate(lockedSuffixToken, false), true);
sandbox.setRunewordPanelState(JSON.stringify(buildReforgePayload({
  reforgeOrbsOwned: 2,
  reforgeLockCandidates: [
    {
      affixToken: lockedPrefixToken,
      displayNameEn: "Stormbound",
      displayNameKo: "폭풍결속",
      slotKind: "prefix"
    }
  ]
})));
assert.strictEqual(
  new vm.Script("reforgeLockTokenState", { filename: "reforge-missing-candidate-reset-test.js" }).runInContext(context),
  "",
  "a lock token missing from the refreshed candidate list was not cleared"
);

sandbox.setRunewordPanelState(JSON.stringify(buildReforgePayload({
  regularAffixCount: 1,
  reforgeOrbsOwned: 2
})));
assert.strictEqual(
  sandbox.selectReforgeLockCandidate(lockedPrefixToken, false),
  false,
  "single-affix base incorrectly allowed a no-op locked reforge"
);
reforgeActionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState)",
  { filename: "reforge-single-affix-guidance-test.js" }
).runInContext(context);
assert.strictEqual(reforgeActionState.reforgeLockCandidates.length, 0);
assert(
  reforgeActionState.reforgeLockHint.includes("At least two regular affixes"),
  "single-affix lock guidance is missing"
);

sandbox.setRunewordPanelState(JSON.stringify(buildReforgePayload({ reforgeOrbsOwned: 2 })));
assert.strictEqual(sandbox.selectReforgeLockCandidate(lockedPrefixToken, false), true);
assert.strictEqual(sandbox.selectReforgeLockCandidate("", false), true);
assert.strictEqual(
  sandbox.buildReforgeCommand(),
  "runeword.reforge",
  "explicit no-lock selection did not restore the standard reforge command"
);

const runeA = "18446744073709551615";
const runeB = "9223372036854775808";
const runeC = "4294967297";
const materialRecipeA = "18446744073709551001";
const materialRecipeB = "18446744073709551002";
sandbox.setUiLanguage("en");
const materialCatalog = [
  {
    token: materialRecipeA,
    name: "Repeated Rune Test",
    runes: "A-A-B",
    summaryEn: "Repeated rune requirement",
    summaryKo: "반복 룬 요구량",
    baseKey: "weapon",
    runeTokens: [runeA, runeA, runeB],
    selected: false
  },
  {
    token: materialRecipeB,
    name: "Ready Rune Test",
    runes: "C",
    summaryEn: "Single rune requirement",
    summaryKo: "단일 룬 요구량",
    baseKey: "armor",
    runeTokens: [runeC],
    selected: true
  }
];

sandbox.setRecipeItems(JSON.stringify(materialCatalog));
sandbox.renderRecipeItems();
const materialNodeBefore = new vm.Script(
  `recipeNodeByToken.get("${materialRecipeA}")`,
  { filename: "recipe-material-node-before.js" }
).runInContext(context);
assert(materialNodeBefore, "material recipe node was not created");
new vm.Script(
  "resolveRecipeSearchDocument(recipeItemsState[0])",
  { filename: "recipe-material-search-cache-prime.js" }
).runInContext(context);
const searchCacheSizeBeforeInventory = new vm.Script(
  "recipeSearchDocumentByToken.size",
  { filename: "recipe-material-cache-size-before.js" }
).runInContext(context);
assert.strictEqual(searchCacheSizeBeforeInventory, 1);

sandbox.setRunewordPanelState(JSON.stringify({
  runeInventoryKnown: true,
  runeInventory: [
    { runeToken: runeA, owned: 1 },
    { runeToken: runeB, owned: 1 },
    { runeToken: runeC, owned: 1 }
  ]
}));
assert.strictEqual(
  new vm.Script("recipeCatalogDomDirty", { filename: "recipe-material-cache-dirty.js" }).runInContext(context),
  false,
  "dynamic rune inventory invalidated the static recipe DOM"
);
assert.strictEqual(
  new vm.Script("recipeSearchDocumentByToken.size", { filename: "recipe-material-cache-retained.js" }).runInContext(context),
  searchCacheSizeBeforeInventory,
  "dynamic rune inventory cleared the static recipe search cache"
);
sandbox.renderRecipeItems();
const materialNodeAfter = new vm.Script(
  `recipeNodeByToken.get("${materialRecipeA}")`,
  { filename: "recipe-material-node-after.js" }
).runInContext(context);
assert.strictEqual(
  materialNodeAfter,
  materialNodeBefore,
  "dynamic rune inventory rebuilt the static recipe card"
);

let repeatedMaterialState = new vm.Script(
  "resolveRecipeMaterialState(recipeItemsState[0])",
  { filename: "recipe-material-repeated-rune.js" }
).runInContext(context);
assert.strictEqual(repeatedMaterialState.known, true);
assert.strictEqual(repeatedMaterialState.requiredFragments, 3);
assert.strictEqual(repeatedMaterialState.coveredFragments, 2);
assert.strictEqual(repeatedMaterialState.missingFragments, 1);
assert.strictEqual(repeatedMaterialState.key, "missing1");
const repeatedMaterialView = new vm.Script(
  `recipeMaterialViewByToken.get("${materialRecipeA}")`,
  { filename: "recipe-material-view.js" }
).runInContext(context);
assert.strictEqual(repeatedMaterialView.badge.textContent, "Missing 1 fragment");
assert.strictEqual(repeatedMaterialView.coverage.textContent, "2/3 fragments covered");
assert(materialNodeAfter.getAttribute("aria-label").includes("Missing 1 fragment"));
assert(materialNodeAfter.getAttribute("aria-label").includes("2 of 3 required fragments covered"));

assert.strictEqual(sandbox.setRecipeMaterialFilter("missing1"), true);
let materialFilteredView = sandbox.resolveRecipeListViewModel();
assert.deepStrictEqual(
  Array.from(materialFilteredView.visibleItems, (item) => item.token),
  [materialRecipeA]
);
assert.strictEqual(
  sandbox.getSelectedRecipeItem().token,
  materialRecipeB,
  "a selected recipe hidden by the material filter was not retained"
);

// A dynamic count-only change must update the existing badge without touching
// the catalog/search identity.
sandbox.setRunewordPanelState(JSON.stringify({
  runeInventoryKnown: true,
  runeInventory: [
    { runeToken: runeA, owned: 2 },
    { runeToken: runeB, owned: 1 },
    { runeToken: runeC, owned: 1 }
  ]
}));
sandbox.renderRecipeItems();
assert.strictEqual(
  new vm.Script(`recipeNodeByToken.get("${materialRecipeA}")`, { filename: "recipe-material-node-count-change.js" }).runInContext(context),
  materialNodeBefore
);
assert.strictEqual(repeatedMaterialView.badge.textContent, "Fragments ready");
assert.strictEqual(repeatedMaterialView.coverage.textContent, "3/3 fragments covered");
assert.strictEqual(
  new vm.Script("recipeSearchDocumentByToken.size", { filename: "recipe-material-cache-count-change.js" }).runInContext(context),
  searchCacheSizeBeforeInventory
);

// A selected-flag-only refresh keeps the static signature and node identity.
sandbox.setRecipeItems(JSON.stringify(materialCatalog.map((item) => ({
  ...item,
  selected: item.token === materialRecipeA
}))));
assert.strictEqual(
  new vm.Script("recipeCatalogDomDirty", { filename: "recipe-material-selected-signature.js" }).runInContext(context),
  false
);
assert.strictEqual(
  new vm.Script(`recipeNodeByToken.get("${materialRecipeA}")`, { filename: "recipe-material-selected-node.js" }).runInContext(context),
  materialNodeBefore
);

// Rune order and duplicates are part of the static contract even when all
// display strings stay identical.
const reorderedMaterialCatalog = materialCatalog.map((item) => item.token === materialRecipeA
  ? { ...item, runeTokens: [runeA, runeB, runeA] }
  : item);
sandbox.setRecipeItems(JSON.stringify(reorderedMaterialCatalog));
assert.strictEqual(
  new vm.Script("recipeCatalogDomDirty", { filename: "recipe-material-rune-signature.js" }).runInContext(context),
  true,
  "runeTokens-only catalog change did not invalidate the static recipe card"
);

// Numeric token payloads and incomplete snapshots fail closed. The material
// filter returns to all, while selection remains independent.
sandbox.setRunewordPanelState(JSON.stringify({
  runeInventoryKnown: true,
  runeInventory: [
    { runeToken: 18446744073709551615, owned: 99 }
  ]
}));
assert.strictEqual(
  new vm.Script("runeInventoryKnownState", { filename: "recipe-material-numeric-token.js" }).runInContext(context),
  false
);
assert.strictEqual(
  new vm.Script("recipeMaterialFilter", { filename: "recipe-material-fail-closed-filter.js" }).runInContext(context),
  "all"
);
assert.strictEqual(sandbox.resolveRecipeListViewModel().activeMaterialFilter, "all");

sandbox.setRecipeItems(JSON.stringify([{ ...materialCatalog[0], runeTokens: [runeA, 17] }]));
assert.strictEqual(
  new vm.Script("recipeItemsState[0].runeTokens", { filename: "recipe-material-static-numeric-token.js" }).runInContext(context),
  null,
  "numeric static rune token was accepted into the recipe contract"
);

const viewMarkup = fs.readFileSync(path.join(VIEW_DIR, "index.html"), "utf8");
assert(
  /id="runewordReforgeLockList"[\s\S]*?role="listbox"/.test(viewMarkup),
  "reforge lock chooser is not exposed as a listbox"
);
assert(
  /id="runewordReforgeCostSummary"[\s\S]*?role="status"[\s\S]*?aria-live="polite"/.test(viewMarkup),
  "reforge cost changes are not exposed through an aria-live status"
);
assert(
  /id="recipeMaterialFilters"[\s\S]*?role="group"[\s\S]*?aria-describedby="recipeMaterialFilterHint"/.test(viewMarkup),
  "material filters are not exposed as a described button group"
);
assert(
  /id="recipeMaterialFilterHint"[\s\S]*?role="status"[\s\S]*?aria-live="polite"/.test(viewMarkup),
  "material-filter availability and scope are not announced accessibly"
);

console.log("Prisma HTML script order: OK");
