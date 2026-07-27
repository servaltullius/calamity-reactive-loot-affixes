      function wireButtons() {
        document.addEventListener("click", handleDelegatedPanelCommandClick);

        if (recipeBaseFilters) {
          recipeBaseFilters.addEventListener("click", handleRecipeFilterClick);
        }

        if (recipeSearchInput) {
          recipeSearchInput.addEventListener("input", () => {
            recipeSearchQuery = recipeSearchInput.value || "";
            schedulePanelRender(panelRenderSection.recipeItems);
          });
        }
      }

      function registerPrismaInteropHandlers() {
        if (typeof PrismaUI_Interop !== "function") {
          return;
        }

        PrismaUI_Interop(prismaInteropMethod.tooltip, (data) => setTooltip(data));
        PrismaUI_Interop(prismaInteropMethod.controlPanel, (data) => setControlPanel(data));
        PrismaUI_Interop(prismaInteropMethod.actionFeedback, (data) => setActionFeedback(data));
        PrismaUI_Interop(prismaInteropMethod.panelHotkeyText, (data) => setPanelHotkeyText(data));
        PrismaUI_Interop(prismaInteropMethod.uiLanguage, (data) => setUiLanguage(data));
        PrismaUI_Interop(prismaInteropMethod.selectedItemName, (data) => setSelectedItemName(data));
        PrismaUI_Interop(prismaInteropMethod.selectedItemSource, (data) => setSelectedItemSource(data));
        PrismaUI_Interop(prismaInteropMethod.inventoryItems, (data) => setInventoryItems(data));
        PrismaUI_Interop(prismaInteropMethod.recipeItems, (data) => setRecipeItems(data));
        PrismaUI_Interop(prismaInteropMethod.runewordPanelState, (data) => setRunewordPanelState(data));
        PrismaUI_Interop(prismaInteropMethod.runewordAffixPreview, (data) => setRunewordAffixPreview(data));
        PrismaUI_Interop(prismaInteropMethod.panelLayout, (data) => setPanelLayout(data));
        PrismaUI_Interop(prismaInteropMethod.tooltipLayout, (data) => setTooltipLayout(data));
      }

      function registerWindowInteropFallbacks() {
        // Browser fallback and debugging hooks.
        window[prismaInteropMethod.tooltip] = setTooltip;
        window[prismaInteropMethod.controlPanel] = setControlPanel;
        window[prismaInteropMethod.actionFeedback] = setActionFeedback;
        window[prismaInteropMethod.panelHotkeyText] = setPanelHotkeyText;
        window[prismaInteropMethod.uiLanguage] = setUiLanguage;
        window[prismaInteropMethod.selectedItemName] = setSelectedItemName;
        window[prismaInteropMethod.selectedItemSource] = setSelectedItemSource;
        window[prismaInteropMethod.inventoryItems] = setInventoryItems;
        window[prismaInteropMethod.recipeItems] = setRecipeItems;
        window[prismaInteropMethod.runewordPanelState] = setRunewordPanelState;
        window[prismaInteropMethod.runewordAffixPreview] = setRunewordAffixPreview;
        window[prismaInteropMethod.panelLayout] = setPanelLayout;
        window[prismaInteropMethod.tooltipLayout] = setTooltipLayout;
      }

      function registerPanelEventHandlers() {
        wireButtons();
        wireMainTabs();
        controlPanel.addEventListener("keydown", handleListboxKeydown);
        tooltipTitle.addEventListener("pointerdown", beginTooltipDrag);
        tooltipTitle.addEventListener("mousedown", beginTooltipDrag);
        tooltipTitle.addEventListener("dragstart", (event) => event.preventDefault());
        panelDragHandle.addEventListener("pointerdown", beginPanelDrag);
        panelResizeRight.addEventListener("pointerdown", (event) => beginPanelResize("x", event));
        panelResizeBottom.addEventListener("pointerdown", (event) => beginPanelResize("y", event));
        panelResizeCorner.addEventListener("pointerdown", (event) => beginPanelResize("xy", event));
        window.addEventListener("keydown", handlePanelKeydown);
        window.addEventListener("pointermove", movePanel);
        window.addEventListener("pointerup", endPanelDrag);
        window.addEventListener("pointercancel", endPanelDrag);
        window.addEventListener("mousemove", movePanel);
        window.addEventListener("mouseup", endPanelDrag);
        if (typeof ResizeObserver === "function") {
          const ro = new ResizeObserver(() => {
            updatePanelUiScale();
          });
          ro.observe(controlPanel);
        }
        window.addEventListener("resize", () => {
          requestAnimationFrame(() => {
            applyAutoScale();
            if (!tooltipLayoutLoaded) {
              tooltipLayout = getDefaultTooltipLayout();
            }
            applyTooltipLayout();
            updatePanelUiScale();
            keepPanelInViewport();
          });
        });
      }

      function initializeCalamityPanel() {
        registerPrismaInteropHandlers();
        registerWindowInteropFallbacks();
        initUiText();
        registerPanelEventHandlers();
        applyAutoScale();
      }

      initializeCalamityPanel();
