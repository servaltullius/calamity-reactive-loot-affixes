#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/Hooks.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>

namespace CalamityAffixes
{
	void EventBridge::Revert(SKSE::SerializationInterface*)
	{
		const std::scoped_lock lock(_stateMutex);

		// Remove any active passive suffix spells before clearing
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player) {
			for (auto* spell : _appliedPassiveSpells) {
				player->RemoveSpell(spell);
			}
		}
		_appliedPassiveSpells.clear();
		_instanceAffixes.clear();
		_equippedInstanceKeysByToken.clear();
		_equippedTokenCacheReady = false;
		_lootPrefixSharedBag = {};
		_lootPrefixWeaponBag = {};
		_lootPrefixArmorBag = {};
		_lootSuffixSharedBag = {};
		_lootSuffixWeaponBag = {};
		_lootSuffixArmorBag = {};
		_lootPreviewAffixes.clear();
		_lootPreviewRecent.clear();
		_lootEvaluatedInstances.clear();
		_lootEvaluatedRecent.clear();
		_lootEvaluatedInsertionsSincePrune = 0;
		_lootCurrencyRollLedger.clear();
		_lootCurrencyRollLedgerRecent.clear();
		_lootChanceEligibleFailStreak = 0;
		_runewordFragmentFailStreak = 0;
		_reforgeOrbFailStreak = 0;
		_activeCounts.clear();
		_activeSlotPenalty.clear();
		_activeCritDamageBonusPct = 0.0f;
		_activeHitTriggerAffixIndices.clear();
		_activeIncomingHitTriggerAffixIndices.clear();
		_activeDotApplyTriggerAffixIndices.clear();
		_activeKillTriggerAffixIndices.clear();
		_activeLowHealthTriggerAffixIndices.clear();
		_instanceStates.clear();
		_runewordRuneFragments.clear();
		_runewordInstanceStates.clear();
		_runewordSelectedBaseKey.reset();
		_runewordBaseCycleCursor = 0;
		_runewordRecipeCycleCursor = 0;
		_runewordTransmuteInProgress = false;
		_playerContainerStash.clear();
		_traps.clear();
		_hasActiveTraps.store(false, std::memory_order_relaxed);
		_dotCooldowns.clear();
		_dotCooldownsLastPruneAt = {};
		_dotObservedMagicEffects.clear();
		_dotTagSafetyWarned = false;
		_dotObservedMagicEffectsCapWarned = false;
		_dotTagSafetySuppressed = false;
		_perTargetCooldownStore.Clear();
		_nonHostileFirstHitGate.Clear();
		_corpseExplosionSeenCorpses.clear();
		_summonCorpseExplosionSeenCorpses.clear();
		_lootRerollGuard.Reset();
		_corpseExplosionState = {};
		_summonCorpseExplosionState = {};
		_procDepth = 0;
		_healthDamageHookSeen = false;
		_healthDamageHookLastAt = {};
		_castOnCritNextAllowed = {};
		_castOnCritCycleCursor = 0;
		_lastHitAt = {};
		_lastHit = {};
		_lastPapyrusHitEventAt = {};
		_lastPapyrusHit = {};
		_recentOwnerHitAt.clear();
		_recentOwnerKillAt.clear();
		_recentOwnerIncomingHitAt.clear();
		_lowHealthTriggerConsumed.clear();
		_lowHealthLastObservedPct.clear();
		_triggerProcBudgetWindowStartMs = 0u;
		_triggerProcBudgetConsumed = 0u;
		_lastHealthDamageSignature = 0;
		_lastHealthDamageSignatureAt = {};
		_miscCurrencyMigrated = false;

		for (auto& affix : _affixes) {
			affix.nextAllowed = {};
		}

		Hooks::ClearRuntimeState();
	}

	void EventBridge::OnFormDelete(RE::VMHandle a_handle)
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm) {
			return;
		}

		auto* policy = vm->GetObjectHandlePolicy();
		if (!policy) {
			return;
		}

		if (!policy->HandleIsType(RE::FormType::Reference, a_handle)) {
			return;
		}

		auto* form = policy->GetObjectForHandle(RE::FormType::Reference, a_handle);
		auto* ref = form ? form->As<RE::TESObjectREFR>() : nullptr;
		if (!ref) {
			return;
		}

		const auto refHandle = static_cast<LootRerollGuard::RefHandle>(ref->GetHandle().native_handle());
		if (refHandle == 0) {
			return;
		}

		{
			const std::scoped_lock lock(_stateMutex);
			_pendingDroppedRefDeletes.push_back(refHandle);
		}
		ScheduleDroppedRefDeleteDrain();
	}

	void EventBridge::ScheduleDroppedRefDeleteDrain()
	{
		{
			const std::scoped_lock lock(_stateMutex);
			if (_pendingDroppedRefDeletes.empty()) {
				return;
			}
		}

		bool expected = false;
		if (!_dropDeleteDrainScheduled.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
			return;
		}

		if (auto* task = SKSE::GetTaskInterface()) {
			task->AddTask([]() {
				if (auto* bridge = EventBridge::GetSingleton(); bridge) {
					bridge->DrainPendingDroppedRefDeletes();
				}
			});
			return;
		}

		// Task interface unavailable: keep pending queue for later safe drain.
		_dropDeleteDrainScheduled.store(false, std::memory_order_release);
	}

	void EventBridge::DrainPendingDroppedRefDeletes()
	{
		std::vector<LootRerollGuard::RefHandle> pending;
		{
			const std::scoped_lock lock(_stateMutex);
			pending.swap(_pendingDroppedRefDeletes);
		}

		for (const auto refHandle : pending) {
			ProcessDroppedRefDeleted(refHandle);
		}

		_dropDeleteDrainScheduled.store(false, std::memory_order_release);

		bool hasMorePending = false;
		{
			const std::scoped_lock lock(_stateMutex);
			hasMorePending = !_pendingDroppedRefDeletes.empty();
		}
		if (hasMorePending) {
			ScheduleDroppedRefDeleteDrain();
		}
	}

	void EventBridge::EraseInstanceRuntimeStates(std::uint64_t a_instanceKey)
	{
		std::erase_if(_instanceStates, [a_instanceKey](const auto& entry) {
			return entry.first.instanceKey == a_instanceKey;
		});
	}

	void EventBridge::ProcessDroppedRefDeleted(LootRerollGuard::RefHandle a_refHandle)
	{
		const std::scoped_lock lock(_stateMutex);

		const auto keyOpt = _lootRerollGuard.ConsumeIfPlayerDropDeleted(a_refHandle);
		if (!keyOpt) {
			return;
		}

		const auto key = *keyOpt;
		const auto it = _instanceAffixes.find(key);
		const bool wasLootEvaluated = IsLootEvaluatedInstance(key);
		const bool hasRunewordState = _runewordInstanceStates.contains(key);
		const bool isSelectedRunewordBase = (_runewordSelectedBaseKey && *_runewordSelectedBaseKey == key);
		if (it == _instanceAffixes.end() &&
			!wasLootEvaluated &&
			!hasRunewordState &&
			!isSelectedRunewordBase) {
			return;
		}

		// Safety net: if the instance is already back in the player inventory, don't prune it.
		if (PlayerHasInstanceKey(key)) {
			return;
		}

		if (it != _instanceAffixes.end()) {
			_instanceAffixes.erase(it);
		}
		ForgetLootEvaluatedInstance(key);
		EraseInstanceRuntimeStates(key);
		_runewordInstanceStates.erase(key);
		if (_runewordSelectedBaseKey && *_runewordSelectedBaseKey == key) {
			_runewordSelectedBaseKey.reset();
		}
		if (_loot.debugLog) {
			SKSE::log::debug("CalamityAffixes: pruned instance affix (world ref deleted) (key={:016X}).", key);
		}
	}

	bool EventBridge::PlayerHasInstanceKey(std::uint64_t a_instanceKey) const
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return false;
		}

		auto* changes = player->GetInventoryChanges();
		if (!changes || !changes->entryList) {
			return false;
		}

		for (auto* entry : *changes->entryList) {
			if (!entry || !entry->extraLists) {
				continue;
			}

			for (auto* xList : *entry->extraLists) {
				if (!xList) {
					continue;
				}

				const auto* uid = xList->GetByType<RE::ExtraUniqueID>();
				if (!uid) {
					continue;
				}

				if (MakeInstanceKey(uid->baseID, uid->uniqueID) == a_instanceKey) {
					return true;
				}
			}
		}

		return false;
	}

	void EventBridge::MaybeMigrateMiscCurrency()
	{
		if (_miscCurrencyMigrated) {
			return;
		}

		// Detect and recover physical MISC items (rune fragments, reforge orbs) lost
		// due to ESP FormID changes between mod versions.
		//
		// Detection: player has affix instance data (_instanceAffixes non-empty,
		// proving they played with the mod) but owns ZERO reforge orbs.  Since the
		// starter grant gives orbs on first use, having affix data with zero orbs
		// is a strong signal of FormID loss.
		//
		// This runs once per save file; after granting, the flag persists in the
		// co-save and prevents repeated grants on subsequent loads.

		if (_instanceAffixes.empty()) {
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		// Check reforge orb — most reliable signal since every player gets starter orbs.
		auto* orb = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_Misc_ReforgeOrb");
		if (!orb) {
			SKSE::log::warn("CalamityAffixes: MISC migration — reforge orb EditorID lookup failed.");
			return;
		}

		const auto ownedOrbs = std::max(0, player->GetItemCount(orb));
		if (ownedOrbs > 0) {
			// Player has orbs — no FormID loss detected.
			return;
		}

		// FormID loss detected: player has affix data but zero orbs.
		SKSE::log::info(
			"CalamityAffixes: MISC migration — FormID loss detected (instances={}, orbs=0). Granting recovery items.",
			_instanceAffixes.size());

		// Grant reforge orbs.
		static constexpr std::uint32_t kMigrationOrbGrant = 5u;
		player->AddObjectToContainer(orb, nullptr,
			static_cast<std::int32_t>(kMigrationOrbGrant), nullptr);

		// Grant rune fragments: give 1 of each known rune type.
		if (_runewordRuneNameByToken.empty()) {
			InitializeRunewordCatalog();
		}
		std::uint32_t fragmentsGranted = 0u;
		for (const auto& [runeToken, runeName] : _runewordRuneNameByToken) {
			if (runeToken == 0u || runeName.empty()) {
				continue;
			}
			const auto owned = RunewordDetail::GetOwnedRunewordFragmentCount(
				player, _runewordRuneNameByToken, runeToken);
			if (owned == 0u) {
				const auto given = RunewordDetail::GrantRunewordFragments(
					player, _runewordRuneNameByToken, runeToken, 1u);
				fragmentsGranted += given;
			}
		}

		_miscCurrencyMigrated = true;

		SKSE::log::info(
			"CalamityAffixes: MISC migration — granted {} orbs and {} rune fragment types.",
			kMigrationOrbGrant, fragmentsGranted);
		RE::DebugNotification("Calamity: recovered missing currency items.");
	}
}
