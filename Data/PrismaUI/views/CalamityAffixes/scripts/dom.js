      function clamp(value, min, max) {
        return Math.max(min, Math.min(max, value));
      }

      const tooltipReferenceDesktop = Object.freeze({
        width: 3840,
        height: 2160,
        right: 70,
        top: 255,
        scale: 2.5,
        fontScale: 1
      });

      function getTooltipViewportRatio() {
        const w = window.innerWidth || tooltipReferenceDesktop.width;
        const h = window.innerHeight || tooltipReferenceDesktop.height;
        return Math.min(w / tooltipReferenceDesktop.width, h / tooltipReferenceDesktop.height);
      }

      function getTooltipAutoScale() {
        return clamp(
          getTooltipViewportRatio() * tooltipReferenceDesktop.scale,
          1.0,
          tooltipReferenceDesktop.scale
        );
      }

      function applyAutoScale() {
        const scale = getTooltipAutoScale();
        document.documentElement.style.setProperty("--scale", scale.toFixed(3));
      }

      const tooltipPanel = document.getElementById("tooltipPanel");
      const tooltipTitle = document.getElementById("tooltipTitle");
      const tooltipText = document.getElementById("tooltipText");
      const tooltipHint = document.getElementById("tooltipHint");
      const quickLaunch = document.getElementById("quickLaunch");
      const quickOpenRuneword = document.getElementById("quickOpenRuneword");
      const controlPanel = document.getElementById("controlPanel");
      const panelTitle = document.getElementById("panelTitle");
      const panelSub = document.getElementById("panelSub");
      const feedback = document.getElementById("feedback");
      const runewordFlowTitle = document.getElementById("runewordFlowTitle");
      const runewordFlowHint = document.getElementById("runewordFlowHint");
      const runewordBaseStep = document.getElementById("runewordBaseStep");
      const runewordRecipeStep = document.getElementById("runewordRecipeStep");
      const runewordActionStep = document.getElementById("runewordActionStep");
      const runewordBaseStepTitle = document.getElementById("runewordBaseStepTitle");
      const runewordRecipeStepTitle = document.getElementById("runewordRecipeStepTitle");
      const runewordActionStepTitle = document.getElementById("runewordActionStepTitle");
      const runewordBaseStepHint = document.getElementById("runewordBaseStepHint");
      const runewordRecipeStepHint = document.getElementById("runewordRecipeStepHint");
      const runewordActionStepHint = document.getElementById("runewordActionStepHint");
      const selectedItemLabel = document.getElementById("selectedItemLabel");
      const selectedItemName = document.getElementById("selectedItemName");
      const selectedItemSource = document.getElementById("selectedItemSource");
      const runewordContextRecipeLabel = document.getElementById("runewordContextRecipeLabel");
      const runewordContextRecipeName = document.getElementById("runewordContextRecipeName");
      const runewordContextRecipeMeta = document.getElementById("runewordContextRecipeMeta");
      const runewordBaseChooserSummary = document.getElementById("runewordBaseChooserSummary");
      const runewordCubeDetailsSummary = document.getElementById("runewordCubeDetailsSummary");
      const affixSelectedItemLabel = document.getElementById("affixSelectedItemLabel");
      const affixSelectedItemName = document.getElementById("affixSelectedItemName");
      const affixSelectedItemMeta = document.getElementById("affixSelectedItemMeta");
      const panelTooltipTitle = document.getElementById("panelTooltipTitle");
      const panelTooltipLead = document.getElementById("panelTooltipLead");
      const panelTooltipText = document.getElementById("panelTooltipText");
      const panelTooltipHint = document.getElementById("panelTooltipHint");
      const inventoryBaseList = document.getElementById("inventoryBaseList");
      const recipeList = document.getElementById("recipeList");
      const recipeSearchInput = document.getElementById("recipeSearchInput");
      const recipeCountLabel = document.getElementById("recipeCountLabel");
      const recipeBaseFilters = document.getElementById("recipeBaseFilters");
      const recipeFilterButtons = Array.from(document.querySelectorAll("[data-recipe-filter]"));
      const runewordCubeGrid = document.getElementById("runewordCubeGrid");
      const runewordRecipeListTitle = document.getElementById("runewordRecipeListTitle");
      const runewordRecipeListHint = document.getElementById("runewordRecipeListHint");
      const runewordBaseListTitle = document.getElementById("runewordBaseListTitle");
      const runewordBaseListHint = document.getElementById("runewordBaseListHint");
      const runewordAffixText = document.getElementById("runewordAffixText");
      const runewordPanelStatus = document.getElementById("runewordPanelStatus");
      const runewordActionHint = document.getElementById("runewordActionHint");
      const runewordActionDetailsSummary = document.getElementById("runewordActionDetailsSummary");
      const runewordItemActionsTitle = document.getElementById("runewordItemActionsTitle");
      const runewordRecoverySummary = document.getElementById("runewordRecoverySummary");
      const runewordInsertButton = document.getElementById("runewordInsertButton");
      const runewordReforgeButton = document.getElementById("runewordReforgeButton");
      const runewordStatusButton = document.getElementById("runewordStatusButton");
      const runewordResetButton = document.getElementById("runewordResetButton");
      const mainRunewordTab = document.getElementById("mainRunewordTab");
      const mainAffixTab = document.getElementById("mainAffixTab");
      const mainAdvancedTab = document.getElementById("mainAdvancedTab");
      const mainTabList = document.getElementById("mainTabList");
      const mainRunewordPane = document.getElementById("mainRunewordPane");
      const mainAffixPane = document.getElementById("mainAffixPane");
      const mainAdvancedPane = document.getElementById("mainAdvancedPane");
      const tooltipUiSectionTitle = document.getElementById("tooltipUiSectionTitle");
      const tooltipUiSummary = document.getElementById("tooltipUiSummary");
      const tooltipTextSmallerButton = document.getElementById("tooltipTextSmallerButton");
      const tooltipTextLargerButton = document.getElementById("tooltipTextLargerButton");
      const tooltipMoveLeftButton = document.getElementById("tooltipMoveLeftButton");
      const tooltipMoveRightButton = document.getElementById("tooltipMoveRightButton");
      const tooltipMoveUpButton = document.getElementById("tooltipMoveUpButton");
      const tooltipMoveDownButton = document.getElementById("tooltipMoveDownButton");
      const tooltipResetButton = document.getElementById("tooltipResetButton");
      const manualModeTitle = document.getElementById("manualModeTitle");
      const manualPrevButton = document.getElementById("manualPrevButton");
      const manualNextButton = document.getElementById("manualNextButton");
      const debugSectionTitle = document.getElementById("debugSectionTitle");
      const debugSectionSummary = document.getElementById("debugSectionSummary");
      const debugSectionBadge = document.getElementById("debugSectionBadge");
      const debugSectionHint = document.getElementById("debugSectionHint");
      const debugGrantNextButton = document.getElementById("debugGrantNextButton");
      const debugGrantSetButton = document.getElementById("debugGrantSetButton");
      const debugGrantStarterOrbsButton = document.getElementById("debugGrantStarterOrbsButton");
      const debugSpawnTestButton = document.getElementById("debugSpawnTestButton");
      const currencyRecoverButton = document.getElementById("currencyRecoverButton");
      const footerCloseButton = document.getElementById("footerCloseButton");
      const controlPanelCloseX = document.getElementById("controlPanelCloseX");
      const panelDragHandle = document.getElementById("panelDragHandle");
      const panelDragHint = document.getElementById("panelDragHint");
      const panelResizeRight = document.getElementById("panelResizeRight");
      const panelResizeBottom = document.getElementById("panelResizeBottom");
      const panelResizeCorner = document.getElementById("panelResizeCorner");
      const tooltipRunewordHint = document.getElementById("tooltipRunewordHint");

