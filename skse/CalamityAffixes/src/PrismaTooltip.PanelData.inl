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
					{ "summary", entry.effectSummaryText },
					{ "detail", entry.effectDetailText },
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
