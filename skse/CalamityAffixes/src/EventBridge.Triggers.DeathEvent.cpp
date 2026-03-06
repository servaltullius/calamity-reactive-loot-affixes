#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/PlayerOwnership.h"
#include "EventBridge.Triggers.Events.Detail.h"

#include <chrono>


namespace CalamityAffixes
{
	using namespace TriggersDetail;

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESDeathEvent* a_event,
		RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!_configLoaded || !_runtimeSettings.enabled || !a_event || !a_event->dead) {
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

		// TESDeathEvent::actorKiller can be Projectile/Hazard/etc; resolve shooter/actorCause for ranged/magic kills.
		RE::Actor* killer = ResolveActorFromCombatRef(killerRef);

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

		if (_loot.debugLog) {
			const auto probe = BuildCorpseCurrencyDropProbe(dying);
			const auto snapshot = SnapshotCorpseCurrencyInventory(dying);
			SKSE::log::info(
				"CalamityAffixes: corpse-drop probe (dying={}, runewordFragmentRecordFound={}, reforgeOrbRecordFound={}, runewordDropListFound={}, reforgeDropListFound={}, chanceNone(runeword/reforge)={}/{}, corpseCurrencySnapshotAtDeathEvent(runewordFragments/reforgeOrbs/runewordLists/reforgeLists)={}/{}/{}/{}).",
				dying->GetName(),
				probe.runewordFragmentRecordFound,
				probe.reforgeOrbRecordFound,
				probe.runewordDropListFound,
				probe.reforgeDropListFound,
				probe.runewordChanceNone,
				probe.reforgeChanceNone,
				snapshot.runewordFragments,
				snapshot.reforgeOrbs,
				snapshot.runewordDropLists,
				snapshot.reforgeDropLists);
		}

		ProcessTrigger(Trigger::kKill, owner, dying, nullptr);

		if (!_affixSpecialActions.corpseExplosionAffixIndices.empty()) {
			ProcessCorpseExplosionKill(owner, dying);
		}

		SendModEvent("CalamityAffixes_Kill", dying);

		return RE::BSEventNotifyControl::kContinue;
	}

}
