#if !defined(CALAMITYAFFIXES_EVENTBRIDGE_CONSTANTS_INL_CONTEXT)
#error "Do not include EventBridge.Constants.inl directly; include CalamityAffixes/EventBridge.h"
#endif

		// Config/event string constants and runtime guards.
		static constexpr std::string_view kDotKeywordEditorID = "CAFF_TAG_DOT";
		static constexpr std::chrono::milliseconds kDotApplyICD{ 1500 };
		static constexpr std::chrono::milliseconds kCastOnCritICD{ 150 };
		static constexpr std::chrono::milliseconds kDuplicateHitWindow{ 100 };
		static constexpr std::chrono::milliseconds kPapyrusHitEventWindow{ 80 };
		static constexpr std::chrono::milliseconds kHealthDamageHookStaleWindow{ 5000 };
		static constexpr std::string_view kRuntimeConfigRelativePath = RuntimePolicy::kRuntimeConfigRelativePath;
		static constexpr std::string_view kRuntimeContractRelativePath = RuntimePolicy::kRuntimeContractRelativePath;
		static constexpr std::string_view kUserSettingsRelativePath = RuntimePolicy::kUserSettingsRelativePath;
		static constexpr std::string_view kManualModeCycleNextEvent = "CalamityAffixes_ModeCycle_Next";
		static constexpr std::string_view kManualModeCyclePrevEvent = "CalamityAffixes_ModeCycle_Prev";
		static constexpr std::string_view kRunewordBaseNextEvent = "CalamityAffixes_Runeword_Base_Next";
		static constexpr std::string_view kRunewordBasePrevEvent = "CalamityAffixes_Runeword_Base_Prev";
		static constexpr std::string_view kRunewordRecipeNextEvent = "CalamityAffixes_Runeword_Recipe_Next";
		static constexpr std::string_view kRunewordRecipePrevEvent = "CalamityAffixes_Runeword_Recipe_Prev";
		static constexpr std::string_view kRunewordInsertRuneEvent = "CalamityAffixes_Runeword_InsertRune";
		static constexpr std::string_view kRunewordStatusEvent = "CalamityAffixes_Runeword_Status";
		static constexpr std::string_view kRunewordGrantNextRuneEvent = "CalamityAffixes_Runeword_GrantNextRune";
		static constexpr std::string_view kRunewordGrantRecipeSetEvent = "CalamityAffixes_Runeword_GrantRecipeSet";
		static constexpr std::string_view kRunewordGrantStarterOrbsEvent = "CalamityAffixes_Runeword_GrantStarterOrbs";
		static constexpr std::string_view kUiSetPanelEvent = "CalamityAffixes_UI_SetPanel";
		static constexpr std::string_view kUiTogglePanelEvent = "CalamityAffixes_UI_TogglePanel";
		static constexpr std::string_view kMcmSetEnabledEvent = RuntimePolicy::kMcmSetEnabledEvent;
		static constexpr std::string_view kMcmSetDebugNotificationsEvent = RuntimePolicy::kMcmSetDebugNotificationsEvent;
		static constexpr std::string_view kMcmSetValidationIntervalEvent = RuntimePolicy::kMcmSetValidationIntervalEvent;
		static constexpr std::string_view kMcmSetProcChanceMultEvent = RuntimePolicy::kMcmSetProcChanceMultEvent;
		static constexpr std::string_view kMcmSetRunewordFragmentChanceEvent = RuntimePolicy::kMcmSetRunewordFragmentChanceEvent;
		static constexpr std::string_view kMcmSetReforgeOrbChanceEvent = RuntimePolicy::kMcmSetReforgeOrbChanceEvent;
		static constexpr std::string_view kMcmSetDotSafetyAutoDisableEvent = RuntimePolicy::kMcmSetDotSafetyAutoDisableEvent;
		static constexpr std::string_view kMcmSetAllowNonHostileFirstHitProcEvent = RuntimePolicy::kMcmSetAllowNonHostileFirstHitProcEvent;
		static constexpr std::string_view kMcmSpawnTestItemEvent = "CalamityAffixes_MCM_SpawnTestItem";
		static constexpr std::string_view kMcmForceRebuildEvent = "CalamityAffixes_MCM_ForceRebuild";
		static constexpr std::string_view kMcmGrantRecoveryPackEvent = "CalamityAffixes_MCM_GrantRecoveryPack";

		// Policy constants.
		static constexpr std::array<float, kMaxRegularAffixesPerItem> kAffixCountWeights = { 70.0f, 22.0f, 8.0f };
		static constexpr std::array<float, kMaxAffixesPerItem> kMultiAffixProcPenalty = { 1.0f, 0.8f, 0.65f, 0.5f };
		static constexpr std::uint32_t kLootChancePityFailThreshold = 3u;
		static constexpr std::uint32_t kRunewordFragmentPityFailThreshold = 99u;
		static constexpr std::uint32_t kReforgeOrbPityFailThreshold = 39u;
		static constexpr std::size_t kLootEvaluatedRecentKeep = 2048;
		static constexpr std::size_t kLootEvaluatedPruneEveryInserts = 128;
		static constexpr std::size_t kLootPreviewCacheMax = 1024;
		static constexpr std::size_t kLootCurrencyLedgerMaxEntries = 8192;
		static constexpr std::uint32_t kLootCurrencyLedgerTtlDays = 30u;

		// Serialization constants.
		static constexpr std::uint32_t kSerializationVersion = 7;
		static constexpr std::uint32_t kSerializationVersionV6 = 6;
		static constexpr std::uint32_t kSerializationVersionV5 = 5;
		static constexpr std::uint32_t kSerializationVersionV4 = 4;
		static constexpr std::uint32_t kSerializationVersionV3 = 3;
		static constexpr std::uint32_t kSerializationVersionV2 = 2;
		static constexpr std::uint32_t kSerializationVersionV1 = 1;
		static constexpr std::uint32_t kSerializationRecordInstanceAffixes = 'IAXF';
		static constexpr std::uint32_t kSerializationRecordInstanceRuntimeStates = 'IRST';
		static constexpr std::uint32_t kSerializationRecordRunewordState = 'RWRD';
		static constexpr std::uint32_t kSerializationRecordLootEvaluated = 'LRLD';
		static constexpr std::uint32_t kSerializationRecordLootCurrencyLedger = 'LCLD';
		static constexpr std::uint32_t kSerializationRecordLootShuffleBags = 'LSBG';
		static constexpr std::uint32_t kSerializationRecordMigrationFlags = 'MFLG';
		static constexpr std::uint32_t kRunewordSerializationVersion = 1;
		static constexpr std::uint32_t kMigrationFlagsVersion = 1;
		static constexpr std::uint32_t kInstanceRuntimeStateSerializationVersion = 1;
		static constexpr std::uint32_t kLootEvaluatedSerializationVersion = 1;
		static constexpr std::uint32_t kLootCurrencyLedgerSerializationVersion = 2;
		static constexpr std::uint32_t kLootCurrencyLedgerSerializationVersionV1 = 1;
		static constexpr std::uint32_t kLootShuffleBagSerializationVersion = 2;

		// Runtime debounce/resync intervals.
		static constexpr std::chrono::milliseconds kRuntimeUserSettingsPersistDebounce{ 250 };
		static constexpr std::chrono::milliseconds kEquipResyncInterval{ 8000 };
