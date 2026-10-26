#if !defined(CALAMITYAFFIXES_EVENTBRIDGE_PUBLIC_API_INL_CONTEXT)
#error "Do not include EventBridge.PublicApi.inl directly; include CalamityAffixes/EventBridge.h"
#endif

		// Lifecycle and config entrypoints.
		static EventBridge* GetSingleton();

		void Register();
		void LoadConfig();

		// Serialization callbacks.
		void Save(SKSE::SerializationInterface* a_intfc);
		void Load(SKSE::SerializationInterface* a_intfc);
		void Revert(SKSE::SerializationInterface* a_intfc);

		// Called from kPostLoadGame messaging callback — game state (player inventory)
		// is fully loaded at this point, unlike the Serialization Load callback.
		void OnPostLoadGame();
		void OnPreLoadGame();
		void OnNewGame();
		void OnPreSaveGame();

		// Called from SKSE SerializationInterface form delete callback.
		// We use this to prune instance-affix entries for player-dropped world refs that are later deleted.
		void OnFormDelete(RE::VMHandle a_handle);

		// Runtime polling/feature gates.
		// Tick lightweight runtime systems that need polling (e.g., ground traps).
		void TickTraps();
		[[nodiscard]] bool HasActiveTraps() const noexcept { return _trapState.hasActiveTraps.load(std::memory_order_relaxed); }
		// Bow draw speed (v2.3.0): synced on the game thread from the trap worker.
		void SyncBowDrawSpeed();
		void RemoveBowDrawSpeed();
		[[nodiscard]] bool WantsBowDrawSpeedSync() const noexcept
		{
			return _bowDrawSpeed.wanted.load(std::memory_order_relaxed) || _bowDrawSpeed.active.load(std::memory_order_relaxed);
		}
		// Land the echo strikes and Doom bursts whose delay has elapsed (main thread).
		void TickEchoStrikes();
		[[nodiscard]] bool HasPendingEchoStrikes() const noexcept
		{
			return _combatState.echoStrike.hasPending.load(std::memory_order_acquire) ||
				_combatState.hasPendingDoom.load(std::memory_order_acquire);
		}
		[[nodiscard]] bool IsRuntimeEnabled() const noexcept { return _runtimeSettings.enabled.load(std::memory_order_relaxed); }
		[[nodiscard]] bool IsHealthDamageRoutingDisabled() const noexcept { return _runtimeSettings.disableHealthDamageRouting; }
		[[nodiscard]] bool IsTrapSystemTickDisabled() const noexcept { return _runtimeSettings.disableTrapSystemTick; }
		[[nodiscard]] bool AllowsPlayerHealthDamageHook() const noexcept { return _runtimeSettings.allowPlayerHealthDamageHook; }
		[[nodiscard]] bool AllowsNonHostilePlayerOwnedOutgoingProcs() const noexcept
		{
			return _runtimeSettings.allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed);
		}
		[[nodiscard]] bool ResolveNonHostileOutgoingFirstHitAllowance(
			RE::Actor* a_owner,
			RE::Actor* a_target,
			bool a_hostileEitherDirection,
			std::chrono::steady_clock::time_point a_now);

		// UI-facing helpers.
		// UI helper: returns the affix tooltip text for this entry (if any).
		// Instance affixes are stored per item instance (ExtraUniqueID -> _instanceTrackingState.instanceAffixes).
		[[nodiscard]] std::optional<std::string> GetInstanceAffixTooltip(
			const RE::InventoryEntryData* a_item,
			std::string_view a_selectedDisplayName = {},
			int a_uiLanguageMode = 2,
			std::string_view a_itemSource = {},
			RE::FormID a_sourceContainerFormID = 0u,
			std::uint64_t a_preferredInstanceKey = 0u);

		// UI helper: enumerate runeword-base candidates from player's inventory.
		[[nodiscard]] std::vector<RunewordBaseInventoryEntry> GetRunewordBaseInventoryEntries();
		// UI helper: select specific runeword base by instance key.
		[[nodiscard]] bool SelectRunewordBase(std::uint64_t a_instanceKey);
		// UI helper: enumerate runeword recipes with selected state.
		[[nodiscard]] std::vector<RunewordRecipeEntry> GetRunewordRecipeEntries();
		// UI helper: select specific runeword recipe by token.
		[[nodiscard]] bool SelectRunewordRecipe(std::uint64_t a_recipeToken);
		// UI helper: current runeword crafting status for Prisma panel.
		[[nodiscard]] RunewordPanelState GetRunewordPanelState();
		// UI helper: affix tooltip for currently selected runeword base.
		[[nodiscard]] std::optional<std::string> GetSelectedRunewordBaseAffixTooltip(int a_uiLanguageMode = 2);
        [[nodiscard]] OperationResult CraftSelectedAffixes(
            AffixCraftAction a_action, std::uint64_t a_expectedInstanceKey,
            std::uint64_t a_selectedToken = 0u);
        [[nodiscard]] OperationResult ExchangeCraftingCurrency(CurrencyExchange a_exchange);
		[[nodiscard]] OperationResult ExpandSelectedRunewordBaseAffixes(
			std::uint64_t a_expectedInstanceKey,
			std::uint8_t a_expectedRegularAffixCount);
		[[nodiscard]] OperationResult ResetSelectedRunewordBaseCalamityState();
		// Debug test plan: complete a runeword on the selected item, no fragments.
		[[nodiscard]] OperationResult DebugApplyRunewordToSelectedBase(std::string_view a_recipeId);
		// The damage hook skipped procs on a corpse hit (debug test plan event).
		void NoteCorpseHitSkipped();
		// Debug test plan event from code outside the bridge lock (hook tasks).
		void NoteDebugTestEventLocked(std::string_view a_key);
		// Prisma "Transmute": the same work as the InsertRune mod event, run
		// synchronously (game-thread UI task) so the panel knows the outcome.
		void InsertRunewordRuneFromPanel();

		// MCM manual recovery: grant starter orbs + missing rune fragments.
		[[nodiscard]] std::string RecoverMiscCurrency();

		// Combat/runtime evaluation entrypoints.
		// Called from Actor::HandleHealthDamage hook to drive hit/incoming-hit procs reliably,
		// even when TESHitEvent is delayed or suppressed by other mods.
		// a_appliedDamage: what the engine actually removed (after crit and
		// Mind over Matter); negative means "same as a_damage".
		void OnHealthDamage(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData,
			float a_damage,
			float a_appliedDamage = -1.0f);

		// Deferred SKSE task from the HandleHealthDamage hook for player damage
		// the hit pipeline does not route (breath/spell ticks reusing an old
		// HitData, shouts, DoTs, falls, ownerless traps). Runs on the game
		// thread after the damage landed; evaluates kLowHealth only.
		void OnPlayerHealthDamageLowHealthOnly(RE::FormID a_attackerFormID, float a_damageTaken);
		// Unsynchronized hint for the hook: false when no low-health affix is
		// active, so unrouted damage does not queue a task at all.
		[[nodiscard]] bool HasActiveLowHealthAffixesHint() const noexcept
		{
			return _lowHealthAffixesActiveHint.load(std::memory_order_relaxed);
		}

		// a_physicalScale: multiplier the hook already applied to this hit's
		// damage (the Calamity crit bonus), so "convert X% of physical" covers
		// the boosted physical part too.
		[[nodiscard]] ConversionResults EvaluateConversion(
			RE::Actor* a_attacker,
			RE::Actor* a_target,
			const RE::HitData* a_hitData,
			float& a_inOutDamage,
			bool a_allowResync = true,
			float a_physicalScale = 1.0f);

		[[nodiscard]] MindOverMatterResult EvaluateMindOverMatter(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData,
			float& a_inOutDamage,
			bool a_allowResync = true);

		// Returns one original-hit batch: at most two qualifying melee procs, or
		// one normal-melee/ranged proc.
		[[nodiscard]] CastOnCritResults EvaluateCastOnCrit(
			RE::Actor* a_attacker,
			RE::Actor* a_target,
			const RE::HitData* a_hitData,
			bool a_allowResync = true);

		[[nodiscard]] float GetCritDamageMultiplier(
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData) const;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESHitEvent* a_event,
			RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) override;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESDeathEvent* a_event,
			RE::BSTEventSource<RE::TESDeathEvent>* a_eventSource) override;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESEquipEvent* a_event,
			RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESActivateEvent* a_event,
			RE::BSTEventSource<RE::TESActivateEvent>* a_eventSource) override;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESMagicEffectApplyEvent* a_event,
			RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* a_eventSource) override;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESContainerChangedEvent* a_event,
			RE::BSTEventSource<RE::TESContainerChangedEvent>* a_eventSource) override;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESUniqueIDChangeEvent* a_event,
			RE::BSTEventSource<RE::TESUniqueIDChangeEvent>* a_eventSource) override;

		RE::BSEventNotifyControl ProcessEvent(
			const SKSE::ModCallbackEvent* a_event,
			RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource) override;
