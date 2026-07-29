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

console.log("Prisma HTML script order: OK");
