const tooltipDefaultsMobile = Object.freeze({ right: 16, top: 180, fontScale: 1 });

let controlPanelOpen = false;
let tooltipTextState = "";
let runewordAffixTextState = "";
let runewordAffixPendingState = false;
let runewordAffixPendingNonce = 0;
const runewordAffixPendingTimeoutMs = 1200;
let reforgeLockTokenState = "";
let reforgeLockBaseKeyState = "";
let panelHotkeyTextState = "F11";
let selectedItemNameState = "";
let selectedItemSourceState = "";
let inventoryItemsState = [];
let recipeItemsState = [];
let recipeSearchQuery = "";
let recipeBaseFilter = "all";
let recipeMaterialFilter = "all";
let runeInventoryKnownState = false;
let runeInventorySignatureState = "unknown";
let runeInventoryOwnedByToken = new Map();
let resourceDashboardSignatureState = "";
let resourceDashboardState = {
  received: false,
  runeInventoryKnown: false,
  runeInventoryExpectedCount: 0,
  runes: [],
  reforgeOrbsKnown: false,
  reforgeOrbsOwned: 0,
  pityKnown: false,
  runewordFragmentFailStreak: 0,
  runewordFragmentFailStreakThreshold: 0,
  reforgeOrbFailStreak: 0,
  reforgeOrbFailStreakThreshold: 0
};
let recipeCatalogSignatureState = "";
let recipeCatalogDomDirty = true;
let confirmedRecipeTokenState = "";
let optimisticRecipeTokenState = "";
let optimisticRecipeSelectionNonce = 0;
const optimisticRecipeSelectionTimeoutMs = 1200;
const recipeNodeByToken = new Map();
const recipeSearchDocumentByToken = new Map();
const recipeMaterialViewByToken = new Map();
let mainTabState = "runeword";
let pendingOpenMainTab = null;
let equippedBuildState = {
  received: false,
  ready: false,
  runtimeEnabled: false,
  equippedAffixSlots: 0,
  entries: []
};
let runewordPanelState = {
  hasBase: false,
  hasRecipe: false,
  isComplete: false,
  recipeName: "",
  insertedRunes: 0,
  totalRunes: 0,
  nextRuneName: "",
  nextRuneOwned: 0,
  canInsert: false,
  missingSummary: "",
  requiredRunes: [],
  regularAffixCount: 0,
  reforgeOrbsOwned: null,
  standardReforgeCost: 1,
  lockedReforgeCost: 2,
  reforgeLockCandidates: []
};
let runewordResetArmedUntil = 0;
let runewordResetArmedBaseKey = "";
const runewordResetConfirmWindowMs = 6000;
let panelDragState = null;
let panelResizeState = null;
let tooltipDragState = null;
let panelLayoutLoaded = false;
let tooltipLayoutLoaded = false;
let tooltipLayout = getDefaultTooltipLayout();
const panelCommandAttribute = "data-cmd";
const panelOpenTabAttribute = "data-open-tab";
const recipeFilterAttribute = "data-recipe-filter";
const recipeMaterialFilterAttribute = "data-recipe-material-filter";
const recipeSelectionCommandPrefix = "runeword.recipe.select:";
const lockedReforgeCommandPrefix = "runeword.reforge:";
const reforgeLockTokenAttribute = "data-reforge-lock-token";
const validRecipeBaseFilters = new Set(["all", "weapon", "armor", "mixed"]);
const validRecipeMaterialFilters = new Set(["all", "ready", "missing1"]);
const panelRenderSection = Object.freeze({
  hotkeyHints: "hotkeyHints",
  selectedItemContext: "selectedItemContext",
  inventoryItems: "inventoryItems",
  recipeItems: "recipeItems",
  runewordPanelState: "runewordPanelState",
  resourceDashboard: "resourceDashboard",
  equippedBuild: "equippedBuild",
  tooltipLayout: "tooltipLayout",
  tooltipPlacement: "tooltipPlacement",
  quickLaunch: "quickLaunch"
});
const prismaInteropMethod = Object.freeze({
  tooltip: "setTooltip",
  controlPanel: "setControlPanel",
  actionFeedback: "setActionFeedback",
  panelHotkeyText: "setPanelHotkeyText",
  uiLanguage: "setUiLanguage",
  selectedItemName: "setSelectedItemName",
  selectedItemSource: "setSelectedItemSource",
  inventoryItems: "setInventoryItems",
  recipeItems: "setRecipeItems",
  runewordPanelState: "setRunewordPanelState",
  runewordAffixPreview: "setRunewordAffixPreview",
  panelLayout: "setPanelLayout",
  tooltipLayout: "setTooltipLayout"
});
const previewInvalidatingCommands = new Set([
  "runeword.insert",
  "runeword.reforge",
  "runeword.reset"
]);
const previewInvalidatingCommandPrefixes = Object.freeze([
  "runeword.base.select:",
  lockedReforgeCommandPrefix
]);
const panelRenderState = {
  queued: false,
  dirty: {
    hotkeyHints: false,
    selectedItemContext: false,
    inventoryItems: false,
    recipeItems: false,
    runewordPanelState: false,
    resourceDashboard: false,
    equippedBuild: false,
    tooltipLayout: false,
    tooltipPlacement: false,
    quickLaunch: false
  }
};

function flushPanelRender() {
  panelRenderState.queued = false;
  const next = { ...panelRenderState.dirty };
  for (const key of Object.keys(panelRenderState.dirty)) {
    panelRenderState.dirty[key] = false;
  }

  if (next.hotkeyHints) {
    updatePanelHotkeyHints();
  }
  if (next.selectedItemContext) {
    renderSelectedItemContext();
  }
  if (next.inventoryItems) {
    renderInventoryItems();
  }
  if (next.recipeItems) {
    renderRecipeItems();
  }
  if (next.runewordPanelState) {
    renderRunewordPanelState();
  }
  if (next.resourceDashboard) {
    renderResourceDashboard();
  }
  if (next.equippedBuild) {
    renderEquippedBuildSummary();
  }
  if (next.tooltipLayout) {
    applyTooltipLayout();
  }
  if (next.tooltipPlacement) {
    applyTooltipPlacement();
  }
  if (next.quickLaunch) {
    applyQuickLaunchVisibility();
  }
}

function schedulePanelRender(...a_sections) {
  let hasDirtySection = false;
  for (const section of a_sections) {
    if (!section || !(section in panelRenderState.dirty)) {
      continue;
    }
    panelRenderState.dirty[section] = true;
    hasDirtySection = true;
  }

  if (!hasDirtySection || panelRenderState.queued) {
    return;
  }

  panelRenderState.queued = true;
  requestAnimationFrame(flushPanelRender);
}
