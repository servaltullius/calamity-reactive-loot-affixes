function resolveRecipeToken(item) {
  return typeof item?.token === "string" ? item.token.trim() : "";
}

const maxUint64DecimalString = "18446744073709551615";

function normalizePositiveUint64DecimalString(value) {
  if (typeof value !== "string") {
    return "";
  }
  const normalized = value.trim();
  if (!/^[1-9][0-9]*$/.test(normalized)) {
    return "";
  }
  if (normalized.length > maxUint64DecimalString.length) {
    return "";
  }
  if (
    normalized.length === maxUint64DecimalString.length &&
    normalized > maxUint64DecimalString
  ) {
    return "";
  }
  return normalized;
}

function normalizeRecipeRuneTokens(raw) {
  if (!Array.isArray(raw) || raw.length === 0) {
    return null;
  }

  const normalized = [];
  for (const value of raw) {
    const token = normalizePositiveUint64DecimalString(value);
    if (!token) {
      return null;
    }
    // Preserve recipe order and duplicates: repeated runes consume repeated fragments.
    normalized.push(token);
  }
  return normalized;
}

function normalizeRecipeCatalogItems(raw) {
  return (Array.isArray(raw) ? raw : []).map((item) => ({
    ...(item && typeof item === "object" ? item : {}),
    runeTokens: normalizeRecipeRuneTokens(item?.runeTokens)
  }));
}

function buildRecipeCatalogSignature(items) {
  const catalog = (Array.isArray(items) ? items : []).map((item) => [
    resolveRecipeToken(item),
    typeof item?.name === "string" ? item.name : "",
    typeof item?.runes === "string" ? item.runes : "",
    typeof item?.summaryKey === "string" ? item.summaryKey : "",
    typeof item?.summaryEn === "string" ? item.summaryEn : "",
    typeof item?.summaryKo === "string" ? item.summaryKo : "",
    typeof item?.summary === "string" ? item.summary : "",
    typeof item?.detailEn === "string" ? item.detailEn : "",
    typeof item?.detailKo === "string" ? item.detailKo : "",
    typeof item?.detail === "string" ? item.detail : "",
    typeof item?.baseKey === "string" ? item.baseKey : "",
    Array.isArray(item?.runeTokens) ? item.runeTokens : null
  ]);
  return JSON.stringify(catalog);
}

function resolveConfirmedRecipeToken(items) {
  const selected = (Array.isArray(items) ? items : []).find(
    (item) => Boolean(item?.selected) && Boolean(resolveRecipeToken(item))
  );
  return selected ? resolveRecipeToken(selected) : "";
}

function getDisplayedRecipeToken() {
  return optimisticRecipeTokenState || confirmedRecipeTokenState;
}

function updateRecipeSelectionDom(previousToken, nextToken) {
  if (previousToken === nextToken) return;

  const previousNode = recipeNodeByToken.get(previousToken);
  if (previousNode) {
    previousNode.classList.remove("selected");
    previousNode.setAttribute("aria-selected", "false");
  }

  const nextNode = recipeNodeByToken.get(nextToken);
  if (nextNode) {
    nextNode.classList.add("selected");
    nextNode.setAttribute("aria-selected", "true");
  }

  const visibleOptions = Array.from(recipeList.children).filter(
    (child) => child instanceof Element && child.getAttribute("role") === "option"
  );
  for (const option of visibleOptions) {
    option.tabIndex = -1;
  }
  const focusedOption = recipeList.contains(document.activeElement)
    ? document.activeElement?.closest?.('[role="option"]')
    : null;
  const preferredOption = visibleOptions.includes(focusedOption)
    ? focusedOption
    : visibleOptions.includes(nextNode)
      ? nextNode
      : visibleOptions[0];
  if (preferredOption) {
    preferredOption.tabIndex = 0;
  }
}

function applyConfirmedRecipeSelection(nextToken) {
  const previousToken = getDisplayedRecipeToken();
  confirmedRecipeTokenState = nextToken || "";
  optimisticRecipeTokenState = "";
  optimisticRecipeSelectionNonce += 1;
  updateRecipeSelectionDom(previousToken, confirmedRecipeTokenState);
}

function beginOptimisticRecipeSelection(nextToken) {
  if (!nextToken) return;

  const previousToken = getDisplayedRecipeToken();
  if (nextToken === confirmedRecipeTokenState) {
    optimisticRecipeTokenState = "";
    updateRecipeSelectionDom(previousToken, confirmedRecipeTokenState);
    return;
  }

  optimisticRecipeTokenState = nextToken;
  updateRecipeSelectionDom(previousToken, nextToken);
  schedulePanelRender(panelRenderSection.tooltipPlacement);
  const pendingNonce = ++optimisticRecipeSelectionNonce;
  window.setTimeout(() => {
    if (pendingNonce !== optimisticRecipeSelectionNonce) return;
    const optimisticToken = getDisplayedRecipeToken();
    optimisticRecipeTokenState = "";
    updateRecipeSelectionDom(optimisticToken, confirmedRecipeTokenState);
    schedulePanelRender(panelRenderSection.tooltipPlacement);
  }, optimisticRecipeSelectionTimeoutMs);
}

function invalidateRecipePresentationCaches() {
  recipeCatalogDomDirty = true;
  recipeNodeByToken.clear();
  recipeSearchDocumentByToken.clear();
  recipeMaterialViewByToken.clear();
}

function getSelectedRecipeItem() {
  const items = Array.isArray(recipeItemsState) ? recipeItemsState : [];
  const displayedToken = getDisplayedRecipeToken();
  if (displayedToken) {
    const byToken = items.find(
      (item) => resolveRecipeToken(item) === displayedToken
    );
    if (byToken) {
      return byToken;
    }
  }

  const selected = items.find((item) => Boolean(item?.selected));
  if (selected) {
    return selected;
  }

  const targetName = typeof runewordPanelState?.recipeName === "string"
    ? runewordPanelState.recipeName.trim().toLowerCase()
    : "";
  if (!targetName) {
    return null;
  }

  const byName = items.find((item) =>
    typeof item?.name === "string" && item.name.trim().toLowerCase() === targetName
  );
  return byName || null;
}

function buildRecipeSearchDocument(item) {
  const name = typeof item?.name === "string" ? item.name.toLowerCase() : "";
  const runes = typeof item?.runes === "string" ? item.runes.toLowerCase() : "";
  const base = resolveRecipeBaseBadge(item).text.toLowerCase();
  const summary = resolveRecipeNumericSummaryText(item).toLowerCase();
  const detail = resolveRecipeDetailText(item).toLowerCase();
  const tooltipText = buildRunewordTooltipLikeText(item, {
    includeName: true,
    includeDetail: true
  }).toLowerCase();

  return [name, runes, base, summary, detail, tooltipText].join("\n");
}

function resolveRecipeSearchDocument(item) {
  const token = resolveRecipeToken(item);
  if (token && recipeSearchDocumentByToken.has(token)) {
    return recipeSearchDocumentByToken.get(token);
  }

  const searchDocument = buildRecipeSearchDocument(item);
  if (token) {
    recipeSearchDocumentByToken.set(token, searchDocument);
  }
  return searchDocument;
}

function resolveRecipeMaterialState(item) {
  const runeTokens = Array.isArray(item?.runeTokens) ? item.runeTokens : null;
  if (!runeInventoryKnownState || !runeTokens || runeTokens.length === 0) {
    return {
      key: "unknown",
      known: false,
      coveredFragments: 0,
      requiredFragments: runeTokens?.length || 0,
      missingFragments: 0
    };
  }

  const requiredByToken = new Map();
  for (const rawToken of runeTokens) {
    const token = normalizePositiveUint64DecimalString(rawToken);
    if (!token) {
      return {
        key: "unknown",
        known: false,
        coveredFragments: 0,
        requiredFragments: runeTokens.length,
        missingFragments: 0
      };
    }
    requiredByToken.set(token, (requiredByToken.get(token) || 0) + 1);
  }

  let coveredFragments = 0;
  for (const [token, required] of requiredByToken) {
    if (!runeInventoryOwnedByToken.has(token)) {
      return {
        key: "unknown",
        known: false,
        coveredFragments: 0,
        requiredFragments: runeTokens.length,
        missingFragments: 0
      };
    }
    const owned = runeInventoryOwnedByToken.get(token);
    coveredFragments += Math.min(required, owned);
  }
  const requiredFragments = runeTokens.length;
  const missingFragments = Math.max(0, requiredFragments - coveredFragments);
  const key = missingFragments === 0
    ? "ready"
    : missingFragments === 1
      ? "missing1"
      : "missing";

  return {
    key,
    known: true,
    coveredFragments,
    requiredFragments,
    missingFragments
  };
}

function resolveRecipeMaterialBadgeText(materialState) {
  switch (materialState.key) {
    case "ready":
      return t("Fragments ready", "룬 조각 준비");
    case "missing1":
      return t("Missing 1 fragment", "룬 조각 1개 부족");
    case "missing":
      return t(
        `Need ${materialState.missingFragments} fragments`,
        `룬 조각 ${materialState.missingFragments}개 필요`
      );
    default:
      return t("Fragments unknown", "룬 조각 확인 불가");
  }
}

function resolveRecipeMaterialCoverageText(materialState) {
  if (!materialState.known) {
    return t("Coverage unavailable", "보유량 확인 불가");
  }
  return t(
    `${materialState.coveredFragments}/${materialState.requiredFragments} fragments covered`,
    `룬 조각 ${materialState.coveredFragments}/${materialState.requiredFragments}개 보유`
  );
}

function resolveRecipeMaterialAccessibleText(materialState) {
  if (!materialState.known) {
    return t(
      "Fragment inventory is unavailable.",
      "룬 조각 보유량을 확인할 수 없습니다."
    );
  }
  return `${resolveRecipeMaterialBadgeText(materialState)}. ${t(
    `${materialState.coveredFragments} of ${materialState.requiredFragments} required fragments covered.`,
    `필요한 룬 조각 ${materialState.requiredFragments}개 중 ${materialState.coveredFragments}개를 보유했습니다.`
  )}`;
}

function updateRecipeMaterialView(item) {
  const token = resolveRecipeToken(item);
  const view = recipeMaterialViewByToken.get(token);
  if (!view) {
    return;
  }

  const materialState = resolveRecipeMaterialState(item);
  view.container.dataset.materialState = materialState.key;
  view.badge.className = `rwRecipeMaterialBadge ${materialState.key}`;
  view.badge.textContent = resolveRecipeMaterialBadgeText(materialState);
  view.coverage.textContent = resolveRecipeMaterialCoverageText(materialState);

  const accessibleText = resolveRecipeMaterialAccessibleText(materialState);
  view.button.setAttribute(
    "aria-label",
    view.staticAriaLabel ? `${view.staticAriaLabel}. ${accessibleText}` : accessibleText
  );
  view.button.title = view.staticTitle
    ? `${view.staticTitle}\n${accessibleText}`
    : accessibleText;
}

function refreshRecipeMaterialViews() {
  for (const item of recipeItemsState) {
    updateRecipeMaterialView(item);
  }
}

function setRecipeBaseFilter(nextFilter) {
  const normalized = validRecipeBaseFilters.has(nextFilter) ? nextFilter : "all";
  if (normalized === recipeBaseFilter) {
    return false;
  }
  recipeBaseFilter = normalized;
  updateRecipeFilterControls();
  schedulePanelRender(panelRenderSection.recipeItems);
  return true;
}

function setRecipeMaterialFilter(nextFilter) {
  const requested = validRecipeMaterialFilters.has(nextFilter) ? nextFilter : "all";
  const normalized = runeInventoryKnownState ? requested : "all";
  if (normalized === recipeMaterialFilter) {
    return false;
  }
  recipeMaterialFilter = normalized;
  updateRecipeFilterControls();
  schedulePanelRender(panelRenderSection.recipeItems);
  return true;
}

function handleRecipeFilterClick(event) {
  const target = event.target;
  if (!(target instanceof Element)) {
    return;
  }
  const button = target.closest(`[${recipeFilterAttribute}]`);
  if (!button || !recipeBaseFilters?.contains(button)) {
    return;
  }
  const nextFilter = button.getAttribute(recipeFilterAttribute) || "all";
  if (setRecipeBaseFilter(nextFilter)) {
    event.preventDefault();
  }
}

function handleRecipeMaterialFilterClick(event) {
  const target = event.target;
  if (!(target instanceof Element)) {
    return;
  }
  const button = target.closest(`[${recipeMaterialFilterAttribute}]`);
  if (!button || button.disabled || !recipeMaterialFilters?.contains(button)) {
    return;
  }
  const nextFilter = button.getAttribute(recipeMaterialFilterAttribute) || "all";
  if (setRecipeMaterialFilter(nextFilter)) {
    event.preventDefault();
  }
}

function resolveRecipeListViewModel() {
  const allItems = Array.isArray(recipeItemsState) ? recipeItemsState : [];
  const query = recipeSearchQuery.trim().toLowerCase();
  const activeFilter = validRecipeBaseFilters.has(recipeBaseFilter)
    ? recipeBaseFilter
    : "all";
  const activeMaterialFilter = runeInventoryKnownState &&
    validRecipeMaterialFilters.has(recipeMaterialFilter)
      ? recipeMaterialFilter
      : "all";
  const visibleItems = allItems.filter((item) => {
    const matchesFilter = activeFilter === "all" ||
      resolveRecipeBaseBadge(item).className === activeFilter;
    if (!matchesFilter) return false;
    const materialState = resolveRecipeMaterialState(item);
    const matchesMaterial = activeMaterialFilter === "all" ||
      materialState.key === activeMaterialFilter;
    if (!matchesMaterial) return false;
    return !query || resolveRecipeSearchDocument(item).includes(query);
  });
  const hasActiveConstraint = Boolean(query) ||
    activeFilter !== "all" ||
    activeMaterialFilter !== "all";

  return {
    allItems,
    visibleItems,
    activeFilter,
    activeMaterialFilter,
    titleText: t(
      `Recipe List (${allItems.length})`,
      `레시피 목록 (${allItems.length})`
    ),
    countText: hasActiveConstraint
      ? t(`Showing ${visibleItems.length}/${allItems.length}`, `표시 ${visibleItems.length}/${allItems.length}`)
      : t(`Total ${allItems.length}`, `총 ${allItems.length}`),
    emptyState: allItems.length === 0
      ? {
          title: t("No runeword recipe available", "사용 가능한 룬워드 레시피가 없습니다"),
          body: t(
            "Recipe data has not reached this panel yet, so there is nothing to choose from.",
            "이 패널에 아직 레시피 데이터가 도착하지 않아 선택할 항목이 없습니다."
          ),
          hint: t(
            "Reopen the panel or reload the mod data if this keeps happening.",
            "계속 비어 있으면 패널을 다시 열거나 모드 데이터를 다시 불러오세요."
          )
        }
        : visibleItems.length === 0
        ? {
            title: t("No recipe matched the current filters", "현재 조건과 일치하는 레시피가 없습니다"),
            body: t(
              "Try another base or fragment filter, rune name, or effect keyword.",
              "다른 베이스 또는 조각 필터, 룬 이름, 효과 키워드로 다시 찾아보세요."
            ),
            hint: t(
              "Filters and search are combined; the selected recipe remains active if hidden.",
              "필터와 검색은 함께 적용되며, 숨겨져도 선택한 레시피는 유지됩니다."
            )
          }
        : null
  };
}

function createRecipeButton(item) {
  const token = resolveRecipeToken(item);
  const name = typeof item?.name === "string" ? item.name : "";
  if (!token || !name) {
    return null;
  }

  const runes = typeof item?.runes === "string" ? item.runes : "";
  const baseBadge = resolveRecipeBaseBadge(item);
  const summaryText = resolveRecipeNumericSummaryText(item);
  const tooltipLikeText = buildRunewordTooltipLikeText(item, {
    includeName: true,
    includeDetail: true
  });
  const selected = token === getDisplayedRecipeToken();

  const button = document.createElement("button");
  button.type = "button";
  button.className = selected ? "cpListItem selected" : "cpListItem";
  button.dataset.recipeToken = token;
  button.setAttribute("role", "option");
  button.setAttribute("aria-selected", selected ? "true" : "false");
  button.tabIndex = selected ? 0 : -1;

  const head = document.createElement("div");
  head.className = "rwRecipeHead";
  const title = document.createElement("div");
  title.className = "rwRecipeName";
  title.textContent = runes ? name + " [" + runes + "]" : name;
  head.appendChild(title);

  const badge = document.createElement("div");
  badge.className = "rwRecipeBaseBadge " + baseBadge.className;
  badge.textContent = baseBadge.text;
  head.appendChild(badge);
  button.appendChild(head);

  if (summaryText) {
    const summary = document.createElement("div");
    summary.className = "rwRecipeSummary";
    summary.textContent = summaryText;
    button.appendChild(summary);
  }

  const hoverText = tooltipLikeText.trim();
  const staticAriaLabel = hoverText
    ? hoverText.replace(/\n/g, ". ")
    : name;
  if (hoverText) {
    button.title = hoverText;
    button.setAttribute("aria-label", staticAriaLabel);
  } else {
    button.setAttribute("aria-label", staticAriaLabel);
  }

  const material = document.createElement("div");
  material.className = "rwRecipeMaterial";
  const materialBadge = document.createElement("span");
  materialBadge.className = "rwRecipeMaterialBadge unknown";
  const materialCoverage = document.createElement("span");
  materialCoverage.className = "rwRecipeMaterialCoverage";
  material.appendChild(materialBadge);
  material.appendChild(materialCoverage);
  button.appendChild(material);

  recipeMaterialViewByToken.set(token, {
    button,
    container: material,
    badge: materialBadge,
    coverage: materialCoverage,
    staticAriaLabel,
    staticTitle: hoverText
  });
  updateRecipeMaterialView(item);

  button.setAttribute(
    panelCommandAttribute,
    recipeSelectionCommandPrefix + token
  );
  return button;
}

function rebuildRecipeCatalogDom() {
  recipeNodeByToken.clear();
  recipeMaterialViewByToken.clear();
  for (const item of recipeItemsState) {
    const token = resolveRecipeToken(item);
    const button = createRecipeButton(item);
    if (token && button) {
      recipeNodeByToken.set(token, button);
    }
  }
  recipeCatalogDomDirty = false;
}

function renderRecipeItems() {
  const viewModel = resolveRecipeListViewModel();

  if (runewordRecipeListTitle) {
    runewordRecipeListTitle.textContent = viewModel.titleText;
  }
  if (recipeCountLabel) {
    recipeCountLabel.textContent = viewModel.countText;
  }

  if (recipeCatalogDomDirty) {
    rebuildRecipeCatalogDom();
  }
  refreshRecipeMaterialViews();

  if (viewModel.emptyState) {
    appendEmptyState(
      recipeList,
      viewModel.emptyState.title,
      viewModel.emptyState.body,
      viewModel.emptyState.hint
    );
    return;
  }

  const selectedToken = getDisplayedRecipeToken();
  const focusedRecipeToken = recipeList.contains(document.activeElement)
    ? document.activeElement?.dataset?.recipeToken || ""
    : "";
  const visibleTokens = new Set(
    viewModel.visibleItems.map((item) => resolveRecipeToken(item))
  );
  const preferredTabToken = visibleTokens.has(focusedRecipeToken)
    ? focusedRecipeToken
    : visibleTokens.has(selectedToken)
      ? selectedToken
      : resolveRecipeToken(viewModel.visibleItems[0]);
  const fragment = document.createDocumentFragment();
  for (const item of viewModel.visibleItems) {
    const token = resolveRecipeToken(item);
    const button = recipeNodeByToken.get(token);
    if (!button) continue;

    const selected = token === selectedToken;
    button.classList.toggle("selected", selected);
    button.setAttribute("aria-selected", selected ? "true" : "false");
    button.tabIndex = token === preferredTabToken ? 0 : -1;
    fragment.appendChild(button);
  }
  recipeList.replaceChildren(fragment);
}
