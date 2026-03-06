#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/LootCurrencySourceHelpers.h"
#include "CalamityAffixes/PointerSafety.h"
#include "EventBridge.Triggers.Events.Detail.h"

#include <chrono>


namespace CalamityAffixes
{
	using namespace TriggersDetail;
	static_assert(!RuntimePolicy::kAllowCorpseActivationRuntimeCurrencyRollInHybridMode);

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESActivateEvent* a_event,
		RE::BSTEventSource<RE::TESActivateEvent>*)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!_configLoaded || !_runtimeSettings.enabled || !a_event || !a_event->actionRef || !a_event->objectActivated) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* actionRef = SanitizeObjectPointer(a_event->actionRef.get());
		auto* sourceRef = SanitizeObjectPointer(a_event->objectActivated.get());
		auto* actionActor = actionRef ? actionRef->As<RE::Actor>() : nullptr;
		if (!actionActor || !actionActor->IsPlayerRef() || !sourceRef) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (_runtimeSettings.combatDebugLog) {
			auto* baseObject = sourceRef->GetBaseObject();
			auto* combatController = actionActor->GetActorRuntimeData().combatController;
			const auto runtimeTargetHandle = actionActor->GetActorRuntimeData().currentCombatTarget.native_handle();
			const auto controllerTargetHandle = combatController ? combatController->targetHandle.native_handle() : 0u;
			const bool murderAlarmBit = actionActor->GetActorRuntimeData().boolBits.any(RE::Actor::BOOL_BITS::kMurderAlarm);
			const bool controllerStarted = combatController ? combatController->startedCombat : false;
			const bool controllerStopped = combatController ? combatController->stoppedCombat : false;
			const std::string_view baseEditorId =
				(baseObject && baseObject->GetFormEditorID())
					? std::string_view(baseObject->GetFormEditorID())
					: std::string_view{};
			SKSE::log::info(
				"CalamityAffixes: activation probe (targetRef={}({:08X}), base={}({:08X}), baseEditorId={}, inCombat={}, runtimeTarget={:08X}, controllerTarget={:08X}, murderAlarm={}, ctrlStarted={}, ctrlStopped={}).",
				sourceRef->GetName(),
				sourceRef->GetFormID(),
				baseObject ? baseObject->GetName() : "<none>",
				baseObject ? baseObject->GetFormID() : 0u,
				baseEditorId,
				actionActor->IsInCombat(),
				runtimeTargetHandle,
				controllerTargetHandle,
				murderAlarmBit,
				controllerStarted,
				controllerStopped);
		}
		if (!IsRuntimeCurrencyDropRollEnabled("activation")) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto sourceTier = detail::ResolveLootCurrencySourceTier(
			sourceRef,
			_loot.bossContainerEditorIdAllowContains,
			_loot.bossContainerEditorIdDenyContains);
		if (sourceTier == LootCurrencySourceTier::kUnknown || sourceTier == LootCurrencySourceTier::kWorld) {
			return RE::BSEventNotifyControl::kContinue;
		}
		if (sourceTier == LootCurrencySourceTier::kCorpse &&
			!RuntimePolicy::kAllowCorpseActivationRuntimeCurrencyRollInHybridMode) {
			// Hybrid contract (SPID corpse authority):
			// corpse currency drops are handled by SPID Item distribution on actor inventory,
			// so activation-time runtime rolls are skipped.
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: activation corpse currency roll skipped (SPID-owned corpse policy in hybrid mode) (sourceRef={:08X}).",
					sourceRef->GetFormID());
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		const float sourceChanceMultiplier = ResolveLootCurrencySourceChanceMultiplier(sourceTier);

		const auto dayStamp = detail::GetInGameDayStamp();
		const auto sourceFormId = sourceRef->GetFormID();
		const auto ledgerKey = detail::BuildLootCurrencyLedgerKey(
			sourceTier,
			sourceFormId,
			0u,
			0u,
			dayStamp);
		if (!TryBeginLootCurrencyLedgerRoll(ledgerKey, dayStamp)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto preActivationSnapshot = SnapshotCorpseCurrencyInventory(sourceRef);
		const bool sourceHasRunewordCurrency =
			preActivationSnapshot.runewordFragments > 0u || preActivationSnapshot.runewordDropLists > 0u;
		const bool sourceHasReforgeCurrency =
			preActivationSnapshot.reforgeOrbs > 0u || preActivationSnapshot.reforgeDropLists > 0u;
		const bool allowRunewordRoll = !sourceHasRunewordCurrency;
		const bool allowReforgeRoll = !sourceHasReforgeCurrency;
		if (!allowRunewordRoll && !allowReforgeRoll) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: activation currency roll skipped (source already has runeword/reforge currency) (sourceRef={:08X}, sourceTier={}, runewordFragments={}, reforgeOrbs={}, runewordLists={}, reforgeLists={}, ledgerKey={:016X}, dayStamp={}).",
					sourceFormId,
					detail::LootCurrencySourceTierLabel(sourceTier),
					preActivationSnapshot.runewordFragments,
					preActivationSnapshot.reforgeOrbs,
					preActivationSnapshot.runewordDropLists,
					preActivationSnapshot.reforgeDropLists,
					ledgerKey,
					dayStamp);
			}

			FinalizeLootCurrencyLedgerRoll(ledgerKey, dayStamp);
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto dropResult = ExecuteCurrencyDropRolls(
			sourceChanceMultiplier,
			sourceRef,
			false,
			1u,
			allowRunewordRoll,
			allowReforgeRoll);
		const bool runewordDropGranted = dropResult.runewordDropGranted;
		const bool reforgeDropGranted = dropResult.reforgeDropGranted;

		FinalizeLootCurrencyLedgerRoll(ledgerKey, dayStamp);

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: activation loot rolls applied (sourceRef={:08X}, sourceTier={}, sourceChanceMult={:.2f}, allowRunewordRoll={}, allowReforgeRoll={}, runewordDropped={}, reforgeDropped={}, ledgerKey={:016X}, dayStamp={}).",
				sourceFormId,
				detail::LootCurrencySourceTierLabel(sourceTier),
				sourceChanceMultiplier,
				allowRunewordRoll,
				allowReforgeRoll,
				runewordDropGranted,
				reforgeDropGranted,
				ledgerKey,
				dayStamp);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

}
