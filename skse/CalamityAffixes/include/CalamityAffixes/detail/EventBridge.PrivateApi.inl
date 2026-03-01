#if !defined(CALAMITYAFFIXES_EVENTBRIDGE_PRIVATE_API_INL_CONTEXT)
#error "Do not include EventBridge.PrivateApi.inl directly; include CalamityAffixes/EventBridge.h"
#endif

		// Runtime lifecycle and data maintenance.
		void ResetRuntimeStateForConfigReload();
		void MaybeResyncEquippedAffixes(std::chrono::steady_clock::time_point a_now);
		void RebuildActiveCounts();
		void RebuildActiveTriggerIndexCaches();
		void MarkLootEvaluatedInstance(std::uint64_t a_instanceKey);
		void ForgetLootEvaluatedInstance(std::uint64_t a_instanceKey);
		void PruneLootEvaluatedInstances();
		[[nodiscard]] bool IsLootEvaluatedInstance(std::uint64_t a_instanceKey) const;
		void RemapInstanceKey(std::uint64_t a_oldKey, std::uint64_t a_newKey);
		void ProcessDroppedRefDeleted(LootRerollGuard::RefHandle a_refHandle);
		void ScheduleDroppedRefDeleteDrain();
		void DrainPendingDroppedRefDeletes();
		void EraseInstanceRuntimeStates(std::uint64_t a_instanceKey);
		[[nodiscard]] const InstanceAffixSlots* FindLootPreviewSlots(std::uint64_t a_instanceKey) const;
		void RememberLootPreviewSlots(std::uint64_t a_instanceKey, const InstanceAffixSlots& a_slots);
		void ForgetLootPreviewSlots(std::uint64_t a_instanceKey);
		[[nodiscard]] std::optional<InstanceAffixSlots> BuildLootPreviewAffixSlots(
			std::uint64_t a_instanceKey,
			LootItemType a_itemType) const;
		[[nodiscard]] static std::uint64_t HashLootPreviewSeed(std::uint64_t a_instanceKey, std::uint64_t a_salt);
		[[nodiscard]] bool PlayerHasInstanceKey(std::uint64_t a_instanceKey) const;
		[[nodiscard]] const std::vector<std::uint64_t>* FindEquippedInstanceKeysForAffixTokenCached(std::uint64_t a_affixToken) const;
		[[nodiscard]] std::optional<std::uint64_t> ResolvePrimaryEquippedInstanceKey(std::uint64_t a_affixToken) const;
		[[nodiscard]] std::vector<std::uint64_t> CollectEquippedInstanceKeysForAffixToken(std::uint64_t a_affixToken) const;
		[[nodiscard]] std::vector<std::uint64_t> CollectEquippedRunewordBaseCandidates(bool a_ensureUniqueId);
		[[nodiscard]] bool ResolvePlayerInventoryInstance(
			std::uint64_t a_instanceKey,
			RE::InventoryEntryData*& a_outEntry,
			RE::ExtraDataList*& a_outXList) const;
		[[nodiscard]] std::optional<LootItemType> ResolveInstanceLootType(std::uint64_t a_instanceKey) const;
		[[nodiscard]] const RunewordRecipe* FindRunewordRecipeByToken(std::uint64_t a_recipeToken) const;
		[[nodiscard]] const RunewordRecipe* GetCurrentRunewordRecipe() const;
		[[nodiscard]] const RunewordRecipe* ResolveCompletedRunewordRecipe(const InstanceAffixSlots& a_slots) const;
		[[nodiscard]] const RunewordRecipe* ResolveCompletedRunewordRecipe(std::uint64_t a_instanceKey) const;
		[[nodiscard]] RunewordApplyBlockReason ResolveRunewordApplyBlockReason(
			std::uint64_t a_instanceKey,
			const RunewordRecipe& a_recipe) const;
		[[nodiscard]] static std::string BuildRunewordApplyBlockMessage(RunewordApplyBlockReason a_reason);
		void InitializeRunewordCatalog();
		bool LoadRuntimeConfigJson(nlohmann::json& a_outJson) const;
		void ApplyLootConfigFromJson(const nlohmann::json& a_configRoot);
		void ApplyRuntimeUserSettingsOverrides();
		void SyncCurrencyDropModeState(std::string_view a_contextTag);
		[[nodiscard]] bool PersistRuntimeUserSettings();
		void MarkRuntimeUserSettingsDirty();
		void MaybeFlushRuntimeUserSettings(std::chrono::steady_clock::time_point a_now, bool a_force = false);
		[[nodiscard]] nlohmann::json BuildRuntimeUserSettingsJson() const;
		[[nodiscard]] std::string BuildRuntimeUserSettingsPayload() const;
		[[nodiscard]] const nlohmann::json* ResolveAffixArray(const nlohmann::json& a_configRoot) const;
		[[nodiscard]] bool InitializeAffixFromJson(const nlohmann::json& a_affix, AffixRuntime& a_out) const;
		void ApplyAffixSlotAndFamilyFromJson(const nlohmann::json& a_affix, AffixRuntime& a_out) const;
		void ApplyAffixKidLootFromJson(const nlohmann::json& a_affix, AffixRuntime& a_out, float& a_outKidChancePct) const;
		[[nodiscard]] bool ApplyAffixRuntimeGateFromJson(
			const nlohmann::json& a_runtime,
			const nlohmann::json& a_action,
			float a_kidChancePct,
			std::string_view a_actionType,
			AffixRuntime& a_out) const;
		void ApplyScrollNoConsumeChanceFromJson(const nlohmann::json& a_action, AffixRuntime& a_out) const;
		void NormalizeParsedAffixRuntimePolicy(AffixRuntime& a_out, std::string_view a_actionType) const;
		void ParseConfiguredAffixesFromJson(const nlohmann::json& a_affixes, RE::TESDataHandler* a_handler);
		[[nodiscard]] bool ApplyRuntimeTriggerConfigFromJson(
			const nlohmann::json& a_runtime,
			std::string_view a_actionType,
			AffixRuntime& a_out) const;
		[[nodiscard]] bool ParseRuntimeActionFromJson(
			const nlohmann::json& a_action,
			std::string_view a_type,
			RE::TESDataHandler* a_handler,
			AffixRuntime& a_out) const;
		[[nodiscard]] bool ParseRuntimeSpellActionFromJson(
			const nlohmann::json& a_action,
			std::string_view a_type,
			RE::TESDataHandler* a_handler,
			AffixRuntime& a_out) const;
		[[nodiscard]] bool ParseRuntimeSpecialActionFromJson(
			const nlohmann::json& a_action,
			std::string_view a_type,
			RE::TESDataHandler* a_handler,
			AffixRuntime& a_out) const;
		[[nodiscard]] bool ParseRuntimeAreaActionFromJson(
			const nlohmann::json& a_action,
			std::string_view a_type,
			RE::TESDataHandler* a_handler,
			AffixRuntime& a_out) const;
		void IndexConfiguredAffixes();
		void IndexAffixLookupKeys(
			const AffixRuntime& a_affix,
			std::size_t a_index,
			bool a_useSynthesizedDuplicateLogFormat,
			bool a_warnOnDuplicate);
		void IndexAffixTriggerBucket(const AffixRuntime& a_affix, std::size_t a_index);
		void IndexAffixSpecialActionBucket(const AffixRuntime& a_affix, std::size_t a_index);
		void IndexAffixLootPool(const AffixRuntime& a_affix, std::size_t a_index);
		void RebuildSharedLootPools();
		void RegisterSynthesizedAffix(AffixRuntime&& a_affix, bool a_warnOnDuplicate);
		[[nodiscard]] static SyntheticRunewordStyle ResolveSyntheticRunewordStyle(const RunewordRecipe& a_recipe);
		[[nodiscard]] static std::string_view DescribeSyntheticRunewordStyle(SyntheticRunewordStyle a_style);
		static void ApplyRunewordIndividualTuning(
			AffixRuntime& a_out,
			std::string_view a_recipeId,
			SyntheticRunewordStyle a_style);
		[[nodiscard]] bool ConfigureSyntheticRunewordStyle(
			AffixRuntime& a_out,
			SyntheticRunewordStyle& a_inOutStyle,
			const RunewordRecipe& a_recipe,
			std::uint32_t a_runeCount,
			const RunewordSynthesis::SpellSet& a_spellSet) const;
		[[nodiscard]] bool ConfigureSyntheticAdaptiveSpell(
			AffixRuntime& a_out,
			float a_runeScale,
			AdaptiveElementMode a_mode,
			RE::SpellItem* a_fire,
			RE::SpellItem* a_frost,
			RE::SpellItem* a_shock,
			float a_procBase,
			float a_procPerRune,
			float a_procMin,
			float a_procMax,
			float a_icdBase,
			float a_icdPerRune,
			float a_icdMin,
			float a_icdMax,
			float a_scaleBase,
			float a_scalePerRune,
			float a_perTargetIcdSeconds,
			Trigger a_trigger = Trigger::kHit) const;
		[[nodiscard]] bool ConfigureSyntheticSingleTargetSpell(
			AffixRuntime& a_out,
			float a_runeScale,
			RE::SpellItem* a_spell,
			float a_procBase,
			float a_procPerRune,
			float a_procMin,
			float a_procMax,
			float a_icdBase,
			float a_icdPerRune,
			float a_icdMin,
			float a_icdMax,
			float a_scaleBase,
			float a_scalePerRune,
			float a_perTargetIcdSeconds,
			Trigger a_trigger = Trigger::kHit) const;
		[[nodiscard]] bool ConfigureSyntheticSelfBuff(
			AffixRuntime& a_out,
			float a_runeScale,
			RE::SpellItem* a_spell,
			Trigger a_trigger,
			float a_procBase,
			float a_procPerRune,
			float a_procMin,
			float a_procMax,
			float a_icdBase,
			float a_icdPerRune,
			float a_icdMin,
			float a_icdMax) const;
		[[nodiscard]] bool ConfigureSyntheticSummonSpell(
			AffixRuntime& a_out,
			float a_runeScale,
			RE::SpellItem* a_spell,
			Trigger a_trigger,
			float a_procBase,
			float a_procPerRune,
			float a_procMin,
			float a_procMax,
			float a_icdBase,
			float a_icdPerRune,
			float a_icdMin,
			float a_icdMax) const;
		[[nodiscard]] bool ApplyLegacySyntheticRunewordFallback(
			AffixRuntime& a_out,
			const RunewordRecipe& a_recipe,
			std::uint32_t a_runeCount,
			const RunewordSynthesis::SpellSet& a_spellSet) const;
		void SynthesizeRunewordRuntimeAffixes();
		void SanitizeRunewordState();
		void CycleRunewordBase(std::int32_t a_direction);
		void CycleRunewordRecipe(std::int32_t a_direction);
		void InsertRunewordRuneIntoSelectedBase();
		void GrantNextRequiredRuneFragment(std::uint32_t a_amount = 1u);
		void GrantCurrentRecipeRuneSet(std::uint32_t a_amount = 1u);
		std::uint32_t GrantReforgeOrbs(std::uint32_t a_amount = 1u);
		[[nodiscard]] bool TryRollRunewordFragmentToken(
			float a_sourceChanceMultiplier,
			std::uint64_t& a_outRuneToken,
			bool& a_outPityTriggered);
		void CommitRunewordFragmentGrant(bool a_success);
		[[nodiscard]] bool TryRollReforgeOrbGrant(
			float a_sourceChanceMultiplier,
			bool& a_outPityTriggered);
		void CommitReforgeOrbGrant(bool a_success);
		[[nodiscard]] bool TryPlaceLootCurrencyItem(
			RE::TESBoundObject* a_dropItem,
			RE::TESObjectREFR* a_sourceContainerRef,
			bool a_forceWorldPlacement) const;
		[[nodiscard]] CurrencyRollExecutionResult ExecuteCurrencyDropRolls(
			float a_sourceChanceMultiplier,
			RE::TESObjectREFR* a_sourceContainerRef,
			bool a_forceWorldPlacement,
			std::uint32_t a_rollCount,
			bool a_allowRunewordRoll = true,
			bool a_allowReforgeRoll = true);
		[[nodiscard]] bool IsRuntimeCurrencyDropRollEnabled(std::string_view a_contextTag) const;
		[[nodiscard]] float ResolveLootCurrencySourceChanceMultiplier(detail::LootCurrencySourceTier a_sourceTier) const noexcept;
		[[nodiscard]] bool TryBeginLootCurrencyLedgerRoll(std::uint64_t a_ledgerKey, std::uint32_t a_dayStamp);
		void FinalizeLootCurrencyLedgerRoll(std::uint64_t a_ledgerKey, std::uint32_t a_dayStamp);
		[[nodiscard]] bool ApplyRunewordResult(
			std::uint64_t a_instanceKey,
			const RunewordRecipe& a_recipe,
			std::string* a_outFailureReason = nullptr);
		void LogRunewordStatus() const;
		InstanceRuntimeState& EnsureInstanceRuntimeState(std::uint64_t a_instanceKey, std::uint64_t a_affixToken);
		[[nodiscard]] const InstanceRuntimeState* FindInstanceRuntimeState(std::uint64_t a_instanceKey, std::uint64_t a_affixToken) const;
		[[nodiscard]] std::size_t ResolveEvolutionStageIndex(const Action& a_action, const InstanceRuntimeState* a_state) const;
		[[nodiscard]] float ResolveEvolutionMultiplier(const Action& a_action, const InstanceRuntimeState* a_state) const;
		void AdvanceRuntimeStateForAffixProc(const AffixRuntime& a_affix);
		void CycleManualModeForEquippedAffixes(std::int32_t a_direction, std::string_view a_affixIdFilter = {});

		// Loot processing path.
		void ProcessLootAcquired(
			RE::FormID a_baseObj,
			std::int32_t a_count,
			std::uint16_t a_uniqueID,
			RE::FormID a_oldContainer,
			bool a_allowRunewordFragmentRoll);
		[[nodiscard]] std::optional<std::size_t> RollLootAffixIndex(
			LootItemType a_itemType,
			const std::vector<std::size_t>* a_exclude = nullptr,
			bool a_skipChanceCheck = false);
		[[nodiscard]] std::optional<std::size_t> RollSuffixIndex(
			LootItemType a_itemType,
			const std::vector<std::string>* a_excludeFamilies = nullptr);
		[[nodiscard]] std::uint8_t RollAffixCount();
		[[nodiscard]] bool RollLootChanceGateForEligibleInstance();
		void EnsureMultiAffixDisplayName(
			RE::InventoryEntryData* a_entry,
			RE::ExtraDataList* a_xList,
			const InstanceAffixSlots& a_slots);
		[[nodiscard]] bool SanitizeInstanceAffixSlotsForCurrentLootRules(
			std::uint64_t a_instanceKey,
			InstanceAffixSlots& a_slots,
			std::string_view a_context);
		void SanitizeAllTrackedLootInstancesForCurrentLootRules(std::string_view a_context);
		[[nodiscard]] bool IsLootObjectEligibleForAffixes(const RE::TESBoundObject* a_object) const;
		[[nodiscard]] bool IsLootArmorEligibleForAffixes(const RE::TESObjectARMO* a_armor) const;
		[[nodiscard]] bool TryClearStaleLootDisplayName(
			RE::InventoryEntryData* a_entry,
			RE::ExtraDataList* a_xList,
			bool a_allowTrailingMarkerClear);
		void CleanupInvalidLootInstance(
			RE::InventoryEntryData* a_entry,
			RE::ExtraDataList* a_xList,
			std::uint64_t a_key,
			std::string_view a_reason);

		[[nodiscard]] std::optional<LootItemType> ParseLootItemType(std::string_view a_kidType) const;
		[[nodiscard]] static const char* DescribeLootItemType(LootItemType a_type);
		[[nodiscard]] std::string FormatLootName(std::string_view a_baseName, std::string_view a_affixName) const;
		[[nodiscard]] float ComputeActiveScrollNoConsumeChancePct() const;
		[[nodiscard]] std::int32_t RollScrollNoConsumeRestoreCount(
			std::int32_t a_consumedCount,
			float a_preserveChancePct);
		[[nodiscard]] static std::uint64_t MakeInstanceKey(RE::FormID a_baseID, std::uint16_t a_uniqueID) noexcept;

		// Health-damage/TESHit trigger routing path.
		[[nodiscard]] bool RouteHealthDamageAsHit(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData,
			RE::FormID a_sourceFormID,
			float a_damage,
			std::chrono::steady_clock::time_point a_now);
		void ProcessOutgoingHealthDamageHit(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData,
			RE::FormID a_sourceFormID,
			std::chrono::steady_clock::time_point a_now);
		void ProcessIncomingHealthDamageHit(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData,
			RE::FormID a_sourceFormID,
			std::chrono::steady_clock::time_point a_now);
		void ProcessLowHealthTriggerFromHealthDamage(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData);
		[[nodiscard]] bool IsPerTargetCooldownBlocked(
			const AffixRuntime& a_affix,
			RE::Actor* a_target,
			std::chrono::steady_clock::time_point a_now,
			PerTargetCooldownKey* a_outKey = nullptr) const;
		void CommitPerTargetCooldown(
			const PerTargetCooldownKey& a_key,
			std::chrono::milliseconds a_perTargetIcd,
			std::chrono::steady_clock::time_point a_now);
		void ProcessImmediateCorpseExplosionFromLethalHit(
			RE::Actor* a_target,
			RE::Actor* a_attacker);
		[[nodiscard]] const std::vector<std::size_t>* ResolveActiveTriggerIndices(Trigger a_trigger) const noexcept;
		void ProcessTrigger(Trigger a_trigger, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData = nullptr);
		void RecordRecentCombatEvent(Trigger a_trigger, RE::Actor* a_owner, std::chrono::steady_clock::time_point a_now);
		void MarkPlayerCombatEvidence(
			std::chrono::steady_clock::time_point a_now,
			PlayerCombatEvidenceSource a_source,
			const RE::Actor* a_player,
			const RE::Actor* a_other) noexcept;
		[[nodiscard]] bool PassesRecentlyGates(
			const AffixRuntime& a_affix,
			RE::Actor* a_owner,
			std::chrono::steady_clock::time_point a_now) const;
		[[nodiscard]] bool PassesLuckyHitGate(
			const AffixRuntime& a_affix,
			Trigger a_trigger,
			const RE::HitData* a_hitData,
			std::chrono::steady_clock::time_point a_now);
		[[nodiscard]] bool PassesLowHealthTriggerGate(
			const AffixRuntime& a_affix,
			RE::Actor* a_owner,
			float a_previousHealthPct,
			float a_currentHealthPct);
		void MarkLowHealthTriggerConsumed(
			const AffixRuntime& a_affix,
			RE::Actor* a_owner);

		// Corpse-explosion helpers.
		void ProcessCorpseExplosionKill(RE::Actor* a_owner, RE::Actor* a_corpse);
		void ProcessSummonCorpseExplosionDeath(RE::Actor* a_owner, RE::Actor* a_corpse);
		void ProcessCorpseExplosionAction(
			RE::Actor* a_owner,
			RE::Actor* a_corpse,
			const std::vector<std::size_t>& a_affixIndices,
			ActionType a_expectedActionType,
			const char* a_actionName,
			bool a_summonMode);
		[[nodiscard]] CorpseExplosionSelection SelectBestCorpseExplosionAffix(
			const std::vector<std::size_t>& a_affixIndices,
			ActionType a_expectedActionType,
			float a_corpseMaxHealth,
			std::chrono::steady_clock::time_point a_now) const;
		void LogCorpseExplosionSelectionSkipped(
			const CorpseExplosionSelection& a_selection,
			RE::Actor* a_corpse,
			const char* a_actionName,
			std::chrono::steady_clock::time_point a_now) const;
		[[nodiscard]] static const char* DescribeCorpseExplosionDenyReason(CorpseExplosionBudgetDenyReason a_reason);
		void LogCorpseExplosionBudgetDenied(
			CorpseExplosionBudgetDenyReason a_reason,
			const AffixRuntime& a_affix,
			RE::Actor* a_corpse,
			const char* a_actionName,
			std::chrono::steady_clock::time_point a_now) const;
		void NotifyCorpseExplosionFired(
			const AffixRuntime& a_affix,
			std::uint32_t a_targetsHit,
			float a_finalDamage,
			const char* a_actionName,
			std::uint32_t a_chainDepth) const;
		[[nodiscard]] bool TryConsumeCorpseExplosionBudget(
			RE::FormID a_corpseFormID,
			const Action& a_action,
			std::chrono::steady_clock::time_point a_now,
			std::uint32_t& a_outChainDepth,
			float& a_outChainMultiplier,
			CorpseExplosionBudgetDenyReason* a_outDenyReason = nullptr,
			bool a_summonMode = false);
		std::uint32_t ExecuteCorpseExplosion(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_corpse, float a_baseDamage);

		// Archmage helpers.
		[[nodiscard]] ArchmageSelection SelectBestArchmageAction(
			std::chrono::steady_clock::time_point a_now,
			RE::Actor* a_owner,
			RE::Actor* a_target,
			const RE::HitData* a_hitData);
		[[nodiscard]] bool ResolveArchmageResourceUsage(
			RE::Actor* a_caster,
			float a_damagePct,
			float a_costPct,
			float& a_outMaxMagicka,
			float& a_outExtraCost,
			float& a_outExtraDamage) const;
		[[nodiscard]] bool ExecuteArchmageCast(
			const Action& a_action,
			RE::Actor* a_caster,
			RE::Actor* a_target,
			std::string_view a_sourceEditorId,
			float a_maxMagicka,
			float a_extraCost,
			float a_extraDamage);
		void ProcessArchmageSpellHit(RE::Actor* a_caster, RE::Actor* a_target, RE::SpellItem* a_sourceSpell, const RE::HitData* a_hitData = nullptr);
		bool ShouldSuppressDuplicateHit(const LastHitKey& a_key, std::chrono::steady_clock::time_point a_now) noexcept;
		bool ShouldSuppressPapyrusHitEvent(const LastHitKey& a_key, std::chrono::steady_clock::time_point a_now) noexcept;
		[[nodiscard]] bool TryConsumeTriggerProcBudget(std::chrono::steady_clock::time_point a_now) noexcept;

		// Action execution helpers (dispatch + typed executors).
		void ExecuteDebugNotifyAction(const Action& a_action);
		[[nodiscard]] RE::TESObjectREFR* ResolveSpellCastTarget(const Action& a_action, RE::Actor* a_target) const;
		[[nodiscard]] float ResolveSpellMagnitudeOverride(
			const Action& a_action,
			RE::SpellItem* a_spell,
			const RE::HitData* a_hitData,
			bool a_logWithoutHitData) const;
		[[nodiscard]] RE::Actor* ResolveAdaptiveAnalysisTarget(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_target) const;
		[[nodiscard]] AdaptiveCastSelection SelectAdaptiveSpellForTarget(const Action& a_action, RE::Actor* a_analysisTarget) const;
		void LogAdaptiveCastSpell(
			const AdaptiveCastSelection& a_selection,
			RE::TESObjectREFR* a_castTarget,
			float a_magnitudeOverride) const;
		void ExecuteCastSpellAction(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData);
		void ExecuteCastSpellAdaptiveElementAction(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData);
		[[nodiscard]] bool SelectSpawnTrapTarget(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData, RE::Actor*& a_outSpawnTarget);
		[[nodiscard]] float ResolveSpawnTrapMagnitudeOverride(const Action& a_action, const RE::HitData* a_hitData) const;
		void EnforcePerAffixTrapCap(const Action& a_action);
		void EnforceGlobalTrapCap();
		[[nodiscard]] TrapInstance BuildSpawnTrapInstance(
			const Action& a_action,
			RE::Actor* a_owner,
			RE::Actor* a_spawnTarget,
			float a_magnitudeOverride,
			std::chrono::steady_clock::time_point a_now);
		void LogSpawnTrapCreated(const TrapInstance& a_trap, const Action& a_action) const;
		void ExecuteSpawnTrapAction(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData);
		void DispatchActionByType(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData);
		void ExecuteActionWithProcDepthGuard(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData);
		void ExecuteAction(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData = nullptr);
