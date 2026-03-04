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
#include "CalamityAffixes/EventNames.h"
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

		// Internal domain types (enums, value-type structs).
		#define CALAMITYAFFIXES_EVENTBRIDGE_TYPES_INL_CONTEXT 1
		#include "detail/EventBridge.Types.inl"
		#undef CALAMITYAFFIXES_EVENTBRIDGE_TYPES_INL_CONTEXT

		// Shared wrappers (implemented via dedicated helper headers).
		static bool IsPlayerOwned(RE::Actor* a_actor);
		static RE::Actor* GetPlayerOwner(RE::Actor* a_actor);
		static void SendModEvent(std::string_view a_eventName, RE::TESForm* a_sender);

		// Config/event string constants, policy constants, serialization constants.
		#define CALAMITYAFFIXES_EVENTBRIDGE_CONSTANTS_INL_CONTEXT 1
		#include "detail/EventBridge.Constants.inl"
		#undef CALAMITYAFFIXES_EVENTBRIDGE_CONSTANTS_INL_CONTEXT

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
		std::unordered_set<std::string> _affixLabelSet;
		// Trigger dispatch caches — all affixes of each trigger type (config-time).
		std::vector<std::size_t> _hitTriggerAffixIndices;
		std::vector<std::size_t> _incomingHitTriggerAffixIndices;
		std::vector<std::size_t> _dotApplyTriggerAffixIndices;
		std::vector<std::size_t> _killTriggerAffixIndices;
		std::vector<std::size_t> _lowHealthTriggerAffixIndices;

		// Active trigger caches — subset currently equipped on the player (runtime).
		std::vector<std::size_t> _activeHitTriggerAffixIndices;
		std::vector<std::size_t> _activeIncomingHitTriggerAffixIndices;
		std::vector<std::size_t> _activeDotApplyTriggerAffixIndices;
		std::vector<std::size_t> _activeKillTriggerAffixIndices;
		std::vector<std::size_t> _activeLowHealthTriggerAffixIndices;

		// Special action caches — affixes with non-standard action types.
		std::vector<std::size_t> _castOnCritAffixIndices;
		std::vector<std::size_t> _convertAffixIndices;
		std::vector<std::size_t> _mindOverMatterAffixIndices;
		std::vector<std::size_t> _archmageAffixIndices;
		std::vector<std::size_t> _corpseExplosionAffixIndices;
		std::vector<std::size_t> _summonCorpseExplosionAffixIndices;

		// Loot pool caches — rollable affixes by item type (config-time).
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
		RuntimeUserSettingsDebounce::State _runtimeUserSettingsPersist{};
		LootRerollGuard _lootRerollGuard{};
		std::vector<LootRerollGuard::RefHandle> _pendingDroppedRefDeletes;
		std::atomic_bool _dropDeleteDrainScheduled{ false };
		std::map<std::pair<RE::FormID, RE::FormID>, std::int32_t> _playerContainerStash;  // {containerID, baseObj} -> count
		bool _miscCurrencyMigrated{ false };
		bool _miscCurrencyRecovered{ false };
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
		// Single coarse-grained lock by design: cross-domain interactions
		// (trigger→loot, loot→serialization, etc.) make fine-grained splitting
		// deadlock-prone without measurable contention benefit in a game mod.
		mutable std::recursive_mutex _stateMutex;

		ResyncScheduler _equipResync{ .nextAtMs = 0, .intervalMs = static_cast<std::uint64_t>(kEquipResyncInterval.count()) };

		CorpseExplosionState _corpseExplosionState{};
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _corpseExplosionSeenCorpses;
		CorpseExplosionState _summonCorpseExplosionState{};
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> _summonCorpseExplosionSeenCorpses;

		#define CALAMITYAFFIXES_EVENTBRIDGE_PRIVATE_API_INL_CONTEXT 1
		#include "detail/EventBridge.PrivateApi.inl"
		#undef CALAMITYAFFIXES_EVENTBRIDGE_PRIVATE_API_INL_CONTEXT
	};
}
