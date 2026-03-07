		void PushCachedInteropPayload(std::string& a_cache, const char* a_method, std::string a_payload)
		{
			if (!IsViewReady()) {
				return;
			}

			if (a_payload == a_cache) {
				return;
			}

			a_cache = std::move(a_payload);
			g_api->InteropCall(g_view, a_method, a_cache.c_str());
		}

		[[nodiscard]] nlohmann::json BuildRunewordBaseInventoryPayload(
			const std::vector<RunewordBaseInventoryEntry>& a_entries)
		{
			nlohmann::json payload = nlohmann::json::array();
			for (const auto& entry : a_entries) {
				payload.push_back({
					{ "key", std::to_string(entry.instanceKey) },
					{ "name", entry.displayName },
					{ "selected", entry.selected }
				});
			}

			return payload;
		}

		void SetRunewordBaseInventoryList(const std::vector<RunewordBaseInventoryEntry>& a_entries)
		{
			PushCachedInteropPayload(
				g_runewordCache.baseListJson,
				kInteropSetInventoryItems,
				BuildRunewordBaseInventoryPayload(a_entries).dump());
		}

		[[nodiscard]] nlohmann::json BuildRunewordRecipePayload(
			const std::vector<RunewordRecipeEntry>& a_entries)
		{
			nlohmann::json payload = nlohmann::json::array();
			for (const auto& entry : a_entries) {
				payload.push_back({
					{ "token", std::to_string(entry.recipeToken) },
					{ "name", entry.displayName },
					{ "runes", entry.runeSequence },
					{ "summaryKey", entry.effectSummaryKey },
					{ "summary", entry.effectSummaryText },
					{ "detail", entry.effectDetailText },
					{ "baseKey", entry.recommendedBaseKey },
					{ "selected", entry.selected }
				});
			}

			return payload;
		}

		void SetRunewordRecipeList(const std::vector<RunewordRecipeEntry>& a_entries)
		{
			PushCachedInteropPayload(
				g_runewordCache.recipeListJson,
				kInteropSetRecipeItems,
				BuildRunewordRecipePayload(a_entries).dump());
		}

		void SetRunewordAffixPreview(std::string_view a_text)
		{
			PushCachedInteropPayload(
				g_runewordCache.affixPreview,
				kInteropSetRunewordAffixPreview,
				std::string(a_text));
		}

		[[nodiscard]] nlohmann::json BuildRunewordPanelStatePayload(const RunewordPanelState& a_state)
		{
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

			return payload;
		}

		void SetRunewordPanelState(const RunewordPanelState& a_state)
		{
			PushCachedInteropPayload(
				g_runewordCache.panelStateJson,
				kInteropSetRunewordPanelState,
				BuildRunewordPanelStatePayload(a_state).dump());
		}

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

		[[nodiscard]] RE::FormID ResolveRefHandleToFormID(RE::RefHandle a_handle)
		{
			if (a_handle == 0) {
				return 0u;
			}
			if (auto ref = RE::TESObjectREFR::LookupByHandle(a_handle); ref) {
				return ref->GetFormID();
			}
			return 0u;
		}

		[[nodiscard]] bool TryPopulateSelectedItemContext(
			SelectedItemContext& a_result,
			RE::ItemList* a_itemList,
			std::string_view a_source,
			RE::FormID a_sourceContainerFormID,
			CalamityAffixes::EventBridge& a_bridge)
		{
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
			a_result.hasSelection = true;
			a_result.itemName = std::move(selectedName);
			a_result.itemSource = std::string(a_source);
			a_result.tooltip = a_bridge.GetInstanceAffixTooltip(
				item->data.objDesc,
				a_result.itemName,
				g_uiLanguageMode.load(),
				a_result.itemSource,
				a_sourceContainerFormID);
			return true;
		}

		[[nodiscard]] bool TryReadInventorySelectedItemContext(
			SelectedItemContext& a_result,
			RE::UI& a_ui,
			CalamityAffixes::EventBridge& a_bridge)
		{
			if (!a_ui.IsMenuOpen(kMenuInventory)) {
				return false;
			}

			auto menu = a_ui.GetMenu<RE::InventoryMenu>();
			if (!menu) {
				return false;
			}

			auto& data = menu->GetRuntimeData();
			return TryPopulateSelectedItemContext(a_result, data.itemList, kItemSourceInventory, 0u, a_bridge);
		}

		[[nodiscard]] bool TryReadBarterSelectedItemContext(
			SelectedItemContext& a_result,
			RE::UI& a_ui,
			CalamityAffixes::EventBridge& a_bridge)
		{
			if (!a_ui.IsMenuOpen(kMenuBarter)) {
				return false;
			}

			auto menu = a_ui.GetMenu<RE::BarterMenu>();
			if (!menu) {
				return false;
			}

			auto& data = menu->GetRuntimeData();
			const auto sourceContainer = ResolveRefHandleToFormID(RE::BarterMenu::GetTargetRefHandle());
			return TryPopulateSelectedItemContext(a_result, data.itemList, kItemSourceBarter, sourceContainer, a_bridge);
		}

		[[nodiscard]] bool TryReadContainerSelectedItemContext(
			SelectedItemContext& a_result,
			RE::UI& a_ui,
			CalamityAffixes::EventBridge& a_bridge)
		{
			if (!a_ui.IsMenuOpen(kMenuContainer)) {
				return false;
			}

			auto menu = a_ui.GetMenu<RE::ContainerMenu>();
			if (!menu) {
				return false;
			}

			auto& data = menu->GetRuntimeData();
			const auto sourceContainer = ResolveRefHandleToFormID(RE::ContainerMenu::GetTargetRefHandle());
			return TryPopulateSelectedItemContext(a_result, data.itemList, kItemSourceContainer, sourceContainer, a_bridge);
		}

		[[nodiscard]] bool TryReadGiftSelectedItemContext(
			SelectedItemContext& a_result,
			RE::UI& a_ui,
			CalamityAffixes::EventBridge& a_bridge)
		{
			if (!a_ui.IsMenuOpen(kMenuGift)) {
				return false;
			}

			auto menu = a_ui.GetMenu<RE::GiftMenu>();
			if (!menu) {
				return false;
			}

			auto& data = menu->GetRuntimeData();
			const auto sourceContainer = ResolveRefHandleToFormID(RE::GiftMenu::GetTargetRefHandle());
			return TryPopulateSelectedItemContext(a_result, data.itemList, kItemSourceGift, sourceContainer, a_bridge);
		}

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

			if (TryReadSelectedItemContextFromMenus(result, *ui, *bridge)) {
				return result;
			}

			return result;
		}

		[[nodiscard]] bool TryReadSelectedItemContextFromMenus(
			SelectedItemContext& a_result,
			RE::UI& a_ui,
			CalamityAffixes::EventBridge& a_bridge)
		{
			for (const auto tryReadContext : {
					 &TryReadInventorySelectedItemContext,
					 &TryReadBarterSelectedItemContext,
					 &TryReadContainerSelectedItemContext,
					 &TryReadGiftSelectedItemContext }) {
				if (tryReadContext(a_result, a_ui, a_bridge)) {
					return true;
				}
			}

			return false;
		}

		[[nodiscard]] bool HasSelectedItemContextChanged(const SelectedItemContext& a_selected) noexcept
		{
			return
				a_selected.itemName != g_viewCache.selectedItemName ||
				a_selected.itemSource != g_viewCache.selectedItemSource;
		}

		[[nodiscard]] std::optional<std::string> BuildSelectedTooltipText(const SelectedItemContext& selected)
		{
			if (!selected.tooltip || selected.tooltip->empty()) {
				return std::nullopt;
			}

			const std::string next = StripRunewordOverlayTooltipLines(*selected.tooltip);
			if (next.empty()) {
				return std::nullopt;
			}

			return next;
		}

		void PushSelectedTooltipToView(std::string a_text)
		{
			g_lastTooltip = std::move(a_text);
			g_prismaTelemetry.tooltipPushes.fetch_add(1u, std::memory_order_relaxed);
			g_api->InteropCall(g_view, kInteropSetTooltip, g_lastTooltip.c_str());
		}

		void ClearSelectedTooltipFromView(bool a_force, bool a_selectionChanged)
		{
			if (!a_force && !a_selectionChanged && g_lastTooltip.empty()) {
				return;
			}

			if (!g_lastTooltip.empty()) {
				g_prismaTelemetry.tooltipClears.fetch_add(1u, std::memory_order_relaxed);
			}
			g_lastTooltip.clear();
			g_api->InteropCall(g_view, kInteropSetTooltip, "");
		}

		bool PushSelectedTooltipSnapshot(bool a_force /*= false*/)
		{
			if (!IsViewReady()) {
				return false;
			}

			const auto selected = GetSelectedItemContext();
			const bool selectionChanged = HasSelectedItemContextChanged(selected);
			SetSelectedItemContext(selected.itemName, selected.itemSource);

			const auto nextTooltip = BuildSelectedTooltipText(selected);
			if (!nextTooltip.has_value()) {
				ClearSelectedTooltipFromView(a_force, selectionChanged);
				return false;
			}

			const auto& next = *nextTooltip;
			if (a_force || selectionChanged || next != g_lastTooltip) {
				PushSelectedTooltipToView(next);
			}
			return true;
		}
