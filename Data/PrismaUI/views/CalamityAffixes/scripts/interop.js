function parseInteropArrayPayload(raw) {
  if (Array.isArray(raw)) {
    return raw;
  }
  if (typeof raw !== "string" || raw.trim().length === 0) {
    return [];
  }

  try {
    const parsed = JSON.parse(raw);
    return Array.isArray(parsed) ? parsed : [];
  } catch (_) {
    return [];
  }
}

function parseInteropObjectPayload(raw) {
  if (raw && typeof raw === "object" && !Array.isArray(raw)) {
    return raw;
  }
  if (typeof raw !== "string" || !raw.trim()) {
    return null;
  }

  try {
    const parsed = JSON.parse(raw);
    return parsed && typeof parsed === "object" && !Array.isArray(parsed)
      ? parsed
      : null;
  } catch (_) {
    return null;
  }
}

function setInventoryItems(raw) {
  const prevSelectedItemName = selectedItemNameState;
  const prevSelectedBaseKey = resolveSelectedRunewordBaseKey();
  const items = parseInteropArrayPayload(raw);

  inventoryItemsState = items;
  const nextSelectedBaseKey = resolveSelectedRunewordBaseKey();
  if (runewordResetArmedUntil !== 0 && nextSelectedBaseKey !== prevSelectedBaseKey) {
    clearRunewordResetConfirmation();
  }
  const selected = items.find((item) => item && item.selected && typeof item.name === "string");
  if (selected && typeof selected.name === "string") {
    selectedItemNameState = selected.name;
    selectedItemSourceState = "equipped";
  } else if (!selectedItemNameState) {
    selectedItemSourceState = "equipped";
  }

  if (selectedItemNameState !== prevSelectedItemName) {
    invalidateRunewordAffixPreview(Boolean(selectedItemNameState));
  }

  schedulePanelRender(
    panelRenderSection.selectedItemContext,
    panelRenderSection.inventoryItems,
    panelRenderSection.runewordPanelState,
    panelRenderSection.tooltipPlacement
  );
}

function setRecipeItems(raw) {
  const nextItems = parseInteropArrayPayload(raw);
  const nextCatalogSignature = buildRecipeCatalogSignature(nextItems);
  const catalogChanged = nextCatalogSignature !== recipeCatalogSignatureState;
  const nextConfirmedToken = resolveConfirmedRecipeToken(nextItems);

  recipeItemsState = nextItems;
  if (catalogChanged) {
    recipeCatalogSignatureState = nextCatalogSignature;
    invalidateRecipePresentationCaches();
  }
  applyConfirmedRecipeSelection(nextConfirmedToken);

  if (catalogChanged) {
    schedulePanelRender(panelRenderSection.recipeItems);
  }
  schedulePanelRender(
    panelRenderSection.runewordPanelState,
    panelRenderSection.tooltipPlacement
  );
}

function setRunewordPanelState(raw) {
  const data = parseInteropObjectPayload(raw) || {};

  runewordPanelState = {
    hasBase: Boolean(data.hasBase),
    hasRecipe: Boolean(data.hasRecipe),
    isComplete: Boolean(data.isComplete),
    recipeName: typeof data.recipeName === "string" ? data.recipeName : "",
    insertedRunes: Number.isFinite(Number(data.insertedRunes)) ? Number(data.insertedRunes) : 0,
    totalRunes: Number.isFinite(Number(data.totalRunes)) ? Number(data.totalRunes) : 0,
    nextRuneName: typeof data.nextRuneName === "string" ? data.nextRuneName : "",
    nextRuneOwned: Number.isFinite(Number(data.nextRuneOwned)) ? Number(data.nextRuneOwned) : 0,
    canInsert: Boolean(data.canInsert),
    missingSummary: typeof data.missingSummary === "string" ? data.missingSummary : "",
    baseCompatibilityWarning: Boolean(data.baseCompatibilityWarning),
    baseCompatibilityMessageEn: typeof data.baseCompatibilityMessageEn === "string"
      ? data.baseCompatibilityMessageEn
      : "",
    baseCompatibilityMessageKo: typeof data.baseCompatibilityMessageKo === "string"
      ? data.baseCompatibilityMessageKo
      : "",
    requiredRunes: Array.isArray(data.requiredRunes)
      ? data.requiredRunes
          .map((entry) => {
            const name = typeof entry?.name === "string" ? entry.name : "";
            const required = Number.isFinite(Number(entry?.required)) ? Number(entry.required) : 0;
            const owned = Number.isFinite(Number(entry?.owned)) ? Number(entry.owned) : 0;
            if (!name || required <= 0) return null;
            return { name, required, owned };
          })
          .filter(Boolean)
      : []
  };

  if (Object.prototype.hasOwnProperty.call(data, "recipeToken")) {
    const recipeToken = typeof data.recipeToken === "string"
      ? data.recipeToken.trim()
      : "";
    applyConfirmedRecipeSelection(recipeToken);
  }

  if (runewordAffixPendingState) {
    runewordAffixPendingState = false;
  }
  schedulePanelRender(
    panelRenderSection.runewordPanelState,
    panelRenderSection.tooltipPlacement
  );
}

function setRunewordAffixPreview(raw) {
  runewordAffixTextState = typeof raw === "string" ? raw : "";
  runewordAffixPendingState = false;
  schedulePanelRender(panelRenderSection.tooltipPlacement);
}

function setTooltip(raw) {
  const value = typeof raw === "string" ? raw : "";
  if (value !== tooltipTextState) {
    tooltipPanel.scrollTop = 0;
  }
  tooltipTextState = value;
  schedulePanelRender(panelRenderSection.tooltipPlacement);
}

function parseControlPanelOpenState(raw) {
  const value = String(raw ?? "").toLowerCase();
  return value === "1" || value === "true" || value === "open";
}

function applyControlPanelOpenState(nextOpen) {
  const expectedDisplay = nextOpen ? "flex" : "none";
  if (
    controlPanelOpen === nextOpen &&
    controlPanel.style.display === expectedDisplay
  ) {
    return;
  }

  controlPanelOpen = nextOpen;
  if (!controlPanelOpen) {
    feedback.textContent = "";
    endPanelDrag();
    persistPanelLayout();
    controlPanel.classList.remove("is-visible");
    controlPanel.style.display = "none";
  } else {
    controlPanel.classList.remove("is-visible");
    controlPanel.style.display = "flex";
    if (!panelLayoutLoaded && !controlPanel.style.left) {
      const rect = controlPanel.getBoundingClientRect();
      controlPanel.style.left = Math.round(rect.left) + "px";
      controlPanel.style.top = Math.round(rect.top) + "px";
      controlPanel.style.right = "auto";
      controlPanel.style.bottom = "auto";
    }
    updatePanelUiScale();
    keepPanelInViewport();
    requestAnimationFrame(() => {
      if (controlPanelOpen) {
        controlPanel.classList.add("is-visible");
      }
    });
  }

  document.body.style.pointerEvents = controlPanelOpen ? "auto" : "none";
  schedulePanelRender(
    panelRenderSection.tooltipPlacement,
    panelRenderSection.quickLaunch
  );

  if (controlPanelOpen) {
    if (pendingOpenMainTab) {
      const next = pendingOpenMainTab;
      pendingOpenMainTab = null;
      setMainTab(next);
      requestAnimationFrame(() => {
        try {
          focusCurrentMainTab();
        } catch (_) {}
      });
    } else {
      setMainTab(mainTabState);
      requestAnimationFrame(() => {
        try {
          focusCurrentMainTab();
        } catch (_) {}
      });
    }
  }
}

function setControlPanel(raw) {
  applyControlPanelOpenState(parseControlPanelOpenState(raw));
}

function setPanelLayout(raw) {
  const data = parseInteropObjectPayload(raw);
  if (!data) {
    return;
  }

  const left = Number(data.left);
  const top = Number(data.top);
  const width = Number(data.width);
  const height = Number(data.height);
  if (!Number.isFinite(left) || !Number.isFinite(top) || !Number.isFinite(width) || !Number.isFinite(height)) {
    return;
  }
  if (width <= 0 || height <= 0) {
    return;
  }

  setPanelAnchoredPosition(left, top);
  controlPanel.style.width = `${Math.round(width)}px`;
  controlPanel.style.height = `${Math.round(height)}px`;
  panelLayoutLoaded = true;
  updatePanelUiScale();
  if (controlPanelOpen) {
    keepPanelInViewport();
  }
}

function setTooltipLayout(raw) {
  const data = parseInteropObjectPayload(raw);
  if (!data) {
    return;
  }

  tooltipLayout = normalizeTooltipLayout(data);
  tooltipLayoutLoaded = true;
  schedulePanelRender(panelRenderSection.tooltipLayout);
}

function setActionFeedback(raw) {
  const value = typeof raw === "string" ? raw : "";
  feedback.textContent = value;
  if (runewordAffixPendingState) {
    runewordAffixPendingState = false;
    schedulePanelRender(panelRenderSection.tooltipPlacement);
  }
}

function setPanelHotkeyText(raw) {
  const value = typeof raw === "string" ? raw.trim() : "";
  if (value && value !== panelHotkeyTextState) {
    panelHotkeyTextState = value;
    schedulePanelRender(panelRenderSection.hotkeyHints);
  }
}

function setUiLanguage(raw) {
  const next = normalizeUiLang(raw);
  if (next === uiLang) {
    return;
  }
  uiLang = next;
  invalidateRecipePresentationCaches();
  initUiText();
}

function setSelectedItemName(raw) {
  const prev = selectedItemNameState;
  const value = typeof raw === "string" ? raw.trim() : "";
  selectedItemNameState = value;
  if (value !== prev) {
    invalidateRunewordAffixPreview(Boolean(value));
  }
  schedulePanelRender(
    panelRenderSection.selectedItemContext,
    panelRenderSection.runewordPanelState,
    panelRenderSection.tooltipPlacement
  );
}

function setSelectedItemSource(raw) {
  const value = typeof raw === "string" ? raw.trim().toLowerCase() : "";
  selectedItemSourceState = value;
  schedulePanelRender(panelRenderSection.selectedItemContext);
}
