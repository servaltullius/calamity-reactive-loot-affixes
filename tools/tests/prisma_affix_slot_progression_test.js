"use strict";

const assert = require("assert");
const { loadScripts } = require("../prisma_view_source.js");

const source = loadScripts();
assert(source.length > 0, "Prisma scripts missing");

function between(startMarker, endMarker) {
  const start = source.indexOf(startMarker);
  const end = source.indexOf(endMarker, start);
  assert(start >= 0 && end > start, `missing source block: ${startMarker}`);
  return source.slice(start, end);
}

const uint64Source = between(
  "\nconst maxUint64DecimalString",
  "\nfunction normalizeRecipeRuneTokens("
);
const reasonNormalizerSource = between(
  "\nfunction normalizeAffixExpandUnavailableReason(",
  "\nfunction normalizeReforgeLockCandidates("
);
const slotProgressSource = between(
  "\nfunction buildAffixExpandCommand(",
  "\nfunction clearAffixExpandPending("
);
const pendingSource = between(
  "\nfunction clearAffixExpandPending(",
  "\nfunction resolveRunewordPanelActionState("
);

const runBehavior = new Function(
  "assert",
  `
    "use strict";
    let uiLang = "en";
    function t(en, ko) {
      if (uiLang === "en") return en;
      if (uiLang === "ko") return ko;
      return en + " / " + ko;
    }
    const affixExpandCommandPrefix = "affix.expand:";
    const validAffixExpandUnavailableReasons = new Set([
      "no_base",
      "requires_first_affix",
      "max_slots",
      "invalid_layout",
      "suffix_slots_disabled",
      "insufficient_orbs",
      "unavailable"
    ]);
    let affixExpandPendingState = null;
    let affixCraftPendingState = null;
    function resolveSelectedRunewordBaseKey() { return ""; }
    ${uint64Source}
    ${reasonNormalizerSource}
    ${slotProgressSource}

    assert.strictEqual(
      buildAffixExpandCommand("18446744073709551615", 2),
      "affix.expand:18446744073709551615:2"
    );
    assert.strictEqual(buildAffixExpandCommand("18446744073709551616", 2), "");
    assert.strictEqual(buildAffixExpandCommand("01", 1), "");
    assert.strictEqual(buildAffixExpandCommand("42", 0), "");
    assert.strictEqual(buildAffixExpandCommand("42", 3), "");
    assert.strictEqual(buildAffixExpandCommand(42, 1), "");

    const readyOne = {
      hasBase: true,
      regularAffixCount: 1,
      regularAffixCountKnown: true,
      maxRegularAffixCount: 3,
      maxRegularAffixCountKnown: true,
      expandAffixCost: 2,
      canExpandAffix: true,
      expandAffixUnavailableReason: "unavailable",
      reforgeOrbsKnown: true,
      reforgeOrbsOwned: 2
    };
    let view = resolveAffixSlotProgressState(readyOne, "42", null);
    assert.strictEqual(view.countText, "1/3");
    assert.strictEqual(view.nextSlotNumber, 2);
    assert.strictEqual(view.expandEnabled, true);
    assert.strictEqual(view.expandCommand, "affix.expand:42:1");
    assert(view.expandButtonLabel.includes("2 Orbs"));
    assert(view.expandHint.includes("preserved"));

    const shortTwo = {
      ...readyOne,
      regularAffixCount: 2,
      expandAffixCost: 4,
      canExpandAffix: false,
      expandAffixUnavailableReason: "insufficient_orbs",
      reforgeOrbsOwned: 3
    };
    view = resolveAffixSlotProgressState(shortTwo, "42", null);
    assert.strictEqual(view.countText, "2/3");
    assert.strictEqual(view.nextSlotNumber, 3);
    assert.strictEqual(view.expandEnabled, false);
    assert.strictEqual(view.unavailableReason, "insufficient_orbs");
    assert(view.progressMeta.includes("1 more"));
    assert(view.expandButtonLabel.includes("4 Orbs"));

    view = resolveAffixSlotProgressState(
      {
        ...readyOne,
        canExpandAffix: false,
        expandAffixUnavailableReason: "suffix_slots_disabled",
        reforgeOrbsOwned: 0
      },
      "42",
      null
    );
    assert.strictEqual(view.unavailableReason, "suffix_slots_disabled");

    view = resolveAffixSlotProgressState(
      {
        ...readyOne,
        canExpandAffix: false,
        expandAffixUnavailableReason: "unavailable",
        reforgeOrbsKnown: false,
        reforgeOrbsOwned: null
      },
      "42",
      null
    );
    assert.strictEqual(view.unavailableReason, "unavailable");

    const empty = {
      ...readyOne,
      regularAffixCount: 0,
      expandAffixCost: null,
      canExpandAffix: false,
      expandAffixUnavailableReason: "requires_first_affix"
    };
    view = resolveAffixSlotProgressState(empty, "42", null);
    assert.strictEqual(view.countText, "0/3");
    assert.strictEqual(view.expandCommand, "");
    assert.strictEqual(view.unavailableReason, "requires_first_affix");
    assert(view.expandButtonLabel.includes("Identify First"));

    const full = {
      ...readyOne,
      regularAffixCount: 3,
      expandAffixCost: null,
      canExpandAffix: false,
      expandAffixUnavailableReason: "max_slots"
    };
    view = resolveAffixSlotProgressState(full, "42", null);
    assert.strictEqual(view.countText, "3/3");
    assert.strictEqual(view.unavailableReason, "max_slots");
    assert(view.expandButtonLabel.includes("Max Slots"));

    view = resolveAffixSlotProgressState(readyOne, "", null);
    assert.strictEqual(view.countText, "—/3");
    assert.strictEqual(view.expandEnabled, false);
    assert.strictEqual(view.unavailableReason, "no_base");

    view = resolveAffixSlotProgressState(
      { ...readyOne, regularAffixCount: "1", regularAffixCountKnown: false },
      "42",
      null
    );
    assert.strictEqual(view.expandEnabled, false);
    assert.strictEqual(view.unavailableReason, "unavailable");

    view = resolveAffixSlotProgressState(
      { ...readyOne, expandAffixUnavailableReason: "secret_backend_detail", canExpandAffix: false },
      "42",
      null
    );
    assert.strictEqual(view.unavailableReason, "unavailable");
    assert(!view.progressMeta.includes("secret_backend_detail"));

    view = resolveAffixSlotProgressState(readyOne, "42", { nonce: 1 });
    assert.strictEqual(view.expandPending, true);
    assert.strictEqual(view.expandEnabled, false);
    assert(view.expandButtonLabel.includes("Unlocking"));

    uiLang = "ko";
    view = resolveAffixSlotProgressState(shortTwo, "42", null);
    assert(view.progressMeta.includes("1개 부족"));
    assert(view.expandButtonLabel.includes("오브 4개"));
  `
);
runBehavior(assert);

const runPendingBehavior = new Function(
  "assert",
  `
    "use strict";
    function t(en, ko) { return en; }
    const affixExpandCommandPrefix = "affix.expand:";
    const validAffixExpandUnavailableReasons = new Set([
      "no_base",
      "requires_first_affix",
      "max_slots",
      "invalid_layout",
      "suffix_slots_disabled",
      "insufficient_orbs",
      "unavailable"
    ]);
    const panelRenderSection = { runewordPanelState: "runewordPanelState" };
    const affixExpandPendingTimeoutMs = 2500;
    let affixExpandPendingState = null;
    let affixCraftPendingState = null;
    let affixExpandPendingNonce = 0;
    let scheduled = 0;
    let chooserClosed = 0;
    let reforgeLockCleared = 0;
    let timeoutCallback = null;
    let feedback = "";
    const affixExpandButton = { disabled: false };
    const runewordReforgeButton = { disabled: false };
    const runewordInsertButton = { disabled: false };
    const runewordResetButton = { disabled: false };
    const window = { setTimeout: (callback, delay) => {
      assert.strictEqual(delay, affixExpandPendingTimeoutMs);
      timeoutCallback = callback;
    } };
    function schedulePanelRender(section) {
      assert.strictEqual(section, panelRenderSection.runewordPanelState);
      scheduled += 1;
    }
    function closeWorkingBaseChooser() { chooserClosed += 1; }
    function clearReforgeLockSelection(scheduleRender) {
      assert.strictEqual(scheduleRender, false);
      reforgeLockCleared += 1;
    }
    function setActionFeedback(message) { feedback = message; }
    function resolveSelectedRunewordBaseKey() { return "42"; }
    const runewordPanelState = {
      hasBase: true,
      regularAffixCount: 1,
      regularAffixCountKnown: true,
      maxRegularAffixCount: 3,
      maxRegularAffixCountKnown: true,
      expandAffixCost: 2,
      canExpandAffix: true,
      expandAffixUnavailableReason: "unavailable",
      reforgeOrbsKnown: true,
      reforgeOrbsOwned: 2
    };
    ${uint64Source}
    ${reasonNormalizerSource}
    ${slotProgressSource}
    ${pendingSource}

    assert.strictEqual(beginAffixExpandPending("affix.expand:42:1"), true);
    assert.strictEqual(affixExpandPendingState.expectedRegularAffixCount, 1);
    assert.strictEqual(affixExpandButton.disabled, true);
    assert.strictEqual(runewordReforgeButton.disabled, true);
    assert.strictEqual(runewordInsertButton.disabled, true);
    assert.strictEqual(runewordResetButton.disabled, true);
    assert.strictEqual(reforgeLockCleared, 1);
    assert.strictEqual(chooserClosed, 1);
    assert.strictEqual(scheduled, 1);
    assert.strictEqual(beginAffixExpandPending("affix.expand:42:1"), false);
    assert(timeoutCallback);
    timeoutCallback();
    assert.strictEqual(affixExpandPendingState, null);
    assert(feedback.includes("timed out"));
    assert.strictEqual(scheduled, 2);

    assert.strictEqual(beginAffixExpandPending("affix.expand:42:2"), false);
    assert.strictEqual(affixExpandPendingState, null);
  `
);
runPendingBehavior(assert);

console.log("Prisma affix slot progression: OK");
