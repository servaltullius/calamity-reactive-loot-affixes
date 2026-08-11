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
  add() {}
  remove() {}
  toggle(_name, enabled) { return Boolean(enabled); }
  contains() { return false; }
}

class FakeElement {
  constructor() {
    this.classList = new FakeClassList();
    this.dataset = {};
    this.style = { setProperty() {} };
    this.scrollTop = 0;
    this.scrollHeight = 0;
    this.clientHeight = 0;
  }

  addEventListener() {}
  appendChild(child) { return child; }
  contains() { return false; }
  focus() {}
  getAttribute() { return null; }
  getBoundingClientRect() {
    return { left: 0, top: 0, right: 0, bottom: 0, width: 0, height: 0 };
  }
  removeAttribute() {}
  setAttribute() {}
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
  createDocumentFragment: () => new FakeElement(),
  createElement: () => new FakeElement(),
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

const viewMarkup = fs.readFileSync(path.join(VIEW_DIR, "index.html"), "utf8");
assert(
  /id="runewordReforgeLockList"[\s\S]*?role="listbox"/.test(viewMarkup),
  "reforge lock chooser is not exposed as a listbox"
);
assert(
  /id="runewordReforgeCostSummary"[\s\S]*?role="status"[\s\S]*?aria-live="polite"/.test(viewMarkup),
  "reforge cost changes are not exposed through an aria-live status"
);

console.log("Prisma HTML script order: OK");
