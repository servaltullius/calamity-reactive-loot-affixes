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
		#include "PrismaTooltip.SharedState.inl"

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
		void QueueTask(std::function<void()> a_task);
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

		#include "PrismaTooltip.SettingsLayout.inl"

		#include "PrismaTooltip.MenuInput.inl"

		#include "PrismaTooltip.ViewState.inl"

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
