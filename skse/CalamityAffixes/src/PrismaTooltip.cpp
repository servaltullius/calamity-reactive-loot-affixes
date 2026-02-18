#include "CalamityAffixes/PrismaTooltip.h"

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
#include "CalamityAffixes/LootUiGuards.h"
#include "CalamityAffixes/PrismaLayoutPersistence.h"

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

		constexpr std::string_view kMenuInventory = RE::InventoryMenu::MENU_NAME;
		constexpr std::string_view kMenuBarter = RE::BarterMenu::MENU_NAME;
		constexpr std::string_view kMenuContainer = RE::ContainerMenu::MENU_NAME;
		constexpr std::string_view kMenuGift = RE::GiftMenu::MENU_NAME;
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
		constexpr std::string_view kTooltipLayoutSavePrefix = "ui.tooltip.save:";
		constexpr std::string_view kItemSourceInventory = "inventory";
		constexpr std::string_view kItemSourceBarter = "barter";
		constexpr std::string_view kItemSourceContainer = "container";
		constexpr std::string_view kItemSourceGift = "gift";
		constexpr std::string_view kPanelLayoutPath = "Data/SKSE/Plugins/CalamityAffixes/prisma_panel_layout.json";
		constexpr std::string_view kTooltipLayoutPath = "Data/SKSE/Plugins/CalamityAffixes/prisma_tooltip_layout.json";
		constexpr int kDefaultTooltipRight = 70;
		constexpr int kDefaultTooltipTop = 255;
		constexpr int kDefaultTooltipFontPermille = 1000;
		constexpr int kMinTooltipFontPermille = 700;
		constexpr int kMaxTooltipFontPermille = 1800;

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
		constexpr std::string_view kRunewordGrantStarterOrbsEvent = "CalamityAffixes_Runeword_GrantStarterOrbs";
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
		std::string g_lastRunewordAffixPreview;
		std::string g_lastPanelLayoutJson = "{}";
		std::string g_lastTooltipLayoutJson = "{}";
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
		bool g_tooltipLayoutLoaded = false;
		bool g_tooltipLayoutNeedsPush = false;

		using PanelLayout = PrismaLayoutPersistence::PanelLayout;
		using TooltipLayout = PrismaLayoutPersistence::TooltipLayout;

		PanelLayout g_panelLayout{};

		TooltipLayout g_tooltipLayout{};

		void SetControlPanelOpenImpl(bool a_open);
		void HandleUiCommand(std::string_view a_command);
		void RefreshControlPanelHotkeyFromMcm(bool a_force = false);
		void RefreshUiLanguageFromMcm(bool a_force = false);
		void LoadPanelLayoutFromDisk();
		void LoadTooltipLayoutFromDisk();
		void SavePanelLayoutToDisk(const PanelLayout& a_layout);
		void SaveTooltipLayoutToDisk(const TooltipLayout& a_layout);
		void PushPanelLayoutToJs();
		void PushTooltipLayoutToJs();
		bool PushSelectedTooltipSnapshot(bool a_force = false);
		void Tick();

		[[nodiscard]] bool IsRelevantMenu(std::string_view a_name) noexcept
		{
			return CalamityAffixes::IsPrismaTooltipRelevantMenu(a_name);
		}

		[[nodiscard]] int CountRelevantMenusOpenFromUi() noexcept
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui) {
				return -1;
			}

			int count = 0;
			if (ui->IsMenuOpen(kMenuInventory)) {
				++count;
			}
			if (ui->IsMenuOpen(kMenuBarter)) {
				++count;
			}
			if (ui->IsMenuOpen(kMenuContainer)) {
				++count;
			}
			if (ui->IsMenuOpen(kMenuGift)) {
				++count;
			}
			return count;
		}

		[[nodiscard]] std::string StripRunewordOverlayTooltipLines(std::string_view a_tooltip)
		{
			if (a_tooltip.empty() || ShouldShowRunewordTooltipInItemOverlay()) {
				return std::string(a_tooltip);
			}

			std::string filtered;
			filtered.reserve(a_tooltip.size());
			std::size_t start = 0;
			while (start <= a_tooltip.size()) {
				const auto end = a_tooltip.find('\n', start);
				const auto lineEnd = end == std::string_view::npos ? a_tooltip.size() : end;
				const auto line = a_tooltip.substr(start, lineEnd - start);
				if (!IsRunewordOverlayTooltipLine(line)) {
					if (!filtered.empty()) {
						filtered.push_back('\n');
					}
					filtered.append(line);
				}
				if (lineEnd == a_tooltip.size()) {
					break;
				}
				start = lineEnd + 1;
			}

			return filtered;
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

			void SetRunewordAffixPreview(std::string_view a_text)
			{
				if (!IsViewReady()) {
					return;
				}

				const std::string next(a_text);
				if (next == g_lastRunewordAffixPreview) {
					return;
				}

				g_lastRunewordAffixPreview = next;
				g_api->InteropCall(g_view, "setRunewordAffixPreview", g_lastRunewordAffixPreview.c_str());
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


		#include "PrismaTooltip.SettingsLayout.inl"


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
					const int openCount = CountRelevantMenusOpenFromUi();
					if (openCount > 0) {
						g_relevantMenusOpen.store(openCount);
					} else {
						// Opening events can race before UI::IsMenuOpen reflects the new state.
						// Fall back to event-based increment so polling doesn't stall at zero.
						g_relevantMenusOpen.fetch_add(1);
					}
					return RE::BSEventNotifyControl::kContinue;
				}

				int openCount = CountRelevantMenusOpenFromUi();
				if (openCount < 0) {
					const int prev = g_relevantMenusOpen.fetch_sub(1);
					openCount = std::max(0, prev - 1);
				}
				g_relevantMenusOpen.store(std::max(0, openCount));

				if (openCount <= 0) {
					g_relevantMenusOpen.store(0);

					if (!g_controlPanelOpen.load() && g_api && g_view && g_api->IsValid(g_view)) {
						// Keep cursor/focus state in sync with menu lifetime.
						// If focus remains after menu close, the cursor can linger until reopening a menu.
						if (g_api->HasFocus(g_view)) {
							g_api->Unfocus(g_view);
						}

						if (IsViewReady()) {
							g_lastTooltip.clear();
							g_api->InteropCall(g_view, "setTooltip", "");
							SetSelectedItemContext({}, {});
						}

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

			auto resolveRefHandleToFormID = [](RE::RefHandle a_handle) -> RE::FormID {
				if (a_handle == 0) {
					return 0u;
				}
				if (auto ref = RE::TESObjectREFR::LookupByHandle(a_handle); ref) {
					return ref->GetFormID();
				}
				return 0u;
			};

			auto readItem = [&](RE::ItemList* a_itemList, std::string_view a_source, RE::FormID a_sourceContainerFormID) -> bool {
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
				result.tooltip = bridge->GetInstanceAffixTooltip(
					item->data.objDesc,
					result.itemName,
					g_uiLanguageMode.load(),
					result.itemSource,
					a_sourceContainerFormID);
				return true;
			};

			if (auto menu = ui->GetMenu<RE::InventoryMenu>(); menu) {
				if (ui->IsMenuOpen(kMenuInventory)) {
					auto& data = menu->GetRuntimeData();
					if (readItem(data.itemList, kItemSourceInventory, 0u)) {
						return result;
					}
				}
			}
			if (auto menu = ui->GetMenu<RE::BarterMenu>(); menu) {
				if (ui->IsMenuOpen(kMenuBarter)) {
					auto& data = menu->GetRuntimeData();
					const auto sourceContainer = resolveRefHandleToFormID(RE::BarterMenu::GetTargetRefHandle());
					if (readItem(data.itemList, kItemSourceBarter, sourceContainer)) {
						return result;
					}
				}
			}
			if (auto menu = ui->GetMenu<RE::ContainerMenu>(); menu) {
				if (ui->IsMenuOpen(kMenuContainer)) {
					auto& data = menu->GetRuntimeData();
					const auto sourceContainer = resolveRefHandleToFormID(RE::ContainerMenu::GetTargetRefHandle());
					if (readItem(data.itemList, kItemSourceContainer, sourceContainer)) {
						return result;
					}
				}
			}
			if (auto menu = ui->GetMenu<RE::GiftMenu>(); menu) {
				if (ui->IsMenuOpen(kMenuGift)) {
					auto& data = menu->GetRuntimeData();
					const auto sourceContainer = resolveRefHandleToFormID(RE::GiftMenu::GetTargetRefHandle());
					if (readItem(data.itemList, kItemSourceGift, sourceContainer)) {
						return result;
					}
				}
			}

			return result;
		}

		bool PushSelectedTooltipSnapshot(bool a_force /*= false*/)
		{
			if (!IsViewReady()) {
				return false;
			}

			const auto selected = GetSelectedItemContext();
			const bool selectionChanged =
				selected.itemName != g_lastSelectedItemName ||
				selected.itemSource != g_lastSelectedItemSource;
			SetSelectedItemContext(selected.itemName, selected.itemSource);

			const auto clearTooltip = [&]() {
				if (a_force || selectionChanged || !g_lastTooltip.empty()) {
					g_lastTooltip.clear();
					g_api->InteropCall(g_view, "setTooltip", "");
				}
			};

			if (!selected.tooltip || selected.tooltip->empty()) {
				clearTooltip();
				return false;
			}

			const std::string next = StripRunewordOverlayTooltipLines(*selected.tooltip);
			if (next.empty()) {
				clearTooltip();
				return false;
			}

			if (a_force || selectionChanged || next != g_lastTooltip) {
				g_lastTooltip = next;
				g_api->InteropCall(g_view, "setTooltip", g_lastTooltip.c_str());
			}
			return true;
		}

		void EnsurePrisma()
		{
			LoadPanelLayoutFromDisk();
			LoadTooltipLayoutFromDisk();

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
					g_lastRunewordAffixPreview.clear();
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
				if (g_tooltipLayout.valid) {
					g_lastTooltipLayoutJson.clear();
					g_tooltipLayoutNeedsPush = true;
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


		#include "PrismaTooltip.HandleUiCommand.inl"

		#include "PrismaTooltip.Tick.inl"

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
