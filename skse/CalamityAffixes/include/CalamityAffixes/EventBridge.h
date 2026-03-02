#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
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
#include "CalamityAffixes/LootCurrencyLedger.h"
#include "CalamityAffixes/LootRerollGuard.h"
#include "CalamityAffixes/MagnitudeScaling.h"
#include "CalamityAffixes/NonHostileFirstHitGate.h"
#include "CalamityAffixes/PerTargetCooldownStore.h"
#include "CalamityAffixes/RuntimeUserSettingsDebounce.h"
#include "CalamityAffixes/RuntimePolicy.h"
#include "CalamityAffixes/ResyncScheduler.h"

namespace CalamityAffixes
{
	namespace RunewordSynthesis
	{
		struct SpellSet;
	}

	class EventBridge final :
		public RE::BSTEventSink<RE::TESHitEvent>,
		public RE::BSTEventSink<RE::TESDeathEvent>,
		public RE::BSTEventSink<RE::TESEquipEvent>,
		public RE::BSTEventSink<RE::TESActivateEvent>,
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
			std::string effectSummaryText{};
			std::string effectDetailText{};
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

		#define CALAMITYAFFIXES_EVENTBRIDGE_PUBLIC_API_INL_CONTEXT 1
		#include "detail/EventBridge.PublicApi.inl"
		#undef CALAMITYAFFIXES_EVENTBRIDGE_PUBLIC_API_INL_CONTEXT

	private:
		EventBridge() = default;

		// Internal domain enums.
		enum class Trigger : std::uint8_t
		{
			kHit,
			kIncomingHit,
			kDotApply,
			kKill,
			kLowHealth,
		};

		enum class PlayerCombatEvidenceSource : std::uint8_t
		{
			kUnknown = 0,
			kHealthDamageRoutedHit,
			kTesHitOutgoing,
			kTesHitIncoming,
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
				std::string displayNameEn{};
				std::string displayNameKo{};
				std::vector<std::string> runeIds{};
				std::vector<std::uint64_t> runeTokens{};
				std::uint64_t resultAffixToken{ 0 };
				std::optional<LootItemType> recommendedBaseType{};
			};

			enum class SyntheticRunewordStyle : std::uint8_t
			{
				kAdaptiveStrike,
				kAdaptiveExposure,
				kSignatureGrief,
				kSignatureInfinity,
				kSignatureEnigma,
				kSignatureCallToArms,
				kFireStrike,
				kFrostStrike,
				kShockStrike,
				kPoisonBloom,
				kTarBloom,
				kSiphonBloom,
				kCurseFragile,
				kCurseSlowAttack,
				kCurseFear,
				kCurseFrenzy,
				kSelfHaste,
				kSelfWard,
				kSelfBarrier,
				kSelfMeditation,
				kSelfPhase,
				kSelfPhoenix,
				kSelfFlameCloak,
				kSelfFrostCloak,
				kSelfShockCloak,
				kSelfOakflesh,
				kSelfStoneflesh,
				kSelfIronflesh,
				kSelfEbonyflesh,
				kSelfMuffle,
				kSelfInvisibility,
				kSoulTrap,
				kSummonFamiliar,
				kSummonFlameAtronach,
				kSummonFrostAtronach,
				kSummonStormAtronach,
				kSummonDremoraLord,
				kLegacyFallback
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
			float lowHealthThresholdPct{ 35.0f };
			float lowHealthRearmPct{ 45.0f };
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

		// Runtime config/state PODs.
		enum class LootNameMarkerPosition : std::uint8_t
		{
			kLeading = 0,
			kTrailing = 1,
		};

		enum class CurrencyDropMode : std::uint8_t
		{
			kHybrid = 2,
		};

			struct LootConfig
			{
				float chancePercent{ 0.0f };
				float runewordFragmentChancePercent{ 8.0f };
				float reforgeOrbChancePercent{ 5.0f };
				float configuredRunewordFragmentChancePercent{ 8.0f };
				float configuredReforgeOrbChancePercent{ 5.0f };
				CurrencyDropMode currencyDropMode{ CurrencyDropMode::kHybrid };
				bool runtimeCurrencyDropsEnabled{ true };
				float lootSourceChanceMultCorpse{ 0.80f };
				float lootSourceChanceMultContainer{ 1.00f };
				float lootSourceChanceMultBossContainer{ 1.15f };
				float lootSourceChanceMultWorld{ 1.00f };
				bool renameItem{ false };
				bool sharedPool{ false };
				bool debugLog{ false };
				bool dotTagSafetyAutoDisable{ false };
				std::uint32_t dotTagSafetyUniqueEffectThreshold{ 96 };
				std::uint32_t trapGlobalMaxActive{ 48 };
				std::uint32_t trapCastBudgetPerTick{ 6 };
				std::uint32_t triggerProcBudgetPerWindow{ 12 };
				std::uint32_t triggerProcBudgetWindowMs{ 100 };
				bool cleanupInvalidLegacyAffixes{ true };
				bool stripTrackedSuffixSlots{ true };
				std::vector<std::string> armorEditorIdDenyContains{};
				std::vector<std::string> bossContainerEditorIdAllowContains{};
				std::vector<std::string> bossContainerEditorIdDenyContains{};
				std::string nameFormat{ "{base} [{affix}]" };
				LootNameMarkerPosition nameMarkerPosition{ LootNameMarkerPosition::kTrailing };
			};

		struct LootShuffleBagState
		{
			std::vector<std::size_t> order{};
			std::size_t cursor{ 0 };
		};

		struct CurrencyRollExecutionResult
		{
			bool runewordDropGranted{ false };
			bool reforgeDropGranted{ false };
		};

		struct PerTargetCooldownKey
		{
			std::uint64_t token{ 0 };
			RE::FormID target{ 0 };
		};

		struct LowHealthTriggerKey
		{
			std::uint64_t token{ 0 };
			RE::FormID owner{ 0 };

			[[nodiscard]] bool operator==(const LowHealthTriggerKey& a_rhs) const noexcept
			{
				return token == a_rhs.token && owner == a_rhs.owner;
			}
		};

		struct LowHealthTriggerKeyHash
		{
			[[nodiscard]] std::size_t operator()(const LowHealthTriggerKey& a_key) const noexcept
			{
				const auto owner64 = static_cast<std::uint64_t>(a_key.owner);
				return static_cast<std::size_t>((a_key.token << 1) ^ (owner64 * 0x9E3779B185EBCA87ull));
			}
		};

		// Runtime state storage.
		// key = (targetFormID << 32) | magicEffectFormID
		std::unordered_map<std::uint64_t, std::chrono::steady_clock::time_point> _dotCooldowns;
		std::chrono::steady_clock::time_point _dotCooldownsLastPruneAt{};
		std::unordered_set<RE::FormID> _dotObservedMagicEffects;
		bool _dotTagSafetyWarned{ false };
		bool _dotObservedMagicEffectsCapWarned{ false };
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
		std::vector<std::size_t> _lowHealthTriggerAffixIndices;
		std::vector<std::size_t> _activeHitTriggerAffixIndices;
		std::vector<std::size_t> _activeIncomingHitTriggerAffixIndices;
		std::vector<std::size_t> _activeDotApplyTriggerAffixIndices;
		std::vector<std::size_t> _activeKillTriggerAffixIndices;
		std::vector<std::size_t> _activeLowHealthTriggerAffixIndices;
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
		std::unordered_set<std::uint64_t> _lootEvaluatedInstances;
		std::deque<std::uint64_t> _lootEvaluatedRecent;
		std::size_t _lootEvaluatedInsertionsSincePrune{ 0 };
		std::unordered_map<std::uint64_t, std::uint32_t> _lootCurrencyRollLedger;
		std::deque<std::uint64_t> _lootCurrencyRollLedgerRecent;
		std::uint32_t _lootChanceEligibleFailStreak{ 0 };
		std::uint32_t _runewordFragmentFailStreak{ 0 };
		std::uint32_t _reforgeOrbFailStreak{ 0 };
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
		bool _runewordTransmuteInProgress{ false };
		std::vector<TrapInstance> _traps;
		std::size_t _trapTickCursor{ 0 };
		std::atomic_bool _hasActiveTraps{ false };
		LootConfig _loot{};
		bool _runtimeEnabled{ true };
		float _runtimeProcChanceMult{ 1.0f };
		std::atomic_bool _allowNonHostilePlayerOwnedOutgoingProcs{ false };
		bool _combatDebugLog{ false };
		bool _disableCombatEvidenceLease{ false };
		bool _disableHealthDamageRouting{ false };
		bool _allowPlayerHealthDamageHook{ false };
		bool _disablePassiveSuffixSpells{ false };
		bool _disableTrapSystemTick{ false };
		bool _disableTrapCasts{ false };
		bool _forceStopAlarmPulse{ false };
		static constexpr std::chrono::milliseconds kRuntimeUserSettingsPersistDebounce{ 250 };
		RuntimeUserSettingsDebounce::State _runtimeUserSettingsPersist{};
		LootRerollGuard _lootRerollGuard{};
		std::vector<LootRerollGuard::RefHandle> _pendingDroppedRefDeletes;
		std::atomic_bool _dropDeleteDrainScheduled{ false };
		std::map<std::pair<RE::FormID, RE::FormID>, std::int32_t> _playerContainerStash;  // {containerID, baseObj} -> count
		bool _miscCurrencyMigrated{ false };
		bool _configLoaded{ false };
		std::atomic_bool _eventSinksRegistered{ false };
		std::atomic<std::uint32_t> _procDepth{ 0 };
		bool _healthDamageHookSeen{ false };
		std::chrono::steady_clock::time_point _healthDamageHookLastAt{};
		std::uint64_t _triggerProcBudgetWindowStartMs{ 0u };
		std::uint32_t _triggerProcBudgetConsumed{ 0u };

		std::chrono::steady_clock::time_point _lastHitAt{};
		LastHitKey _lastHit{};
		std::chrono::steady_clock::time_point _lastPapyrusHitEventAt{};
		LastHitKey _lastPapyrusHit{};
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _recentOwnerHitAt;
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _recentOwnerKillAt;
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _outgoingHitPerTargetLastAt;
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _recentOwnerIncomingHitAt;
		std::chrono::steady_clock::time_point _playerCombatEvidenceExpiresAt{};
		std::unordered_map<LowHealthTriggerKey, bool, LowHealthTriggerKeyHash> _lowHealthTriggerConsumed;
		std::unordered_map<RE::FormID, float> _lowHealthLastObservedPct;

		std::uint64_t _lastHealthDamageSignature{ 0 };
		std::chrono::steady_clock::time_point _lastHealthDamageSignatureAt{};

		std::chrono::steady_clock::time_point _castOnCritNextAllowed{};
		std::size_t _castOnCritCycleCursor{ 0 };

		static std::mt19937::result_type MakeRngSeed() noexcept
		{
			auto rd = std::random_device{}();
			auto ts = static_cast<std::mt19937::result_type>(
				std::chrono::steady_clock::now().time_since_epoch().count());
			// Mix entropy sources to defend against poor random_device implementations.
			rd ^= ts;
			rd ^= static_cast<std::mt19937::result_type>(reinterpret_cast<std::uintptr_t>(&rd) >> 3);
			return rd;
		}
		std::mt19937 _rng{ MakeRngSeed() };
		mutable std::mutex _rngMutex;
		mutable std::recursive_mutex _stateMutex;

		static constexpr std::chrono::milliseconds kEquipResyncInterval{ 8000 };
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

		#define CALAMITYAFFIXES_EVENTBRIDGE_PRIVATE_API_INL_CONTEXT 1
		#include "detail/EventBridge.PrivateApi.inl"
		#undef CALAMITYAFFIXES_EVENTBRIDGE_PRIVATE_API_INL_CONTEXT
	};
}
