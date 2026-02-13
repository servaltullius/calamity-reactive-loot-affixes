#include "CalamityAffixes/PrismaTooltip.h"

#include <array>
#include <atomic>
#include <cctype>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include <nlohmann/json.hpp>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include "CalamityAffixes/EventBridge.h"

#include "PrismaUI_API.h"

namespace CalamityAffixes::PrismaTooltip
{
	namespace
	{
		constexpr auto kPollInterval = std::chrono::milliseconds(200);
		constexpr auto kHotkeyRefreshInterval = std::chrono::seconds(1);
		constexpr auto kViewPath = "CalamityAffixes/index.html";
		constexpr auto kViewOrder = 5000;
		constexpr auto kMouseButtonOffset = 256u;
		constexpr std::uint32_t kFallbackPanelHotkey = RE::BSKeyboardDevice::Keys::kF11;
		constexpr int kDefaultUiLanguageMode = 2;  // 0=en, 1=ko, 2=both

		constexpr std::string_view kMenuInventory = "InventoryMenu";
		constexpr std::string_view kMcmKeybindSettingsPath = "Data/MCM/Settings/keybinds.json";
		constexpr std::string_view kMcmModSettingsPath = "Data/MCM/Settings/CalamityAffixes.ini";
		constexpr std::string_view kMcmModDefaultSettingsPath = "Data/MCM/Config/CalamityAffixes/settings.ini";
		constexpr std::string_view kMcmModName = "CalamityAffixes";
		constexpr std::string_view kPanelToggleKeybindId = "prisma_panel_toggle";
		constexpr std::string_view kMcmGeneralSectionLower = "general";
		constexpr std::string_view kMcmUiLanguageKeyLower = "iuilanguage";
		constexpr std::string_view kRunewordBaseSelectPrefix = "runeword.base.select:";
		constexpr std::string_view kRunewordRecipeSelectPrefix = "runeword.recipe.select:";
		constexpr std::string_view kPanelLayoutSavePrefix = "ui.layout.save:";
		constexpr std::string_view kItemSourceInventory = "inventory";
		constexpr std::string_view kPanelLayoutPath = "Data/SKSE/Plugins/CalamityAffixes/prisma_panel_layout.json";

		constexpr std::string_view kManualModeCycleNextEvent = "CalamityAffixes_ModeCycle_Next";
		constexpr std::string_view kManualModeCyclePrevEvent = "CalamityAffixes_ModeCycle_Prev";
		constexpr std::string_view kRunewordBaseNextEvent = "CalamityAffixes_Runeword_Base_Next";
		constexpr std::string_view kRunewordBasePrevEvent = "CalamityAffixes_Runeword_Base_Prev";
		constexpr std::string_view kRunewordRecipeNextEvent = "CalamityAffixes_Runeword_Recipe_Next";
		constexpr std::string_view kRunewordRecipePrevEvent = "CalamityAffixes_Runeword_Recipe_Prev";
		constexpr std::string_view kRunewordInsertRuneEvent = "CalamityAffixes_Runeword_InsertRune";
		constexpr std::string_view kRunewordStatusEvent = "CalamityAffixes_Runeword_Status";
		constexpr std::string_view kRunewordGrantNextRuneEvent = "CalamityAffixes_Runeword_GrantNextRune";
		constexpr std::string_view kRunewordGrantRecipeSetEvent = "CalamityAffixes_Runeword_GrantRecipeSet";
		constexpr std::string_view kMcmSpawnTestItemEvent = "CalamityAffixes_MCM_SpawnTestItem";

		std::atomic_bool g_installed{ false };
		std::atomic_bool g_domReady{ false };
		std::atomic_bool g_loggedMissingApi{ false };
		std::atomic_int g_relevantMenusOpen{ 0 };
		std::atomic_bool g_gatePollingOnMenus{ false };
		std::atomic_bool g_controlPanelOpen{ false };
		std::atomic_uint32_t g_controlPanelHotkey{ 0u };
		std::atomic_int g_uiLanguageMode{ kDefaultUiLanguageMode };

		PRISMA_UI_API::IVPrismaUI1* g_api = nullptr;
		PRISMA_UI_API::PrismaView g_view = 0;

		std::string g_lastTooltip;
		std::string g_lastSelectedItemName;
		std::string g_lastSelectedItemSource;
		std::string g_lastRunewordBaseListJson = "[]";
		std::string g_lastRunewordRecipeListJson = "[]";
		std::string g_lastRunewordPanelStateJson = "{}";
		std::string g_lastPanelLayoutJson = "{}";
		std::string g_lastPanelHotkeyText;
		std::string g_lastUiLanguageCode;
		std::jthread g_worker;

		bool g_lastPanelStateKnown = false;
		bool g_lastPanelState = false;
		std::chrono::steady_clock::time_point g_nextHotkeyRefreshAt{};
		std::chrono::steady_clock::time_point g_nextUiLanguageRefreshAt{};
		std::filesystem::file_time_type g_hotkeySettingsMtime{};
		std::filesystem::file_time_type g_uiLanguageSettingsMtime{};
		bool g_hotkeySettingsKnown = false;
		bool g_uiLanguageSettingsKnown = false;
		std::mutex g_hotkeyRefreshLock;
		std::filesystem::path g_uiLanguageSettingsPath;
		std::mutex g_uiLanguageRefreshLock;
		bool g_panelLayoutLoaded = false;
		bool g_panelLayoutNeedsPush = false;

		struct PanelLayout
		{
			int left{ 0 };
			int top{ 0 };
			int width{ 0 };
			int height{ 0 };
			bool valid{ false };
		};
		PanelLayout g_panelLayout{};

		void SetControlPanelOpenImpl(bool a_open);
		void HandleUiCommand(std::string_view a_command);
		void RefreshControlPanelHotkeyFromMcm(bool a_force = false);
		void RefreshUiLanguageFromMcm(bool a_force = false);
		void LoadPanelLayoutFromDisk();
		void SavePanelLayoutToDisk(const PanelLayout& a_layout);
		void PushPanelLayoutToJs();
		[[nodiscard]] bool ParsePanelLayout(std::string_view a_payload, PanelLayout& a_out);
		void Tick();

		[[nodiscard]] bool IsRelevantMenu(std::string_view a_name) noexcept
		{
			return a_name == kMenuInventory;
		}

		[[nodiscard]] bool IsViewReady() noexcept
		{
			return g_api && g_view && g_api->IsValid(g_view) && g_domReady.load();
		}

		[[nodiscard]] bool IsGameplayReady() noexcept
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !player->GetParentCell()) {
				return false;
			}

			auto* ui = RE::UI::GetSingleton();
			if (ui &&
				(ui->IsMenuOpen("MainMenu") ||
				 ui->IsMenuOpen("LoadingMenu") ||
				 ui->IsMenuOpen("RaceSex Menu"))) {
				return false;
			}

			return true;
		}

		void SetVisible(bool a_visible)
		{
			if (!g_api || !g_view || !g_api->IsValid(g_view)) {
				return;
			}

			const bool hidden = g_api->IsHidden(g_view);
			if (a_visible) {
				if (hidden) {
					g_api->Show(g_view);
				}
			} else {
				if (!hidden) {
					g_api->Hide(g_view);
				}
			}
		}

		void SetControlPanelJsState(bool a_open)
		{
			if (!IsViewReady()) {
				return;
			}

			const char* value = a_open ? "1" : "0";
			g_api->InteropCall(g_view, "setControlPanel", value);

			// Fallback path if JS interop registration failed for any reason.
			g_api->Invoke(g_view, a_open
				? "if (window.setControlPanel) { window.setControlPanel('1'); }"
				: "if (window.setControlPanel) { window.setControlPanel('0'); }");
		}

		void SetSelectedItemContext(std::string_view a_itemName, std::string_view a_itemSource)
		{
			if (!IsViewReady()) {
				return;
			}

			const std::string itemName(a_itemName);
			const std::string itemSource(a_itemSource);
			if (itemName == g_lastSelectedItemName && itemSource == g_lastSelectedItemSource) {
				return;
			}

			g_lastSelectedItemName = itemName;
			g_lastSelectedItemSource = itemSource;
			g_api->InteropCall(g_view, "setSelectedItemName", g_lastSelectedItemName.c_str());
			g_api->InteropCall(g_view, "setSelectedItemSource", g_lastSelectedItemSource.c_str());
		}

		void SetRunewordBaseInventoryList(const std::vector<EventBridge::RunewordBaseInventoryEntry>& a_entries)
		{
			if (!IsViewReady()) {
				return;
			}

			nlohmann::json payload = nlohmann::json::array();
			for (const auto& entry : a_entries) {
				payload.push_back({
					{ "key", std::to_string(entry.instanceKey) },
					{ "name", entry.displayName },
					{ "selected", entry.selected }
				});
			}

			const std::string encoded = payload.dump();
			if (encoded == g_lastRunewordBaseListJson) {
				return;
			}

			g_lastRunewordBaseListJson = encoded;
			g_api->InteropCall(g_view, "setInventoryItems", g_lastRunewordBaseListJson.c_str());
		}

		void SetRunewordRecipeList(const std::vector<EventBridge::RunewordRecipeEntry>& a_entries)
		{
			if (!IsViewReady()) {
				return;
			}

			nlohmann::json payload = nlohmann::json::array();
			for (const auto& entry : a_entries) {
				payload.push_back({
					{ "token", std::to_string(entry.recipeToken) },
					{ "name", entry.displayName },
					{ "runes", entry.runeSequence },
					{ "summaryKey", entry.effectSummaryKey },
					{ "baseKey", entry.recommendedBaseKey },
					{ "selected", entry.selected }
				});
			}

			const std::string encoded = payload.dump();
			if (encoded == g_lastRunewordRecipeListJson) {
				return;
			}

			g_lastRunewordRecipeListJson = encoded;
			g_api->InteropCall(g_view, "setRecipeItems", g_lastRunewordRecipeListJson.c_str());
		}

			void SetRunewordPanelState(const EventBridge::RunewordPanelState& a_state)
			{
				if (!IsViewReady()) {
					return;
				}

				nlohmann::json requiredRunes = nlohmann::json::array();
				for (const auto& req : a_state.requiredRunes) {
					requiredRunes.push_back({
						{ "name", req.runeName },
						{ "required", req.required },
						{ "owned", req.owned },
					});
				}

				const nlohmann::json payload{
					{ "hasBase", a_state.hasBase },
					{ "hasRecipe", a_state.hasRecipe },
					{ "isComplete", a_state.isComplete },
					{ "recipeName", a_state.recipeName },
					{ "insertedRunes", a_state.insertedRunes },
					{ "totalRunes", a_state.totalRunes },
					{ "nextRuneName", a_state.nextRuneName },
					{ "nextRuneOwned", a_state.nextRuneOwned },
					{ "canInsert", a_state.canInsert },
					{ "missingSummary", a_state.missingSummary },
					{ "requiredRunes", requiredRunes }
				};

			const std::string encoded = payload.dump();
			if (encoded == g_lastRunewordPanelStateJson) {
				return;
			}

			g_lastRunewordPanelStateJson = encoded;
			g_api->InteropCall(g_view, "setRunewordPanelState", g_lastRunewordPanelStateJson.c_str());
		}

		void ApplyControlPanelState(bool a_open)
		{
			if (!IsViewReady()) {
				return;
			}

			if (!IsGameplayReady()) {
				SetControlPanelJsState(false);
				if (g_api->HasFocus(g_view)) {
					g_api->Unfocus(g_view);
				}
				SetVisible(false);
				return;
			}

			SetControlPanelJsState(a_open);
			if (a_open) {
				SetVisible(true);
				if (!g_api->HasFocus(g_view)) {
					// Always use FocusMenu so the panel is modal (prevents click-through to InventoryMenu).
					g_api->Focus(g_view, false, false);
				}
			} else {
				if (g_api->HasFocus(g_view)) {
					g_api->Unfocus(g_view);
				}
				if (g_lastTooltip.empty()) {
					SetVisible(false);
				}
			}
		}

		void PushUiFeedback(std::string_view a_message)
		{
			if (!IsViewReady()) {
				return;
			}

			const std::string message(a_message);
			g_api->InteropCall(g_view, "setActionFeedback", message.c_str());
		}

		[[nodiscard]] bool EmitModEvent(std::string_view a_eventName, std::string_view a_strArg = {}, float a_numArg = 0.0f)
		{
			auto* source = SKSE::GetModCallbackEventSource();
			if (!source) {
				return false;
			}

			const std::string eventName(a_eventName);
			const std::string strArg(a_strArg);
			SKSE::ModCallbackEvent event{
				RE::BSFixedString(eventName.c_str()),
				RE::BSFixedString(strArg.c_str()),
				a_numArg,
				nullptr
			};
			source->SendEvent(&event);
			return true;
		}

		void QueueTask(std::function<void()> a_task)
		{
			if (!a_task) {
				return;
			}

			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask(std::move(a_task));
			}
		}

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
				if (g_hotkeySettingsKnown) {
					g_hotkeySettingsKnown = false;
					g_controlPanelHotkey.store(0u);
				}
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
				return;
			}

			const auto previous = g_controlPanelHotkey.exchange(resolvedKey);
			g_hotkeySettingsKnown = true;
			g_hotkeySettingsMtime = mtime;

			if (previous != resolvedKey) {
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
				const auto previous = g_uiLanguageMode.exchange(kDefaultUiLanguageMode);
				g_uiLanguageSettingsKnown = false;
				g_uiLanguageSettingsPath.clear();
				if (previous != kDefaultUiLanguageMode) {
					SKSE::log::info("CalamityAffixes: UI language reset to default (both).");
				}
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
				SKSE::log::info("CalamityAffixes: UI language updated (mode={}).", resolvedMode);
			}
		}

		[[nodiscard]] bool ParsePanelLayout(std::string_view a_payload, PanelLayout& a_out)
		{
			auto trim = [](std::string_view& text) {
				while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
					text.remove_prefix(1);
				}
				while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
					text.remove_suffix(1);
				}
			};

			std::array<int, 4> values{};
			size_t cursor = 0;
			for (size_t i = 0; i < values.size(); ++i) {
				const size_t next = a_payload.find(',', cursor);
				if (next == std::string_view::npos && i < values.size() - 1) {
					return false;
				}

				std::string_view token =
					next == std::string_view::npos
						? a_payload.substr(cursor)
						: a_payload.substr(cursor, next - cursor);
				trim(token);
				if (token.empty()) {
					return false;
				}

				int parsed = 0;
				const auto* first = token.data();
				const auto* last = first + token.size();
				const auto [ptr, ec] = std::from_chars(first, last, parsed);
				if (ec != std::errc{} || ptr != last) {
					return false;
				}
				values[i] = parsed;

				if (next == std::string_view::npos) {
					cursor = a_payload.size();
				} else {
					cursor = next + 1;
				}
			}

			if (cursor < a_payload.size()) {
				std::string_view trailing = a_payload.substr(cursor);
				trim(trailing);
				if (!trailing.empty()) {
					return false;
				}
			}

			if (values[2] <= 0 || values[3] <= 0) {
				return false;
			}

			a_out.left = values[0];
			a_out.top = values[1];
			a_out.width = values[2];
			a_out.height = values[3];
			a_out.valid = true;
			return true;
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

			std::ifstream in(path);
			if (!in.good()) {
				return;
			}

			try {
				nlohmann::json j;
				in >> j;

				PanelLayout loaded{};
				loaded.left = j.value("left", 0);
				loaded.top = j.value("top", 0);
				loaded.width = j.value("width", 0);
				loaded.height = j.value("height", 0);
				loaded.valid = loaded.width > 0 && loaded.height > 0;
				if (!loaded.valid) {
					SKSE::log::warn("CalamityAffixes: invalid Prisma panel layout file at {}.", kPanelLayoutPath);
					return;
				}

				g_panelLayout = loaded;
				g_panelLayoutNeedsPush = true;
				g_lastPanelLayoutJson.clear();
				SKSE::log::info("CalamityAffixes: loaded Prisma panel layout from {}.", kPanelLayoutPath);
			} catch (const std::exception& e) {
				SKSE::log::warn("CalamityAffixes: failed to parse panel layout {} ({})", kPanelLayoutPath, e.what());
			}
		}

		void SavePanelLayoutToDisk(const PanelLayout& a_layout)
		{
			if (!a_layout.valid) {
				return;
			}

			const std::filesystem::path path{ std::string(kPanelLayoutPath) };
			std::error_code ec;
			std::filesystem::create_directories(path.parent_path(), ec);
			if (ec) {
				SKSE::log::warn("CalamityAffixes: failed to create layout directory for {} ({})", kPanelLayoutPath, ec.message());
				return;
			}

			const nlohmann::json j{
				{ "left", a_layout.left },
				{ "top", a_layout.top },
				{ "width", a_layout.width },
				{ "height", a_layout.height }
			};

			std::ofstream out(path, std::ios::trunc);
			if (!out.good()) {
				SKSE::log::warn("CalamityAffixes: failed to open panel layout file {} for write.", kPanelLayoutPath);
				return;
			}

			out << j.dump(2);
			if (!out.good()) {
				SKSE::log::warn("CalamityAffixes: failed to write panel layout file {}.", kPanelLayoutPath);
				return;
			}

			g_lastPanelLayoutJson = j.dump();
		}

		void PushPanelLayoutToJs()
		{
			if (!IsViewReady() || !g_panelLayout.valid) {
				return;
			}

			const nlohmann::json j{
				{ "left", g_panelLayout.left },
				{ "top", g_panelLayout.top },
				{ "width", g_panelLayout.width },
				{ "height", g_panelLayout.height }
			};
			const std::string payload = j.dump();
			if (payload == g_lastPanelLayoutJson) {
				return;
			}

			g_lastPanelLayoutJson = payload;
			g_api->InteropCall(g_view, "setPanelLayout", g_lastPanelLayoutJson.c_str());
		}

		class PanelHotkeyEventSink final : public RE::BSInputDeviceManager::Sink
		{
		public:
			using Event = RE::InputEvent*;

			RE::BSEventNotifyControl ProcessEvent(
				const Event* a_event,
				RE::BSTEventSource<Event>*) override
			{
				RefreshControlPanelHotkeyFromMcm(false);

				auto configuredHotkey = g_controlPanelHotkey.load();
				if (configuredHotkey == 0u) {
					// Fallback: allow opening the panel even if the user hasn't configured a keybind yet.
					configuredHotkey = kFallbackPanelHotkey;
				}
				if (configuredHotkey == 0u) {
					return RE::BSEventNotifyControl::kContinue;
				}

				const auto* inputEvent = a_event ? *a_event : nullptr;
				for (auto* current = inputEvent; current; current = current->next) {
					const auto* buttonEvent = current->AsButtonEvent();
					if (!buttonEvent || !buttonEvent->IsDown()) {
						continue;
					}

					const auto pressedKey = NormalizeButtonKeyCode(buttonEvent);
					if (pressedKey == 0u || pressedKey != configuredHotkey) {
						continue;
					}

					if (!IsGameplayReady()) {
						return RE::BSEventNotifyControl::kContinue;
					}

					QueueTask([]() {
						SetControlPanelOpenImpl(!g_controlPanelOpen.load());
					});
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
				if (!a_event) {
					return RE::BSEventNotifyControl::kContinue;
				}

				const char* raw = a_event->menuName.c_str();
				const std::string_view menuName = raw ? std::string_view(raw) : std::string_view{};
				if (!IsRelevantMenu(menuName)) {
					return RE::BSEventNotifyControl::kContinue;
				}

				if (a_event->opening) {
					g_relevantMenusOpen.fetch_add(1);
					return RE::BSEventNotifyControl::kContinue;
				}

				const int prev = g_relevantMenusOpen.fetch_sub(1);
				if (prev <= 1) {
					g_relevantMenusOpen.store(0);

					if (!g_controlPanelOpen.load() && IsViewReady()) {
						g_lastTooltip.clear();
						g_api->InteropCall(g_view, "setTooltip", "");
						SetVisible(false);
					}
				}

				return RE::BSEventNotifyControl::kContinue;
			}
		};

		MenuEventSink g_menuSink;
		PanelHotkeyEventSink g_hotkeySink;

		[[nodiscard]] std::string ResolveSelectedItemDisplayName(const RE::ItemList::Item* a_item)
		{
			if (!a_item) {
				return {};
			}

			std::string selectedName;

			if (a_item->obj.IsObject()) {
				RE::GFxValue value;
				if (a_item->obj.GetMember("name", &value) && value.IsString()) {
					const char* text = value.GetString();
					if (text && *text) {
						selectedName = text;
					}
				}

				if (selectedName.empty() && a_item->obj.GetMember("text", &value) && value.IsString()) {
					const char* text = value.GetString();
					if (text && *text) {
						selectedName = text;
					}
				}
			}

			if (selectedName.empty() && a_item->data.objDesc) {
				const char* text = a_item->data.objDesc->GetDisplayName();
				if (text && *text) {
					selectedName = text;
				}
			}

			return selectedName;
		}

		struct SelectedItemContext
		{
			std::string itemName{};
			std::string itemSource{};
			std::optional<std::string> tooltip{ std::nullopt };
			bool hasSelection{ false };
		};

		SelectedItemContext GetSelectedItemContext()
		{
			SelectedItemContext result;

			auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
			if (!bridge) {
				return result;
			}

			auto* ui = RE::UI::GetSingleton();
			if (!ui) {
				return result;
			}

			auto readItem = [&](RE::ItemList* a_itemList, std::string_view a_source) -> bool {
				if (!a_itemList) {
					return false;
				}

				auto* item = a_itemList->GetSelectedItem();
				if (!item) {
					return false;
				}

				auto selectedName = ResolveSelectedItemDisplayName(item);
				if (selectedName.empty()) {
					selectedName = "<Unknown>";
				}
				result.hasSelection = true;
				result.itemName = std::move(selectedName);
				result.itemSource = std::string(a_source);
				result.tooltip = bridge->GetInstanceAffixTooltip(item->data.objDesc, result.itemName, g_uiLanguageMode.load());
				return true;
			};

			if (auto menu = ui->GetMenu<RE::InventoryMenu>(); menu) {
				auto& data = menu->GetRuntimeData();
				if (readItem(data.itemList, kItemSourceInventory)) {
					return result;
				}
			}

			return result;
		}

		void EnsurePrisma()
		{
			LoadPanelLayoutFromDisk();

			if (g_api && g_view && g_api->IsValid(g_view)) {
				return;
			}

			if (!g_api) {
				auto* api = static_cast<PRISMA_UI_API::IVPrismaUI1*>(
					PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
				if (!api) {
					if (!g_loggedMissingApi.exchange(true)) {
						SKSE::log::error("CalamityAffixes: PrismaUI API not available. Is PrismaUI installed and loaded?");
					}
					return;
				}

				g_api = api;
				SKSE::log::info("CalamityAffixes: PrismaUI API acquired.");
			}

			if (!g_view) {
				g_domReady = false;
				g_lastPanelStateKnown = false;
				g_lastUiLanguageCode.clear();
				g_view = g_api->CreateView(
					kViewPath,
					[](PRISMA_UI_API::PrismaView) {
						g_domReady = true;
					});
				g_api->SetOrder(g_view, kViewOrder);
				g_api->Hide(g_view);

				g_api->RegisterJSListener(g_view, "calamityCommand", [](const char* a_argument) {
					const std::string command = a_argument ? std::string(a_argument) : std::string();
					QueueTask([command]() {
						HandleUiCommand(command);
					});
				});

				if (g_panelLayout.valid) {
					g_lastPanelLayoutJson.clear();
					g_panelLayoutNeedsPush = true;
				}

				SKSE::log::info("CalamityAffixes: PrismaUI view created (view={}, path={}).", g_view, kViewPath);
			}
		}

		void SetControlPanelOpenImpl(bool a_open)
		{
			g_controlPanelOpen.store(a_open);
			EnsurePrisma();
			ApplyControlPanelState(a_open);
			PushUiFeedback(a_open ? "Prisma panel opened." : "Prisma panel closed.");
		}

		void HandleUiCommand(std::string_view a_command)
		{
			if (a_command.empty()) {
				return;
			}

			if (a_command == "ui.toggle") {
				SetControlPanelOpenImpl(!g_controlPanelOpen.load());
				return;
			}
			if (a_command == "ui.open") {
				SetControlPanelOpenImpl(true);
				return;
			}
			if (a_command == "ui.close") {
				SetControlPanelOpenImpl(false);
				return;
			}

			if (a_command.rfind(kRunewordBaseSelectPrefix, 0) == 0) {
				const auto keyText = a_command.substr(kRunewordBaseSelectPrefix.size());
				std::uint64_t keyValue = 0;
				const auto* first = keyText.data();
				const auto* last = first + keyText.size();
				const auto [ptr, ec] = std::from_chars(first, last, keyValue);
				if (ec != std::errc{} || ptr != last || keyValue == 0u) {
					PushUiFeedback("Invalid base selection key.");
					return;
				}

				auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				if (!bridge) {
					PushUiFeedback("Runeword system unavailable.");
					return;
				}

				if (!bridge->SelectRunewordBase(keyValue)) {
					PushUiFeedback("Failed to select runeword base.");
					return;
				}

				SetRunewordBaseInventoryList(bridge->GetRunewordBaseInventoryEntries());
				PushUiFeedback("Runeword base selected.");
				return;
			}

			if (a_command.rfind(kRunewordRecipeSelectPrefix, 0) == 0) {
				const auto tokenText = a_command.substr(kRunewordRecipeSelectPrefix.size());
				std::uint64_t tokenValue = 0;
				const auto* first = tokenText.data();
				const auto* last = first + tokenText.size();
				const auto [ptr, ec] = std::from_chars(first, last, tokenValue);
				if (ec != std::errc{} || ptr != last || tokenValue == 0u) {
					PushUiFeedback("Invalid recipe selection key.");
					return;
				}

				auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				if (!bridge) {
					PushUiFeedback("Runeword system unavailable.");
					return;
				}

				if (!bridge->SelectRunewordRecipe(tokenValue)) {
					PushUiFeedback("Failed to select runeword recipe.");
					return;
				}

				SetRunewordRecipeList(bridge->GetRunewordRecipeEntries());
				PushUiFeedback("Runeword recipe selected.");
				return;
			}

			if (a_command.rfind(kPanelLayoutSavePrefix, 0) == 0) {
				PanelLayout parsed{};
				if (!ParsePanelLayout(a_command.substr(kPanelLayoutSavePrefix.size()), parsed)) {
					SKSE::log::warn("CalamityAffixes: invalid panel layout payload: {}", a_command);
					return;
				}

				g_panelLayout = parsed;
				g_panelLayoutLoaded = true;
				g_panelLayoutNeedsPush = false;
				SavePanelLayoutToDisk(g_panelLayout);
				return;
			}

			bool sent = false;
			std::string_view feedback = {};
			if (a_command == "mode.next") {
				sent = EmitModEvent(kManualModeCycleNextEvent);
				feedback = "Manual mode -> next";
			} else if (a_command == "mode.prev") {
				sent = EmitModEvent(kManualModeCyclePrevEvent);
				feedback = "Manual mode -> previous";
			} else if (a_command == "runeword.base.next") {
				sent = EmitModEvent(kRunewordBaseNextEvent);
				feedback = "Runeword base -> next";
			} else if (a_command == "runeword.base.prev") {
				sent = EmitModEvent(kRunewordBasePrevEvent);
				feedback = "Runeword base -> previous";
			} else if (a_command == "runeword.recipe.next") {
				sent = EmitModEvent(kRunewordRecipeNextEvent);
				feedback = "Runeword recipe -> next";
				} else if (a_command == "runeword.recipe.prev") {
					sent = EmitModEvent(kRunewordRecipePrevEvent);
					feedback = "Runeword recipe -> previous";
					} else if (a_command == "runeword.insert") {
						auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
						CalamityAffixes::EventBridge::RunewordPanelState before{};
						if (bridge) {
						before = bridge->GetRunewordPanelState();
					}

					sent = EmitModEvent(kRunewordInsertRuneEvent);

						if (sent && bridge) {
							// SendEvent is synchronous; refresh status immediately so UI feedback isn't misleading.
							const auto after = bridge->GetRunewordPanelState();
							SetRunewordPanelState(after);

							if (after.isComplete && !before.isComplete) {
								std::string msg = "Runeword: transmuted ";
								msg.append(after.recipeName.empty() ? "Runeword" : after.recipeName);
								PushUiFeedback(msg);
								return;
							}

							if (after.isComplete) {
								PushUiFeedback("Runeword: already complete");
								return;
							}

							if (!after.canInsert && after.hasRecipe) {
								if (!after.missingSummary.empty()) {
									std::string msg = "Runeword: missing fragments ";
									msg.append(after.missingSummary);
									PushUiFeedback(msg);
									return;
								}

								if (!after.nextRuneName.empty()) {
									std::string msg = "Runeword: missing fragment ";
									msg.append(after.nextRuneName);
									msg.append(" (owned ");
									msg.append(std::to_string(after.nextRuneOwned));
									msg.push_back(')');
									PushUiFeedback(msg);
									return;
								}
							}
						}

						feedback = "Runeword -> transmute requested";
					} else if (a_command == "runeword.status") {
						sent = EmitModEvent(kRunewordStatusEvent);
						feedback = "Runeword -> status";
				} else if (a_command == "runeword.grant.next") {
				sent = EmitModEvent(kRunewordGrantNextRuneEvent, {}, 1.0f);
				feedback = "Debug -> +1 next fragment";
			} else if (a_command == "runeword.grant.set") {
				sent = EmitModEvent(kRunewordGrantRecipeSetEvent, {}, 1.0f);
				feedback = "Debug -> +1 recipe set";
			} else if (a_command == "spawn.test") {
				sent = EmitModEvent(kMcmSpawnTestItemEvent, {}, 1.0f);
				feedback = "Spawned test item request";
			} else {
				std::string note = "Unknown command: ";
				note.append(a_command);
				PushUiFeedback(note);
				return;
			}

			if (!sent) {
				PushUiFeedback("Failed to send command event.");
				return;
			}

			PushUiFeedback(feedback);
		}

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
		}
	}

	void Install()
	{
		bool expected = false;
		if (!g_installed.compare_exchange_strong(expected, true)) {
			return;
		}

		if (auto* ui = RE::UI::GetSingleton()) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(&g_menuSink);
			g_gatePollingOnMenus = true;
		}
		if (auto* input = RE::BSInputDeviceManager::GetSingleton()) {
			input->AddEventSink(&g_hotkeySink);
		}
		RefreshControlPanelHotkeyFromMcm(true);
		RefreshUiLanguageFromMcm(true);
		LoadPanelLayoutFromDisk();

		g_worker = std::jthread([](std::stop_token stopToken) {
			while (!stopToken.stop_requested()) {
				std::this_thread::sleep_for(kPollInterval);

				if (g_gatePollingOnMenus.load() &&
					g_relevantMenusOpen.load() <= 0 &&
					!g_controlPanelOpen.load()) {
					continue;
				}

				if (auto* tasks = SKSE::GetTaskInterface()) {
					tasks->AddTask([]() {
						Tick();
					});
				}
			}
		});

		SKSE::log::info("CalamityAffixes: Prisma tooltip/control overlay enabled (poll={}ms).", kPollInterval.count());
	}

	bool IsAvailable()
	{
		if (g_api) {
			return true;
		}

		auto* api = static_cast<PRISMA_UI_API::IVPrismaUI1*>(
			PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
		if (!api) {
			return false;
		}

		g_api = api;
		return true;
	}

	void SetControlPanelOpen(bool a_open)
	{
		QueueTask([a_open]() {
			SetControlPanelOpenImpl(a_open);
		});
	}

	void ToggleControlPanel()
	{
		QueueTask([]() {
			SetControlPanelOpenImpl(!g_controlPanelOpen.load());
		});
	}
}
