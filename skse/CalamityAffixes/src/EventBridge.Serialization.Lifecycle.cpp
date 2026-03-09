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
		_lootState.ResetForLoadOrRevert();
		_activeCounts.clear();
		_activeCritDamageBonusPct = 0.0f;
		_activeHitTriggerAffixIndices.clear();
		_activeIncomingHitTriggerAffixIndices.clear();
		_activeDotApplyTriggerAffixIndices.clear();
		_activeKillTriggerAffixIndices.clear();
		_activeLowHealthTriggerAffixIndices.clear();
		_instanceStates.clear();
		_runewordState.ResetSelectionAndProgress();
		_trapState.Reset();
		_corpseExplosionSeenCorpses.clear();
		_summonCorpseExplosionSeenCorpses.clear();
		_corpseExplosionState = {};
		_summonCorpseExplosionState = {};
		_combatState.ResetTransientState();
			_miscCurrencyMigrated = false;
		_miscCurrencyRecovered = false;

		for (auto& affix : _affixes) {
			affix.nextAllowed = {};
		}

		Hooks::ClearRuntimeState();
	}

	void EventBridge::OnPostLoadGame()
	{
		const std::scoped_lock lock(_stateMutex);
		MaybeMigrateMiscCurrency();
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
			_lootState.pendingDroppedRefDeletes.push_back(refHandle);
		}
		ScheduleDroppedRefDeleteDrain();
	}

	void EventBridge::ScheduleDroppedRefDeleteDrain()
	{
		{
			const std::scoped_lock lock(_stateMutex);
			if (_lootState.pendingDroppedRefDeletes.empty()) {
				return;
			}
		}

		bool expected = false;
		if (!_lootState.dropDeleteDrainScheduled.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
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
		_lootState.dropDeleteDrainScheduled.store(false, std::memory_order_release);
	}

	void EventBridge::DrainPendingDroppedRefDeletes()
	{
		std::vector<LootRerollGuard::RefHandle> pending;
		{
			const std::scoped_lock lock(_stateMutex);
			pending.swap(_lootState.pendingDroppedRefDeletes);
		}

		for (const auto refHandle : pending) {
			ProcessDroppedRefDeleted(refHandle);
		}

		_lootState.dropDeleteDrainScheduled.store(false, std::memory_order_release);

		bool hasMorePending = false;
		{
			const std::scoped_lock lock(_stateMutex);
			hasMorePending = !_lootState.pendingDroppedRefDeletes.empty();
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

		const auto keyOpt = _lootState.rerollGuard.ConsumeIfPlayerDropDeleted(a_refHandle);
		if (!keyOpt) {
			return;
		}

		const auto key = *keyOpt;
		const auto it = _instanceAffixes.find(key);
		const bool wasLootEvaluated = IsLootEvaluatedInstance(key);
		const bool hasRunewordState = _runewordState.instanceStates.contains(key);
		const bool isSelectedRunewordBase = (_runewordState.selectedBaseKey && *_runewordState.selectedBaseKey == key);
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
		_runewordState.instanceStates.erase(key);
		if (_runewordState.selectedBaseKey && *_runewordState.selectedBaseKey == key) {
			_runewordState.selectedBaseKey.reset();
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
		// Automatic migration disabled — use MCM "Recover Currency" button instead.
		_miscCurrencyMigrated = true;
	}

	std::string EventBridge::RecoverMiscCurrency()
	{
		const std::scoped_lock lock(_stateMutex);

		if (_miscCurrencyRecovered) {
			return "Already recovered this save. / 이미 복구 완료된 세이브입니다.";
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return "Player not available.";
		}

		auto* orb = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_Misc_ReforgeOrb");
		if (!orb) {
			SKSE::log::warn("CalamityAffixes: RecoverMiscCurrency — reforge orb EditorID lookup failed.");
			return "Reforge orb lookup failed.";
		}

		// Grant reforge orbs.
		static constexpr std::uint32_t kRecoveryOrbGrant = 5u;
		player->AddObjectToContainer(orb, nullptr,
			static_cast<std::int32_t>(kRecoveryOrbGrant), nullptr);

		// Grant rune fragments: give 1 of each known rune type the player doesn't own.
		if (_runewordState.runeNameByToken.empty()) {
			InitializeRunewordCatalog();
		}
		std::uint32_t fragmentsGranted = 0u;
		for (const auto& [runeToken, runeName] : _runewordState.runeNameByToken) {
			if (runeToken == 0u || runeName.empty()) {
				continue;
			}
			const auto owned = RunewordDetail::GetOwnedRunewordFragmentCount(
				player, _runewordState.runeNameByToken, runeToken);
			if (owned == 0u) {
				const auto given = RunewordDetail::GrantRunewordFragments(
					player, _runewordState.runeNameByToken, runeToken, 1u);
				fragmentsGranted += given;
			}
		}

		_miscCurrencyRecovered = true;

		SKSE::log::info(
			"CalamityAffixes: RecoverMiscCurrency — granted {} orbs and {} rune fragment types.",
			kRecoveryOrbGrant, fragmentsGranted);
		EmitHudNotification("Calamity: recovered missing currency items.");

		std::string result = "Granted ";
		result.append(std::to_string(kRecoveryOrbGrant));
		result.append(" orbs, ");
		result.append(std::to_string(fragmentsGranted));
		result.append(" fragment types.");
		return result;
	}
}
