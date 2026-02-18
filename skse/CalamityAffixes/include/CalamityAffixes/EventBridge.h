#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include <RE/Skyrim.h>
#include <SKSE/Events.h>
#include <SKSE/SKSE.h>

#include "CalamityAffixes/AdaptiveElement.h"
#include "CalamityAffixes/AffixToken.h"
#include "CalamityAffixes/InstanceAffixSlots.h"
#include "CalamityAffixes/InstanceStateKey.h"
#include "CalamityAffixes/LootRerollGuard.h"
#include "CalamityAffixes/MagnitudeScaling.h"
#include "CalamityAffixes/NonHostileFirstHitGate.h"
#include "CalamityAffixes/PerTargetCooldownStore.h"
#include "CalamityAffixes/ResyncScheduler.h"

namespace CalamityAffixes
{
	class EventBridge final :
		public RE::BSTEventSink<RE::TESHitEvent>,
		public RE::BSTEventSink<RE::TESDeathEvent>,
		public RE::BSTEventSink<RE::TESEquipEvent>,
		public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>,
		public RE::BSTEventSink<RE::TESContainerChangedEvent>,
		public RE::BSTEventSink<RE::TESUniqueIDChangeEvent>,
		public RE::BSTEventSink<SKSE::ModCallbackEvent>
	{
	public:
		// Public result/value DTOs.
		struct ConversionResult
		{
			RE::SpellItem* spell{ nullptr };
			float convertedDamage{ 0.0f };
			float effectiveness{ 1.0f };
			bool noHitEffectArt{ false };
		};

		struct CastOnCritResult
		{
			RE::SpellItem* spell{ nullptr };
			float effectiveness{ 1.0f };
			float magnitudeOverride{ 0.0f };
			bool noHitEffectArt{ false };
		};

		struct MindOverMatterResult
		{
			float redirectedDamage{ 0.0f };
			float consumedMagicka{ 0.0f };
			float redirectPct{ 0.0f };
		};

		struct RunewordBaseInventoryEntry
		{
			std::uint64_t instanceKey{ 0 };
			std::string displayName{};
			bool selected{ false };
		};

		struct RunewordRecipeEntry
		{
			std::uint64_t recipeToken{ 0 };
			std::string displayName{};
			std::string runeSequence{};
			std::string effectSummaryKey{};
			std::string recommendedBaseKey{};
			bool selected{ false };
		};

		struct RunewordRuneRequirement
		{
			std::string runeName{};
			std::uint32_t required{ 0 };
			std::uint32_t owned{ 0 };
		};

		struct RunewordPanelState
		{
			bool hasBase{ false };
			bool hasRecipe{ false };
			bool isComplete{ false };
			std::string recipeName{};
			std::uint32_t insertedRunes{ 0 };
			std::uint32_t totalRunes{ 0 };
			std::string nextRuneName{};
			std::uint32_t nextRuneOwned{ 0 };
			// NOTE: legacy name: treated as "can transmute" in Transmute-only UI.
			bool canInsert{ false };
			std::string missingSummary{};
			std::vector<RunewordRuneRequirement> requiredRunes{};
		};

		struct OperationResult
		{
			bool success{ false };
			std::string message{};
		};

		// Lifecycle and config entrypoints.
		static EventBridge* GetSingleton();

		void Register();
		void LoadConfig();

		// Serialization callbacks.
		void Save(SKSE::SerializationInterface* a_intfc);
		void Load(SKSE::SerializationInterface* a_intfc);
		void Revert(SKSE::SerializationInterface* a_intfc);

		// Called from SKSE SerializationInterface form delete callback.
		// We use this to prune instance-affix entries for player-dropped world refs that are later deleted.
		void OnFormDelete(RE::VMHandle a_handle);

		// Runtime polling/feature gates.
		// Tick lightweight runtime systems that need polling (e.g., ground traps).
		void TickTraps();
		[[nodiscard]] bool HasActiveTraps() const noexcept { return _hasActiveTraps.load(std::memory_order_relaxed); }
		[[nodiscard]] bool AllowsNonHostilePlayerOwnedOutgoingProcs() const noexcept
		{
			return _allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed);
		}
		[[nodiscard]] bool ResolveNonHostileOutgoingFirstHitAllowance(
			RE::Actor* a_owner,
			RE::Actor* a_target,
			bool a_hostileEitherDirection,
			std::chrono::steady_clock::time_point a_now);

		// UI-facing helpers.
		// UI helper: returns the affix tooltip text for this entry (if any).
		// Instance affixes are stored per item instance (ExtraUniqueID -> _instanceAffixes).
		[[nodiscard]] std::optional<std::string> GetInstanceAffixTooltip(
			const RE::InventoryEntryData* a_item,
			std::string_view a_selectedDisplayName = {},
			int a_uiLanguageMode = 2,
			std::string_view a_itemSource = {});

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
		[[nodiscard]] OperationResult ReforgeSelectedRunewordBaseWithOrb();

		// Combat/runtime evaluation entrypoints.
		// Called from Actor::HandleHealthDamage hook to drive hit/incoming-hit procs reliably,
		// even when TESHitEvent is delayed or suppressed by other mods.
		void OnHealthDamage(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData,
			float a_damage);

		[[nodiscard]] ConversionResult EvaluateConversion(
			RE::Actor* a_attacker,
			RE::Actor* a_target,
			const RE::HitData* a_hitData,
			float& a_inOutDamage);

		[[nodiscard]] MindOverMatterResult EvaluateMindOverMatter(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData,
			float& a_inOutDamage);

		[[nodiscard]] CastOnCritResult EvaluateCastOnCrit(
			RE::Actor* a_attacker,
			RE::Actor* a_target,
			const RE::HitData* a_hitData);

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

	private:
		EventBridge() = default;

		// Internal domain enums.
		enum class Trigger : std::uint8_t
		{
			kHit,
			kIncomingHit,
			kDotApply,
			kKill,
		};

		enum class ActionType : std::uint8_t
		{
			kDebugNotify,
			kCastSpell,
			kCastSpellAdaptiveElement,
			kCastOnCrit,
			kConvertDamage,
			kMindOverMatter,
			kArchmage,
			kCorpseExplosion,
			kSummonCorpseExplosion,
			kSpawnTrap,
		};

		enum class TrapSpawnAt : std::uint8_t
		{
			kOwnerFeet,
			kTargetFeet,
		};

		enum class LootItemType : std::uint8_t
		{
			kWeapon,
			kArmor,
		};

		enum class AffixSlot : std::uint8_t
		{
			kNone,      // runeword-only or legacy
			kPrefix,    // active proc effects
			kSuffix,    // passive stat bonuses
		};

		enum class Element : std::uint8_t
		{
			kNone,
			kFire,
			kFrost,
			kShock,
		};

		struct Action
		{
			ActionType type{ ActionType::kDebugNotify };
			std::uint64_t sourceToken{ 0 };
			std::string text{};
			RE::SpellItem* spell{ nullptr };
			float effectiveness{ 1.0f };
			float magnitudeOverride{ 0.0f };
			MagnitudeScaling magnitudeScaling{};
			bool noHitEffectArt{ false };
			bool debugNotify{ false };
			bool applyToSelf{ false };

			// CastSpellAdaptiveElement
			AdaptiveElementMode adaptiveMode{ AdaptiveElementMode::kWeakestResist };
			RE::SpellItem* adaptiveFire{ nullptr };
			RE::SpellItem* adaptiveFrost{ nullptr };
			RE::SpellItem* adaptiveShock{ nullptr };

			// ConvertDamage
			Element element{ Element::kNone };
			float convertPct{ 0.0f };

			// MindOverMatter
			float mindOverMatterDamageToMagickaPct{ 0.0f };
			float mindOverMatterMaxRedirectPerHit{ 0.0f };

			// Archmage
			float archmageDamagePctOfMaxMagicka{ 0.0f };
			float archmageCostPctOfMaxMagicka{ 0.0f };

			// CorpseExplosion
			float corpseExplosionFlatDamage{ 0.0f };
			float corpseExplosionPctOfCorpseMaxHealth{ 0.0f };
			float corpseExplosionRadius{ 0.0f };
			std::uint32_t corpseExplosionMaxTargets{ 0 };
			std::uint32_t corpseExplosionMaxChainDepth{ 1 };
			float corpseExplosionChainFalloff{ 1.0f };
			std::chrono::milliseconds corpseExplosionChainWindow{ 0 };
			std::chrono::milliseconds corpseExplosionRateLimitWindow{ 0 };
			std::uint32_t corpseExplosionMaxExplosionsPerWindow{ 0 };

			// SpawnTrap
			TrapSpawnAt trapSpawnAt{ TrapSpawnAt::kTargetFeet };
			float trapRadius{ 0.0f };
			std::chrono::milliseconds trapTtl{ 0 };
			std::chrono::milliseconds trapArmDelay{ 0 };
			std::chrono::milliseconds trapRearmDelay{ 0 };
			std::uint32_t trapMaxActive{ 0 };
			std::uint32_t trapMaxTriggers{ 1 };
			std::uint32_t trapMaxTargetsPerTrigger{ 1 };
			std::vector<RE::SpellItem*> trapExtraSpells{};
			bool trapRequireCritOrPowerAttack{ false };
			bool trapRequireWeaponHit{ true };

			// Evolution (growth): each successful proc grants XP and scales magnitude by stage.
			bool evolutionEnabled{ false };
			std::uint32_t evolutionXpPerProc{ 1 };
			std::vector<std::uint32_t> evolutionThresholds{};
			std::vector<float> evolutionMultipliers{};

			// Mode cycle: rotate spell payload across modes after N successful procs.
			bool modeCycleEnabled{ false };
			std::vector<RE::SpellItem*> modeCycleSpells{};
			std::vector<std::string> modeCycleLabels{};
			std::uint32_t modeCycleSwitchEveryProcs{ 1 };
			bool modeCycleManualOnly{ false };
		};

		struct TrapInstance
		{
			std::uint64_t sourceToken{ 0 };
			RE::FormID ownerFormID{ 0 };
			RE::NiPoint3 position{};
			float radius{ 0.0f };

			RE::SpellItem* spell{ nullptr };
			float effectiveness{ 1.0f };
			float magnitudeOverride{ 0.0f };
			bool noHitEffectArt{ false };
			RE::SpellItem* extraSpell{ nullptr };

			std::chrono::steady_clock::time_point armedAt{};
			std::chrono::steady_clock::time_point expiresAt{};
			std::chrono::steady_clock::time_point createdAt{};

			std::chrono::milliseconds rearmDelay{ 0 };
			std::uint32_t maxTriggers{ 1 };
			std::uint32_t triggeredCount{ 0 };
			std::uint32_t maxTargetsPerTrigger{ 1 };
		};

			struct InstanceRuntimeState
			{
				std::uint32_t evolutionXp{ 0 };
				std::uint32_t modeCycleCounter{ 0 };
				std::uint32_t modeIndex{ 0 };
			};

			struct RunewordRecipe
			{
				std::string id{};
				std::uint64_t token{ 0 };
				std::string displayName{};
				std::vector<std::string> runeIds{};
				std::vector<std::uint64_t> runeTokens{};
				std::uint64_t resultAffixToken{ 0 };
				std::optional<LootItemType> recommendedBaseType{};
			};

			struct RunewordInstanceState
			{
				std::uint64_t recipeToken{ 0 };
				std::uint32_t insertedRunes{ 0 };
			};

		struct AffixRuntime
		{
			std::string id{};
			std::uint64_t token{ 0 };
			std::string keywordEditorId{};
			std::string label{};
			std::string displayName{};
			std::string displayNameEn{};
			std::string displayNameKo{};
			RE::BGSKeyword* keyword{ nullptr };
			std::optional<LootItemType> lootType{};
			Trigger trigger{ Trigger::kHit };
			float procChancePct{ 0.0f };
			// Weighted loot source:
			// - lootWeight >= 0 uses that value
			// - lootWeight < 0 means "not rollable from loot-time pool"
			// Values <=0 are excluded from weighted rolling.
			float lootWeight{ -1.0f };
			[[nodiscard]] constexpr float EffectiveLootWeight() const noexcept { return (lootWeight >= 0.0f) ? lootWeight : 0.0f; }
			std::chrono::milliseconds icd{ 0 };
			std::chrono::milliseconds perTargetIcd{ 0 };
			std::chrono::milliseconds requireRecentlyHit{ 0 };
			std::chrono::milliseconds requireRecentlyKill{ 0 };
			std::chrono::milliseconds requireNotHitRecently{ 0 };
			float luckyHitChancePct{ 0.0f };
			float luckyHitProcCoefficient{ 1.0f };
			Action action{};

			// Prefix/Suffix classification
			AffixSlot slot{ AffixSlot::kNone };
			std::string family{};            // suffix family grouping (e.g., "max_health")
			RE::SpellItem* passiveSpell{ nullptr };  // suffix: Ability spell to add/remove on equip
			float critDamageBonusPct{ 0.0f };        // suffix: crit damage bonus (applied in HandleHealthDamage hook)
			float scrollNoConsumeChancePct{ 0.0f };  // equipped: additive chance to preserve consumed scrolls

			std::chrono::steady_clock::time_point nextAllowed{};
		};

		struct LastHitKey
		{
			bool outgoing{ false };
			RE::FormID aggressor{ 0 };
			RE::FormID target{ 0 };
			RE::FormID source{ 0 };
		};

		// Shared wrappers (implemented via dedicated helper headers).
		static bool IsPlayerOwned(RE::Actor* a_actor);
		static RE::Actor* GetPlayerOwner(RE::Actor* a_actor);
		static void SendModEvent(std::string_view a_eventName, RE::TESForm* a_sender);

		// Config/event string constants and runtime guards.
		static constexpr std::string_view kDotKeywordEditorID = "CAFF_TAG_DOT";
		static constexpr std::chrono::milliseconds kDotApplyICD{ 1500 };
		static constexpr std::chrono::milliseconds kCastOnCritICD{ 150 };
		static constexpr std::chrono::milliseconds kDuplicateHitWindow{ 100 };
		static constexpr std::chrono::milliseconds kHealthDamageHookStaleWindow{ 5000 };
		static constexpr std::string_view kRuntimeConfigRelativePath = "Data/SKSE/Plugins/CalamityAffixes/affixes.json";
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
		static constexpr std::string_view kUiSetPanelEvent = "CalamityAffixes_UI_SetPanel";
		static constexpr std::string_view kUiTogglePanelEvent = "CalamityAffixes_UI_TogglePanel";
		static constexpr std::string_view kMcmSetEnabledEvent = "CalamityAffixes_MCM_SetEnabled";
		static constexpr std::string_view kMcmSetDebugNotificationsEvent = "CalamityAffixes_MCM_SetDebugNotifications";
		static constexpr std::string_view kMcmSetValidationIntervalEvent = "CalamityAffixes_MCM_SetValidationInterval";
		static constexpr std::string_view kMcmSetProcChanceMultEvent = "CalamityAffixes_MCM_SetProcChanceMult";
		static constexpr std::string_view kMcmSetLootChanceEvent = "CalamityAffixes_MCM_SetLootChance";
		static constexpr std::string_view kMcmSetDotSafetyAutoDisableEvent = "CalamityAffixes_MCM_SetDotSafetyAutoDisable";
		static constexpr std::string_view kMcmSetAllowNonHostileFirstHitProcEvent = "CalamityAffixes_MCM_SetAllowNonHostileFirstHitProc";
		static constexpr std::string_view kMcmSpawnTestItemEvent = "CalamityAffixes_MCM_SpawnTestItem";

		static constexpr std::array<float, kMaxAffixesPerItem> kAffixCountWeights = { 70.0f, 25.0f, 5.0f };
		static constexpr std::array<float, kMaxAffixesPerItem> kMultiAffixProcPenalty = { 1.0f, 0.8f, 0.65f };
		static constexpr std::uint32_t kLootChancePityFailThreshold = 3u;
		static constexpr std::size_t kLootEvaluatedRecentKeep = 2048;
		static constexpr std::size_t kLootEvaluatedPruneEveryInserts = 128;
		static constexpr std::size_t kLootPreviewCacheMax = 1024;

		static constexpr std::uint32_t kSerializationVersion = 6;
		static constexpr std::uint32_t kSerializationVersionV5 = 5;
		static constexpr std::uint32_t kSerializationVersionV4 = 4;
		static constexpr std::uint32_t kSerializationVersionV3 = 3;
		static constexpr std::uint32_t kSerializationVersionV2 = 2;
		static constexpr std::uint32_t kSerializationVersionV1 = 1;
		static constexpr std::uint32_t kSerializationRecordInstanceAffixes = 'IAXF';
		static constexpr std::uint32_t kSerializationRecordInstanceRuntimeStates = 'IRST';
		static constexpr std::uint32_t kSerializationRecordRunewordState = 'RWRD';
		static constexpr std::uint32_t kSerializationRecordLootEvaluated = 'LRLD';
			static constexpr std::uint32_t kSerializationRecordLootShuffleBags = 'LSBG';
			static constexpr std::uint32_t kRunewordSerializationVersion = 1;
			static constexpr std::uint32_t kInstanceRuntimeStateSerializationVersion = 1;
			static constexpr std::uint32_t kLootEvaluatedSerializationVersion = 1;
			static constexpr std::uint32_t kLootShuffleBagSerializationVersion = 2;

		// Runtime config/state PODs.
		enum class LootNameMarkerPosition : std::uint8_t
		{
			kLeading = 0,
			kTrailing = 1,
		};

		struct LootConfig
		{
			float chancePercent{ 0.0f };
			float runewordFragmentChancePercent{ 1.0f };
			float reforgeOrbChancePercent{ 6.0f };
			bool renameItem{ false };
			bool sharedPool{ false };
			bool debugLog{ false };
			bool dotTagSafetyAutoDisable{ false };
			std::uint32_t dotTagSafetyUniqueEffectThreshold{ 96 };
			std::uint32_t trapGlobalMaxActive{ 64 };
			std::uint32_t trapCastBudgetPerTick{ 8 };
			std::uint32_t triggerProcBudgetPerWindow{ 12 };
			std::uint32_t triggerProcBudgetWindowMs{ 100 };
			bool cleanupInvalidLegacyAffixes{ true };
			bool stripTrackedSuffixSlots{ true };
			std::vector<std::string> armorEditorIdDenyContains{};
			std::string nameFormat{ "{base} [{affix}]" };
			LootNameMarkerPosition nameMarkerPosition{ LootNameMarkerPosition::kTrailing };
		};

		struct LootShuffleBagState
		{
			std::vector<std::size_t> order{};
			std::size_t cursor{ 0 };
		};

		struct SelectedLootPreviewHint
		{
			std::uint64_t instanceKey{ 0u };
			std::uint64_t recordedAtMs{ 0u };
		};

		struct PerTargetCooldownKey
		{
			std::uint64_t token{ 0 };
			RE::FormID target{ 0 };
		};

		// Runtime state storage.
		// key = (targetFormID << 32) | magicEffectFormID
		std::unordered_map<std::uint64_t, std::chrono::steady_clock::time_point> _dotCooldowns;
		std::chrono::steady_clock::time_point _dotCooldownsLastPruneAt{};
		std::unordered_set<RE::FormID> _dotObservedMagicEffects;
		bool _dotTagSafetyWarned{ false };
		bool _dotTagSafetySuppressed{ false };

		PerTargetCooldownStore _perTargetCooldownStore{};
		NonHostileFirstHitGate _nonHostileFirstHitGate{};

		std::vector<AffixRuntime> _affixes;
		std::vector<std::uint32_t> _activeCounts;
		float _activeCritDamageBonusPct{ 0.0f };
		std::unordered_map<std::string, std::size_t> _affixIndexById;
		std::unordered_map<std::uint64_t, std::size_t> _affixIndexByToken;
		std::vector<std::size_t> _hitTriggerAffixIndices;
		std::vector<std::size_t> _incomingHitTriggerAffixIndices;
		std::vector<std::size_t> _dotApplyTriggerAffixIndices;
		std::vector<std::size_t> _killTriggerAffixIndices;
		std::vector<std::size_t> _castOnCritAffixIndices;
		std::vector<std::size_t> _convertAffixIndices;
		std::vector<std::size_t> _mindOverMatterAffixIndices;
		std::vector<std::size_t> _archmageAffixIndices;
		std::vector<std::size_t> _corpseExplosionAffixIndices;
		std::vector<std::size_t> _summonCorpseExplosionAffixIndices;
		std::vector<std::size_t> _lootWeaponAffixes;
		std::vector<std::size_t> _lootArmorAffixes;
		std::vector<std::size_t> _lootWeaponSuffixes;
		std::vector<std::size_t> _lootArmorSuffixes;
		std::vector<std::size_t> _lootSharedAffixes;
		std::vector<std::size_t> _lootSharedSuffixes;
		LootShuffleBagState _lootPrefixSharedBag{};
		LootShuffleBagState _lootPrefixWeaponBag{};
		LootShuffleBagState _lootPrefixArmorBag{};
		LootShuffleBagState _lootSuffixSharedBag{};
		LootShuffleBagState _lootSuffixWeaponBag{};
		LootShuffleBagState _lootSuffixArmorBag{};
		std::unordered_set<RE::SpellItem*> _appliedPassiveSpells;
		std::unordered_map<std::uint64_t, InstanceAffixSlots> _instanceAffixes;
		std::unordered_map<std::uint64_t, InstanceAffixSlots> _lootPreviewAffixes;
		std::deque<std::uint64_t> _lootPreviewRecent;
		std::unordered_map<RE::FormID, std::deque<SelectedLootPreviewHint>> _lootPreviewSelectedByBaseObj;
		std::unordered_set<std::uint64_t> _lootEvaluatedInstances;
		std::deque<std::uint64_t> _lootEvaluatedRecent;
		std::size_t _lootEvaluatedInsertionsSincePrune{ 0 };
		std::uint32_t _lootChanceEligibleFailStreak{ 0 };
		std::vector<float> _activeSlotPenalty;
		std::unordered_map<InstanceStateKey, InstanceRuntimeState, InstanceStateKeyHash> _instanceStates;
		std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> _equippedInstanceKeysByToken;
		bool _equippedTokenCacheReady{ false };
		std::vector<RunewordRecipe> _runewordRecipes;
		std::unordered_map<std::uint64_t, std::size_t> _runewordRecipeIndexByToken;
		std::unordered_map<std::uint64_t, std::size_t> _runewordRecipeIndexByResultAffixToken;
		std::unordered_map<std::uint64_t, std::string> _runewordRuneNameByToken;
		std::vector<std::uint64_t> _runewordRuneTokenPool;
		std::vector<double> _runewordRuneTokenWeights;
		std::unordered_map<std::uint64_t, std::uint32_t> _runewordRuneFragments;
		std::unordered_map<std::uint64_t, RunewordInstanceState> _runewordInstanceStates;
		std::optional<std::uint64_t> _runewordSelectedBaseKey{};
		std::uint32_t _runewordBaseCycleCursor{ 0 };
		std::uint32_t _runewordRecipeCycleCursor{ 0 };
		std::vector<TrapInstance> _traps;
		std::atomic_bool _hasActiveTraps{ false };
		LootConfig _loot{};
		bool _runtimeEnabled{ true };
		float _runtimeProcChanceMult{ 1.0f };
		std::atomic_bool _allowNonHostilePlayerOwnedOutgoingProcs{ false };
		LootRerollGuard _lootRerollGuard{};
		std::map<std::pair<RE::FormID, RE::FormID>, std::int32_t> _playerContainerStash;  // {containerID, baseObj} -> count
		bool _configLoaded{ false };
		std::uint32_t _procDepth{ 0 };
		bool _healthDamageHookSeen{ false };
		std::chrono::steady_clock::time_point _healthDamageHookLastAt{};
		std::uint64_t _triggerProcBudgetWindowStartMs{ 0u };
		std::uint32_t _triggerProcBudgetConsumed{ 0u };

		std::chrono::steady_clock::time_point _lastHitAt{};
		LastHitKey _lastHit{};
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _recentOwnerHitAt;
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _recentOwnerKillAt;
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _recentOwnerIncomingHitAt;

		std::uint64_t _lastHealthDamageSignature{ 0 };
		std::chrono::steady_clock::time_point _lastHealthDamageSignatureAt{};

		std::chrono::steady_clock::time_point _castOnCritNextAllowed{};
		std::size_t _castOnCritCycleCursor{ 0 };

		std::mt19937 _rng{ std::random_device{}() };

		static constexpr std::chrono::milliseconds kEquipResyncInterval{ 5000 };
		ResyncScheduler _equipResync{ .nextAtMs = 0, .intervalMs = static_cast<std::uint64_t>(kEquipResyncInterval.count()) };

		// Special-action selection/runtime structs.
		struct CorpseExplosionState
		{
			std::chrono::steady_clock::time_point lastExplosionAt{};
			std::chrono::steady_clock::time_point chainAnchorAt{};
			std::chrono::steady_clock::time_point rateWindowStartAt{};
			std::uint32_t chainDepth{ 0 };
			std::uint32_t explosionsInWindow{ 0 };
		};

		enum class CorpseExplosionBudgetDenyReason : std::uint8_t
		{
			kNone = 0,
			kDuplicateCorpse,
			kRateLimited,
			kChainDepthExceeded,
		};

		struct CorpseExplosionSelection
		{
			std::optional<std::size_t> bestIdx{};
			float bestBaseDamage{ 0.0f };
			std::uint32_t activeAffixes{ 0 };
			std::uint32_t blockedCooldown{ 0 };
			std::uint32_t blockedNoSpell{ 0 };
			std::uint32_t blockedZeroDamage{ 0 };
		};

		struct ArchmageSelection
		{
			std::optional<std::size_t> bestIdx{};
			const Action* bestAction{ nullptr };
			float bestDamagePct{ 0.0f };
			float bestCostPct{ 0.0f };
		};

		struct AdaptiveCastSelection
		{
			AdaptiveElement pick{ AdaptiveElement::kFire };
			RE::SpellItem* spell{ nullptr };
			float resistFire{ 0.0f };
			float resistFrost{ 0.0f };
			float resistShock{ 0.0f };
		};

		enum class RunewordApplyBlockReason : std::uint8_t
		{
			kNone = 0,
			kMissingResultAffix,
			kAffixSlotsFull,
		};

		CorpseExplosionState _corpseExplosionState{};
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _corpseExplosionSeenCorpses;
		CorpseExplosionState _summonCorpseExplosionState{};
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _summonCorpseExplosionSeenCorpses;

		// Runtime lifecycle and data maintenance.
		void ResetRuntimeStateForConfigReload();
		void MaybeResyncEquippedAffixes(std::chrono::steady_clock::time_point a_now);
		void RebuildActiveCounts();
		void MarkLootEvaluatedInstance(std::uint64_t a_instanceKey);
		void ForgetLootEvaluatedInstance(std::uint64_t a_instanceKey);
		void PruneLootEvaluatedInstances();
		[[nodiscard]] bool IsLootEvaluatedInstance(std::uint64_t a_instanceKey) const;
		void RemapInstanceKey(std::uint64_t a_oldKey, std::uint64_t a_newKey);
		void ProcessDroppedRefDeleted(LootRerollGuard::RefHandle a_refHandle);
		void EraseInstanceRuntimeStates(std::uint64_t a_instanceKey);
		[[nodiscard]] const InstanceAffixSlots* FindLootPreviewSlots(std::uint64_t a_instanceKey) const;
		[[nodiscard]] std::uint64_t FindSelectedLootPreviewKey(RE::FormID a_baseObj, std::uint64_t a_nowMs);
		void RememberSelectedLootPreviewKey(RE::FormID a_baseObj, std::uint64_t a_instanceKey, std::uint64_t a_nowMs);
		void ForgetSelectedLootPreviewKeyForInstance(std::uint64_t a_instanceKey);
		void RememberLootPreviewSlots(std::uint64_t a_instanceKey, const InstanceAffixSlots& a_slots);
		void ForgetLootPreviewSlots(std::uint64_t a_instanceKey);
		[[nodiscard]] bool RebindPendingLootPreviewForFallbackCandidate(
			RE::FormID a_baseObj,
			std::uint16_t a_uniqueID,
			RE::InventoryChanges* a_changes,
			RE::ExtraDataList* a_targetXList,
			std::uint64_t a_preferredSourcePreviewKey = 0u);
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
		[[nodiscard]] std::optional<float> LoadLootChancePercentFromMcmSettings() const;
		bool PersistLootChancePercentToMcmSettings(float a_chancePercent, bool a_overwriteExisting) const;
		void ApplyLootConfigFromJson(const nlohmann::json& a_configRoot);
		[[nodiscard]] const nlohmann::json* ResolveAffixArray(const nlohmann::json& a_configRoot) const;
		void IndexConfiguredAffixes();
		void RebuildSharedLootPools();
		void RegisterSynthesizedAffix(AffixRuntime&& a_affix, bool a_warnOnDuplicate);
		void SynthesizeRunewordRuntimeAffixes();
		void SanitizeRunewordState();
		void CycleRunewordBase(std::int32_t a_direction);
		void CycleRunewordRecipe(std::int32_t a_direction);
		void InsertRunewordRuneIntoSelectedBase();
		void GrantNextRequiredRuneFragment(std::uint32_t a_amount = 1u);
		void GrantCurrentRecipeRuneSet(std::uint32_t a_amount = 1u);
		void MaybeGrantRandomRunewordFragment();
		void MaybeGrantRandomReforgeOrb();
		void ApplyRunewordResult(std::uint64_t a_instanceKey, const RunewordRecipe& a_recipe);
		void LogRunewordStatus() const;
		InstanceRuntimeState& EnsureInstanceRuntimeState(std::uint64_t a_instanceKey, std::uint64_t a_affixToken);
		[[nodiscard]] const InstanceRuntimeState* FindInstanceRuntimeState(std::uint64_t a_instanceKey, std::uint64_t a_affixToken) const;
		[[nodiscard]] std::size_t ResolveEvolutionStageIndex(const Action& a_action, const InstanceRuntimeState* a_state) const;
		[[nodiscard]] float ResolveEvolutionMultiplier(const Action& a_action, const InstanceRuntimeState* a_state) const;
		void AdvanceRuntimeStateForAffixProc(const AffixRuntime& a_affix);
		void CycleManualModeForEquippedAffixes(std::int32_t a_direction, std::string_view a_affixIdFilter = {});

		// Loot processing path.
		void ProcessLootAcquired(RE::FormID a_baseObj, std::int32_t a_count, std::uint16_t a_uniqueID, bool a_allowRunewordFragmentRoll);
		[[nodiscard]] std::optional<std::size_t> RollLootAffixIndex(
			LootItemType a_itemType,
			const std::vector<std::size_t>* a_exclude = nullptr,
			bool a_skipChanceCheck = false);
		[[nodiscard]] std::optional<std::size_t> RollSuffixIndex(
			LootItemType a_itemType,
			const std::vector<std::string>* a_excludeFamilies = nullptr);
		[[nodiscard]] std::uint8_t RollAffixCount();
		[[nodiscard]] bool RollLootChanceGateForEligibleInstance();
		void ApplyMultipleAffixes(
			RE::InventoryChanges* a_changes,
			RE::InventoryEntryData* a_entry,
			RE::ExtraDataList* a_xList,
			LootItemType a_itemType,
			bool a_allowRunewordFragmentRoll);
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
		void ProcessTrigger(Trigger a_trigger, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData = nullptr);
		void RecordRecentCombatEvent(Trigger a_trigger, RE::Actor* a_owner, std::chrono::steady_clock::time_point a_now);
		[[nodiscard]] bool PassesRecentlyGates(
			const AffixRuntime& a_affix,
			RE::Actor* a_owner,
			std::chrono::steady_clock::time_point a_now) const;
		[[nodiscard]] bool PassesLuckyHitGate(
			const AffixRuntime& a_affix,
			Trigger a_trigger,
			const RE::HitData* a_hitData,
			std::chrono::steady_clock::time_point a_now);

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
		void ExecuteCastSpellAdaptiveElementAction(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData);
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
	};
}
