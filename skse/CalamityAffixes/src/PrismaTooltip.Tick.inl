		[[nodiscard]] bool TickShouldWaitForReadyView(std::chrono::steady_clock::time_point now)
		{
			if (g_api && g_view && g_api->IsValid(g_view) && g_domReady.load()) {
				return false;
			}

			MaybeLogPrismaTelemetry(now, g_controlPanelOpen.load() || g_relevantMenusOpen.load() > 0);
			return true;
		}

		void PushTickUiSettingsAndLayouts()
		{
			PushPanelHotkeyToJs();
			PushUiLanguageToJs();

			if (g_panelLayoutState.needsPush) {
				PushPanelLayoutToJs();
				g_panelLayoutState.needsPush = false;
			}
			if (g_tooltipLayoutState.needsPush) {
				PushTooltipLayoutToJs();
				g_tooltipLayoutState.needsPush = false;
			}
		}

		[[nodiscard]] bool HandleTickWhileGameplayUnavailable(std::chrono::steady_clock::time_point now)
		{
			if (IsGameplayReady()) {
				return false;
			}

			g_lastPanelStateKnown = false;
			ClearTooltipViewState(true);
			SetControlPanelJsState(false);
			if (g_api->HasFocus(g_view)) {
				g_api->Unfocus(g_view);
			}
			SetVisible(false);
			MaybeLogPrismaTelemetry(now, true);
			return true;
		}

		void SyncTickControlPanelState(bool a_panelRequested)
		{
			if (!g_lastPanelStateKnown || g_lastPanelState != a_panelRequested) {
				ApplyControlPanelState(a_panelRequested);
				g_lastPanelState = a_panelRequested;
				g_lastPanelStateKnown = true;
			}
		}

		[[nodiscard]] bool HandleTickWhenNoRelevantMenusRemain(
			bool a_panelRequested,
			std::chrono::steady_clock::time_point now)
		{
			// Failsafe: heal stale menu-open counters from actual UI state.
			// Some close paths can race event delivery and leave cursor/focus behind.
			const int observedRelevantMenus = CountRelevantMenusOpenFromUi();
			if (observedRelevantMenus < 0) {
				return false;
			}

			g_relevantMenusOpen.store(observedRelevantMenus);
			if (a_panelRequested || observedRelevantMenus != 0) {
				return false;
			}

			ClearTooltipViewState(false);
			if (g_api->HasFocus(g_view)) {
				g_api->Unfocus(g_view);
			}
			SetVisible(false);
			MaybeLogPrismaTelemetry(now, false);
			return true;
		}

		void RefreshTickRunewordPanelBindings(bool a_panelRequested)
		{
			if (!a_panelRequested) {
				return;
			}

			if (auto* bridge = CalamityAffixes::EventBridge::GetSingleton()) {
				RefreshRunewordPanelBindings(*bridge);
			}
		}

		void FinalizeTickTooltipPresentation(
			bool a_panelRequested,
			std::chrono::steady_clock::time_point now)
		{
			const bool hasTooltip = PushSelectedTooltipSnapshot(false);
			if (!hasTooltip) {
				if (!a_panelRequested && g_api->HasFocus(g_view)) {
					g_api->Unfocus(g_view);
				}

				if (!a_panelRequested) {
					// Keep the view visible in inventory so the quick-launch button can open the control panel
					// even when the selected item has no affix tooltip.
					const bool inventoryOpen = g_gatePollingOnMenus.load() && g_relevantMenusOpen.load() > 0;
					SetVisible(inventoryOpen);
				}
				MaybeLogPrismaTelemetry(now, a_panelRequested || g_relevantMenusOpen.load() > 0);
				return;
			}

			SetVisible(true);
			if (!a_panelRequested && g_api->HasFocus(g_view)) {
				// Stability mode: tooltip-only state should never own cursor/focus.
				// This disables drag-to-move, but prevents lingering cursor state after menu close.
				g_api->Unfocus(g_view);
			}
			MaybeLogPrismaTelemetry(now, true);
		}

		void Tick()
		{
			const auto now = std::chrono::steady_clock::now();
			PrismaTelemetryController::RecordTickRun();
			RefreshControlPanelHotkeyFromMcm(false);
			RefreshUiLanguageFromMcm(false);
			EnsurePrisma();

			if (TickShouldWaitForReadyView(now)) {
				return;
			}

			PushTickUiSettingsAndLayouts();

			if (HandleTickWhileGameplayUnavailable(now)) {
				return;
			}

			const bool panelRequested = g_controlPanelOpen.load();
			SyncTickControlPanelState(panelRequested);
			if (HandleTickWhenNoRelevantMenusRemain(panelRequested, now)) {
				return;
			}

			RefreshTickRunewordPanelBindings(panelRequested);
			FinalizeTickTooltipPresentation(panelRequested, now);
		}
