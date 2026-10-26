		class PrismaCommandController final
		{
		public:
			[[nodiscard]] bool HandleStructuredUiCommand(std::string_view a_command) const
			{
				return
					HandlePanelVisibilityCommand(a_command) ||
					HandleRunewordSelectionCommand(a_command) ||
					HandleLayoutSaveCommand(a_command) ||
					HandleImmediateRunewordCommand(a_command);
			}

			void HandleEventBackedUiCommand(std::string_view a_command) const
			{
				// These ride gameplay mod events, which EventBridge drops while the
				// runtime is off: say so instead of reporting a request that never runs.
				if (IsGameplayEventCommand(a_command) && IsRuntimeDisabled()) {
					PushUiFeedback(kRuntimeDisabledFeedback);
					return;
				}

				bool sent = false;
				std::string_view feedback = {};
				if (!DispatchEventBackedCommand(a_command, sent, feedback)) {
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

		private:
			static constexpr std::string_view kRuntimeDisabledFeedback = "Calamity is disabled in the MCM.";

			[[nodiscard]] static bool IsRuntimeDisabled()
			{
				const auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				return bridge && !bridge->IsRuntimeEnabled();
			}

			[[nodiscard]] static bool IsGameplayEventCommand(std::string_view a_command) noexcept
			{
				return a_command == "mode.next" || a_command == "mode.prev" ||
				       a_command == "runeword.base.next" || a_command == "runeword.base.prev" ||
				       a_command == "runeword.recipe.next" || a_command == "runeword.recipe.prev" ||
				       a_command == "runeword.status" ||
				       a_command == "runeword.grant.next" || a_command == "runeword.grant.set";
			}

			[[nodiscard]] static std::optional<std::uint64_t> ParseUiCommandUint64(std::string_view a_text)
			{
				std::uint64_t value = 0;
				const auto* first = a_text.data();
				const auto* last = first + a_text.size();
				const auto [ptr, ec] = std::from_chars(first, last, value);
				if (ec != std::errc{} || ptr != last || value == 0u) {
					return std::nullopt;
				}
				return value;
			}

			[[nodiscard]] static bool HandlePanelVisibilityCommand(std::string_view a_command)
			{
				if (a_command == "ui.textinput:1" || a_command == "ui.textinput:0") {
					g_panelTextInputFocused.store(a_command.back() == '1', std::memory_order_relaxed);
					return true;
				}
				if (a_command == "ui.toggle") {
					SetControlPanelOpenImpl(!g_controlPanelOpen.load());
					return true;
				}
				if (a_command == "ui.open") {
					SetControlPanelOpenImpl(true);
					return true;
				}
				if (a_command == "ui.close") {
					SetControlPanelOpenImpl(false);
					return true;
				}

				return false;
			}

			[[nodiscard]] static bool HandleRunewordSelectionCommand(std::string_view a_command)
			{
				if (a_command.rfind(kRunewordBaseSelectPrefix, 0) == 0) {
					const auto keyValue = ParseUiCommandUint64(a_command.substr(kRunewordBaseSelectPrefix.size()));
					if (!keyValue) {
						PushUiFeedback("Invalid base selection key.");
						return true;
					}

					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					if (!bridge) {
						PushUiFeedback("Runeword system unavailable.");
						return true;
					}

					if (!bridge->SelectRunewordBase(*keyValue)) {
						PushUiFeedback("Failed to select runeword base.");
						return true;
					}

					RefreshRunewordPanelBindings(*bridge, false);
					PushUiFeedback("Runeword base selected.");
					return true;
				}

				if (a_command.rfind(kRunewordRecipeSelectPrefix, 0) == 0) {
					const auto tokenValue = ParseUiCommandUint64(a_command.substr(kRunewordRecipeSelectPrefix.size()));
					if (!tokenValue) {
						PushUiFeedback("Invalid recipe selection key.");
						return true;
					}

					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					if (!bridge) {
						PushUiFeedback("Runeword system unavailable.");
						return true;
					}

					if (!bridge->SelectRunewordRecipe(*tokenValue)) {
						PushUiFeedback("Failed to select runeword recipe.");
						return true;
					}

					RefreshRunewordPanelBindings(*bridge, false);
					PushUiFeedback("Runeword recipe selected.");
					return true;
				}

				return false;
			}

			[[nodiscard]] static bool HandleLayoutSaveCommand(std::string_view a_command)
			{
				if (a_command.rfind(kPanelLayoutSavePrefix, 0) == 0) {
					PanelLayout parsed{};
					if (!PrismaLayoutPersistence::ParsePanelLayoutPayload(
							a_command.substr(kPanelLayoutSavePrefix.size()),
							parsed)) {
						SKSE::log::warn("CalamityAffixes: invalid panel layout payload: {}", a_command);
						return true;
					}

					g_panelLayoutState.value = parsed;
					g_panelLayoutState.loaded = true;
					g_panelLayoutState.needsPush = false;
					SavePanelLayoutToDisk(g_panelLayoutState.value);
					return true;
				}

				if (a_command.rfind(kTooltipLayoutSavePrefix, 0) == 0) {
					TooltipLayout parsed{};
					if (!PrismaLayoutPersistence::ParseTooltipLayoutPayload(
							a_command.substr(kTooltipLayoutSavePrefix.size()),
							kMinTooltipFontPermille,
							kMaxTooltipFontPermille,
							parsed)) {
						SKSE::log::warn("CalamityAffixes: invalid tooltip layout payload: {}", a_command);
						return true;
					}

					g_tooltipLayoutState.value = parsed;
					g_tooltipLayoutState.loaded = true;
					g_tooltipLayoutState.needsPush = false;
					SaveTooltipLayoutToDisk(g_tooltipLayoutState.value);
					return true;
				}

				return false;
			}

			[[nodiscard]] static bool HandleImmediateRunewordCommand(std::string_view a_command)
			{
				if (a_command == "runeword.insert") {
					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					if (bridge && !bridge->IsRuntimeEnabled()) {
						PushUiFeedback(kRuntimeDisabledFeedback);
						return true;
					}
					CalamityAffixes::RunewordPanelState before{};
					if (bridge) {
						before = bridge->GetRunewordPanelState();
						// Direct call (game-thread task, takes the state lock itself) so
						// the result is known before the feedback below; the mod event
						// is deferred and would land after it.
						bridge->InsertRunewordRuneFromPanel();
					} else if (!EmitModEvent(EventNames::kRunewordInsertRune)) {
						PushUiFeedback("Failed to send command event.");
						return true;
					}

					if (bridge) {
						const auto after = bridge->GetRunewordPanelState();
						RefreshRunewordPanelBindings(*bridge, false);
						PushSelectedTooltipSnapshot(true);

						if (after.isComplete && !before.isComplete) {
							std::string msg = "Runeword: transmuted ";
							const auto& recipeName = after.recipeNameEn.empty() ? after.recipeName : after.recipeNameEn;
							msg.append(recipeName.empty() ? "Runeword" : recipeName);
							PushUiFeedback(msg);
							return true;
						}

						if (after.isComplete) {
							PushUiFeedback("Runeword: already complete");
							return true;
						}

						if (!after.canInsert && after.hasRecipe) {
							if (!after.missingSummary.empty()) {
								std::string msg = "Runeword: missing fragments ";
								msg.append(after.missingSummary);
								PushUiFeedback(msg);
								return true;
							}

							if (!after.nextRuneName.empty()) {
								std::string msg = "Runeword: missing fragment ";
								msg.append(after.nextRuneName);
								msg.append(" (owned ");
								msg.append(std::to_string(after.nextRuneOwned));
								msg.push_back(')');
								PushUiFeedback(msg);
								return true;
							}
						}
					}

					PushUiFeedback("Runeword -> transmute requested");
					return true;
				}

				if (a_command.rfind(kAffixExpandPrefix, 0) == 0) {
					const auto payload = a_command.substr(kAffixExpandPrefix.size());
					const auto keys = CalamityAffixes::ParseAffixExpandCommandKeys(payload);
					if (!keys) {
						PushUiFeedback("Invalid affix-expansion base or expected count.");
						return true;
					}

					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					if (!bridge) {
						PushUiFeedback("Affix expansion system unavailable.");
						return true;
					}

					const auto outcome = bridge->ExpandSelectedRunewordBaseAffixes(
						keys->expectedInstanceKey,
						keys->expectedRegularAffixCount);
					RefreshRunewordPanelBindings(*bridge, false);
					PushSelectedTooltipSnapshot(true);
					PushUiFeedback(outcome.message.empty() ? "Affix expansion action processed." : outcome.message);
					return true;
				}

                if (a_command.starts_with("affix.identify:") || a_command.starts_with("affix.reforge:") ||
                    a_command.starts_with("affix.scour:") || a_command.starts_with("runeword.remove:")) {
                    const auto separator = a_command.find(':');
                    const auto payload = a_command.substr(separator + 1u);
                    auto action = CalamityAffixes::AffixCraftAction::kIdentify;
                    std::uint64_t baseKey = 0u;
                    std::uint64_t token = 0u;
                    if (a_command.starts_with("affix.reforge:")) {
                        action = CalamityAffixes::AffixCraftAction::kReforge;
                        if (const auto keys = CalamityAffixes::ParseLockedReforgeCommandKeys(payload)) {
                            baseKey = keys->expectedInstanceKey;
                            token = keys->affixToken;
                        }
                    } else {
                        action = a_command.starts_with("affix.scour:") ? CalamityAffixes::AffixCraftAction::kScour :
                            (a_command.starts_with("runeword.remove:") ? CalamityAffixes::AffixCraftAction::kRemoveRuneword : action);
                        baseKey = ParseUiCommandUint64(payload).value_or(0u);
                    }
                    auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
                    if (!bridge || baseKey == 0u) {
                        if (bridge) {
                            g_runewordCache.panelStateJson.clear();
                            RefreshRunewordPanelBindings(*bridge, false);
                        }
                        PushUiFeedback("Invalid crafting selection. Select the base again.");
                        return true;
                    }
                    const auto outcome = bridge->CraftSelectedAffixes(action, baseKey, token);
                    // Always push the panel state, even when a refused craft left it
                    // unchanged: that push is what releases the panel's pending lock
                    // (otherwise the buttons stay grey for 8 s and the real reason is
                    // replaced by "No response from crafting").
                    g_runewordCache.panelStateJson.clear();
                    RefreshRunewordPanelBindings(*bridge, false);
                    PushSelectedTooltipSnapshot(true);
                    PushUiFeedback(outcome.message);
                    return true;
                }

                if (a_command == "currency.exchange:reforge" || a_command == "currency.exchange:scour") {
                    auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
                    if (!bridge) {
                        PushUiFeedback("Currency system unavailable.");
                        return true;
                    }
                    const auto outcome = bridge->ExchangeCraftingCurrency(
                        a_command == "currency.exchange:scour" ? CalamityAffixes::CurrencyExchange::kScouringOrb :
                                                                 CalamityAffixes::CurrencyExchange::kReforgeOrb);
                    // See the crafting branch: a refused exchange must still push state.
                    g_runewordCache.panelStateJson.clear();
                    RefreshRunewordPanelBindings(*bridge, false);
                    PushUiFeedback(outcome.message);
                    return true;
                }

				if (a_command.starts_with("spawn.testrw:")) {
					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					if (!bridge) {
						PushUiFeedback("Runeword system unavailable.");
						return true;
					}
					const auto outcome = bridge->DebugApplyRunewordToSelectedBase(
						a_command.substr(std::string_view("spawn.testrw:").size()));
					RefreshRunewordPanelBindings(*bridge, false);
					PushSelectedTooltipSnapshot(true);
					PushUiFeedback(outcome.message);
					return true;
				}

				if (a_command == "runeword.reset") {
					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					if (!bridge) {
						PushUiFeedback("Reset system unavailable.");
						return true;
					}

					const auto outcome = bridge->ResetSelectedRunewordBaseCalamityState();
					RefreshRunewordPanelBindings(*bridge, false);
					PushSelectedTooltipSnapshot(true);
					PushUiFeedback(outcome.message.empty() ? "Reset action processed." : outcome.message);
					return true;
				}

				if (a_command == "currency.recover") {
					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					if (!bridge) {
						PushUiFeedback("Currency system unavailable.");
						return true;
					}
					const auto result = bridge->RecoverMiscCurrency();
					RefreshRunewordPanelBindings(*bridge, false);
					PushUiFeedback(result);
					return true;
				}

				return false;
			}

			[[nodiscard]] static bool DispatchEventBackedCommand(
				std::string_view a_command,
				bool& a_outSent,
				std::string_view& a_outFeedback)
			{
				a_outSent = false;
				a_outFeedback = {};

				if (a_command == "mode.next") {
					a_outSent = EmitModEvent(EventNames::kManualModeCycleNext);
					a_outFeedback = "Manual mode -> next";
					return true;
				}
				if (a_command == "mode.prev") {
					a_outSent = EmitModEvent(EventNames::kManualModeCyclePrev);
					a_outFeedback = "Manual mode -> previous";
					return true;
				}
				if (a_command == "runeword.base.next") {
					a_outSent = EmitModEvent(EventNames::kRunewordBaseNext);
					a_outFeedback = "Runeword base -> next";
					return true;
				}
				if (a_command == "runeword.base.prev") {
					a_outSent = EmitModEvent(EventNames::kRunewordBasePrev);
					a_outFeedback = "Runeword base -> previous";
					return true;
				}
				if (a_command == "runeword.recipe.next") {
					a_outSent = EmitModEvent(EventNames::kRunewordRecipeNext);
					a_outFeedback = "Runeword recipe -> next";
					return true;
				}
				if (a_command == "runeword.recipe.prev") {
					a_outSent = EmitModEvent(EventNames::kRunewordRecipePrev);
					a_outFeedback = "Runeword recipe -> previous";
					return true;
				}
				if (a_command == "runeword.status") {
					a_outSent = EmitModEvent(EventNames::kRunewordStatus);
					a_outFeedback = "Runeword -> status";
					return true;
				}
				if (a_command == "runeword.grant.next") {
					a_outSent = EmitModEvent(EventNames::kRunewordGrantNextRune, {}, 1.0f);
					a_outFeedback = "Debug -> +1 next fragment";
					return true;
				}
				if (a_command == "runeword.grant.set") {
					a_outSent = EmitModEvent(EventNames::kRunewordGrantRecipeSet, {}, 1.0f);
					a_outFeedback = "Debug -> +1 recipe set";
					return true;
				}
				if (a_command == "runeword.grant.orb3") {
					a_outSent = EmitModEvent(EventNames::kRunewordGrantStarterOrbs, {}, 3.0f);
					a_outFeedback = "Debug -> starter crafting kit";
					return true;
				}
				if (a_command == "spawn.test") {
					a_outSent = EmitModEvent(EventNames::kMcmSpawnTestItem, {}, 1.0f);
					a_outFeedback = "Spawned test item request";
					return true;
				}
				if (a_command == "spawn.trapaffix") {
					a_outSent = EmitModEvent(EventNames::kMcmGrantTrapAffix, {}, 1.0f);
					a_outFeedback = "Trap affix grant requested";
					return true;
				}
				// Test sets (v2.3.0): spawn.testset:<id> puts a whole scenario's affixes on
				// the selected item. The id rides the same grant event (ids start at 11).
				if (a_command.starts_with("spawn.testset:")) {
					const auto idText = a_command.substr(std::string_view("spawn.testset:").size());
					int id = 0;
					for (const char c : idText) {
						if (c < '0' || c > '9' || id > 99) {
							id = 0;
							break;
						}
						id = id * 10 + (c - '0');
					}
					if (id >= 11 && id <= 19) {
						a_outSent = EmitModEvent(EventNames::kMcmGrantTrapAffix, {}, static_cast<float>(id));
						a_outFeedback = "Test set requested";
					}
					return true;
				}
				if (a_command == "spawn.trapworldprobe" || a_command == "spawn.trapprobe") {
					a_outSent = EmitModEvent(EventNames::kMcmTrapMarkerProbe, {}, 1.0f);
					a_outFeedback = "Production trap world-ref probe requested";
					return true;
				}

				return false;
			}
		};

		[[nodiscard]] PrismaCommandController BuildPrismaCommandController()
		{
			return {};
		}

		[[nodiscard]] bool HandleStructuredUiCommand(std::string_view a_command)
		{
			return BuildPrismaCommandController().HandleStructuredUiCommand(a_command);
		}

		void HandleEventBackedUiCommand(std::string_view a_command)
		{
			BuildPrismaCommandController().HandleEventBackedUiCommand(a_command);
		}

		void HandleUiCommand(std::string_view a_command)
		{
			if (a_command.empty()) {
				return;
			}
			PrismaTelemetryController::RecordUiCommand();

			auto controller = BuildPrismaCommandController();
			if (controller.HandleStructuredUiCommand(a_command)) {
				return;
			}

			controller.HandleEventBackedUiCommand(a_command);
			if (a_command.rfind("runeword.", 0) == 0 ||
				a_command.rfind("mode.", 0) == 0) {
				RequestRunewordPanelRefresh(false);
			}
		}
