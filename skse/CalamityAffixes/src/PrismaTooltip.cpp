#include "CalamityAffixes/PrismaTooltip.h"

#include <atomic>
#include <cctype>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include <nlohmann/json.hpp>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/EventNames.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "CalamityAffixes/PrismaLayoutPersistence.h"
#include "CalamityAffixes/UserSettingsPersistence.h"

#include "PrismaUI_API.h"

namespace CalamityAffixes::PrismaTooltip
{
	namespace
	{
		constexpr auto kPollInterval = std::chrono::milliseconds(300);
		constexpr auto kHotkeyRefreshInterval = std::chrono::seconds(2);
		constexpr auto kViewPath = "CalamityAffixes/index.html";
		constexpr auto kViewOrder = 5000;
		constexpr auto kMouseButtonOffset = 256u;
		constexpr std::uint32_t kFallbackPanelHotkey = RE::BSKeyboardDevice::Keys::kF11;
		constexpr int kDefaultUiLanguageMode = 2;  // 0=en, 1=ko, 2=both
		constexpr auto kPrismaCommandListener = "calamityCommand";
		constexpr auto kInteropSetTooltip = "setTooltip";
		constexpr auto kInteropSetControlPanel = "setControlPanel";
		constexpr auto kInteropSetActionFeedback = "setActionFeedback";
		constexpr auto kInteropSetPanelHotkeyText = "setPanelHotkeyText";
		constexpr auto kInteropSetUiLanguage = "setUiLanguage";
		constexpr auto kInteropSetSelectedItemName = "setSelectedItemName";
		constexpr auto kInteropSetSelectedItemSource = "setSelectedItemSource";
		constexpr auto kInteropSetInventoryItems = "setInventoryItems";
		constexpr auto kInteropSetRecipeItems = "setRecipeItems";
		constexpr auto kInteropSetRunewordPanelState = "setRunewordPanelState";
		constexpr auto kInteropSetRunewordAffixPreview = "setRunewordAffixPreview";
		constexpr auto kInteropSetPanelLayout = "setPanelLayout";
		constexpr auto kInteropSetTooltipLayout = "setTooltipLayout";

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
		constexpr std::string_view kUserSettingsPath = "Data/SKSE/Plugins/CalamityAffixes/user_settings.json";
		constexpr int kDefaultTooltipRight = 70;
		constexpr int kDefaultTooltipTop = 255;
		constexpr int kDefaultTooltipFontPermille = 1000;
		constexpr int kMinTooltipFontPermille = 700;
		constexpr int kMaxTooltipFontPermille = 1800;

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
		std::jthread g_worker;

		bool g_lastPanelStateKnown = false;
		bool g_lastPanelState = false;

		using PanelLayout = PrismaLayoutPersistence::PanelLayout;
		using TooltipLayout = PrismaLayoutPersistence::TooltipLayout;

		struct PrismaViewCacheState
		{
			std::string selectedItemName{};
			std::string selectedItemSource{};
			std::string panelHotkeyText{};
			std::string uiLanguageCode{};
		};

		struct PrismaRunewordCacheState
		{
			std::string baseListJson = "[]";
			std::string recipeListJson = "[]";
			std::string panelStateJson = "{}";
			std::string affixPreview{};
		};

		template <class LayoutT>
		struct PrismaLayoutCacheState
		{
			LayoutT value{};
			bool loaded = false;
			bool needsPush = false;
			std::string lastJson = "{}";
		};

		PrismaViewCacheState g_viewCache{};
		PrismaRunewordCacheState g_runewordCache{};
		PrismaLayoutCacheState<PanelLayout> g_panelLayoutState{};
		PrismaLayoutCacheState<TooltipLayout> g_tooltipLayoutState{};

		struct PrismaSettingsRefreshState
		{
			std::mutex lock;
			std::chrono::steady_clock::time_point nextRefreshAt{};
			std::filesystem::file_time_type sourceMtime{};
			std::filesystem::path sourcePath{};
			bool hasSourceSnapshot = false;
		};

		struct SelectedItemContext;

		PrismaSettingsRefreshState g_panelHotkeyRefresh{};
		PrismaSettingsRefreshState g_uiLanguageRefresh{};

		struct PrismaTelemetryCounters
		{
			std::atomic<std::uint64_t> workerEnqueues{ 0u };
			std::atomic<std::uint64_t> tickRuns{ 0u };
			std::atomic<std::uint64_t> tooltipClears{ 0u };
			std::atomic<std::uint64_t> tooltipPushes{ 0u };
			std::atomic<std::uint64_t> selectedContextRefreshes{ 0u };
			std::atomic<std::uint64_t> panelRefreshes{ 0u };
			std::atomic<std::uint64_t> uiCommands{ 0u };
		};

		PrismaTelemetryCounters g_prismaTelemetry{};
		std::chrono::steady_clock::time_point g_nextTelemetryLogAt{};

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
		[[nodiscard]] bool TryReadPanelHotkeyFromMcmJson(std::istream& a_in, std::uint32_t& a_outResolvedKey);
		[[nodiscard]] bool TryResolveUiLanguageSettingsPath(std::filesystem::path& a_outPath);
		[[nodiscard]] bool TryReadUiLanguageModeFromIni(std::istream& a_in, int& a_outResolvedMode);
		[[nodiscard]] std::optional<PanelLayout> TryLoadPanelLayoutStateFromDisk();
		[[nodiscard]] std::optional<TooltipLayout> TryLoadTooltipLayoutStateFromDisk();
		void SetRunewordBaseInventoryList(const std::vector<RunewordBaseInventoryEntry>& a_entries);
		void SetRunewordRecipeList(const std::vector<RunewordRecipeEntry>& a_entries);
		void SetRunewordAffixPreview(std::string_view a_text);
		void SetRunewordPanelState(const RunewordPanelState& a_state);
		[[nodiscard]] bool TryReadSelectedItemContextFromMenus(
			SelectedItemContext& a_result,
			RE::UI& a_ui,
			CalamityAffixes::EventBridge& a_bridge);
		void ClearSelectedTooltipFromView(bool a_force, bool a_selectionChanged);
		bool PushSelectedTooltipSnapshot(bool a_force = false);
		void ClearTooltipViewState(bool a_clearRunewordPanelData);
		void RefreshRunewordPanelBindings(CalamityAffixes::EventBridge& a_bridge);
		void MaybeLogPrismaTelemetry(std::chrono::steady_clock::time_point a_now, bool a_uiActive);
		[[nodiscard]] bool HandleStructuredUiCommand(std::string_view a_command);
		void HandleEventBackedUiCommand(std::string_view a_command);
		[[nodiscard]] bool EnsurePrismaApiAvailable();
		void PrepareForNewPrismaView();
		void RegisterPrismaCommandListener();
		void QueueCachedLayoutStateForNewView();
		void CreatePrismaView();
		void Tick();
		void RegisterPrismaEventSinks();
		void StartPrismaWorker();
		[[nodiscard]] std::uint32_t ResolveConfiguredControlPanelHotkey() noexcept;
		[[nodiscard]] bool ShouldToggleControlPanelForButton(
			const RE::ButtonEvent* a_event,
			std::uint32_t a_configuredHotkey) noexcept;
		void QueueToggleControlPanelTask();
		[[nodiscard]] bool IsRelevantMenuEvent(const RE::MenuOpenCloseEvent& a_event) noexcept;
		void HandleRelevantMenuOpened();
		[[nodiscard]] int ResolveRelevantMenuCountAfterClose() noexcept;
		void SyncViewStateAfterRelevantMenusClosed(int a_openCount);

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
			g_api->InteropCall(g_view, kInteropSetControlPanel, value);

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
			if (itemName == g_viewCache.selectedItemName && itemSource == g_viewCache.selectedItemSource) {
				return;
			}

			g_viewCache.selectedItemName = itemName;
			g_viewCache.selectedItemSource = itemSource;
			g_prismaTelemetry.selectedContextRefreshes.fetch_add(1u, std::memory_order_relaxed);
			g_api->InteropCall(g_view, kInteropSetSelectedItemName, g_viewCache.selectedItemName.c_str());
			g_api->InteropCall(g_view, kInteropSetSelectedItemSource, g_viewCache.selectedItemSource.c_str());
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
			g_api->InteropCall(g_view, kInteropSetActionFeedback, message.c_str());
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
				g_prismaTelemetry.workerEnqueues.fetch_add(1u, std::memory_order_relaxed);
				tasks->AddTask(std::move(a_task));
			}
		}

		void ClearTooltipViewState(bool a_clearRunewordPanelData)
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
				g_prismaTelemetry.tooltipClears.fetch_add(1u, std::memory_order_relaxed);
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

		void RefreshRunewordPanelBindings(CalamityAffixes::EventBridge& a_bridge)
		{
			g_prismaTelemetry.panelRefreshes.fetch_add(1u, std::memory_order_relaxed);
			SetRunewordBaseInventoryList(a_bridge.GetRunewordBaseInventoryEntries());
			SetRunewordRecipeList(a_bridge.GetRunewordRecipeEntries());
			SetRunewordPanelState(a_bridge.GetRunewordPanelState());
			SetRunewordAffixPreview(a_bridge.GetSelectedRunewordBaseAffixTooltip(g_uiLanguageMode.load()).value_or(""));
		}

		void MaybeLogPrismaTelemetry(std::chrono::steady_clock::time_point a_now, bool a_uiActive)
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

		#include "PrismaTooltip.SettingsLayout.inl"

		#include "PrismaTooltip.MenuInput.inl"

		#include "PrismaTooltip.PanelData.inl"

		#include "PrismaTooltip.HandleUiCommand.inl"

		#include "PrismaTooltip.ViewLifecycle.inl"

		#include "PrismaTooltip.Tick.inl"

	}

	void Install()
	{
		bool expected = false;
		if (!g_installed.compare_exchange_strong(expected, true)) {
			return;
		}

		RegisterPrismaEventSinks();
		RefreshControlPanelHotkeyFromMcm(true);
		RefreshUiLanguageFromMcm(true);
		LoadPanelLayoutFromDisk();
		StartPrismaWorker();

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
