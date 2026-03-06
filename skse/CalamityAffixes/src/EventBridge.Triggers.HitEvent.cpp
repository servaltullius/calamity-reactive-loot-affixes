#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/CombatContext.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/PlayerOwnership.h"
#include "CalamityAffixes/TriggerGuards.h"
#include "EventBridge.Triggers.Events.Detail.h"

#include <chrono>
#include <cstdint>


namespace CalamityAffixes
{
	using namespace TriggersDetail;

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESHitEvent* a_event,
		RE::BSTEventSource<RE::TESHitEvent>*)
	{
		if (!a_event || !a_event->cause || !a_event->target) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* causeRef = SanitizeObjectPointer(a_event->cause.get());
		auto* targetRef = SanitizeObjectPointer(a_event->target.get());
		if (!causeRef || !targetRef) {
			return RE::BSEventNotifyControl::kContinue;
		}
		const std::scoped_lock lock(_stateMutex);
		const auto now = std::chrono::steady_clock::now();
		MaybeFlushRuntimeUserSettings(now, false);

		if (!_runtimeSettings.enabled) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* aggressor = ResolveActorFromCombatRef(causeRef);
		auto* target = targetRef->As<RE::Actor>();
		if (!aggressor || !target) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto relation = BuildCombatTriggerContext(target, aggressor);

		if (_loot.debugLog) {
			static std::uint32_t windowCount = 0;
			static auto windowStart = now;
			windowCount += 1;

			if ((now - windowStart) >= std::chrono::seconds(2)) {
				const auto* hitData = HitDataUtil::GetLastHitData(target);
				const auto sourceFormID = hitData && hitData->weapon ?
					hitData->weapon->GetFormID() :
					(hitData && hitData->attackDataSpell ? hitData->attackDataSpell->GetFormID() : 0u);

				SKSE::log::info(
					"CalamityAffixes: TESHitEvent seen (count={}, cause={}, target={}, source=0x{:X}, hasHitData={}, hasWeapon={}, hasSpell={}).",
					windowCount,
					aggressor->GetName(),
					target->GetName(),
					sourceFormID,
					(hitData != nullptr),
					(hitData && hitData->weapon != nullptr),
					(hitData && hitData->attackDataSpell != nullptr));

				windowCount = 0;
				windowStart = now;
			}
		}

		// NOTE: Some modlists can override vfunc hooks. If HandleHealthDamage is not routed through our hook
		// for this actor, fall back to TESHitEvent-driven procs so affixes still work.
		const bool hookRouted = CalamityAffixes::Hooks::IsHandleHealthDamageHooked(target);
		if (!hookRouted) {
			// KID can patch keywords after kDataLoaded; re-sync equipped counts occasionally so procs become active.
			MaybeResyncEquippedAffixes(now);
		}

		if (!hookRouted) {
			if (_loot.debugLog) {
				static bool loggedOnce = false;
				if (!loggedOnce) {
					loggedOnce = true;
					SKSE::log::debug("CalamityAffixes: HandleHealthDamage hook not routed; enabling TESHitEvent fallback.");
				}
			}

			const bool internalProcSource = IsInternalProcSpellSource(a_event->source);
			if (internalProcSource) {
				if (_loot.debugLog) {
					static std::uint32_t suppressedInternalProcHits = 0;
					suppressedInternalProcHits += 1;
					if (suppressedInternalProcHits <= 3) {
						SKSE::log::debug(
							"CalamityAffixes: TESHitEvent fallback ignored internal proc spell source (source=0x{:X}).",
							a_event->source);
					}
				}
			} else {
			// Outgoing (player-owned).
			if (_configLoaded && relation.attackerIsPlayerOwned && relation.playerOwner) {
				if (relation.hostileEitherDirection) {
					MarkPlayerCombatEvidence(
						now,
						PlayerCombatEvidenceSource::kTesHitOutgoing,
						relation.playerOwner,
						target);
					const LastHitKey key{
						.outgoing = true,
						.aggressor = aggressor->GetFormID(),
						.target = target->GetFormID(),
						.source = a_event->source
					};

						if (!ShouldSuppressDuplicateHit(key, now)) {
							const auto* hitData = HitDataUtil::GetLastHitData(target);
							const bool hasCommittedHitData =
								hitData &&
								HitDataUtil::HitDataMatchesActors(hitData, target, aggressor) &&
								HitDataUtil::HasHitLikeSource(hitData, aggressor);

							// Some weapon types (especially modded weapons with custom
							// animations) raise TESHitEvent before HitData is fully
							// committed to the target actor.  The engine may fire the
							// first event with stale/incomplete hitData (missing or
							// actor-mismatched source data) and a follow-up event with
							// the real data.
							// Reset duplicate tracking so the follow-up is processed.
							if (!hasCommittedHitData) {
								_combatState.lastHitAt = {};
							} else {
								ProcessTrigger(Trigger::kHit, relation.playerOwner, target, hitData);

							if (aggressor->IsPlayerRef()) {
								const auto coc = EvaluateCastOnCrit(aggressor, target, hitData);
								if (coc.spell) {
									if (auto* magicCaster = aggressor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
										magicCaster->CastSpellImmediate(
											coc.spell,
											coc.noHitEffectArt,
											target,
											coc.effectiveness,
											false,
											coc.magnitudeOverride,
											aggressor);
									}
								}
							}

							if (aggressor->IsPlayerRef() && !_affixSpecialActions.archmageAffixIndices.empty()) {
								auto* source = RE::TESForm::LookupByID<RE::TESForm>(a_event->source);
								auto* spell = source ? source->As<RE::SpellItem>() : nullptr;
								if (spell) {
									ProcessArchmageSpellHit(aggressor, target, spell, hitData);
								}
							}
						}
					}
				}
			}

			// Incoming (player hit).
			if (_configLoaded && relation.targetIsPlayer && relation.hostileEitherDirection) {
				MarkPlayerCombatEvidence(
					now,
					PlayerCombatEvidenceSource::kTesHitIncoming,
					target,
					aggressor);
				const LastHitKey key{
					.outgoing = false,
					.aggressor = aggressor->GetFormID(),
					.target = target->GetFormID(),
					.source = a_event->source
				};

					if (!ShouldSuppressDuplicateHit(key, now)) {
						const auto* hitData = HitDataUtil::GetLastHitData(target);
						const bool hasCommittedHitData =
							hitData &&
							HitDataUtil::HitDataMatchesActors(hitData, target, aggressor) &&
							HitDataUtil::HasHitLikeSource(hitData, aggressor);

						// Some weapon types (especially modded weapons with custom
						// animations) raise TESHitEvent before HitData is fully
						// committed to the target actor.  The engine may fire the
						// first event with stale/incomplete hitData (missing or
						// actor-mismatched source data) and a follow-up event with
						// the real data.
						// Reset duplicate tracking so the follow-up is processed.
						if (!hasCommittedHitData) {
							_combatState.lastHitAt = {};
						} else {
							// Hook-compat fallback:
							// If HandleHealthDamage hook is not routed, emulate MoM by refunding Health after hit landed.
							// (Hook path reduces damage pre-hit; TESHitEvent fallback can only correct post-hit.)
							if (!target->IsDead()) {
								float fallbackIncomingDamage = std::max(
									0.0f,
									hitData->totalDamage - hitData->resistedPhysicalDamage - hitData->resistedTypedDamage);
								if (fallbackIncomingDamage > 0.0f) {
									const auto mom = EvaluateMindOverMatter(target, aggressor, hitData, fallbackIncomingDamage);
									if (mom.redirectedDamage > 0.0f) {
										if (auto* avOwner = target->AsActorValueOwner()) {
											avOwner->RestoreActorValue(
												RE::ACTOR_VALUE_MODIFIER::kDamage,
												RE::ActorValue::kHealth,
												mom.redirectedDamage);
											if (_loot.debugLog) {
												SKSE::log::debug(
													"CalamityAffixes: TESHitEvent fallback applied MoM heal correction (redirected={}).",
													mom.redirectedDamage);
											}
										}
									}
								}
							}

							if (!target->IsDead()) {
								ProcessTrigger(Trigger::kIncomingHit, target, aggressor, hitData);
								ProcessTrigger(Trigger::kLowHealth, target, aggressor, hitData);
							}
						}
						}
					}
				}
		}

		// Only forward the hit signal to Papyrus when the C++ HandleHealthDamage
		// hook is NOT active for this target.  When hookRouted is true, all proc
		// processing is handled in C++ (via the hook → OnHealthDamage path).
		// Sending the ModEvent unconditionally caused Papyrus AffixManager to
		// fire a second, duplicate proc spell for every hit.
		if (!hookRouted) {
			if (ShouldSendPlayerOwnedHitEvent(
					relation.attackerIsPlayerOwned,
					relation.hasPlayerOwner,
					relation.hostileEitherDirection,
					false,
					relation.targetIsPlayer)) {
				const LastHitKey key{
					.outgoing = true,
					.aggressor = aggressor->GetFormID(),
					.target = target->GetFormID(),
					.source = a_event->source
				};
				if (!ShouldSuppressPapyrusHitEvent(key, now)) {
					SendModEvent("CalamityAffixes_Hit", target);
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

}
