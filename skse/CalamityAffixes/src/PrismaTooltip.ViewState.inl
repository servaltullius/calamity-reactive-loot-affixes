		class PrismaTelemetryController final
		{
		public:
			static void RecordWorkerEnqueue() noexcept
			{
				g_prismaTelemetry.workerEnqueues.fetch_add(1u, std::memory_order_relaxed);
			}

			static void RecordTooltipClear() noexcept
			{
				g_prismaTelemetry.tooltipClears.fetch_add(1u, std::memory_order_relaxed);
			}

			static void RecordTooltipPush() noexcept
			{
				g_prismaTelemetry.tooltipPushes.fetch_add(1u, std::memory_order_relaxed);
			}

			static void RecordSelectedContextRefresh() noexcept
			{
				g_prismaTelemetry.selectedContextRefreshes.fetch_add(1u, std::memory_order_relaxed);
			}

			static void RecordPanelRefresh() noexcept
			{
				g_prismaTelemetry.panelRefreshes.fetch_add(1u, std::memory_order_relaxed);
			}

			static void RecordUiCommand() noexcept
			{
				g_prismaTelemetry.uiCommands.fetch_add(1u, std::memory_order_relaxed);
			}

			static void RecordTickRun() noexcept
			{
				g_prismaTelemetry.tickRuns.fetch_add(1u, std::memory_order_relaxed);
			}

			static void MaybeLog(std::chrono::steady_clock::time_point a_now, bool a_uiActive)
			{
				if (!a_uiActive) {
					return;
				}

				if (g_nextTelemetryLogAt.time_since_epoch().count() != 0 && a_now < g_nextTelemetryLogAt) {
					return;
				}
				g_nextTelemetryLogAt = a_now + std::chrono::seconds(15);

				SKSE::log::debug(
					"CalamityAffixes: Prisma telemetry (ticks={}, queued={}, clears={}, tooltipPushes={}, contextRefreshes={}, panelRefreshes={}, commands={}, menus={}, panelOpen={}).",
					g_prismaTelemetry.tickRuns.load(std::memory_order_relaxed),
					g_prismaTelemetry.workerEnqueues.load(std::memory_order_relaxed),
					g_prismaTelemetry.tooltipClears.load(std::memory_order_relaxed),
					g_prismaTelemetry.tooltipPushes.load(std::memory_order_relaxed),
					g_prismaTelemetry.selectedContextRefreshes.load(std::memory_order_relaxed),
					g_prismaTelemetry.panelRefreshes.load(std::memory_order_relaxed),
					g_prismaTelemetry.uiCommands.load(std::memory_order_relaxed),
					g_relevantMenusOpen.load(std::memory_order_relaxed),
					g_controlPanelOpen.load(std::memory_order_relaxed));
			}
		};

		class PrismaViewStateController final
		{
		public:
			static void SetSelectedItemContext(std::string_view a_itemName, std::string_view a_itemSource)
			{
				if (!IsViewReady()) {
					return;
				}

				const std::string itemName(a_itemName);
				const std::string itemSource(a_itemSource);
				if (itemName == g_viewCache.selectedItemName && itemSource == g_viewCache.selectedItemSource) {
					return;
				}

				g_viewCache.selectedItemName = itemName;
				g_viewCache.selectedItemSource = itemSource;
				PrismaTelemetryController::RecordSelectedContextRefresh();
				g_api->InteropCall(g_view, kInteropSetSelectedItemName, g_viewCache.selectedItemName.c_str());
				g_api->InteropCall(g_view, kInteropSetSelectedItemSource, g_viewCache.selectedItemSource.c_str());
			}

			static void ClearTooltipViewState(bool a_clearRunewordPanelData)
			{
				bool tooltipCleared = false;
				if (!g_lastTooltip.empty()) {
					g_lastTooltip.clear();
					tooltipCleared = true;
					if (IsViewReady()) {
						g_api->InteropCall(g_view, kInteropSetTooltip, "");
					}
				}
				if (tooltipCleared) {
					PrismaTelemetryController::RecordTooltipClear();
				}

				SetSelectedItemContext({}, {});
				if (!a_clearRunewordPanelData) {
					return;
				}

				SetRunewordBaseInventoryList({});
				SetRunewordRecipeList({});
				SetRunewordPanelState({});
				SetRunewordAffixPreview("");
			}

			static void RefreshRunewordPanelBindings(CalamityAffixes::EventBridge& a_bridge)
			{
				PrismaTelemetryController::RecordPanelRefresh();
				SetRunewordBaseInventoryList(a_bridge.GetRunewordBaseInventoryEntries());
				SetRunewordRecipeList(a_bridge.GetRunewordRecipeEntries());
				SetRunewordPanelState(a_bridge.GetRunewordPanelState());
				SetRunewordAffixPreview(a_bridge.GetSelectedRunewordBaseAffixTooltip(g_uiLanguageMode.load()).value_or(""));
			}
		};

		void QueueTask(std::function<void()> a_task)
		{
			if (!a_task) {
				return;
			}

			if (auto* tasks = SKSE::GetTaskInterface()) {
				PrismaTelemetryController::RecordWorkerEnqueue();
				tasks->AddTask(std::move(a_task));
			}
		}

		void SetSelectedItemContext(std::string_view a_itemName, std::string_view a_itemSource)
		{
			PrismaViewStateController::SetSelectedItemContext(a_itemName, a_itemSource);
		}

		void ClearTooltipViewState(bool a_clearRunewordPanelData)
		{
			PrismaViewStateController::ClearTooltipViewState(a_clearRunewordPanelData);
		}

		void RefreshRunewordPanelBindings(CalamityAffixes::EventBridge& a_bridge)
		{
			PrismaViewStateController::RefreshRunewordPanelBindings(a_bridge);
		}

		void MaybeLogPrismaTelemetry(std::chrono::steady_clock::time_point a_now, bool a_uiActive)
		{
			PrismaTelemetryController::MaybeLog(a_now, a_uiActive);
		}
