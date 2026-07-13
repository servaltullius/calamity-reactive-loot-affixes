	[[nodiscard]] const char* ResolvePrismaStatusHudText(PrismaAvailabilityStatus a_status) noexcept
	{
		switch (a_status) {
		case PrismaAvailabilityStatus::kApiUnavailable:
			return "Calamity: Prisma UI unavailable; full UI is disabled.";
		case PrismaAvailabilityStatus::kViewLoading:
			return "Calamity: Prisma UI detected; Calamity view is loading.";
		case PrismaAvailabilityStatus::kReady:
			return "Calamity: Prisma UI ready.";
		case PrismaAvailabilityStatus::kViewFailed:
			return "Calamity: Prisma UI view failed. Check the SKSE log.";
		case PrismaAvailabilityStatus::kDomReadyTimedOut:
			return "Calamity: Prisma UI view timed out. Check the SKSE log.";
		case PrismaAvailabilityStatus::kNotChecked:
		default:
			return "Calamity: Prisma UI status unknown.";
		}
	}

	[[nodiscard]] bool IsPrismaFailureStatus(PrismaAvailabilityStatus a_status) noexcept
	{
		return a_status == PrismaAvailabilityStatus::kApiUnavailable ||
			a_status == PrismaAvailabilityStatus::kViewFailed ||
			a_status == PrismaAvailabilityStatus::kDomReadyTimedOut;
	}

	void NotifyPrismaPanelFailureOnce(PrismaAvailabilityStatus a_status)
	{
		if (!IsPrismaFailureStatus(a_status) || g_userFailureNotificationShown.exchange(true)) {
			return;
		}
		RE::DebugNotification(ResolvePrismaStatusHudText(a_status));
	}

	void ScheduleNextPrismaAttempt(std::chrono::steady_clock::time_point a_now) noexcept
	{
		g_nextPrismaAttemptAt = a_now + kPrismaRetryInterval;
	}

	bool EnsurePrismaApiAvailable(
		std::chrono::steady_clock::time_point a_now,
		bool a_forceAttempt)
	{
		if (g_api) {
			return true;
		}
		if (!a_forceAttempt &&
			g_nextPrismaAttemptAt.time_since_epoch().count() != 0 &&
			a_now < g_nextPrismaAttemptAt) {
			return false;
		}

		auto* api = static_cast<PRISMA_UI_API::IVPrismaUI1*>(
			PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
		if (!api) {
			g_prismaStatus.store(PrismaAvailabilityStatus::kApiUnavailable);
			ScheduleNextPrismaAttempt(a_now);
			if (!g_loggedMissingApi.exchange(true)) {
				SKSE::log::error("CalamityAffixes: PrismaUI API not available. Is PrismaUI installed and loaded?");
			}
			return false;
		}

		g_api = api;
		g_loggedMissingApi.store(false);
		g_nextPrismaAttemptAt = {};
		g_prismaStatus.store(PrismaAvailabilityStatus::kNotChecked);
		SKSE::log::info("CalamityAffixes: PrismaUI API acquired.");
		return true;
	}

	void DeactivatePrismaViewReadiness()
	{
		const std::scoped_lock lock(g_prismaViewReadinessMutex);
		g_activePrismaView.store(0, std::memory_order_release);
		g_domReadyView.store(0, std::memory_order_release);
		g_pendingDomReadyViews.fill(0);
	}

	[[nodiscard]] bool ActivatePrismaViewReadiness(PRISMA_UI_API::PrismaView a_view)
	{
		const std::scoped_lock lock(g_prismaViewReadinessMutex);
		g_activePrismaView.store(a_view, std::memory_order_release);

		bool wasAlreadyReady = false;
		for (const auto pendingView : g_pendingDomReadyViews) {
			wasAlreadyReady = wasAlreadyReady || pendingView == a_view;
		}
		g_pendingDomReadyViews.fill(0);
		g_domReadyView.store(wasAlreadyReady ? a_view : 0, std::memory_order_release);
		return wasAlreadyReady;
	}

	void RecordPrismaViewDomReady(PRISMA_UI_API::PrismaView a_view)
	{
		if (!a_view) {
			return;
		}

		const std::scoped_lock lock(g_prismaViewReadinessMutex);
		const auto activeView = g_activePrismaView.load(std::memory_order_acquire);
		if (activeView == a_view) {
			g_domReadyView.store(a_view, std::memory_order_release);
			return;
		}
		if (activeView != 0) {
			return;
		}

		for (auto& pendingView : g_pendingDomReadyViews) {
			if (pendingView == a_view) {
				return;
			}
			if (pendingView == 0) {
				pendingView = a_view;
				return;
			}
		}
	}

	void PrepareForNewPrismaView()
	{
		DeactivatePrismaViewReadiness();
		g_prismaViewCreatedAt = {};
		g_lastPanelStateKnown = false;
		g_lastTooltip.clear();
		g_viewCache = {};
		g_runewordCache.baseListJson.clear();
		g_runewordCache.recipeListJson.clear();
		g_runewordCache.panelStateJson.clear();
		g_runewordCache.affixPreview.clear();
		RequestRunewordPanelRefresh(true);
		g_nextPanelSafetyRefreshAt = {};
	}

	void RegisterPrismaCommandListener()
	{
		g_api->RegisterJSListener(g_view, kPrismaCommandListener, [](const char* a_argument) {
			const std::string command = a_argument ? std::string(a_argument) : std::string();
			QueueTask([command]() {
				HandleUiCommand(command);
			});
		});
	}

	void QueueCachedLayoutStateForNewView()
	{
		if (g_panelLayoutState.value.valid) {
			g_panelLayoutState.lastJson.clear();
			g_panelLayoutState.needsPush = true;
		}
		if (g_tooltipLayoutState.value.valid) {
			g_tooltipLayoutState.lastJson.clear();
			g_tooltipLayoutState.needsPush = true;
		}
	}

	void MarkPrismaViewDomReady(PRISMA_UI_API::PrismaView a_view)
	{
		if (!a_view ||
			g_activePrismaView.load(std::memory_order_acquire) != a_view ||
			g_domReadyView.load(std::memory_order_acquire) != a_view) {
			return;
		}

		const auto previous = g_prismaStatus.exchange(PrismaAvailabilityStatus::kReady);
		g_loggedViewFailure.store(false);
		g_loggedDomReadyTimeout.store(false);
		g_userFailureNotificationShown.store(false);
		if (previous != PrismaAvailabilityStatus::kReady) {
			SKSE::log::info("CalamityAffixes: PrismaUI DOM ready (view={}).", a_view);
		}
	}

	PrismaAvailabilityStatus CreatePrismaView(std::chrono::steady_clock::time_point a_now)
	{
		PrepareForNewPrismaView();
		g_prismaStatus.store(PrismaAvailabilityStatus::kViewLoading);

		const auto candidate = g_api->CreateView(kViewPath, [](PRISMA_UI_API::PrismaView a_view) {
			RecordPrismaViewDomReady(a_view);
		});
		if (!candidate || !g_api->IsValid(candidate)) {
			DeactivatePrismaViewReadiness();
			g_view = 0;
			g_prismaViewCreatedAt = {};
			g_prismaStatus.store(PrismaAvailabilityStatus::kViewFailed);
			ScheduleNextPrismaAttempt(a_now);
			if (!g_loggedViewFailure.exchange(true)) {
				SKSE::log::error(
					"CalamityAffixes: PrismaUI CreateView failed or returned an invalid view (view={}, path={}).",
					candidate,
					kViewPath);
			}
			return PrismaAvailabilityStatus::kViewFailed;
		}

		g_view = candidate;
		const bool wasAlreadyReady = ActivatePrismaViewReadiness(candidate);
		g_prismaViewCreatedAt = a_now;
		g_nextPrismaAttemptAt = {};
		g_api->SetOrder(g_view, kViewOrder);
		g_api->Hide(g_view);
		RegisterPrismaCommandListener();
		QueueCachedLayoutStateForNewView();

		if (wasAlreadyReady || g_domReadyView.load(std::memory_order_acquire) == g_view) {
			MarkPrismaViewDomReady(g_view);
		}

		SKSE::log::info("CalamityAffixes: PrismaUI view created (view={}, path={}).", g_view, kViewPath);
		return g_prismaStatus.load();
	}

	void ResetPrismaViewForRetry(
		PrismaAvailabilityStatus a_status,
		std::chrono::steady_clock::time_point a_now)
	{
		DeactivatePrismaViewReadiness();
		g_view = 0;
		g_prismaViewCreatedAt = {};
		g_prismaStatus.store(a_status);
		ScheduleNextPrismaAttempt(a_now);

		if (g_controlPanelOpen.exchange(false)) {
			NotifyPrismaPanelFailureOnce(a_status);
		}
	}

	PrismaAvailabilityStatus EnsurePrisma(bool a_forceAttempt)
	{
		LoadPanelLayoutFromDisk();
		LoadTooltipLayoutFromDisk();

		const auto now = std::chrono::steady_clock::now();
		if (!EnsurePrismaApiAvailable(now, a_forceAttempt)) {
			return g_prismaStatus.load();
		}

		if (g_view) {
			if (!g_api->IsValid(g_view)) {
				if (!g_loggedViewFailure.exchange(true)) {
					SKSE::log::warn(
						"CalamityAffixes: PrismaUI view became invalid; scheduling recreation (view={}).",
						g_view);
				}
				ResetPrismaViewForRetry(PrismaAvailabilityStatus::kViewFailed, now);
				if (!a_forceAttempt) {
					return PrismaAvailabilityStatus::kViewFailed;
				}
			} else if (g_domReadyView.load(std::memory_order_acquire) == g_view) {
				MarkPrismaViewDomReady(g_view);
				return PrismaAvailabilityStatus::kReady;
			} else if (
				g_prismaViewCreatedAt.time_since_epoch().count() != 0 &&
				(now - g_prismaViewCreatedAt) >= kPrismaDomReadyTimeout) {
				const auto expiredView = g_view;
				ResetPrismaViewForRetry(PrismaAvailabilityStatus::kDomReadyTimedOut, now);
				g_api->Destroy(expiredView);
				if (!g_loggedDomReadyTimeout.exchange(true)) {
					SKSE::log::error(
						"CalamityAffixes: PrismaUI DOM ready timed out after {} seconds (view={}, path={}).",
						std::chrono::duration_cast<std::chrono::seconds>(kPrismaDomReadyTimeout).count(),
						expiredView,
						kViewPath);
				}
				if (!a_forceAttempt) {
					return PrismaAvailabilityStatus::kDomReadyTimedOut;
				}
			} else {
				g_prismaStatus.store(PrismaAvailabilityStatus::kViewLoading);
				return PrismaAvailabilityStatus::kViewLoading;
			}
		}

		if (!a_forceAttempt &&
			g_nextPrismaAttemptAt.time_since_epoch().count() != 0 &&
			now < g_nextPrismaAttemptAt) {
			return g_prismaStatus.load();
		}

		return CreatePrismaView(now);
	}

	void SetControlPanelOpenImpl(bool a_open)
	{
		if (!a_open) {
			g_controlPanelOpen.store(false);
			ApplyControlPanelState(false);
			g_lastPanelState = false;
			g_lastPanelStateKnown = true;
			PushUiFeedback("Prisma panel closed.");
			return;
		}
		RequestRunewordPanelRefresh(g_runewordCache.recipeListJson.empty());

		const auto status = EnsurePrisma(true);
		if (IsPrismaFailureStatus(status)) {
			g_controlPanelOpen.store(false);
			NotifyPrismaPanelFailureOnce(status);
			return;
		}

		g_controlPanelOpen.store(true);
		if (status == PrismaAvailabilityStatus::kReady) {
			g_lastPanelState = true;
			g_lastPanelStateKnown = IsGameplayReady();
			ApplyControlPanelState(true);
			PushUiFeedback("Prisma panel opened.");
		}
	}

	void RegisterPrismaEventSinks()
	{
		if (auto* ui = RE::UI::GetSingleton()) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(&g_menuSink);
			g_gatePollingOnMenus = true;
		}
		if (auto* input = RE::BSInputDeviceManager::GetSingleton()) {
			input->AddEventSink(&g_hotkeySink);
		}
	}

	class TickTaskPendingReset final
	{
	public:
		~TickTaskPendingReset()
		{
			g_tickTaskPending.store(false, std::memory_order_release);
		}
	};

	void StartPrismaWorker()
	{
		g_worker = std::jthread([](std::stop_token stopToken) {
			while (!stopToken.stop_requested()) {
				std::this_thread::sleep_for(kPollInterval);

				if (g_gatePollingOnMenus.load() &&
					g_relevantMenusOpen.load() <= 0 &&
					!g_controlPanelOpen.load() &&
					g_prismaStatus.load() != PrismaAvailabilityStatus::kViewLoading) {
					continue;
				}

				if (auto* tasks = SKSE::GetTaskInterface()) {
					bool expected = false;
					if (!g_tickTaskPending.compare_exchange_strong(
							expected, true, std::memory_order_acq_rel)) {
						PrismaTelemetryController::RecordCoalescedTick();
						continue;
					}
					PrismaTelemetryController::RecordWorkerEnqueue();
					tasks->AddTask([]() {
						[[maybe_unused]] const TickTaskPendingReset pendingReset{};
						Tick();
					});
				}
			}
		});
	}
