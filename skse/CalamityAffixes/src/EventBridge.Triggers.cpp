#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LowHealthTriggerSnapshot.h"
#include "CalamityAffixes/TriggerDispatchSnapshot.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace CalamityAffixes
{
	bool EventBridge::CanProcessTriggerDispatch(
		Trigger a_trigger,
		RE::Actor* a_owner,
		RE::Actor* a_target,
		std::vector<std::size_t>& a_outIndices) const
	{
		a_outIndices.clear();
		if (!_configLoaded || !_runtimeSettings.enabled.load(std::memory_order_relaxed) || !a_owner || !a_target) {
			return false;
		}

		if (_combatState.procDepth > 0) {
			return false;
		}

		// Hand back a copy rather than a pointer into the live cache: the
		// dispatch loop re-enters the engine, and a re-entrant
		// RebuildActiveCounts would reallocate that cache underneath it.
		// See TriggerDispatchSnapshot.h.
		(void)detail::SnapshotTriggerIndices(ResolveActiveTriggerIndices(a_trigger), a_outIndices);
		return true;
	}

	bool EventBridge::TryProcessTriggerAffix(
		std::size_t a_affixIndex,
		Trigger a_trigger,
		RE::Actor* a_owner,
		RE::Actor* a_target,
		const RE::HitData* a_hitData,
		std::chrono::steady_clock::time_point a_now,
		float a_lowHealthPreviousPct,
		float a_lowHealthCurrentPct,
		bool& a_loggedProcBudgetDenied)
	{
		if (a_affixIndex >= _affixRuntimeState.affixes.size() || a_affixIndex >= _affixRuntimeState.activeCounts.size() || _affixRuntimeState.activeCounts[a_affixIndex] == 0) {
			return false;
		}

		auto& affix = _affixRuntimeState.affixes[a_affixIndex];
		PerTargetCooldownKey perTargetKey{};
		if (!PassesTriggerProcPreconditions(
				affix,
				a_trigger,
				a_owner,
				a_target,
				a_hitData,
				a_now,
				a_lowHealthPreviousPct,
				a_lowHealthCurrentPct,
				&perTargetKey)) {
			return false;
		}

		// Check SpawnTrap-specific hit/target requirements before chance, budget,
		// and ICD commits so an ineligible hit cannot consume the next valid proc.
		if (affix.action.type == ActionType::kSpawnTrap) {
			RE::Actor* ignoredSpawnTarget = nullptr;
			const char* failureReason = "unknown";
			if (!SelectSpawnTrapTarget(
					affix.action,
					a_owner,
					a_target,
					a_hitData,
					ignoredSpawnTarget,
					&failureReason)) {
				if (_loot.debugLog) {
					SKSE::log::debug(
						"CalamityAffixes: trap proc precondition skipped (affixId={}, reason={}).",
						affix.id,
						failureReason);
				}
				return false;
			}
		}

		// Validate every failure-prone action prerequisite before chance, proc
		// budget, ICD, evolution XP, or low-health state is committed.
		if (!CanExecuteAction(affix, a_owner, a_target, a_hitData)) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: action preflight skipped before proc state commit (affixId={}, actionType={}).",
					affix.id,
					static_cast<std::uint32_t>(affix.action.type));
			}
			return false;
		}

		const float chance = ResolveTriggerProcChancePct(affix, a_affixIndex);
		if (!RollTriggerProcChance(chance)) {
			return false;
		}

		if (!TryConsumeTriggerProcBudget(a_now)) {
			if (_loot.debugLog && !a_loggedProcBudgetDenied) {
				SKSE::log::debug(
					"CalamityAffixes: trigger proc budget exhausted (budget={} / windowMs={}).",
					_loot.triggerProcBudgetPerWindow,
					_loot.triggerProcBudgetWindowMs);
				a_loggedProcBudgetDenied = true;
			}
			return false;
		}

		const bool usesPerTargetIcd = (affix.perTargetIcd.count() > 0 && a_target && affix.token != 0u);
		CommitTriggerProcRuntime(affix, perTargetKey, usesPerTargetIcd, chance, a_now);

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: proc (affixId={}, trigger={}, chancePct={}, target={}, hasHitData={})",
				affix.id,
				static_cast<std::uint32_t>(a_trigger),
				chance,
				a_target ? a_target->GetName() : "<none>",
				(a_hitData != nullptr));
		}
		if (_loot.debugHudNotifications) {
			const std::string note = std::string("CAFF proc: ") + affix.id;
			EmitDebugHudNotification(note.c_str());
		}

		AdvanceRuntimeStateForAffixProc(affix);
		ExecuteActionWithProcDepthGuard(affix, a_owner, a_target, a_hitData);
		MarkLowHealthTriggerConsumed(affix, a_owner);
		return true;
	}

	void EventBridge::FinalizeTriggerDispatch(
		Trigger a_trigger,
		RE::Actor* a_owner,
		RE::FormID a_lowHealthOwnerFormID,
		bool a_hasLowHealthSnapshot,
		float a_lowHealthCurrentPct,
		std::chrono::steady_clock::time_point a_now)
	{
		if (a_hasLowHealthSnapshot) {
			// Update once per trigger pass so all low-health affixes evaluate against the same health snapshot.
			StoreLowHealthTriggerSnapshot(a_lowHealthOwnerFormID, a_lowHealthCurrentPct);
		}

		RecordRecentCombatEvent(a_trigger, a_owner, a_now);
	}

	void EventBridge::ProcessTrigger(Trigger a_trigger, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		// Owned by this frame: the loop below dispatches affix actions that
		// re-enter the engine, and a re-entrant RebuildActiveCounts rebuilds
		// the live index caches.  Iterating our own copy keeps the pass valid.
		std::vector<std::size_t> indices;
		if (!CanProcessTriggerDispatch(a_trigger, a_owner, a_target, indices)) {
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		const auto lowHealthOwnerFormID = (a_trigger == Trigger::kLowHealth && a_owner) ? a_owner->GetFormID() : 0u;
		const auto lowHealthSnapshot = BuildLowHealthTriggerSnapshot(
			a_trigger == Trigger::kLowHealth,
			lowHealthOwnerFormID,
			lowHealthOwnerFormID != 0u ? ResolveLowHealthTriggerCurrentPct(a_trigger, a_owner) : 100.0f,
			ResolveLowHealthTriggerPreviousPct(lowHealthOwnerFormID));

		if (indices.empty()) {
			FinalizeTriggerDispatch(
				a_trigger,
				a_owner,
				lowHealthSnapshot.ownerFormID,
				lowHealthSnapshot.hasSnapshot,
				lowHealthSnapshot.currentPct,
				now);
			return;
		}

		bool loggedProcBudgetDenied = false;
		for (const auto i : indices) {
			(void)TryProcessTriggerAffix(
				i,
				a_trigger,
				a_owner,
				a_target,
				a_hitData,
				now,
				lowHealthSnapshot.previousPct,
				lowHealthSnapshot.currentPct,
				loggedProcBudgetDenied);
		}

		FinalizeTriggerDispatch(
			a_trigger,
			a_owner,
			lowHealthSnapshot.ownerFormID,
			lowHealthSnapshot.hasSnapshot,
			lowHealthSnapshot.currentPct,
			now);
	}
}
