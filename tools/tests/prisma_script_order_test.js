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

// The resource dashboard is independent from base/recipe selection and uses a
// complete named rune snapshot, explicit orb availability, and serialized pity
// counters. Zero-owned rune types remain visible.
const resourceRunes = Array.from({ length: 33 }, (_, index) => ({
  runeToken: String(1000 + index),
  runeName: `Rune ${String(33 - index).padStart(2, "0")}`,
  owned: index % 3 === 0 ? 0 : index
}));
const resourceRuneTotalExpected = resourceRunes.reduce((sum, rune) => sum + rune.owned, 0);
const resourceRuneKindsExpected = resourceRunes.filter((rune) => rune.owned > 0).length;
const resourcePayload = {
  runeInventoryKnown: true,
  runeInventoryExpectedCount: 33,
  runeInventory: resourceRunes,
  reforgeOrbsKnown: true,
  reforgeOrbsOwned: 7,
  pityKnown: true,
  runewordFragmentFailStreak: 99,
  runewordFragmentFailStreakThreshold: 99,
  reforgeOrbFailStreak: 12,
  reforgeOrbFailStreakThreshold: 39
};

sandbox.setRunewordPanelState(JSON.stringify(resourcePayload));
sandbox.renderResourceDashboard();
const storedResourceState = new vm.Script(
  "resourceDashboardState",
  { filename: "resource-dashboard-state-contract-test.js" }
).runInContext(context);
assert.strictEqual(storedResourceState.received, true);
assert.strictEqual(storedResourceState.runeInventoryKnown, true);
assert.strictEqual(storedResourceState.runes.length, 33);
assert.strictEqual(storedResourceState.reforgeOrbsKnown, true);
assert.strictEqual(storedResourceState.reforgeOrbsOwned, 7);
assert.strictEqual(storedResourceState.pityKnown, true);
assert.strictEqual(storedResourceState.runewordFragmentFailStreakThreshold, 99);
assert.strictEqual(element("resourceDashboardSection").getAttribute("aria-busy"), "false");
assert.strictEqual(element("resourceOrbDashboardSection").getAttribute("aria-busy"), "false");
assert.strictEqual(element("resourceRuneTotal").textContent, String(resourceRuneTotalExpected));
assert.strictEqual(
  element("resourceRuneKinds").textContent,
  `${resourceRuneKindsExpected} / 33`
);
assert.strictEqual(element("resourceReforgeOrbs").textContent, "7");
assert.strictEqual(element("resourceIdentifyScrolls").textContent, "—", "missing currency counts stay unknown");
sandbox.setRunewordPanelState(JSON.stringify({
  ...resourcePayload,
  identifyScrollsKnown: true, identifyScrollsOwned: 4,
  scouringOrbsKnown: true, scouringOrbsOwned: 1
}));
sandbox.renderResourceDashboard();
assert.strictEqual(element("resourceIdentifyScrolls").textContent, "4");
assert.strictEqual(element("resourceScouringOrbs").textContent, "1");
sandbox.setRunewordPanelState(JSON.stringify(resourcePayload));
sandbox.renderResourceDashboard();
assert.strictEqual(element("resourceRuneList").children.length, 33);
assert.strictEqual(
  element("resourceRuneList").children[0].children[0].textContent,
  "Rune 01",
  "rune inventory is not sorted by display name"
);
assert(
  element("resourceRuneList").children.some((item) => item.className.includes("empty")),
  "zero-owned rune types were omitted"
);
assert.strictEqual(element("resourceFragmentPityValue").textContent, "99 / 99");
assert.strictEqual(element("resourceFragmentPityProgress").getAttribute("max"), "99");
assert.strictEqual(element("resourceFragmentPityProgress").getAttribute("value"), "99");
assert(
  element("resourceFragmentPityProgress").getAttribute("aria-valuetext").includes(
    "the next eligible ordinary drop roll is guaranteed"
  )
);
assert(
  element("resourceFragmentPityState").textContent.includes(
    "Next eligible ordinary drop roll: guaranteed"
  )
);
assert.strictEqual(element("resourceOrbPityValue").textContent, "12 / 39");
assert.strictEqual(element("resourceOrbPityProgress").getAttribute("max"), "39");
assert.strictEqual(element("resourceOrbPityProgress").getAttribute("value"), "12");
assert.strictEqual(
  sandbox.applyResourceDashboardSnapshot(resourcePayload),
  false,
  "an unchanged resource snapshot requested another dashboard render"
);

// The shared working-base picker is authoritative for mutating actions. A
// separately inspected inventory item must not replace that target label.
const workingBaseKey = "9001";
sandbox.setInventoryItems(JSON.stringify([
  { key: workingBaseKey, name: "Steel Sword · Working Base", selected: true },
  { key: "9002", name: "Leather Armor", selected: false }
]));
sandbox.renderInventoryItems();
assert.strictEqual(element("workingBaseName").textContent, "Steel Sword · Working Base");
assert(
  element("workingBaseMeta").textContent.includes("authoritative target"),
  "working-base mutation scope is not explicit"
);
sandbox.setSelectedItemName("Hovered Iron Helmet");
sandbox.renderSelectedItemContext();
assert.strictEqual(
  element("workingBaseName").textContent,
  "Steel Sword · Working Base",
  "hovered inventory item replaced the authoritative working-base label"
);
assert.strictEqual(element("affixSelectedItemName").textContent, "Hovered Iron Helmet");

// The equipped-base collector is intentionally unbounded. The picker must
// render every supplied candidate instead of relying on a scroll-window slice.
const manyWorkingBases = Array.from({ length: 34 }, (_, index) => ({
  key: String(9100 + index),
  name: `Equipped Base ${String(index + 1).padStart(2, "0")}`,
  selected: index === 33
}));
sandbox.setInventoryItems(JSON.stringify(manyWorkingBases));
sandbox.renderInventoryItems();
assert.strictEqual(
  element("inventoryBaseList").children.length,
  34,
  "the working-base picker omitted equipped candidates"
);
assert.strictEqual(
  element("inventoryBaseList").children[33].dataset.baseKey,
  manyWorkingBases[33].key,
  "the final equipped candidate was not rendered"
);
assert.strictEqual(
  element("inventoryBaseList").children[33].getAttribute("aria-selected"),
  "true",
  "selection state was lost at the end of the expanded candidate list"
);

// Invalid or incomplete dynamic data fails closed after receipt: it is
// unavailable rather than permanently presented as startup synchronization or
// fabricated zeroes. A streak past its threshold invalidates the pity pair.
sandbox.setRunewordPanelState(JSON.stringify({
  ...resourcePayload,
  runeInventory: resourceRunes.map((rune, index) => index === 0
    ? { runeToken: rune.runeToken, owned: rune.owned }
    : rune),
  runewordFragmentFailStreak: 100
}));
sandbox.renderResourceDashboard();
assert.strictEqual(
  new vm.Script("resourceDashboardState.runeInventoryKnown", {
    filename: "resource-dashboard-missing-name-test.js"
  }).runInContext(context),
  false
);
assert.strictEqual(
  new vm.Script("resourceDashboardState.pityKnown", {
    filename: "resource-dashboard-invalid-pity-test.js"
  }).runInContext(context),
  false
);
assert.strictEqual(element("resourceDashboardSection").getAttribute("aria-busy"), "false");
assert(element("resourceDashboardSync").textContent.includes("Partially unavailable"));
assert.strictEqual(element("resourceRuneTotal").textContent, "—");
assert.strictEqual(element("resourceRuneKinds").textContent, "— / —");
assert.strictEqual(element("resourceFragmentPityValue").textContent, "— / —");
assert.strictEqual(element("resourceFragmentPityProgress").getAttribute("aria-busy"), "false");
assert(
  element("resourceFragmentPityState").textContent.includes("unavailable")
);
assert(
  element("resourceRuneDetailsSummary").textContent.includes("Unavailable")
);

// Restore the fixture used by the following build/runeword tests.
sandbox.setRunewordPanelState(JSON.stringify(compatibilityPayload));

const buildSummaryPayload = {
  ready: true,
  runtimeEnabled: true,
  equippedAffixSlots: 9,
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
      procRollStackCount: 2,
      castOnCritSelectionLimited: false,
      hasNormalWeaponHitProcRoll: true,
      normalWeaponHitProcChancePct: 9.8,
      normalWeaponHitProcStackCount: 2,
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
      token: "18446744073709551605",
      displayNameEn: "Crit Cast: Firebolt",
      displayNameKo: "치명 시전: 파이어볼트",
      group: "offense",
      triggerKey: "hit",
      slotKind: "prefix",
      suffixState: "none",
      equippedCount: 1,
      hasProcRoll: true,
      procRollChancePct: 100,
      procRollStackCount: 1,
      castOnCritSelectionLimited: true,
      hasNormalWeaponHitProcRoll: true,
      normalWeaponHitProcChancePct: 45,
      normalWeaponHitProcStackCount: 1,
      hasLuckyHitGate: false,
      luckyHitGateChancePct: 0
    },
    {
      token: "18446744073709551610",
      displayNameEn: "Vitality II",
      displayNameKo: "활력 II",
      group: "passive",
      triggerKey: "passive",
      slotKind: "suffix",
      suffixState: "highest",
      equippedCount: 1,
      suffixTierRank: 2,
      suffixFamilyRankPoints: 3,
      effectiveSuffixTierRank: 3,
      effectiveSuffixDisplayNameEn: "Vitality III",
      effectiveSuffixDisplayNameKo: "활력 III",
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
assert.strictEqual(storedEquippedBuildState.entries.length, 8);
assert.strictEqual(storedEquippedBuildState.entries[0].token, "18446744073709551613");
assert.strictEqual(storedEquippedBuildState.entries[0].hasPassiveContribution, true);
assert.strictEqual(storedEquippedBuildState.entries[0].passiveContributionActive, true);
assert.strictEqual(storedEquippedBuildState.entries[0].passiveSpellDisabled, false);
assert.strictEqual(storedEquippedBuildState.entries[1].hasPassiveContribution, false);
assert.strictEqual(storedEquippedBuildState.entries[1].passiveContributionActive, false);
assert.strictEqual(storedEquippedBuildState.entries[1].passiveSpellDisabled, false);
assert.strictEqual(storedEquippedBuildState.entries[2].procRollChancePct, 100);
assert.strictEqual(storedEquippedBuildState.entries[0].procRollStackCount, 2);
assert.strictEqual(storedEquippedBuildState.entries[0].castOnCritSelectionLimited, false);
assert.strictEqual(storedEquippedBuildState.entries[0].normalWeaponHitProcChancePct, 9.8);
assert.strictEqual(storedEquippedBuildState.entries[0].normalWeaponHitProcStackCount, 2);
assert.strictEqual(storedEquippedBuildState.entries[3].castOnCritSelectionLimited, true);
assert.strictEqual(storedEquippedBuildState.entries[4].effectiveSuffixTierRank, 3);
assert.strictEqual(sandbox.resolveEquippedBuildViewState(storedEquippedBuildState), "ready");

sandbox.renderEquippedBuildSummary();
assert.strictEqual(element("equippedBuildGroups").hidden, false);
assert.strictEqual(element("equippedBuildChanceHint").hidden, false);
assert.strictEqual(element("equippedBuildGroups").getAttribute("aria-busy"), "false");
assert.strictEqual(element("equippedBuildOffenseCount").textContent, "3");
assert.strictEqual(element("equippedBuildDefenseCount").textContent, "1");
assert.strictEqual(element("equippedBuildKillCount").textContent, "1");
assert.strictEqual(element("equippedBuildPassiveCount").textContent, "6");
assert(
  element("equippedBuildPassiveCount").getAttribute("aria-label").includes(
    "6 effect copies shown in this group"
  )
);
assert.strictEqual(element("equippedBuildOffenseList").children.length, 2);
assert.strictEqual(element("equippedBuildPassiveList").children.length, 5);
assert.strictEqual(element("equippedBuildSlotCount").textContent, "9 slots / 슬롯 9개");

function collectFakeElementText(node) {
  return [node.textContent, ...node.children.map(collectFakeElementText)].join(" ");
}

const offenseText = collectFakeElementText(element("equippedBuildOffenseList"));
assert(offenseText.includes("On hit"));
assert(offenseText.includes("Effective conditional proc chance 24.5%"));
assert(offenseText.includes("2-copy diminishing chance · one action"));
assert(offenseText.includes("Normal weapon hit 9.8% effective"));
assert(offenseText.includes("Normal-hit 2-copy diminishing chance · one action"));
assert(offenseText.includes("Per-candidate roll 100% · max 2 melee crit/power, 1 ranged"));
assert(offenseText.includes("Normal melee: selected candidate rolls 45% · max 1 action"));
assert(offenseText.includes("Lucky Hit gate 30%"));
assert(offenseText.includes("Passive also active"));
const passiveText = collectFakeElementText(element("equippedBuildPassiveList"));
assert(passiveText.includes("Storm Brand"));
assert(passiveText.includes("Promoted to Vitality III · 3 rank points"));
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

// Affix expansion is driven by one authoritative panel payload and emits an
// exact decimal uint64 base key plus the expected 1/2 regular-affix count.
const expansionBaseKey = "18446744073709551615";
sandbox.setInventoryItems(JSON.stringify([
  { key: expansionBaseKey, name: "Expansion Base", selected: true }
]));
sandbox.setRunewordPanelState(JSON.stringify({
  hasBase: true,
  hasRecipe: true,
  canInsert: true,
  isComplete: false,
  regularAffixCount: 1,
  maxRegularAffixCount: 3,
  expandAffixCost: 2,
  canExpandAffix: true,
  expandAffixUnavailableReason: "",
  reforgeOrbsKnown: true,
  reforgeOrbsOwned: 2,
  standardReforgeCost: 1,
  lockedReforgeCost: 2
}));
sandbox.renderRunewordPanelState();
let expansionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState).affixSlotState",
  { filename: "affix-expansion-ready-contract-test.js" }
).runInContext(context);
assert.strictEqual(expansionState.expandEnabled, true);
assert.strictEqual(
  expansionState.expandCommand,
  `affix.expand:${expansionBaseKey}:1`
);
assert.strictEqual(element("affixSlotProgressCount").textContent, "1/3");
assert.strictEqual(element("affixSlotProgressTrack").getAttribute("aria-valuenow"), "1");
assert.strictEqual(element("affixExpandButton").disabled, false);
assert.strictEqual(
  element("affixExpandButton").getAttribute("data-cmd"),
  `affix.expand:${expansionBaseKey}:1`
);
let expansionActionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState)",
  { filename: "affix-expansion-pending-contract-test.js" }
).runInContext(context);
assert.strictEqual(expansionActionState.canTransmute, true);
assert.strictEqual(
  sandbox.beginAffixExpandPending(`affix.expand:${expansionBaseKey}:1`),
  true
);
expansionActionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState)",
  { filename: "affix-expansion-pending-block-contract-test.js" }
).runInContext(context);
assert.strictEqual(expansionActionState.canTransmute, false);
assert.strictEqual(expansionActionState.reforgeEnabled, false);
assert.strictEqual(expansionActionState.resetEnabled, false);
assert.strictEqual(element("runewordInsertButton").disabled, true);
assert.strictEqual(element("runewordReforgeButton").disabled, true);
assert.strictEqual(element("runewordResetButton").disabled, true);
sandbox.clearAffixExpandPending(false);

sandbox.setRunewordPanelState(JSON.stringify({
  hasBase: true,
  regularAffixCount: 2,
  maxRegularAffixCount: 3,
  expandAffixCost: 4,
  canExpandAffix: false,
  expandAffixUnavailableReason: "insufficient_orbs",
  reforgeOrbsKnown: true,
  reforgeOrbsOwned: 3,
  standardReforgeCost: 1,
  lockedReforgeCost: 2
}));
sandbox.renderRunewordPanelState();
expansionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState).affixSlotState",
  { filename: "affix-expansion-short-contract-test.js" }
).runInContext(context);
assert.strictEqual(expansionState.expandEnabled, false);
assert.strictEqual(expansionState.unavailableReason, "insufficient_orbs");
assert(element("affixSlotProgressMeta").textContent.includes("1 more"));
assert.strictEqual(element("affixExpandButton").disabled, true);
assert.strictEqual(element("affixExpandButton").getAttribute("data-cmd"), null);

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
    "replace only the selected regular affix"
  ),
  "completed-runeword Reforge hint must describe the regular-affix-only runtime contract"
);
assert(
  completedRunewordActionState.reforgeHint.includes(
    "룬워드 성장 상태는 유지됩니다"
  ),
  "completed-runeword Reforge hint must say the runeword is preserved"
);
assert(
  !completedRunewordActionState.reforgeHint.includes("runeword effect + affixes"),
  "completed-runeword Reforge hint still promises to reroll the runeword"
);

const reforgeBaseA = "4294967297";
const reforgeBaseB = "4294967298";
const lockedReforgeCommandPrefix = "affix.reforge:";
const lockedPrefixToken = "18446744073709551614";
const lockedSuffixToken = "9223372036854775807";
const buildReforgePayload = (overrides = {}) => ({
  hasBase: true,
  hasRecipe: false,
  regularAffixCount: 3,
  reforgeOrbsKnown: true,
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
assert.strictEqual(reforgeActionState.reforgeCommand, "");
assert.strictEqual(reforgeActionState.reforgeCost, 2);
assert.strictEqual(reforgeActionState.reforgeEnabled, false);
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
  reforgeActionState.reforgeCostSummary.includes("Owned: 1"),
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
  true,
  "single-affix base must allow selected-slot replacement"
);
reforgeActionState = new vm.Script(
  "resolveRunewordPanelActionState(runewordPanelState)",
  { filename: "reforge-single-affix-guidance-test.js" }
).runInContext(context);
assert.strictEqual(reforgeActionState.reforgeLockCandidates.length, 2);
assert(
  reforgeActionState.reforgeLockHint.includes("Choose the one affix to reroll"),
  "single-affix lock guidance is missing"
);

sandbox.setRunewordPanelState(JSON.stringify(buildReforgePayload({ reforgeOrbsOwned: 2 })));
assert.strictEqual(sandbox.selectReforgeLockCandidate(lockedPrefixToken, false), true);
assert.strictEqual(sandbox.selectReforgeLockCandidate("", false), true);
assert.strictEqual(
  sandbox.buildReforgeCommand(),
  "",
  "clearing the selection must disable reforge"
);

// Currency crafting through the actual classic-script UI and command dispatcher.
const craftedCommands = [];
sandbox.calamityCommand = command => craftedCommands.push(command);
const emptyCraftPayload = {
  hasBase: true, regularAffixCount: 0, maxRegularAffixCount: 3,
  identifyScrollsKnown: true, identifyScrollsOwned: 2,
  scouringOrbsKnown: true, scouringOrbsOwned: 1,
  reforgeOrbsKnown: true, reforgeOrbsOwned: 4,
  reforgeLockCandidates: [], debugTools: false
};
sandbox.setInventoryItems(JSON.stringify([{ key: reforgeBaseA, name: "Base A", selected: true }]));
sandbox.setRunewordPanelState(JSON.stringify(emptyCraftPayload));
sandbox.renderRunewordPanelState();
assert.strictEqual(element("affixIdentifyButton").disabled, false);
assert.strictEqual(element("affixScourButton").disabled, true);
assert.strictEqual(element("runewordReforgeButton").disabled, true);
assert.strictEqual(element("runewordResetButton").disabled, true, "free reset must require debug mode");
assert.strictEqual(element("runewordRecoveryDetails").hidden, true, "reset section is hidden outside debug mode");
sandbox.dispatchPanelCommand(element("affixIdentifyButton"));
sandbox.dispatchPanelCommand(element("affixIdentifyButton"));
assert.deepStrictEqual(craftedCommands, [`affix.identify:${reforgeBaseA}`], "double click must charge at most once");
const identifiedPayload = {
  ...emptyCraftPayload, regularAffixCount: 1,
  identifyScrollsOwned: 1, reforgeLockCandidates: [buildReforgePayload().reforgeLockCandidates[0]]
};
sandbox.setRunewordPanelState(JSON.stringify(identifiedPayload));
sandbox.renderRunewordPanelState();
assert.strictEqual(element("affixIdentifyButton").disabled, true, "identified equipment cannot be identified again");
assert.strictEqual(element("affixScourButton").disabled, false);
assert.strictEqual(sandbox.selectReforgeLockCandidate(lockedPrefixToken, false), true);
sandbox.renderRunewordPanelState();
assert.strictEqual(element("runewordReforgeButton").disabled, false, "one-affix gear can be reforged");
sandbox.dispatchPanelCommand(element("runewordReforgeButton"));
assert.strictEqual(craftedCommands.at(-1), `affix.reforge:${reforgeBaseA}:${lockedPrefixToken}`);
sandbox.setRunewordPanelState(JSON.stringify(identifiedPayload));
sandbox.renderRunewordPanelState();
const beforeScour = craftedCommands.length;
sandbox.dispatchPanelCommand(element("affixScourButton"));
assert.strictEqual(craftedCommands.length, beforeScour, "scouring needs confirmation");
sandbox.dispatchPanelCommand(element("affixScourButton"));
sandbox.dispatchPanelCommand(element("affixScourButton"));
assert.strictEqual(craftedCommands.length, beforeScour + 1, "confirmed scouring must not repeat while pending");
assert.strictEqual(craftedCommands.at(-1), `affix.scour:${reforgeBaseA}`);
// Scour rerolls in place: the slot count stays, so identify stays unavailable.
sandbox.setRunewordPanelState(JSON.stringify({ ...identifiedPayload, scouringOrbsOwned: 0 }));
sandbox.renderRunewordPanelState();
assert.strictEqual(element("affixIdentifyButton").disabled, true, "scoured gear keeps its slots and is not re-identified");
assert.strictEqual(element("affixScourButton").disabled, true, "scouring needs a Scouring Orb");
// A finished 3-affix base must keep every crafting path open.
const fullPayload = {
  ...identifiedPayload, regularAffixCount: 3,
  reforgeLockCandidates: buildReforgePayload().reforgeLockCandidates
};
sandbox.setRunewordPanelState(JSON.stringify(fullPayload));
assert.strictEqual(sandbox.selectReforgeLockCandidate(lockedPrefixToken, false), true);
sandbox.renderRunewordPanelState();
assert.strictEqual(element("affixIdentifyButton").disabled, true);
assert.strictEqual(element("affixScourButton").disabled, false, "3-affix gear can be scoured");
assert.strictEqual(element("runewordReforgeButton").disabled, false, "3-affix gear can be reforged");
// Legacy tokens count as regular slots but are never reforge candidates.
sandbox.setRunewordPanelState(JSON.stringify({ ...identifiedPayload, reforgeLockCandidates: [] }));
sandbox.renderRunewordPanelState();
assert.strictEqual(element("affixIdentifyButton").disabled, true);
assert.strictEqual(element("affixScourButton").disabled, false, "legacy layouts are repaired by scour");
assert.strictEqual(element("runewordReforgeButton").disabled, true);
sandbox.setRunewordPanelState(JSON.stringify(emptyCraftPayload));
sandbox.renderRunewordPanelState();
sandbox.setRunewordPanelState(JSON.stringify({ ...emptyCraftPayload, identifyScrollsKnown: false }));
sandbox.renderRunewordPanelState();
assert.strictEqual(element("affixIdentifyButton").disabled, true, "unknown currency must fail closed");
sandbox.setRunewordPanelState(JSON.stringify({ ...emptyCraftPayload, identifyScrollsOwned: 0 }));
sandbox.renderRunewordPanelState();
assert.strictEqual(element("affixIdentifyButton").disabled, true);
sandbox.setRunewordPanelState(JSON.stringify(identifiedPayload));
sandbox.renderRunewordPanelState();
sandbox.dispatchPanelCommand(element("affixScourButton"));
const beforeChangedBase = craftedCommands.length;
sandbox.setInventoryItems(JSON.stringify([{ key: reforgeBaseB, name: "Base B", selected: true }]));
sandbox.renderRunewordPanelState();
sandbox.dispatchPanelCommand(element("affixScourButton"));
assert.strictEqual(craftedCommands.length, beforeChangedBase, "confirmation cannot transfer to another base");
new vm.Script("scourConfirmation = null").runInContext(context);
delete sandbox.calamityCommand;

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
  runeInventoryExpectedCount: 3,
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
  runeInventoryExpectedCount: 3,
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
  runeInventoryExpectedCount: 1,
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
assert(
  /id="resourceDashboardSection"[\s\S]*?aria-busy="true"/.test(viewMarkup),
  "resource dashboard startup synchronization is not exposed accessibly"
);
assert.strictEqual(
  (viewMarkup.match(/class="rdProgress"/g) || []).length,
  2,
  "resource dashboard does not expose both native progress elements"
);

console.log("Prisma HTML script order: OK");
