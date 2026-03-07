	[[nodiscard]] std::uint32_t NormalizeButtonKeyCode(const RE::ButtonEvent* a_event) noexcept
	{
		if (!a_event) {
			return 0;
		}

		auto keyCode = a_event->GetIDCode();
		if (keyCode == 0) {
			return 0;
		}

		switch (a_event->device.get()) {
		case RE::INPUT_DEVICES::kKeyboard:
			return keyCode;
		case RE::INPUT_DEVICES::kMouse:
			return kMouseButtonOffset + keyCode;
		default:
			return 0;
		}
	}

	[[nodiscard]] std::uint32_t ResolveConfiguredControlPanelHotkey() noexcept
	{
		auto configuredHotkey = g_controlPanelHotkey.load();
		if (configuredHotkey == 0u) {
			// Fallback: allow opening the panel even if the user hasn't configured a keybind yet.
			configuredHotkey = kFallbackPanelHotkey;
		}
		return configuredHotkey;
	}

	[[nodiscard]] bool ShouldToggleControlPanelForButton(
		const RE::ButtonEvent* a_event,
		std::uint32_t a_configuredHotkey) noexcept
	{
		if (!a_event || !a_event->IsDown()) {
			return false;
		}

		const auto pressedKey = NormalizeButtonKeyCode(a_event);
		if (pressedKey == 0u || pressedKey != a_configuredHotkey) {
			return false;
		}

		return IsGameplayReady();
	}

	void QueueToggleControlPanelTask()
	{
		QueueTask([]() {
			SetControlPanelOpenImpl(!g_controlPanelOpen.load());
		});
	}

	[[nodiscard]] bool IsRelevantMenuEvent(const RE::MenuOpenCloseEvent& a_event) noexcept
	{
		const char* raw = a_event.menuName.c_str();
		const std::string_view menuName = raw ? std::string_view(raw) : std::string_view{};
		return IsRelevantMenu(menuName);
	}

	void HandleRelevantMenuOpened()
	{
		const int openCount = CountRelevantMenusOpenFromUi();
		if (openCount > 0) {
			g_relevantMenusOpen.store(openCount);
		} else {
			// Opening events can race before UI::IsMenuOpen reflects the new state.
			// Fall back to event-based increment so polling doesn't stall at zero.
			g_relevantMenusOpen.fetch_add(1);
		}
	}

	[[nodiscard]] int ResolveRelevantMenuCountAfterClose() noexcept
	{
		int openCount = CountRelevantMenusOpenFromUi();
		if (openCount < 0) {
			const int prev = g_relevantMenusOpen.fetch_sub(1);
			openCount = std::max(0, prev - 1);
		}
		g_relevantMenusOpen.store(std::max(0, openCount));
		return openCount;
	}

	void SyncViewStateAfterRelevantMenusClosed(int a_openCount)
	{
		if (a_openCount > 0) {
			return;
		}

		g_relevantMenusOpen.store(0);

		if (!g_controlPanelOpen.load() && g_api && g_view && g_api->IsValid(g_view)) {
			// Keep cursor/focus state in sync with menu lifetime.
			// If focus remains after menu close, the cursor can linger until reopening a menu.
			if (g_api->HasFocus(g_view)) {
				g_api->Unfocus(g_view);
			}

			if (IsViewReady()) {
				ClearTooltipViewState(false);
			}

			SetVisible(false);
		}
	}

	class PanelHotkeyEventSink final : public RE::BSInputDeviceManager::Sink
	{
	public:
		using Event = RE::InputEvent*;

		RE::BSEventNotifyControl ProcessEvent(
			const Event* a_event,
			RE::BSTEventSource<Event>*) override
		{
			const auto configuredHotkey = ResolveConfiguredControlPanelHotkey();
			if (configuredHotkey == 0u) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const auto* inputEvent = a_event ? *a_event : nullptr;
			for (auto* current = inputEvent; current; current = current->next) {
				const auto* buttonEvent = current->AsButtonEvent();
				if (!ShouldToggleControlPanelForButton(buttonEvent, configuredHotkey)) {
					continue;
				}

				QueueToggleControlPanelTask();
				return RE::BSEventNotifyControl::kContinue;
			}

			return RE::BSEventNotifyControl::kContinue;
		}
	};

	class MenuEventSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::MenuOpenCloseEvent* a_event,
			RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
		{
			if (!a_event || !IsRelevantMenuEvent(*a_event)) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (a_event->opening) {
				HandleRelevantMenuOpened();
				return RE::BSEventNotifyControl::kContinue;
			}

			SyncViewStateAfterRelevantMenusClosed(ResolveRelevantMenuCountAfterClose());

			return RE::BSEventNotifyControl::kContinue;
		}
	};

	MenuEventSink g_menuSink;
	PanelHotkeyEventSink g_hotkeySink;
