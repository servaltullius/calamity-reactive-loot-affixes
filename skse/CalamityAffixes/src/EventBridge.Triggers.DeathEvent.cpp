#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/PlayerOwnership.h"
#include "EventBridge.Triggers.Events.Detail.h"

#include <chrono>


namespace CalamityAffixes
{
	using namespace TriggersDetail;

	static_assert(RuntimePolicy::kAllowCorpseDeathRuntimeCurrencyRoll);
	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESDeathEvent* a_event,
		RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}
		// Keep event references alive for the entire handler. Using raw pointers directly from
		// TESDeathEvent can race with reference release under heavy combat churn.
		const auto dyingRefHolder = a_event->actorDying;
		const auto killerRefHolder = a_event->actorKiller;

		auto* dyingRef = SanitizeObjectPointer(dyingRefHolder.get());
		auto* killerRef = SanitizeObjectPointer(killerRefHolder.get());
		auto* dying = dyingRef ? dyingRef->As<RE::Actor>() : nullptr;
		if (!dying) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!a_event->dead) {
			ForgetCorpseCurrencyProcessed(dying->GetFormID());
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!_configLoaded || !_runtimeSettings.enabled) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MaybeResyncEquippedAffixes(now);

		// Independent path: player-owned summoned actor death can trigger dedicated summon corpse explosion.
		if (!_affixSpecialActions.summonCorpseExplosionAffixIndices.empty() &&
			dying->IsSummoned() &&
			IsPlayerOwned(dying)) {
			if (auto* summonOwner = GetPlayerOwner(dying); summonOwner) {
				ProcessSummonCorpseExplosionDeath(summonOwner, dying);
			}
		}

		if (!killerRef) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// TESDeathEvent::actorKiller may be an Actor or Projectile; resolve only the projectile shooter/actorCause. Environment refs remain ineligible.
		const auto killerHolder = ResolveActorFromCombatRef(killerRef);
		auto* killer = killerHolder.get();

		if (!killer) {
			static auto lastWarnAt = std::chrono::steady_clock::time_point{};
			if (lastWarnAt.time_since_epoch().count() == 0 || (now - lastWarnAt) > std::chrono::seconds(2)) {
				lastWarnAt = now;
				if (_loot.debugLog) {
					SKSE::log::info(
						"CalamityAffixes: TESDeathEvent killer attribution failed (dying={}, killerRefPtr=0x{:X}); skipping Kill triggers.",
						dying->GetName(),
						reinterpret_cast<std::uintptr_t>(killerRef));
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!IsPlayerOwned(killer)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* owner = GetPlayerOwner(killer);
		if (!owner) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Only react to hostile kills (avoid friendly-fire weirdness).
		const bool hostile = owner->IsHostileToActor(dying) || dying->IsHostileToActor(owner);
		if (!hostile) {
			static auto lastInfoAt = std::chrono::steady_clock::time_point{};
			if (lastInfoAt.time_since_epoch().count() == 0 || (now - lastInfoAt) > std::chrono::seconds(2)) {
				lastInfoAt = now;
				if (_loot.debugLog) {
					SKSE::log::info(
						"CalamityAffixes: Kill ignored (non-hostile) (dying={}, killer={}).",
						dying->GetName(),
						killer->GetName());
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		const bool corpseCurrencyEligible = detail::IsCorpseRuntimeCurrencyDropEligible({
			.eventIsDeath = a_event->dead,
			.dyingIsDead = dying->IsDead(),
			.dyingIsPlayerOwned = IsPlayerOwned(dying),
			.dyingIsPlayerTeammate = dying->IsPlayerTeammate(),
			.dyingIsSummoned = dying->IsSummoned(),
			.dyingIsCommanded = dying->IsCommandedActor(),
			.dyingIsChild = dying->IsChild(),
			.killerIsPlayerOwned = true,
			.hostileToPlayerOwner = hostile
		});

		if (corpseCurrencyEligible && _loot.runtimeCorpseDeathCurrencyDropsEnabled) {
			const auto dayStamp = detail::GetInGameDayStamp();
			const auto corpseFormId = dying->GetFormID();
			const auto processedMask = GetCorpseCurrencyProcessedMask(corpseFormId, dayStamp);
			const auto snapshot = SnapshotCorpseCurrencyInventory(dying);
			const bool hasRunewordCurrency =
				snapshot.runewordFragments > 0u || snapshot.runewordDropLists > 0u;
			const bool hasReforgeCurrency =
				snapshot.reforgeOrbs > 0u || snapshot.reforgeDropLists > 0u;
			const auto allowance = detail::ResolveCorpseCurrencyRollAllowance(
				processedMask,
				hasRunewordCurrency,
				hasReforgeCurrency);

			CurrencyRollExecutionResult dropResult{};
			if (allowance.runeword || allowance.reforge) {
				dropResult = ExecuteCorpseCurrencyDropRolls(
					ResolveLootCurrencySourceChanceMultiplier(detail::LootCurrencySourceTier::kCorpse),
					dying,
					1u,
					allowance.runeword,
					allowance.reforge);
			}

			const auto newlyProcessedMask = static_cast<std::uint8_t>(
				detail::kCorpseCurrencyAllProcessed & static_cast<std::uint8_t>(~processedMask));
			MarkCorpseCurrencyProcessed(corpseFormId, dayStamp, newlyProcessedMask);

			if (_loot.debugLog) {
				const auto probe = BuildCorpseCurrencyDropProbe(dying);
				SKSE::log::info(
					"CalamityAffixes: corpse-only currency roll (dying={}({:08X}), sourceMult={:.2f}, previousMask={}, newlyProcessedMask={}, existing(runeword/reforge)={}/{}, allow(runeword/reforge)={}/{}, granted(runeword/reforge)={}/{}, inventory(runewordFragments/reforgeOrbs/runewordLists/reforgeLists)={}/{}/{}/{}, records(fragment/orb/lists)={}/{}/{}/{}, legacyChanceNone(runeword/reforge)={}/{}).",
					dying->GetName(),
					corpseFormId,
					ResolveLootCurrencySourceChanceMultiplier(detail::LootCurrencySourceTier::kCorpse),
					processedMask,
					newlyProcessedMask,
					hasRunewordCurrency,
					hasReforgeCurrency,
					allowance.runeword,
					allowance.reforge,
					dropResult.runewordDropGranted,
					dropResult.reforgeDropGranted,
					snapshot.runewordFragments,
					snapshot.reforgeOrbs,
					snapshot.runewordDropLists,
					snapshot.reforgeDropLists,
					probe.runewordFragmentRecordFound,
					probe.reforgeOrbRecordFound,
					probe.runewordDropListFound,
					probe.reforgeDropListFound,
					probe.runewordChanceNone,
					probe.reforgeChanceNone);
			}
		} else if (_loot.debugLog && !corpseCurrencyEligible) {
			SKSE::log::debug(
				"CalamityAffixes: corpse currency skipped by eligibility (dying={}({:08X}), dead={}, playerOwned={}, teammate={}, summoned={}, commanded={}, child={}, hostile={}).",
				dying->GetName(),
				dying->GetFormID(),
				dying->IsDead(),
				IsPlayerOwned(dying),
				dying->IsPlayerTeammate(),
				dying->IsSummoned(),
				dying->IsCommandedActor(),
				dying->IsChild(),
				hostile);
		}

		ProcessTrigger(Trigger::kKill, owner, dying, nullptr);

		if (!_affixSpecialActions.corpseExplosionAffixIndices.empty()) {
			ProcessCorpseExplosionKill(owner, dying);
		}

		SendModEvent("CalamityAffixes_Kill", dying);

		return RE::BSEventNotifyControl::kContinue;
	}

}
