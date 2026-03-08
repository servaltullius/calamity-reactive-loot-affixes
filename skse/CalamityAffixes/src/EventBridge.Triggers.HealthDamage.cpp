#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/CombatContext.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/Hooks.h"

#include <chrono>
#include <string>

namespace CalamityAffixes
{
	void EventBridge::OnHealthDamage(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData,
		float a_damage)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!_configLoaded || !_runtimeSettings.enabled || !a_target) {
			return;
		}

		if (_runtimeSettings.disableHealthDamageRouting) {
			if (_runtimeSettings.combatDebugLog) {
				static bool loggedOnce = false;
				if (!loggedOnce) {
					loggedOnce = true;
					SKSE::log::info("CalamityAffixes: OnHealthDamage routing disabled by runtime setting.");
				}
			}
			return;
		}
		// H2: MaybeResyncEquippedAffixes removed — the main-thread ProcessEvent handlers
		// already drive resync; calling it from the hook thread risks data races.

		_combatState.healthDamageHookSeen = true;

		static bool loggedHookIsActive = false;
		if (!loggedHookIsActive) {
			loggedHookIsActive = true;
			SKSE::log::info("CalamityAffixes: HandleHealthDamage callback is active (hit procs routed here).");
		}

		if (_loot.debugLog) {
			static std::uint32_t windowCount = 0;
			static auto windowStart = now;
			windowCount += 1;

			if ((now - windowStart) >= std::chrono::seconds(2)) {
				SKSE::log::info(
					"CalamityAffixes: HandleHealthDamage seen (count={}, target={}, attacker={}, hasHitData={}, hasWeapon={}, hasSpell={}).",
					windowCount,
					a_target->GetName(),
					a_attacker ? a_attacker->GetName() : "<none>",
					(a_hitData != nullptr),
					(a_hitData && a_hitData->weapon != nullptr),
					(a_hitData && a_hitData->attackDataSpell != nullptr));

				windowCount = 0;
				windowStart = now;
			}
		}

		// Fallback: if hitData was not captured by ThunkImpl (e.g., filtered
		// by ResolveStableHitDataForSpecialActions for CoC/Conversion safety),
		// re-fetch from the target.  OnHealthDamage runs on the main thread
		// where lastHitData should reflect the current hit.
		// RouteHealthDamageAsHit will still validate the re-fetched data.
		const RE::HitData* effectiveHitData = a_hitData;
		if (!effectiveHitData) {
			effectiveHitData = HitDataUtil::GetLastHitData(a_target);
		}

		const auto sourceFormID = HitDataUtil::GetHitSourceFormID(effectiveHitData, a_attacker);
		const bool routedAsHit = RouteHealthDamageAsHit(a_target, a_attacker, effectiveHitData, sourceFormID, a_damage, now);

		const auto context = BuildCombatTriggerContext(a_target, a_attacker);
		const bool playerRelevantCombatSignal =
			context.targetIsPlayer ||
			(context.attackerIsPlayerOwned && context.hasPlayerOwner);
		const bool shouldMarkCombatEvidence =
			playerRelevantCombatSignal && context.hostileEitherDirection;
		if (routedAsHit) {
			if (shouldMarkCombatEvidence) {
				const auto* player = context.targetIsPlayer ? a_target : context.playerOwner;
				const auto* other = context.targetIsPlayer ? a_attacker : a_target;
				MarkPlayerCombatEvidence(
					now,
					PlayerCombatEvidenceSource::kHealthDamageRoutedHit,
					player,
					other);
				_combatState.healthDamageHookLastAt = now;
			} else if (_runtimeSettings.combatDebugLog && playerRelevantCombatSignal) {
				static auto nextNonHostileLogAt = std::chrono::steady_clock::time_point{};
				if (nextNonHostileLogAt.time_since_epoch().count() == 0 || now >= nextNonHostileLogAt) {
					nextNonHostileLogAt = now + std::chrono::seconds(2);
					SKSE::log::info(
						"CalamityAffixes: skipped HealthDamage combat evidence mark (non-hostile relation, target={}, attacker={}).",
						a_target ? a_target->GetName() : "<none>",
						a_attacker ? a_attacker->GetName() : "<none>");
				}
			}
			ProcessOutgoingHealthDamageHit(a_target, a_attacker, effectiveHitData, sourceFormID, now);
			ProcessIncomingHealthDamageHit(a_target, a_attacker, effectiveHitData, sourceFormID, now);
			ProcessImmediateCorpseExplosionFromLethalHit(a_target, a_attacker);
		}

		ProcessLowHealthTriggerFromHealthDamage(a_target, a_attacker, effectiveHitData);
	}
}
