function resolveRunewordInspectorTexts(recipePreviewText, baseAffixText, pending) {
  // Base affixes and the recipe preview are separate boxes: merging them into
  // one scroll area forced players to scroll past the recipe to read their own
  // item, so each box resolves its own text and fallback.
  return {
    baseText: baseAffixText
      ? baseAffixText
      : pending
        ? t("Refreshing affix preview...", "어픽스 미리보기를 갱신 중입니다.")
        : t(
            "Select an equipped base to see its current affixes.",
            "착용 베이스를 선택하면 현재 어픽스가 표시됩니다."
          ),
    recipeText: recipePreviewText
      ? recipePreviewText
      : t(
          "Select a recipe to preview its effect.",
          "레시피를 선택하면 효과 미리보기가 표시됩니다."
        )
  };
}

function applyTooltipPlacement() {
  const hasTooltip = Boolean(tooltipTextState);
  const hasRunewordAffix = Boolean(runewordAffixTextState);
  const recipePreviewText = buildRecipePreviewTooltipText(
    getSelectedRecipeItem()
  );

  if (panelTooltipText) {
    if (hasTooltip) {
      panelTooltipText.textContent = tooltipTextState;
    } else if (hasRunewordAffix) {
      // No inventory hover, but a runeword base is selected: show that item's
      // affix text instead of a dead-end empty state.
      panelTooltipText.textContent =
        t("[Selected base]", "[선택된 베이스 기준]") + "\n" + runewordAffixTextState;
    } else {
      appendEmptyState(
        panelTooltipText,
        t("No item focused yet", "확인할 아이템이 아직 없습니다"),
        t(
          "Open your inventory (Tab) while this panel is open, then hover an item — its affix text appears here.",
          "패널을 연 채로 인벤토리(Tab)를 열고 아이템에 마우스를 올리면 어픽스 텍스트가 여기 표시됩니다."
        ),
        t(
          "Selecting a runeword base in the Runeword tab also shows that item's affixes here.",
          "룬워드 탭에서 베이스를 선택해도 그 아이템의 어픽스가 여기 표시됩니다."
        )
      );
    }
  }

  if (panelTooltipHint) {
    panelTooltipHint.style.display = hasTooltip || hasRunewordAffix ? "none" : "block";
  }

  const inspectorTexts = resolveRunewordInspectorTexts(
    recipePreviewText,
    hasRunewordAffix ? runewordAffixTextState : "",
    runewordAffixPendingState
  );
  if (runewordBaseAffixText) {
    runewordBaseAffixText.textContent = inspectorTexts.baseText;
  }
  if (runewordAffixText) {
    runewordAffixText.textContent = inspectorTexts.recipeText;
  }

  if (!hasTooltip) {
    tooltipPanel.style.display = "none";
    tooltipText.textContent = "";
    applyQuickLaunchVisibility();
    return;
  }

  if (controlPanelOpen) {
    tooltipPanel.style.display = "none";
    tooltipText.textContent = "";
    applyQuickLaunchVisibility();
    return;
  }

  tooltipPanel.style.display = "block";
  tooltipText.textContent = tooltipTextState;
  applyTooltipLayout();
  applyQuickLaunchVisibility();
}

function setMainTab(nextTab) {
  const previousTab = mainTabState;
  const tab =
    nextTab === "runeword"
      ? "runeword"
      : nextTab === "advanced"
        ? "advanced"
        : "affix";
  if (previousTab !== tab) {
    closeWorkingBaseChooser(false);
  }
  mainTabState = tab;

  if (previousTab === "advanced" && tab !== "advanced" && runewordResetArmedUntil !== 0) {
    clearRunewordResetConfirmation();
  }

  const isRuneword = tab === "runeword";
  const isAffix = tab === "affix";
  const isAdvanced = tab === "advanced";

  if (mainRunewordTab && mainRunewordPane) {
    mainRunewordTab.setAttribute("aria-selected", isRuneword ? "true" : "false");
    mainRunewordTab.tabIndex = isRuneword ? 0 : -1;
    mainRunewordPane.hidden = !isRuneword;
  }

  if (mainAffixTab && mainAffixPane) {
    mainAffixTab.setAttribute("aria-selected", isAffix ? "true" : "false");
    mainAffixTab.tabIndex = isAffix ? 0 : -1;
    mainAffixPane.hidden = !isAffix;
  }

  if (mainAdvancedTab && mainAdvancedPane) {
    mainAdvancedTab.setAttribute("aria-selected", isAdvanced ? "true" : "false");
    mainAdvancedTab.tabIndex = isAdvanced ? 0 : -1;
    mainAdvancedPane.hidden = !isAdvanced;
  }
}

function wireMainTabs() {
  const tabs = [
    { id: "affix", button: mainAffixTab },
    { id: "runeword", button: mainRunewordTab },
    { id: "advanced", button: mainAdvancedTab }
  ];

  for (const item of tabs) {
    if (!item.button) continue;
    item.button.addEventListener("click", () => setMainTab(item.id));
  }

  const onTabKeydown = (event) => {
    if (event.key === "ArrowLeft" || event.key === "ArrowRight") {
      event.preventDefault();
      const order = ["affix", "runeword", "advanced"];
      const currentIndex = Math.max(0, order.indexOf(mainTabState));
      const delta = event.key === "ArrowRight" ? 1 : -1;
      const nextIndex = (currentIndex + delta + order.length) % order.length;
      const next = order[nextIndex];
      setMainTab(next);

      const nextButton =
        next === "runeword"
          ? mainRunewordTab
          : next === "affix"
            ? mainAffixTab
            : mainAdvancedTab;
      if (nextButton && typeof nextButton.focus === "function") {
        nextButton.focus();
      }
      return;
    }

    if (event.key === "Home") {
      event.preventDefault();
      setMainTab("affix");
      if (mainAffixTab && typeof mainAffixTab.focus === "function") {
        mainAffixTab.focus();
      }
      return;
    }

    if (event.key === "End") {
      event.preventDefault();
      setMainTab("advanced");
      if (mainAdvancedTab && typeof mainAdvancedTab.focus === "function") {
        mainAdvancedTab.focus();
      }
    }
  };

  if (mainRunewordTab) {
    mainRunewordTab.addEventListener("keydown", onTabKeydown);
  }
  if (mainAffixTab) {
    mainAffixTab.addEventListener("keydown", onTabKeydown);
  }
  if (mainAdvancedTab) {
    mainAdvancedTab.addEventListener("keydown", onTabKeydown);
  }
}

function panelSizeBounds() {
  const margin = 8;
  const minWidth = window.innerWidth <= 900 ? 360 : 760;
  const minHeight = window.innerWidth <= 900 ? 260 : 500;
  const maxWidth = Math.max(minWidth, window.innerWidth - margin * 2);
  const maxHeight = Math.max(minHeight, window.innerHeight - margin * 2);
  return { minWidth, minHeight, maxWidth, maxHeight, margin };
}

function resolvePanelLayoutMode(width) {
  if (width >= 1100) return "wide";
  if (width >= 900) return "medium";
  return "narrow";
}

function applyPanelLayoutMode(width) {
  const nextMode = resolvePanelLayoutMode(width);
  if (controlPanel.dataset.layout === nextMode) {
    return false;
  }
  controlPanel.dataset.layout = nextMode;
  return true;
}

function updatePanelUiScale() {
  const rect = controlPanel.getBoundingClientRect();
  if (!rect.width || !rect.height) {
    document.documentElement.style.setProperty("--panel-ui-scale", "1");
    return;
  }

  applyPanelLayoutMode(rect.width);
  const widthRatio = rect.width / 1320;
  const heightRatio = rect.height / 860;
  const nextScale = clamp(Math.min(widthRatio, heightRatio) * 1.25, 0.9, 1.75);
  document.documentElement.style.setProperty("--panel-ui-scale", nextScale.toFixed(3));
  if (typeof fitWorkingBaseChooserToPanel === "function") {
    fitWorkingBaseChooserToPanel();
  }
}

function setPanelAnchoredPosition(left, top) {
  controlPanel.style.left = `${Math.round(left)}px`;
  controlPanel.style.top = `${Math.round(top)}px`;
  controlPanel.style.right = "auto";
  controlPanel.style.bottom = "auto";
}

function clearPanelDragVisualState() {
  controlPanel.style.transform = "";
  controlPanel.style.willChange = "";
}

function schedulePanelDragFrame() {
  if (!panelDragState || panelDragState.framePending) {
    return;
  }

  panelDragState.framePending = true;
  requestAnimationFrame(() => {
    if (!panelDragState) {
      clearPanelDragVisualState();
      return;
    }

    panelDragState.framePending = false;
    const deltaX = Math.round(panelDragState.nextLeft) - panelDragState.baseLeft;
    const deltaY = Math.round(panelDragState.nextTop) - panelDragState.baseTop;
    controlPanel.style.transform = `translate3d(${deltaX}px, ${deltaY}px, 0)`;
  });
}

function applyPendingPanelResize() {
  if (!panelResizeState) return;
  panelResizeState.framePending = false;
  controlPanel.style.width = Math.round(panelResizeState.nextWidth) + "px";
  controlPanel.style.height = Math.round(panelResizeState.nextHeight) + "px";
  updatePanelUiScale();
}

function schedulePanelResizeFrame() {
  if (!panelResizeState || panelResizeState.framePending) {
    return;
  }
  panelResizeState.framePending = true;
  requestAnimationFrame(applyPendingPanelResize);
}

function keepPanelInViewport() {
  if (!controlPanelOpen) return;
  if (!controlPanel.style.left || !controlPanel.style.top) return;

  const rect = controlPanel.getBoundingClientRect();
  const { minWidth, minHeight, maxWidth, maxHeight, margin } = panelSizeBounds();

  const safeWidth = clamp(rect.width, minWidth, maxWidth);
  const safeHeight = clamp(rect.height, minHeight, maxHeight);
  if (Math.abs(safeWidth - rect.width) > 0.5) {
    controlPanel.style.width = `${Math.round(safeWidth)}px`;
  }
  if (Math.abs(safeHeight - rect.height) > 0.5) {
    controlPanel.style.height = `${Math.round(safeHeight)}px`;
  }

  const maxLeft = Math.max(margin, window.innerWidth - safeWidth - margin);
  const maxTop = Math.max(margin, window.innerHeight - safeHeight - margin);
  const nextLeft = clamp(rect.left, margin, maxLeft);
  const nextTop = clamp(rect.top, margin, maxTop);

  setPanelAnchoredPosition(nextLeft, nextTop);
  updatePanelUiScale();
}

function persistPanelLayout() {
  if (!controlPanel.style.left || !controlPanel.style.top) {
    return;
  }

  const rect = controlPanel.getBoundingClientRect();
  const left = Math.round(rect.left);
  const top = Math.round(rect.top);
  const width = Math.round(rect.width);
  const height = Math.round(rect.height);
  if (width <= 0 || height <= 0) {
    return;
  }

  sendCommand(`ui.layout.save:${left},${top},${width},${height}`);
}

function beginPanelDrag(event) {
  if (!controlPanelOpen) return;
  if (panelResizeState) return;
  if (event.button !== undefined && event.button !== 0) return;
  if (event.target && typeof event.target.closest === "function") {
    if (event.target.closest("[data-nodrag]")) {
      return;
    }
  }

  const rect = controlPanel.getBoundingClientRect();
  const baseLeft = Math.round(rect.left);
  const baseTop = Math.round(rect.top);
  setPanelAnchoredPosition(baseLeft, baseTop);
  controlPanel.style.willChange = "transform";
  controlPanel.style.transform = "translate3d(0, 0, 0)";

  panelDragState = {
    pointerId: event.pointerId,
    offsetX: event.clientX - rect.left,
    offsetY: event.clientY - rect.top,
    baseLeft,
    baseTop,
    width: rect.width,
    height: rect.height,
    nextLeft: baseLeft,
    nextTop: baseTop,
    framePending: false
  };

  if (typeof panelDragHandle.setPointerCapture === "function") {
    panelDragHandle.setPointerCapture(event.pointerId);
  }
  event.preventDefault();
}

function beginPanelResize(mode, event) {
  if (!controlPanelOpen) return;
  if (panelDragState) return;
  if (event.button !== undefined && event.button !== 0) return;

  const rect = controlPanel.getBoundingClientRect();
  setPanelAnchoredPosition(rect.left, rect.top);
  controlPanel.style.width = `${Math.round(rect.width)}px`;
  controlPanel.style.height = `${Math.round(rect.height)}px`;

  panelResizeState = {
    mode,
    pointerId: event.pointerId,
    startX: event.clientX,
    startY: event.clientY,
    startLeft: rect.left,
    startTop: rect.top,
    startWidth: rect.width,
    startHeight: rect.height,
    nextWidth: rect.width,
    nextHeight: rect.height,
    framePending: false
  };

  const target = event.currentTarget;
  if (target && typeof target.setPointerCapture === "function") {
    target.setPointerCapture(event.pointerId);
  }
  event.preventDefault();
  event.stopPropagation();
}

function beginTooltipDrag(event) {
  if (controlPanelOpen) return;
  if (!tooltipTextState) return;
  if (tooltipDragState) return;
  if (panelDragState || panelResizeState) return;
  if (event.button !== undefined && event.button !== 0) return;

  tooltipLayout = normalizeTooltipLayout(tooltipLayout);
  tooltipDragState = {
    pointerId: typeof event.pointerId === "number" ? event.pointerId : null,
    startX: event.clientX,
    startY: event.clientY,
    startRight: tooltipLayout.right,
    startTop: tooltipLayout.top
  };

  tooltipTitle.classList.add("dragging");
  if (
    tooltipDragState.pointerId !== null &&
    typeof tooltipTitle.setPointerCapture === "function"
  ) {
    try {
      tooltipTitle.setPointerCapture(tooltipDragState.pointerId);
    } catch (_) {}
  }

  setActionFeedback(t("Drag to move tooltip.", "드래그해서 툴팁 위치를 이동하세요."));
  event.preventDefault();
  event.stopPropagation();
}

function movePanel(event) {
  if (tooltipDragState) {
    if (tooltipDragState.pointerId !== null) {
      if (event.pointerId !== tooltipDragState.pointerId) return;
    } else if (event.type.startsWith("pointer")) {
      return;
    }

    const deltaX = event.clientX - tooltipDragState.startX;
    const deltaY = event.clientY - tooltipDragState.startY;
    tooltipLayout = {
      ...tooltipLayout,
      right: tooltipDragState.startRight - deltaX,
      top: tooltipDragState.startTop + deltaY
    };
    applyTooltipLayout();
    event.preventDefault();
    return;
  }

  if (panelResizeState) {
    if (event.pointerId !== panelResizeState.pointerId) return;

    const { minWidth, minHeight, maxWidth, maxHeight } = panelSizeBounds();
    const deltaX = event.clientX - panelResizeState.startX;
    const deltaY = event.clientY - panelResizeState.startY;

    let nextWidth = panelResizeState.startWidth;
    let nextHeight = panelResizeState.startHeight;

    if (panelResizeState.mode === "x" || panelResizeState.mode === "xy") {
      nextWidth = panelResizeState.startWidth + deltaX;
    }
    if (panelResizeState.mode === "y" || panelResizeState.mode === "xy") {
      nextHeight = panelResizeState.startHeight + deltaY;
    }

    const limitWidth = Math.min(maxWidth, window.innerWidth - panelResizeState.startLeft - 8);
    const limitHeight = Math.min(maxHeight, window.innerHeight - panelResizeState.startTop - 8);
    nextWidth = clamp(nextWidth, minWidth, Math.max(minWidth, limitWidth));
    nextHeight = clamp(nextHeight, minHeight, Math.max(minHeight, limitHeight));

    panelResizeState.nextWidth = nextWidth;
    panelResizeState.nextHeight = nextHeight;
    schedulePanelResizeFrame();
    event.preventDefault();
    return;
  }

  if (!panelDragState) return;
  if (event.pointerId !== panelDragState.pointerId) return;

  const margin = 8;
  const maxLeft = Math.max(margin, window.innerWidth - panelDragState.width - margin);
  const maxTop = Math.max(margin, window.innerHeight - panelDragState.height - margin);

  panelDragState.nextLeft = clamp(event.clientX - panelDragState.offsetX, margin, maxLeft);
  panelDragState.nextTop = clamp(event.clientY - panelDragState.offsetY, margin, maxTop);
  schedulePanelDragFrame();
  event.preventDefault();
}

function endPanelDrag(event) {
  if (tooltipDragState) {
    if (tooltipDragState.pointerId !== null) {
      if (event && event.pointerId !== tooltipDragState.pointerId) return;
    } else if (event && event.type.startsWith("pointer")) {
      return;
    }

    const wasCancel = Boolean(event && event.type === "pointercancel");
    const dragStart = {
      right: tooltipDragState.startRight,
      top: tooltipDragState.startTop
    };
    const dragPointerId = tooltipDragState.pointerId;
    if (
      dragPointerId !== null &&
      typeof tooltipTitle.releasePointerCapture === "function"
    ) {
      try {
        tooltipTitle.releasePointerCapture(dragPointerId);
      } catch (_) {}
    }

    tooltipDragState = null;
    tooltipTitle.classList.remove("dragging");
    applyTooltipLayout();

    const moved =
      Math.abs(tooltipLayout.right - dragStart.right) >= 1 ||
      Math.abs(tooltipLayout.top - dragStart.top) >= 1;

    if (wasCancel) {
      setActionFeedback(t("Tooltip move canceled.", "툴팁 이동이 취소되었습니다."));
      return;
    }

    if (!moved) {
      return;
    }

    persistTooltipLayout();
    setActionFeedback(
      t(
        `Tooltip moved (right ${tooltipLayout.right}px, top ${tooltipLayout.top}px).`,
        `툴팁 이동됨 (우측 ${tooltipLayout.right}px, 상단 ${tooltipLayout.top}px).`
      )
    );
    return;
  }

  if (panelResizeState) {
    if (!event || event.pointerId === panelResizeState.pointerId) {
      applyPendingPanelResize();
      const id = panelResizeState.pointerId;
      const targets = [panelResizeRight, panelResizeBottom, panelResizeCorner];
      for (const target of targets) {
        if (target && typeof target.releasePointerCapture === "function") {
          try {
            target.releasePointerCapture(id);
          } catch (_) {}
        }
      }
      panelResizeState = null;
      keepPanelInViewport();
      persistPanelLayout();
    }
    return;
  }

  if (!panelDragState) return;
  if (event && event.pointerId !== panelDragState.pointerId) return;

  if (typeof panelDragHandle.releasePointerCapture === "function") {
    try {
      panelDragHandle.releasePointerCapture(panelDragState.pointerId);
    } catch (_) {}
  }
  setPanelAnchoredPosition(panelDragState.nextLeft, panelDragState.nextTop);
  clearPanelDragVisualState();
  panelDragState = null;
  keepPanelInViewport();
  persistPanelLayout();
}
