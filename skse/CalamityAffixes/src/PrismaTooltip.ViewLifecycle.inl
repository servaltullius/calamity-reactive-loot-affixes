	bool EnsurePrismaApiAvailable()
	{
		if (g_api) {
			return true;
		}

		auto* api = static_cast<PRISMA_UI_API::IVPrismaUI1*>(
			PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
		if (!api) {
			if (!g_loggedMissingApi.exchange(true)) {
				SKSE::log::error("CalamityAffixes: PrismaUI API not available. Is PrismaUI installed and loaded?");
			}
			return false;
		}

		g_api = api;
		SKSE::log::info("CalamityAffixes: PrismaUI API acquired.");
		return true;
	}

	void PrepareForNewPrismaView()
	{
		g_domReady = false;
		g_lastPanelStateKnown = false;
		g_viewCache.uiLanguageCode.clear();
		g_runewordCache.affixPreview.clear();
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

	void CreatePrismaView()
	{
		PrepareForNewPrismaView();
		g_view = g_api->CreateView(
			kViewPath,
			[](PRISMA_UI_API::PrismaView) {
				g_domReady = true;
			});
		g_api->SetOrder(g_view, kViewOrder);
		g_api->Hide(g_view);

		RegisterPrismaCommandListener();
		QueueCachedLayoutStateForNewView();

		SKSE::log::info("CalamityAffixes: PrismaUI view created (view={}, path={}).", g_view, kViewPath);
	}

	void EnsurePrisma()
	{
		LoadPanelLayoutFromDisk();
		LoadTooltipLayoutFromDisk();

		if (g_api && g_view && g_api->IsValid(g_view)) {
			return;
		}

		if (!EnsurePrismaApiAvailable()) {
			return;
		}

		if (!g_view) {
			CreatePrismaView();
		}
	}

	void SetControlPanelOpenImpl(bool a_open)
	{
		g_controlPanelOpen.store(a_open);
		EnsurePrisma();
		ApplyControlPanelState(a_open);
		PushUiFeedback(a_open ? "Prisma panel opened." : "Prisma panel closed.");
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

	void StartPrismaWorker()
	{
		g_worker = std::jthread([](std::stop_token stopToken) {
			while (!stopToken.stop_requested()) {
				std::this_thread::sleep_for(kPollInterval);

				if (g_gatePollingOnMenus.load() &&
					g_relevantMenusOpen.load() <= 0 &&
					!g_controlPanelOpen.load()) {
					continue;
				}

				if (auto* tasks = SKSE::GetTaskInterface()) {
					PrismaTelemetryController::RecordWorkerEnqueue();
					tasks->AddTask([]() {
						Tick();
					});
				}
			}
		});
	}
