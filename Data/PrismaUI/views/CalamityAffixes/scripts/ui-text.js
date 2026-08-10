function resolveRecipeFilterLabel(filter) {
  switch (filter) {
    case "weapon":
      return t("Weapon", "무기");
    case "armor":
      return t("Armor", "방어구");
    case "mixed":
      return t("Mixed", "혼합");
    default:
      return t("All", "전체");
  }
}

function updateRecipeFilterControls() {
  if (recipeBaseFilters) {
    recipeBaseFilters.setAttribute(
      "aria-label",
      t("Filter recipes by base type", "베이스 유형으로 레시피 필터")
    );
  }
  for (const button of recipeFilterButtons) {
    const filter = button.getAttribute(recipeFilterAttribute) || "all";
    button.textContent = resolveRecipeFilterLabel(filter);
    button.setAttribute("aria-pressed", filter === recipeBaseFilter ? "true" : "false");
  }
}

function initUiText() {
  document.documentElement.lang = uiLang === "ko" ? "ko" : "en";
  tooltipTitle.textContent = t("Affix", "어픽스");
  tooltipHint.textContent = t(
    "Shows affixes of the selected inventory item.",
    "인벤토리에서 선택한 아이템의 어픽스를 표시합니다."
  );
  if (panelTitle) {
    panelTitle.textContent = t("Calamity Controls", "칼래미티 조작");
  }
  if (panelSub) {
    panelSub.textContent = t(
      "This is the main Prisma control panel. Close with ESC or the Close button below.",
      "Prisma 메인 조작 패널입니다. ESC 또는 아래 Close 버튼으로 닫습니다."
    );
  }
  if (mainTabList) {
    mainTabList.setAttribute("aria-label", t("Calamity tabs", "칼래미티 탭"));
  }
  if (controlPanelCloseX) {
    controlPanelCloseX.setAttribute("aria-label", t("Close panel", "패널 닫기"));
  }
  if (quickOpenRuneword) {
    quickOpenRuneword.setAttribute("aria-label", t("Open Calamity panel", "칼래미티 패널 열기"));
  }
  if (tooltipRunewordHint) {
    tooltipRunewordHint.setAttribute("aria-label", t("Open Calamity panel", "칼래미티 패널 열기"));
  }
  if (runewordFlowTitle) {
    runewordFlowTitle.textContent = t("Runeword Workbench", "룬워드 작업대");
  }
  if (runewordFlowHint) {
    runewordFlowHint.textContent = t(
      "Start by selecting one equipped base.",
      "먼저 착용 중인 베이스 아이템 하나를 선택하세요."
    );
  }
  if (runewordBaseStepTitle) {
    runewordBaseStepTitle.textContent = t("Base Selection", "베이스 선택");
  }
  if (runewordRecipeStepTitle) {
    runewordRecipeStepTitle.textContent = t("Recipe Explorer", "레시피 탐색기");
  }
  if (runewordActionStepTitle) {
    runewordActionStepTitle.textContent = t("Review & Action", "검토 및 실행");
  }
  if (runewordBaseStepHint) {
    runewordBaseStepHint.textContent = t(
      "Pick one compatible equipped item and keep it locked while you compare recipes.",
      "호환되는 착용 아이템 하나를 고정한 뒤 레시피를 비교하세요."
    );
  }
  if (runewordRecipeStepHint) {
    runewordRecipeStepHint.textContent = t(
      "This is the main workspace. Search, compare, and choose the recipe before you commit.",
      "이곳이 메인 작업 영역입니다. 검색하고 비교한 뒤 적용할 레시피를 고르세요."
    );
  }
  if (runewordActionStepHint) {
    runewordActionStepHint.textContent = t(
      "Review the requirements, then execute when ready.",
      "요구 조건을 확인한 뒤 준비되면 실행하세요."
    );
  }
  if (runewordBaseChooserSummary) {
    runewordBaseChooserSummary.textContent = t("Change Base", "베이스 변경");
  }
  if (runewordCubeDetailsSummary) {
    runewordCubeDetailsSummary.textContent = t("Rune Grid", "룬 그리드");
  }
  if (runewordActionDetailsSummary) {
    runewordActionDetailsSummary.textContent = t(
      "Selected Recipe Preview",
      "선택 레시피 미리보기"
    );
  }
  if (runewordBaseAffixSummary) {
    runewordBaseAffixSummary.textContent = t(
      "Selected Base Affixes",
      "선택 베이스 어픽스"
    );
  }
  selectedItemLabel.textContent = t("Current Base", "현재 베이스");
  if (runewordContextRecipeLabel) {
    runewordContextRecipeLabel.textContent = t("Selected Recipe", "선택 레시피");
  }
  if (runewordContextRecipeName) {
    runewordContextRecipeName.textContent = t("No recipe selected", "선택된 레시피 없음");
  }
  if (runewordContextRecipeMeta) {
    runewordContextRecipeMeta.textContent = t(
      "Search the center explorer and select one recipe to review requirements.",
      "중앙 탐색기에서 레시피를 선택하면 요구 조건을 검토할 수 있습니다."
    );
  }
  if (affixSelectedItemLabel) {
    affixSelectedItemLabel.textContent = t("Selected Item", "선택 아이템");
  }
  if (affixSelectedItemMeta) {
    affixSelectedItemMeta.textContent = t(
      "Highlight one inventory item to mirror its affix details here.",
      "인벤토리에서 아이템 하나를 가리키면 여기에서 어픽스 상세를 확인할 수 있습니다."
    );
  }
  if (panelTooltipTitle) {
    panelTooltipTitle.textContent = t("Item Affix Details", "아이템 어픽스 상세");
  }
  if (panelTooltipLead) {
    panelTooltipLead.textContent = t(
      "Focus one item at a time to read affix text without opening the full runeword flow.",
      "한 번에 아이템 하나에 집중해서 어픽스 텍스트를 읽을 수 있습니다."
    );
  }
  if (panelTooltipHint) {
    panelTooltipHint.textContent = t(
      "Open your inventory (Tab) and hover an item, or select a base in the Runeword tab.",
      "인벤토리(Tab)를 열어 아이템을 가리키거나, 룬워드 탭에서 베이스를 선택하세요."
    );
  }
  runewordRecipeListTitle.textContent = t("Search And Compare", "검색 및 비교");
  runewordRecipeListHint.textContent = t(
    "Select a recipe to load its requirements and actions on the right.",
    "레시피를 선택하면 오른쪽 검토 영역에 요구 조건과 실행 기능이 표시됩니다."
  );
  if (recipeSearchInput) {
    recipeSearchInput.setAttribute("aria-label", t("Search runeword recipe", "룬워드 레시피 검색"));
    recipeSearchInput.placeholder = t("Search recipe...", "레시피 검색...");
  }
  updateRecipeFilterControls();
  runewordBaseListTitle.textContent = t("Compatible Equipped Bases", "호환 착용 베이스");
  runewordBaseListHint.textContent = t(
    "Asterisks after a name show its affix count: * one, ** two, *** three or more.",
    "이름 뒤 별표는 어픽스 수입니다: * 1개 · ** 2개 · *** 3개 이상."
  );
  if (runewordCubeGrid) {
    runewordCubeGrid.setAttribute("aria-label", t("Horadric cube", "호라드릭 큐브"));
  }
  if (mainRunewordTab) {
    mainRunewordTab.textContent = t("Runeword", "룬워드");
  }
  if (mainAffixTab) {
    mainAffixTab.textContent = t("Item Affixes", "아이템 어픽스");
  }
  if (mainAdvancedTab) {
    mainAdvancedTab.textContent = t("Advanced", "고급");
  }
  if (tooltipUiSectionTitle) {
    tooltipUiSectionTitle.textContent = t("Tooltip UI", "툴팁 UI");
  }
  if (tooltipTextSmallerButton) {
    tooltipTextSmallerButton.textContent = t("Text -", "글자 -");
  }
  if (tooltipTextLargerButton) {
    tooltipTextLargerButton.textContent = t("Text +", "글자 +");
  }
  if (tooltipMoveLeftButton) {
    tooltipMoveLeftButton.textContent = t("Move Left", "왼쪽 이동");
  }
  if (tooltipMoveRightButton) {
    tooltipMoveRightButton.textContent = t("Move Right", "오른쪽 이동");
  }
  if (tooltipMoveUpButton) {
    tooltipMoveUpButton.textContent = t("Move Up", "위로 이동");
  }
  if (tooltipMoveDownButton) {
    tooltipMoveDownButton.textContent = t("Move Down", "아래로 이동");
  }
  if (tooltipResetButton) {
    tooltipResetButton.textContent = t("Reset Tooltip", "툴팁 초기화");
  }
  panelDragHint.textContent = t(
    "Drag this header to move panel",
    "이 헤더를 드래그해 패널 위치 이동"
  );
  if (runewordStatusButton) {
    runewordStatusButton.textContent = t("Check Status", "상태 확인");
  }
  if (runewordReforgeButton) {
    runewordReforgeButton.textContent = t("Reforge", "재련");
  }
  if (runewordResetButton && Date.now() >= runewordResetArmedUntil) {
    runewordResetButton.textContent = t("Reset Selected Base", "선택 베이스 초기화");
  }
  if (runewordItemActionsTitle) {
    runewordItemActionsTitle.textContent = t("Item Actions", "아이템 작업");
  }
  if (runewordRecoverySummary) {
    runewordRecoverySummary.textContent = t("Recovery & Reset", "복구 및 초기화");
  }
  if (manualModeMeta) {
    manualModeMeta.textContent = t(
      "Cycles the element of adaptive-element affixes by hand instead of auto-picking by enemy resistances.",
      "적응 원소 어픽스의 원소를 적 저항 자동 선택 대신 수동으로 순환합니다."
    );
  }
  if (manualModeTitle) {
    manualModeTitle.textContent = t("Manual Mode", "수동 모드");
  }
  if (manualPrevButton) {
    manualPrevButton.textContent = t("Manual Prev", "수동 이전");
  }
  if (manualNextButton) {
    manualNextButton.textContent = t("Manual Next", "수동 다음");
  }
  if (debugSectionTitle) {
    debugSectionTitle.textContent = t("Debug Tools", "디버그 도구");
  }
  if (debugSectionBadge) {
    debugSectionBadge.textContent = t("Use carefully", "주의");
  }
  if (debugSectionHint) {
    debugSectionHint.textContent = t(
      "These actions are for testing and recovery. Use them only when you understand the effect.",
      "테스트 및 복구용 기능입니다. 효과를 이해할 때만 사용하세요."
    );
  }
  if (debugGrantNextButton) {
    debugGrantNextButton.textContent = t("+1 Next Fragment", "다음 조각 +1");
  }
  if (debugGrantSetButton) {
    debugGrantSetButton.textContent = t("+1 Recipe Set", "레시피 세트 +1");
  }
  if (debugGrantStarterOrbsButton) {
    debugGrantStarterOrbsButton.textContent = t(
      "Starter +3 Reforge Orbs",
      "스타트 재련 오브 +3"
    );
  }
  if (debugGrantTrapAffixButton) {
    debugGrantTrapAffixButton.textContent = t(
      "Grant Trap Affix to Selected Base",
      "선택 베이스에 함정 어픽스"
    );
  }
  if (debugTrapProbeButton) {
    debugTrapProbeButton.textContent = t(
      "Trap Marker Probe at Feet",
      "발밑 함정 마커 프로브"
    );
  }
  if (debugSpawnTestButton) {
    debugSpawnTestButton.textContent = t("Spawn Test Item", "테스트 아이템 지급");
  }
  if (currencyRecoverButton) {
    currencyRecoverButton.textContent = t("Recover Currency", "재료 복구");
  }
  if (footerCloseButton) {
    footerCloseButton.textContent = t("Close Panel", "패널 닫기");
  }
  setMainTab(mainTabState);
  schedulePanelRender(
    panelRenderSection.hotkeyHints,
    panelRenderSection.selectedItemContext,
    panelRenderSection.recipeItems,
    panelRenderSection.inventoryItems,
    panelRenderSection.runewordPanelState,
    panelRenderSection.tooltipLayout,
    panelRenderSection.tooltipPlacement,
    panelRenderSection.quickLaunch
  );
}

function focusCurrentMainTab() {
  const focusEl =
    mainTabState === "affix"
      ? mainAffixTab
      : mainTabState === "advanced"
        ? mainAdvancedTab
        : mainRunewordTab;
  if (focusEl && typeof focusEl.focus === "function") {
    focusEl.focus();
  }
}

function clearChildren(node) {
  if (!node) return;
  while (node.firstChild) {
    node.removeChild(node.firstChild);
  }
}

function appendEmptyState(node, title, body, hint = "") {
  if (!node) return;
  clearChildren(node);

  const wrap = document.createElement("div");
  wrap.className = "cpEmptyState";

  const titleEl = document.createElement("div");
  titleEl.className = "cpEmptyTitle";
  titleEl.textContent = title;
  wrap.appendChild(titleEl);

  if (body) {
    const bodyEl = document.createElement("div");
    bodyEl.className = "cpEmptyBody";
    bodyEl.textContent = body;
    wrap.appendChild(bodyEl);
  }

  if (hint) {
    const hintEl = document.createElement("div");
    hintEl.className = "cpEmptyHint";
    hintEl.textContent = hint;
    wrap.appendChild(hintEl);
  }

  node.appendChild(wrap);
}

function applyQuickLaunchVisibility() {
  if (!quickLaunch) return;
  // The always-on quick-launch pill is helpful for discovery, but it is also
  // visually noisy in inventory menus. Prefer a "silent" UI by default.
  // Users can still open/close the panel via the hotkey (default: F11),
  // and the tooltip UI already provides a contextual hint when relevant.
  quickLaunch.style.display = "none";
}

function invalidateRunewordAffixPreview(pending) {
  runewordAffixTextState = "";
  runewordAffixPendingState = Boolean(pending);
  if (!runewordAffixPendingState) {
    runewordAffixPendingNonce += 1;
    schedulePanelRender(panelRenderSection.tooltipPlacement);
    return;
  }

  schedulePanelRender(panelRenderSection.tooltipPlacement);

  const pendingNonce = ++runewordAffixPendingNonce;
  // Failsafe: never leave preview in perpetual "refreshing" state.
  window.setTimeout(() => {
    if (!runewordAffixPendingState) return;
    if (pendingNonce !== runewordAffixPendingNonce) return;
    runewordAffixPendingState = false;
    schedulePanelRender(panelRenderSection.tooltipPlacement);
  }, runewordAffixPendingTimeoutMs);
}


