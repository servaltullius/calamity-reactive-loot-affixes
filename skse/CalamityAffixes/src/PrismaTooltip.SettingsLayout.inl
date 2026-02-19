		[[nodiscard]] std::string DescribeHotkey(std::uint32_t a_keyCode)
		{
			if (a_keyCode == 0u) {
				return {};
			}

			if (a_keyCode >= kMouseButtonOffset) {
				const auto mouse = a_keyCode - kMouseButtonOffset;
				switch (mouse) {
				case 0:
					return "Mouse1";
				case 1:
					return "Mouse2";
				case 2:
					return "Mouse3";
				default:
					return "Mouse" + std::to_string(mouse + 1);
				}
			}

			switch (a_keyCode) {
			case RE::BSKeyboardDevice::Keys::kEscape:
				return "Esc";
			case RE::BSKeyboardDevice::Keys::kNum1:
				return "1";
			case RE::BSKeyboardDevice::Keys::kNum2:
				return "2";
			case RE::BSKeyboardDevice::Keys::kNum3:
				return "3";
			case RE::BSKeyboardDevice::Keys::kNum4:
				return "4";
			case RE::BSKeyboardDevice::Keys::kNum5:
				return "5";
			case RE::BSKeyboardDevice::Keys::kNum6:
				return "6";
			case RE::BSKeyboardDevice::Keys::kNum7:
				return "7";
			case RE::BSKeyboardDevice::Keys::kNum8:
				return "8";
			case RE::BSKeyboardDevice::Keys::kNum9:
				return "9";
			case RE::BSKeyboardDevice::Keys::kNum0:
				return "0";
			case RE::BSKeyboardDevice::Keys::kMinus:
				return "-";
			case RE::BSKeyboardDevice::Keys::kEquals:
				return "=";
			case RE::BSKeyboardDevice::Keys::kBackspace:
				return "Backspace";
			case RE::BSKeyboardDevice::Keys::kTab:
				return "Tab";
			case RE::BSKeyboardDevice::Keys::kQ:
				return "Q";
			case RE::BSKeyboardDevice::Keys::kW:
				return "W";
			case RE::BSKeyboardDevice::Keys::kE:
				return "E";
			case RE::BSKeyboardDevice::Keys::kR:
				return "R";
			case RE::BSKeyboardDevice::Keys::kT:
				return "T";
			case RE::BSKeyboardDevice::Keys::kY:
				return "Y";
			case RE::BSKeyboardDevice::Keys::kU:
				return "U";
			case RE::BSKeyboardDevice::Keys::kI:
				return "I";
			case RE::BSKeyboardDevice::Keys::kO:
				return "O";
			case RE::BSKeyboardDevice::Keys::kP:
				return "P";
			case RE::BSKeyboardDevice::Keys::kBracketLeft:
				return "[";
			case RE::BSKeyboardDevice::Keys::kBracketRight:
				return "]";
			case RE::BSKeyboardDevice::Keys::kSpacebar:
				return "Space";
			case RE::BSKeyboardDevice::Keys::kEnter:
				return "Enter";
			case RE::BSKeyboardDevice::Keys::kLeftShift:
				return "LShift";
			case RE::BSKeyboardDevice::Keys::kRightShift:
				return "RShift";
			case RE::BSKeyboardDevice::Keys::kLeftControl:
				return "LCtrl";
			case RE::BSKeyboardDevice::Keys::kRightControl:
				return "RCtrl";
			case RE::BSKeyboardDevice::Keys::kLeftAlt:
				return "LAlt";
			case RE::BSKeyboardDevice::Keys::kRightAlt:
				return "RAlt";
			case RE::BSKeyboardDevice::Keys::kA:
				return "A";
			case RE::BSKeyboardDevice::Keys::kS:
				return "S";
			case RE::BSKeyboardDevice::Keys::kD:
				return "D";
			case RE::BSKeyboardDevice::Keys::kF:
				return "F";
			case RE::BSKeyboardDevice::Keys::kG:
				return "G";
			case RE::BSKeyboardDevice::Keys::kH:
				return "H";
			case RE::BSKeyboardDevice::Keys::kJ:
				return "J";
			case RE::BSKeyboardDevice::Keys::kK:
				return "K";
			case RE::BSKeyboardDevice::Keys::kL:
				return "L";
			case RE::BSKeyboardDevice::Keys::kZ:
				return "Z";
			case RE::BSKeyboardDevice::Keys::kX:
				return "X";
			case RE::BSKeyboardDevice::Keys::kC:
				return "C";
			case RE::BSKeyboardDevice::Keys::kV:
				return "V";
			case RE::BSKeyboardDevice::Keys::kB:
				return "B";
			case RE::BSKeyboardDevice::Keys::kN:
				return "N";
			case RE::BSKeyboardDevice::Keys::kM:
				return "M";
			case RE::BSKeyboardDevice::Keys::kF1:
				return "F1";
			case RE::BSKeyboardDevice::Keys::kF2:
				return "F2";
			case RE::BSKeyboardDevice::Keys::kF3:
				return "F3";
			case RE::BSKeyboardDevice::Keys::kF4:
				return "F4";
			case RE::BSKeyboardDevice::Keys::kF5:
				return "F5";
			case RE::BSKeyboardDevice::Keys::kF6:
				return "F6";
			case RE::BSKeyboardDevice::Keys::kF7:
				return "F7";
			case RE::BSKeyboardDevice::Keys::kF8:
				return "F8";
			case RE::BSKeyboardDevice::Keys::kF9:
				return "F9";
			case RE::BSKeyboardDevice::Keys::kF10:
				return "F10";
			case RE::BSKeyboardDevice::Keys::kF11:
				return "F11";
			case RE::BSKeyboardDevice::Keys::kF12:
				return "F12";
			case RE::BSKeyboardDevice::Keys::kUp:
				return "Up";
			case RE::BSKeyboardDevice::Keys::kDown:
				return "Down";
			case RE::BSKeyboardDevice::Keys::kLeft:
				return "Left";
			case RE::BSKeyboardDevice::Keys::kRight:
				return "Right";
			default:
				break;
			}

			return "Key" + std::to_string(a_keyCode);
		}

		[[nodiscard]] std::string_view TrimAscii(std::string_view a_text) noexcept
		{
			while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.front()))) {
				a_text.remove_prefix(1);
			}
			while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.back()))) {
				a_text.remove_suffix(1);
			}
			return a_text;
		}

		[[nodiscard]] std::string ToLowerAscii(std::string_view a_text)
		{
			std::string lower;
			lower.reserve(a_text.size());
			for (char ch : a_text) {
				lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
			}
			return lower;
		}

		[[nodiscard]] int NormalizeUiLanguageMode(int a_value) noexcept
		{
			switch (a_value) {
			case 0:
			case 1:
			case 2:
				return a_value;
			default:
				return kDefaultUiLanguageMode;
			}
		}

		[[nodiscard]] int ParseUiLanguageMode(std::string_view a_value) noexcept
		{
			const auto trimmed = TrimAscii(a_value);
			if (trimmed.empty()) {
				return kDefaultUiLanguageMode;
			}

			const auto lower = ToLowerAscii(trimmed);

			int parsedInt = 0;
			const auto* begin = lower.data();
			const auto* end = begin + lower.size();
			const auto [ptr, ec] = std::from_chars(begin, end, parsedInt);
			if (ec == std::errc{} && ptr == end) {
				return NormalizeUiLanguageMode(parsedInt);
			}

			if (lower == "en" || lower == "english") {
				return 0;
			}
			if (lower == "ko" || lower == "korean" || lower == "kr") {
				return 1;
			}
			if (lower == "both" || lower == "bilingual" || lower == "en+ko" || lower == "ko+en") {
				return 2;
			}

			return kDefaultUiLanguageMode;
		}

		[[nodiscard]] const char* ResolveUiLanguageCode(int a_mode) noexcept
		{
			switch (NormalizeUiLanguageMode(a_mode)) {
			case 0:
				return "en";
			case 1:
				return "ko";
			default:
				return "both";
			}
		}

		[[nodiscard]] std::optional<nlohmann::json> LoadUserUiSettingsSection()
		{
			nlohmann::json root;
			if (!UserSettingsPersistence::LoadJsonObject(kUserSettingsPath, root) || !root.is_object()) {
				return std::nullopt;
			}

			const auto uiIt = root.find("ui");
			if (uiIt == root.end() || !uiIt->is_object()) {
				return std::nullopt;
			}
			return *uiIt;
		}

		[[nodiscard]] std::optional<std::uint32_t> LoadPanelHotkeyFromUserSettings()
		{
			const auto ui = LoadUserUiSettingsSection();
			if (!ui) {
				return std::nullopt;
			}

			const auto hotkeyIt = ui->find("panelHotkey");
			if (hotkeyIt == ui->end()) {
				return std::nullopt;
			}

			if (hotkeyIt->is_number_unsigned()) {
				const auto rawKeyCode = hotkeyIt->get<std::uint64_t>();
				const auto maxKeyCode = static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max());
				if (rawKeyCode <= maxKeyCode) {
					return static_cast<std::uint32_t>(rawKeyCode);
				}
				return std::nullopt;
			}
			if (hotkeyIt->is_number_integer()) {
				const int keyCode = hotkeyIt->get<int>();
				if (keyCode >= 0) {
					return static_cast<std::uint32_t>(keyCode);
				}
			}

			return std::nullopt;
		}

		void PersistPanelHotkeyToUserSettings(std::uint32_t a_keyCode)
		{
			const bool ok = UserSettingsPersistence::UpdateJsonObject(
				kUserSettingsPath,
				[&](nlohmann::json& root) {
					root["version"] = 1;
					auto& ui = root["ui"];
					if (!ui.is_object()) {
						ui = nlohmann::json::object();
					}
					ui["panelHotkey"] = a_keyCode;
				});

			if (!ok) {
				SKSE::log::warn("CalamityAffixes: failed to persist panel hotkey to {}.", kUserSettingsPath);
			}
		}

		void RestorePanelHotkeyFromUserSettings()
		{
			const auto persistedHotkey = LoadPanelHotkeyFromUserSettings();
			const std::uint32_t resolvedKey = persistedHotkey.value_or(0u);
			const auto previous = g_controlPanelHotkey.exchange(resolvedKey);
			if (previous != resolvedKey && persistedHotkey.has_value()) {
				SKSE::log::info("CalamityAffixes: Prisma panel hotkey restored from {} (keycode={}).", kUserSettingsPath, resolvedKey);
			}
		}

		[[nodiscard]] std::optional<int> LoadUiLanguageModeFromUserSettings()
		{
			const auto ui = LoadUserUiSettingsSection();
			if (!ui) {
				return std::nullopt;
			}

			const auto languageIt = ui->find("uiLanguageMode");
			if (languageIt == ui->end()) {
				return std::nullopt;
			}
			if (languageIt->is_number_integer()) {
				return NormalizeUiLanguageMode(languageIt->get<int>());
			}
			if (languageIt->is_string()) {
				return ParseUiLanguageMode(languageIt->get<std::string>());
			}

			return std::nullopt;
		}

		void PersistUiLanguageModeToUserSettings(int a_mode)
		{
			const int normalizedMode = NormalizeUiLanguageMode(a_mode);
			const bool ok = UserSettingsPersistence::UpdateJsonObject(
				kUserSettingsPath,
				[&](nlohmann::json& root) {
					root["version"] = 1;
					auto& ui = root["ui"];
					if (!ui.is_object()) {
						ui = nlohmann::json::object();
					}
					ui["uiLanguageMode"] = normalizedMode;
				});

			if (!ok) {
				SKSE::log::warn("CalamityAffixes: failed to persist UI language mode to {}.", kUserSettingsPath);
			}
		}

		void RestoreUiLanguageModeFromUserSettings()
		{
			const auto persistedMode = LoadUiLanguageModeFromUserSettings();
			const int resolvedMode = NormalizeUiLanguageMode(persistedMode.value_or(kDefaultUiLanguageMode));
			const auto previous = g_uiLanguageMode.exchange(resolvedMode);
			if (previous != resolvedMode && persistedMode.has_value()) {
				SKSE::log::info("CalamityAffixes: UI language restored from {} (mode={}).", kUserSettingsPath, resolvedMode);
			}
		}

		void PushPanelHotkeyToJs()
		{
			if (!IsViewReady()) {
				return;
			}

			auto keyCode = g_controlPanelHotkey.load();
			if (keyCode == 0u) {
				keyCode = kFallbackPanelHotkey;
			}

			const auto text = DescribeHotkey(keyCode);
			if (text == g_lastPanelHotkeyText) {
				return;
			}

			g_lastPanelHotkeyText = text;
			g_api->InteropCall(g_view, "setPanelHotkeyText", g_lastPanelHotkeyText.c_str());
		}

		void PushUiLanguageToJs()
		{
			if (!IsViewReady()) {
				return;
			}

			const std::string mode = ResolveUiLanguageCode(g_uiLanguageMode.load());
			if (mode == g_lastUiLanguageCode) {
				return;
			}

			g_lastUiLanguageCode = mode;
			g_api->InteropCall(g_view, "setUiLanguage", g_lastUiLanguageCode.c_str());
		}

		void RefreshControlPanelHotkeyFromMcm(bool a_force)
		{
			const auto now = std::chrono::steady_clock::now();
			std::scoped_lock lk{ g_hotkeyRefreshLock };

			if (!a_force &&
				g_nextHotkeyRefreshAt.time_since_epoch().count() != 0 &&
				now < g_nextHotkeyRefreshAt) {
				return;
			}
			g_nextHotkeyRefreshAt = now + kHotkeyRefreshInterval;

			const std::filesystem::path settingsPath{ std::string(kMcmKeybindSettingsPath) };
			std::error_code ec;
			const bool exists = std::filesystem::exists(settingsPath, ec);
			if (ec || !exists) {
				g_hotkeySettingsKnown = false;
				RestorePanelHotkeyFromUserSettings();
				return;
			}

			const auto mtime = std::filesystem::last_write_time(settingsPath, ec);
			if (ec) {
				return;
			}
			if (!a_force && g_hotkeySettingsKnown && mtime == g_hotkeySettingsMtime) {
				return;
			}

			std::ifstream in(settingsPath);
			if (!in.good()) {
				return;
			}

			std::uint32_t resolvedKey = 0u;
			try {
				nlohmann::json j;
				in >> j;

				if (const auto keybindsIt = j.find("keybinds");
					keybindsIt != j.end() && keybindsIt->is_array()) {
					for (const auto& entry : *keybindsIt) {
						if (!entry.is_object()) {
							continue;
						}

						const std::string modName = entry.value("modName", std::string{});
						const std::string id = entry.value("id", std::string{});
						if (modName != kMcmModName || id != kPanelToggleKeybindId) {
							continue;
						}

						if (const auto keyIt = entry.find("keycode");
							keyIt != entry.end() && keyIt->is_number_integer()) {
							const int keyCode = keyIt->get<int>();
							if (keyCode > 0) {
								resolvedKey = static_cast<std::uint32_t>(keyCode);
							}
						}
						break;
					}
				}
			} catch (const std::exception& e) {
				SKSE::log::warn("CalamityAffixes: failed to parse {} ({})",
					kMcmKeybindSettingsPath,
					e.what());
				RestorePanelHotkeyFromUserSettings();
				return;
			}

			const auto previous = g_controlPanelHotkey.exchange(resolvedKey);
			g_hotkeySettingsKnown = true;
			g_hotkeySettingsMtime = mtime;

			if (previous != resolvedKey) {
				PersistPanelHotkeyToUserSettings(resolvedKey);
				SKSE::log::info("CalamityAffixes: Prisma panel hotkey updated (keycode={}).", resolvedKey);
			}
		}

		void RefreshUiLanguageFromMcm(bool a_force)
		{
			const auto now = std::chrono::steady_clock::now();
			std::scoped_lock lk{ g_uiLanguageRefreshLock };

			if (!a_force &&
				g_nextUiLanguageRefreshAt.time_since_epoch().count() != 0 &&
				now < g_nextUiLanguageRefreshAt) {
				return;
			}
			g_nextUiLanguageRefreshAt = now + kHotkeyRefreshInterval;

			std::error_code ec;
			const std::filesystem::path settingsPath{ std::string(kMcmModSettingsPath) };
			const std::filesystem::path defaultSettingsPath{ std::string(kMcmModDefaultSettingsPath) };

			std::filesystem::path activePath;
			if (std::filesystem::exists(settingsPath, ec) && !ec) {
				activePath = settingsPath;
			} else {
				ec.clear();
				if (std::filesystem::exists(defaultSettingsPath, ec) && !ec) {
					activePath = defaultSettingsPath;
				}
			}

			if (activePath.empty()) {
				RestoreUiLanguageModeFromUserSettings();
				g_uiLanguageSettingsKnown = false;
				g_uiLanguageSettingsPath.clear();
				return;
			}

			const auto mtime = std::filesystem::last_write_time(activePath, ec);
			if (ec) {
				return;
			}

			if (!a_force && g_uiLanguageSettingsKnown &&
				activePath == g_uiLanguageSettingsPath &&
				mtime == g_uiLanguageSettingsMtime) {
				return;
			}

			std::ifstream in(activePath);
			if (!in.good()) {
				return;
			}

			int resolvedMode = kDefaultUiLanguageMode;
			std::string sectionLower;
			std::string line;
			while (std::getline(in, line)) {
				std::string_view text(line);
				if (text.size() >= 3 &&
					static_cast<unsigned char>(text[0]) == 0xEF &&
					static_cast<unsigned char>(text[1]) == 0xBB &&
					static_cast<unsigned char>(text[2]) == 0xBF) {
					text.remove_prefix(3);
				}

				if (const auto comment = text.find_first_of(";#"); comment != std::string_view::npos) {
					text = text.substr(0, comment);
				}

				text = TrimAscii(text);
				if (text.empty()) {
					continue;
				}

				if (text.front() == '[' && text.back() == ']') {
					sectionLower = ToLowerAscii(TrimAscii(text.substr(1, text.size() - 2)));
					continue;
				}

				if (sectionLower != kMcmGeneralSectionLower) {
					continue;
				}

				const auto sep = text.find('=');
				if (sep == std::string_view::npos) {
					continue;
				}

				const auto keyLower = ToLowerAscii(TrimAscii(text.substr(0, sep)));
				if (keyLower != kMcmUiLanguageKeyLower) {
					continue;
				}

				resolvedMode = ParseUiLanguageMode(text.substr(sep + 1));
				break;
			}

			resolvedMode = NormalizeUiLanguageMode(resolvedMode);
			const auto previous = g_uiLanguageMode.exchange(resolvedMode);
			g_uiLanguageSettingsKnown = true;
			g_uiLanguageSettingsPath = activePath;
			g_uiLanguageSettingsMtime = mtime;

			if (previous != resolvedMode) {
				PersistUiLanguageModeToUserSettings(resolvedMode);
				SKSE::log::info("CalamityAffixes: UI language updated (mode={}).", resolvedMode);
			}
		}

		void LoadPanelLayoutFromDisk()
		{
			if (g_panelLayoutLoaded) {
				return;
			}
			g_panelLayoutLoaded = true;

			const std::filesystem::path path{ std::string(kPanelLayoutPath) };
			std::error_code ec;
			if (!std::filesystem::exists(path, ec) || ec) {
				return;
			}

			try {
				const auto loaded = PrismaLayoutPersistence::LoadPanelLayoutFile(kPanelLayoutPath);
				if (!loaded) {
					SKSE::log::warn("CalamityAffixes: invalid Prisma panel layout file at {}.", kPanelLayoutPath);
					return;
				}

				g_panelLayout = *loaded;
				g_panelLayoutNeedsPush = true;
				g_lastPanelLayoutJson.clear();
				SKSE::log::info("CalamityAffixes: loaded Prisma panel layout from {}.", kPanelLayoutPath);
			} catch (const std::exception& e) {
				SKSE::log::warn("CalamityAffixes: failed to parse panel layout {} ({})", kPanelLayoutPath, e.what());
			}
		}

		void LoadTooltipLayoutFromDisk()
		{
			if (g_tooltipLayoutLoaded) {
				return;
			}
			g_tooltipLayoutLoaded = true;

			const std::filesystem::path path{ std::string(kTooltipLayoutPath) };
			std::error_code ec;
			if (!std::filesystem::exists(path, ec) || ec) {
				return;
			}

			try {
				const auto loaded = PrismaLayoutPersistence::LoadTooltipLayoutFile(
					kTooltipLayoutPath,
					kDefaultTooltipRight,
					kDefaultTooltipTop,
					kDefaultTooltipFontPermille,
					kMinTooltipFontPermille,
					kMaxTooltipFontPermille);
				if (!loaded) {
					SKSE::log::warn("CalamityAffixes: invalid Prisma tooltip layout file at {}.", kTooltipLayoutPath);
					return;
				}

				g_tooltipLayout = *loaded;
				g_tooltipLayoutNeedsPush = true;
				g_lastTooltipLayoutJson.clear();
				SKSE::log::info("CalamityAffixes: loaded Prisma tooltip layout from {}.", kTooltipLayoutPath);
			} catch (const std::exception& e) {
				SKSE::log::warn("CalamityAffixes: failed to parse tooltip layout {} ({})", kTooltipLayoutPath, e.what());
			}
		}

		void SavePanelLayoutToDisk(const PanelLayout& a_layout)
		{
			if (!a_layout.valid) {
				return;
			}

			if (!PrismaLayoutPersistence::SavePanelLayoutFile(kPanelLayoutPath, a_layout)) {
				SKSE::log::warn("CalamityAffixes: failed to save panel layout file {}.", kPanelLayoutPath);
				return;
			}

			g_lastPanelLayoutJson = PrismaLayoutPersistence::EncodePanelLayoutJson(a_layout);
		}

		void SaveTooltipLayoutToDisk(const TooltipLayout& a_layout)
		{
			if (!a_layout.valid) {
				return;
			}

			if (!PrismaLayoutPersistence::SaveTooltipLayoutFile(kTooltipLayoutPath, a_layout)) {
				SKSE::log::warn("CalamityAffixes: failed to save tooltip layout file {}.", kTooltipLayoutPath);
				return;
			}

			g_lastTooltipLayoutJson = PrismaLayoutPersistence::EncodeTooltipLayoutJsonForDisk(a_layout);
		}

		void PushPanelLayoutToJs()
		{
			if (!IsViewReady() || !g_panelLayout.valid) {
				return;
			}

			const std::string payload = PrismaLayoutPersistence::EncodePanelLayoutJson(g_panelLayout);
			if (payload == g_lastPanelLayoutJson) {
				return;
			}

			g_lastPanelLayoutJson = payload;
			g_api->InteropCall(g_view, "setPanelLayout", g_lastPanelLayoutJson.c_str());
		}

		void PushTooltipLayoutToJs()
		{
			if (!IsViewReady() || !g_tooltipLayout.valid) {
				return;
			}

			const std::string payload = PrismaLayoutPersistence::EncodeTooltipLayoutJsonForJs(g_tooltipLayout);
			if (payload == g_lastTooltipLayoutJson) {
				return;
			}

			g_lastTooltipLayoutJson = payload;
			g_api->InteropCall(g_view, "setTooltipLayout", g_lastTooltipLayoutJson.c_str());
		}
