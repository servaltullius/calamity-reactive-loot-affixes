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

					RefreshRunewordPanelBindings(*bridge);
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

					RefreshRunewordPanelBindings(*bridge);
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
					CalamityAffixes::RunewordPanelState before{};
					if (bridge) {
						before = bridge->GetRunewordPanelState();
					}

					const bool sent = EmitModEvent(EventNames::kRunewordInsertRune);
					if (!sent) {
						PushUiFeedback("Failed to send command event.");
						return true;
					}

					if (bridge) {
						// SendEvent is synchronous; refresh status immediately so UI feedback isn't misleading.
						const auto after = bridge->GetRunewordPanelState();
						RefreshRunewordPanelBindings(*bridge);
						PushSelectedTooltipSnapshot(true);

						if (after.isComplete && !before.isComplete) {
							std::string msg = "Runeword: transmuted ";
							msg.append(after.recipeName.empty() ? "Runeword" : after.recipeName);
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

				if (a_command == "runeword.reforge") {
					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					if (!bridge) {
						PushUiFeedback("Reforge system unavailable.");
						return true;
					}

					const auto outcome = bridge->ReforgeSelectedRunewordBaseWithOrb();
					RefreshRunewordPanelBindings(*bridge);
					PushSelectedTooltipSnapshot(true);
					PushUiFeedback(outcome.message.empty() ? "Reforge action processed." : outcome.message);
					return true;
				}

				if (a_command == "currency.recover") {
					auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
					if (!bridge) {
						PushUiFeedback("Currency system unavailable.");
						return true;
					}
					const auto result = bridge->RecoverMiscCurrency();
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
					a_outFeedback = "Debug -> +3 reforge orbs";
					return true;
				}
				if (a_command == "spawn.test") {
					a_outSent = EmitModEvent(EventNames::kMcmSpawnTestItem, {}, 1.0f);
					a_outFeedback = "Spawned test item request";
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
		}
