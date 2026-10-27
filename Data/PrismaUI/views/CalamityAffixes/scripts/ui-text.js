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

function resolveRecipeMaterialFilterLabel(filter) {
  switch (filter) {
    case "ready":
      return t("Fragments ready", "룬 조각 준비");
    case "missing1":
      return t("Missing 1 fragment", "룬 조각 1개 부족");
    default:
      return t("Any fragments", "조각 무관");
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
  if (recipeMaterialFilters) {
    recipeMaterialFilters.setAttribute(
      "aria-label",
      t(
        "Filter recipes by current fragment inventory",
        "현재 룬 조각 보유량으로 레시피 필터"
      )
    );
    recipeMaterialFilters.setAttribute(
      "aria-disabled",
      runeInventoryKnownState ? "false" : "true"
    );
  }
  for (const button of recipeMaterialFilterButtons) {
    const filter = button.getAttribute(recipeMaterialFilterAttribute) || "all";
    button.textContent = resolveRecipeMaterialFilterLabel(filter);
    button.setAttribute(
      "aria-pressed",
      filter === recipeMaterialFilter ? "true" : "false"
    );
    button.disabled = !runeInventoryKnownState;
  }
  if (recipeMaterialFilterHint) {
    recipeMaterialFilterHint.textContent = runeInventoryKnownState
      ? t("Based on fragments you own.", "보유한 룬 조각 기준")
      : t(
          "Loading fragments; showing all recipes.",
          "룬 조각을 불러오는 중이라 모든 레시피를 표시합니다."
        );
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
    panelTitle.textContent = t("Calamity", "칼라미티");
  }
  if (mainTabList) {
    mainTabList.setAttribute("aria-label", t("Calamity tabs", "칼라미티 탭"));
  }
  if (controlPanelCloseX) {
    controlPanelCloseX.setAttribute("aria-label", t("Close panel", "패널 닫기"));
  }
  if (quickOpenRuneword) {
    quickOpenRuneword.setAttribute("aria-label", t("Open Calamity panel", "칼라미티 패널 열기"));
  }
  if (tooltipRunewordHint) {
    tooltipRunewordHint.setAttribute("aria-label", t("Open Calamity panel", "칼라미티 패널 열기"));
  }
  if (runewordBaseStepTitle) {
    runewordBaseStepTitle.textContent = t("Item", "장비 선택");
  }
  if (runewordRecipeStepTitle) {
    runewordRecipeStepTitle.textContent = t("Recipe Explorer", "레시피 탐색기");
  }
  if (runewordActionStepTitle) {
    runewordActionStepTitle.textContent = t("Review & Action", "검토 및 실행");
  }
  if (runewordBaseChooserSummary) {
    runewordBaseChooserSummary.textContent = t("Change Item", "장비 변경");
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
    // The slot list below is the main view; this is the full tooltip text.
    runewordBaseAffixSummary.textContent = t(
      "Full Item Effects",
      "장비 효과 전체 보기"
    );
  }
  if (selectedItemLabel) {
    selectedItemLabel.textContent = t("Inspected Item", "확인 중인 아이템");
  }
  if (affixSelectedItemLabel) {
    affixSelectedItemLabel.textContent = t("Inspected Inventory Item", "확인 중인 인벤토리 아이템");
  }
  if (affixInspectionSummary) {
    affixInspectionSummary.textContent = t("Inspect Highlighted Item", "가리킨 아이템 확인");
  }
  if (affixSelectedItemMeta) {
    affixSelectedItemMeta.textContent = t(
      "The item you're pointing at in your inventory.",
      "인벤토리에서 가리킨 아이템입니다."
    );
  }
  updateEquippedBuildStaticText();
  if (panelTooltipTitle) {
    panelTooltipTitle.textContent = t("Item Affix Details", "아이템 어픽스 상세");
  }
  if (panelTooltipHint) {
    panelTooltipHint.textContent = t(
      "Open your inventory (Tab) and point at an item.",
      "인벤토리(Tab)를 열고 아이템을 가리키세요."
    );
  }
  runewordRecipeListTitle.textContent = t("Search And Compare", "검색 및 비교");
  if (recipeSearchInput) {
    recipeSearchInput.setAttribute("aria-label", t("Search runeword recipe", "룬워드 레시피 검색"));
    recipeSearchInput.placeholder = t("Search recipe...", "레시피 검색...");
  }
  updateRecipeFilterControls();
  runewordBaseListTitle.textContent = t("Equipped Items", "착용 중인 장비");
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
  if (panelDragHandle) {
    panelDragHandle.title = t("Drag to move", "드래그해서 이동");
  }
  if (runewordStatusButton) {
    runewordStatusButton.textContent = t("Check Status", "상태 확인");
  }
  if (runewordReforgeButton) {
    runewordReforgeButton.textContent = t("Reforge", "재련");
  }
  if (runewordResetButton && Date.now() >= runewordResetArmedUntil) {
    runewordResetButton.textContent = t("Reset Selected Item", "선택한 장비 초기화");
  }
  if (runewordItemActionsTitle) {
    runewordItemActionsTitle.textContent = t(
      "Reforge Affixes",
      "어픽스 재련"
    );
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
      "Starter Crafting Kit",
      "스타터 제작 재료"
    );
  }
  if (debugGrantTrapAffixButton) {
    debugGrantTrapAffixButton.textContent = t(
      "Grant Test Affix to Selected Item",
      "선택한 장비에 테스트 어픽스"
    );
  }
  if (debugTestPlanLabel) {
    debugTestPlanLabel.textContent = t("Test Plan", "테스트 목록");
  }
  renderDebugTestPlan();
  if (debugTrapProbeButton) {
    debugTrapProbeButton.textContent = t(
      "Trap Marker Test",
      "함정 표시 테스트"
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
    panelRenderSection.resourceDashboard,
    panelRenderSection.equippedBuild,
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
    if (pendingNonce !== runewordAffixPendingNonce) return;
    settleRunewordAffixPreview();
  }, runewordAffixPendingTimeoutMs);
}

// Ends a pending refresh that got no new preview text. The runtime does not
// resend text it already sent, so the last pushed text is still current.
function settleRunewordAffixPreview() {
  if (!runewordAffixPendingState) return false;
  runewordAffixPendingState = false;
  if (!runewordAffixTextState) {
    runewordAffixTextState = runewordAffixConfirmedTextState;
  }
  schedulePanelRender(panelRenderSection.tooltipPlacement);
  return true;
}

// Debug test plan (v2.3.0): the checks still waiting for an in-game look. Each
// row says what to do, sets the selected item up in one click, and turns to
// "Seen" once the runtime reports the event (debugTestEvents in the panel state).
// Empty between test rounds: add a row when a change needs an in-game look, and
// drop it once the game has reported it.
const debugTestPlanItems = [
  {
    id: "staleStatMigration", base: "any", setup: "", events: ["migration:cleaned"],
    title: ["Old save cleanup", "옛 세이브 정리"],
    todo: [
      "Load a save made with 2.2.8 (or an rc) while wearing Gladiator, Eagle Eye or Enchanter gear. The log should show \"stale stat migration\" lines and no warning; attack speed, bow and enchanting values look normal.",
      "2.2.8(또는 rc)에서 검투사·독수리눈·부여술사 장비를 낀 채 저장한 세이브를 불러오세요. 로그에 \"stale stat migration\" 줄이 찍히고 경고가 없어야 하며, 공격 속도·활·부여 수치가 정상이어야 합니다."
    ]
  },
  {
    id: "mcmLanguage", base: "any", setup: "", events: ["mcm:language"],
    title: ["MCM language", "MCM 언어"],
    todo: [
      "In the MCM, change Language: the open page should switch at once, the right-hand values (e.g. Player-centric) should show, and nothing should be missing. Also press F11 with the cursor in the recipe search box: the panel should close.",
      "MCM에서 언어를 바꾸세요. 열린 페이지가 바로 바뀌고, 오른쪽 상태 문구(예: 플레이어 중심)가 보이고, 빠진 항목이 없어야 합니다. 레시피 검색칸에 커서를 둔 채 F11을 눌러 패널이 닫히는지도 보세요."
    ]
  },
  {
    id: "illusionRuneword", base: "armor", setup: "spawn.testrw:rw_peace", events: ["affix:runeword_peace_final"],
    title: ["Runeword Peace (calm)", "룬워드 피스(진정)"],
    todo: [
      "Wear it and get hit by bandits or other people up to level 60: the attacker should stop fighting for a few seconds. Dragons, draugr, Daedra and Dwarven machines are not calmed.",
      "장착하고 레벨 60 이하의 산적 같은 사람에게 맞으세요. 공격자가 몇 초 동안 싸움을 멈춰야 합니다. 드래곤·드라우그·데이드라·드워프 기계는 진정되지 않습니다."
    ]
  },
  {
    id: "elementalConversion", base: "weapon", setup: "spawn.testset:18", events: ["convert:hit"],
    title: ["Fire conversion", "화염 전환"],
    todo: [
      "Puts Fire Infusion (100%) on the item. Hit enemies quickly: each hit's fire damage should land at once and the fire hit effect should still show.",
      "화염 주입(100%)을 붙입니다. 적을 빠르게 연타하세요. 타격마다 화염 피해가 바로 들어가고 불 피격 연출이 그대로 보여야 합니다."
    ]
  },
  {
    id: "invisibilityEnds", base: "armor", setup: "spawn.testset:19", events: ["affix:shadow_veil", "affix:silent_step"],
    title: ["Invisibility ends cleanly", "투명화가 깔끔히 끝나는지"],
    todo: [
      "Puts Shadow Counter and Silent Step on the item. Let both turn you invisible (get hit, then kill). After the effect ends, enemies must see you again and you must not stay invisible.",
      "그림자 반격과 고요한 걸음을 붙입니다. 맞고 처치해서 두 효과로 투명해진 뒤, 효과가 끝나면 적이 다시 나를 알아보고 투명이 남지 않아야 합니다."
    ]
  }
];

function renderDebugTestPlan() {
  if (!debugTestPlan) return;
  const empty = debugTestPlanItems.length === 0;
  for (const element of [debugTestPlanLabel, debugTestPlanHint, debugTestPlan]) {
    if (element) element.hidden = empty;
  }
  if (empty) return;
  const seen = new Set(Array.isArray(runewordPanelState?.debugTestEvents) ? runewordPanelState.debugTestEvents : []);
  const done = debugTestPlanItems.filter((item) => item.events.every((key) => seen.has(key))).length;
  if (debugTestPlanHint) {
    // Quote the controls with the same helper that labels them: in both-language
    // mode the button and chip show English, so the Korean half names that.
    const setUpLabel = tCompact("Set up", "준비");
    const seenLabel = tCompact("Seen", "확인됨");
    debugTestPlanHint.textContent = t(
      `${done}/${debugTestPlanItems.length} seen this session. Select a weapon or armor as the row says, then Set up: affix rows replace every affix on it; runeword rows put the runeword in its first slot (a prefix there goes, suffixes stay). Equip it and do what the row says; the row turns to Seen when the game reports the event.`,
      `이번 세션에서 ${done}/${debugTestPlanItems.length}개 확인됨. 줄에 적힌 무기·방어구를 고른 뒤 "${setUpLabel}" 버튼을 누르세요. 어픽스 줄은 그 장비의 어픽스를 전부 바꾸고, 룬워드 줄은 첫 칸에 룬워드를 넣습니다(그 칸의 접두는 사라지고 접미는 남음). 장착하고 적힌 대로 하면, 게임이 효과를 감지하는 순간 줄이 "${seenLabel}" 표시로 바뀝니다.`
    );
  }
  debugTestPlan.textContent = "";
  for (const item of debugTestPlanItems) {
    const isSeen = item.events.every((key) => seen.has(key));
    const row = document.createElement("div");
    row.className = isSeen ? "cpTestRow seen" : "cpTestRow";
    const text = document.createElement("div");
    text.className = "cpTestRowText";
    const title = document.createElement("div");
    title.className = "cpTestRowTitle";
    const armor = item.base === "armor";
    title.textContent = item.base === "any"
      ? t(item.title[0], item.title[1])
      : t(`${item.title[0]} · ${armor ? "armor" : "weapon"}`, `${item.title[1]} · ${armor ? "방어구" : "무기"}`);
    const todo = document.createElement("div");
    todo.className = "cpTestRowTodo";
    todo.textContent = t(item.todo[0], item.todo[1]);
    text.appendChild(title);
    text.appendChild(todo);
    const state = document.createElement("span");
    state.className = isSeen ? "cpTestRowState seen" : "cpTestRowState";
    state.textContent = isSeen ? tCompact("Seen", "확인됨") : tCompact("Waiting", "대기");
    state.title = isSeen ? t("Seen", "확인됨") : t("Waiting", "대기");
    row.appendChild(text);
    row.appendChild(state);
    // Rows that need no item (an old save, an MCM setting) have no Set up button.
    if (item.setup) {
      const button = document.createElement("button");
      button.className = "cpButton cpTestRowButton";
      button.setAttribute(panelCommandAttribute, item.setup);
      button.textContent = tCompact("Set up", "준비");
      button.title = t("Set up", "준비");
      row.appendChild(button);
    }
    debugTestPlan.appendChild(row);
  }
}
