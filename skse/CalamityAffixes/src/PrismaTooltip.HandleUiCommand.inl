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
				SetRunewordPanelState(bridge->GetRunewordPanelState());
				SetRunewordAffixPreview(bridge->GetSelectedRunewordBaseAffixTooltip(g_uiLanguageMode.load()).value_or(""));
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
				SetRunewordPanelState(bridge->GetRunewordPanelState());
				SetRunewordAffixPreview(bridge->GetSelectedRunewordBaseAffixTooltip(g_uiLanguageMode.load()).value_or(""));
				PushUiFeedback("Runeword recipe selected.");
				return;
			}

			if (a_command.rfind(kPanelLayoutSavePrefix, 0) == 0) {
				PanelLayout parsed{};
				if (!PrismaLayoutPersistence::ParsePanelLayoutPayload(
						a_command.substr(kPanelLayoutSavePrefix.size()),
						parsed)) {
					SKSE::log::warn("CalamityAffixes: invalid panel layout payload: {}", a_command);
					return;
				}

				g_panelLayout = parsed;
				g_panelLayoutLoaded = true;
				g_panelLayoutNeedsPush = false;
				SavePanelLayoutToDisk(g_panelLayout);
				return;
			}

			if (a_command.rfind(kTooltipLayoutSavePrefix, 0) == 0) {
				TooltipLayout parsed{};
				if (!PrismaLayoutPersistence::ParseTooltipLayoutPayload(
						a_command.substr(kTooltipLayoutSavePrefix.size()),
						kMinTooltipFontPermille,
						kMaxTooltipFontPermille,
						parsed)) {
					SKSE::log::warn("CalamityAffixes: invalid tooltip layout payload: {}", a_command);
					return;
				}

				g_tooltipLayout = parsed;
				g_tooltipLayoutLoaded = true;
				g_tooltipLayoutNeedsPush = false;
				SaveTooltipLayoutToDisk(g_tooltipLayout);
				return;
			}

			bool sent = false;
			std::string_view feedback = {};
			if (a_command == "mode.next") {
				sent = EmitModEvent(EventNames::kManualModeCycleNext);
				feedback = "Manual mode -> next";
			} else if (a_command == "mode.prev") {
				sent = EmitModEvent(EventNames::kManualModeCyclePrev);
				feedback = "Manual mode -> previous";
			} else if (a_command == "runeword.base.next") {
				sent = EmitModEvent(EventNames::kRunewordBaseNext);
				feedback = "Runeword base -> next";
			} else if (a_command == "runeword.base.prev") {
				sent = EmitModEvent(EventNames::kRunewordBasePrev);
				feedback = "Runeword base -> previous";
			} else if (a_command == "runeword.recipe.next") {
				sent = EmitModEvent(EventNames::kRunewordRecipeNext);
				feedback = "Runeword recipe -> next";
			} else if (a_command == "runeword.recipe.prev") {
				sent = EmitModEvent(EventNames::kRunewordRecipePrev);
				feedback = "Runeword recipe -> previous";
			} else if (a_command == "runeword.insert") {
				auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				CalamityAffixes::EventBridge::RunewordPanelState before{};
				if (bridge) {
					before = bridge->GetRunewordPanelState();
				}

				sent = EmitModEvent(EventNames::kRunewordInsertRune);

				if (sent && bridge) {
					// SendEvent is synchronous; refresh status immediately so UI feedback isn't misleading.
					const auto after = bridge->GetRunewordPanelState();
					SetRunewordPanelState(after);
					SetRunewordAffixPreview(bridge->GetSelectedRunewordBaseAffixTooltip(g_uiLanguageMode.load()).value_or(""));
					PushSelectedTooltipSnapshot(true);

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
				sent = EmitModEvent(EventNames::kRunewordStatus);
				feedback = "Runeword -> status";
			} else if (a_command == "runeword.reforge") {
				auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				if (!bridge) {
					PushUiFeedback("Reforge system unavailable.");
					return;
				}

				const auto outcome = bridge->ReforgeSelectedRunewordBaseWithOrb();
				SetRunewordPanelState(bridge->GetRunewordPanelState());
				SetRunewordBaseInventoryList(bridge->GetRunewordBaseInventoryEntries());
				SetRunewordAffixPreview(bridge->GetSelectedRunewordBaseAffixTooltip(g_uiLanguageMode.load()).value_or(""));
				PushSelectedTooltipSnapshot(true);
				PushUiFeedback(outcome.message.empty() ? "Reforge action processed." : outcome.message);
				return;
			} else if (a_command == "runeword.grant.next") {
				sent = EmitModEvent(EventNames::kRunewordGrantNextRune, {}, 1.0f);
				feedback = "Debug -> +1 next fragment";
			} else if (a_command == "runeword.grant.set") {
				sent = EmitModEvent(EventNames::kRunewordGrantRecipeSet, {}, 1.0f);
				feedback = "Debug -> +1 recipe set";
			} else if (a_command == "runeword.grant.orb3") {
				sent = EmitModEvent(EventNames::kRunewordGrantStarterOrbs, {}, 3.0f);
				feedback = "Debug -> +3 reforge orbs";
			} else if (a_command == "currency.recover") {
				auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				if (!bridge) {
					PushUiFeedback("Currency system unavailable.");
					return;
				}
				const auto result = bridge->RecoverMiscCurrency();
				PushUiFeedback(result);
				return;
			} else if (a_command == "spawn.test") {
				sent = EmitModEvent(EventNames::kMcmSpawnTestItem, {}, 1.0f);
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
