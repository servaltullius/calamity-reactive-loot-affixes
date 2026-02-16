		void Tick()
		{
			RefreshControlPanelHotkeyFromMcm(false);
			RefreshUiLanguageFromMcm(false);
			EnsurePrisma();

			if (!g_api || !g_view || !g_api->IsValid(g_view) || !g_domReady.load()) {
				return;
			}

			PushPanelHotkeyToJs();
			PushUiLanguageToJs();

			if (g_panelLayoutNeedsPush) {
				PushPanelLayoutToJs();
				g_panelLayoutNeedsPush = false;
			}
			if (g_tooltipLayoutNeedsPush) {
				PushTooltipLayoutToJs();
				g_tooltipLayoutNeedsPush = false;
			}

			if (!IsGameplayReady()) {
				g_lastPanelStateKnown = false;
				if (!g_lastTooltip.empty()) {
					g_lastTooltip.clear();
					g_api->InteropCall(g_view, "setTooltip", "");
				}
				SetSelectedItemContext({}, {});
				SetRunewordBaseInventoryList({});
				SetRunewordRecipeList({});
				SetRunewordPanelState({});
				SetControlPanelJsState(false);
				if (g_api->HasFocus(g_view)) {
					g_api->Unfocus(g_view);
				}
				SetVisible(false);
				return;
			}

			const bool panelRequested = g_controlPanelOpen.load();
			if (!g_lastPanelStateKnown || g_lastPanelState != panelRequested) {
				ApplyControlPanelState(panelRequested);
				g_lastPanelState = panelRequested;
				g_lastPanelStateKnown = true;
			}

			if (panelRequested) {
				if (auto* bridge = CalamityAffixes::EventBridge::GetSingleton()) {
					SetRunewordBaseInventoryList(bridge->GetRunewordBaseInventoryEntries());
					SetRunewordRecipeList(bridge->GetRunewordRecipeEntries());
					SetRunewordPanelState(bridge->GetRunewordPanelState());
				}
			}

			const auto selected = GetSelectedItemContext();

			const auto& tooltip = selected.tooltip;
			if (!tooltip || tooltip->empty()) {
				if (!g_lastTooltip.empty()) {
					g_lastTooltip.clear();
					g_api->InteropCall(g_view, "setTooltip", "");
				}
				if (!panelRequested && g_api->HasFocus(g_view)) {
					g_api->Unfocus(g_view);
				}

				if (!panelRequested) {
					// Keep the view visible in inventory so the quick-launch button can open the control panel
					// even when the selected item has no affix tooltip.
					const bool inventoryOpen = g_gatePollingOnMenus.load() && g_relevantMenusOpen.load() > 0;
					SetVisible(inventoryOpen);
				}
				return;
			}

			const std::string next = *tooltip;
			if (next != g_lastTooltip) {
				g_lastTooltip = next;
				g_api->InteropCall(g_view, "setTooltip", g_lastTooltip.c_str());
			}

			SetVisible(true);
			if (!panelRequested && !g_api->HasFocus(g_view)) {
				// Non-modal focus keeps inventory usable while still allowing tooltip mouse drag input.
				g_api->Focus(g_view, false, true);
			}
		}
