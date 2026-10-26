#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/HostileEffectGuard.h"

#include "CalamityAffixes/CombatContext.h"
#include "CalamityAffixes/CorpseHitFilter.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/TesHitFallbackPolicy.h"
#include "CalamityAffixes/PlayerOwnership.h"
#include "CalamityAffixes/TriggerGuards.h"
#include "EventBridge.Triggers.Events.Detail.h"

#include <chrono>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>


namespace CalamityAffixes
{
	using namespace TriggersDetail;

	namespace
	{
		using FallbackHitContentMap = std::unordered_map<std::uint64_t, FallbackHitContentRecord>;

		// Bounded like the hook ledger, but by age only: a record keeps blocking
		// its swing for as long as it is plausible a spell reads it back.
		constexpr std::size_t kFallbackHitContentMaxEntries = 1024u;
		constexpr auto kFallbackHitContentPruneAge = std::chrono::minutes(10);

		[[nodiscard]] bool IsReusedFallbackHitData(
			const FallbackHitContentMap& a_map,
			std::uint64_t a_pairKey,
			std::uint64_t a_content) noexcept
		{
			const auto it = a_map.find(a_pairKey);
			return detail::IsReusedFallbackHitContent(
				a_content,
				it != a_map.end(),
				it != a_map.end() ? it->second.content : 0u);
		}

		void RecordFallbackHitData(
			FallbackHitContentMap& a_map,
			std::uint64_t a_pairKey,
			std::uint64_t a_content,
			std::chrono::steady_clock::time_point a_now)
		{
			if (a_content == 0u) {
				return;
			}
			a_map[a_pairKey] = FallbackHitContentRecord{ .content = a_content, .at = a_now };
			if (a_map.size() > kFallbackHitContentMaxEntries) {
				std::erase_if(a_map, [&](const auto& a_entry) {
					return (a_now - a_entry.second.at) >= kFallbackHitContentPruneAge;
				});
			}
		}
	}

	RE::BSEventNotifyControl EventBridge::HandleHitEvent(
		const RE::TESHitEvent* a_event,
		EventStateLock& lock,
		const EngineEventContext& a_context)
	{
		if (!a_event || !a_event->cause || !a_event->target) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* causeRef = SanitizeObjectPointer(a_event->cause.get());
		auto* targetRef = SanitizeObjectPointer(a_event->target.get());
		if (!causeRef || !targetRef) {
			return RE::BSEventNotifyControl::kContinue;
		}
		const auto now = a_context.observedAt;
		MaybeFlushRuntimeUserSettings(now, false);

		if (!_runtimeSettings.enabled.load(std::memory_order_relaxed)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Ignore synchronous TESHitEvent reentry caused by proc actions that are already
		// resolving this hit. Without this, incoming-hit/low-health fallback dispatch can
		// observe its own proc side effects and spiral into duplicate follow-up work.
		if (a_context.procOrigin || _combatState.procDepth > 0) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto aggressorHolder = ResolveActorFromCombatRef(causeRef);
		auto* aggressor = aggressorHolder.get();
		auto* target = targetRef->As<RE::Actor>();
		if (!aggressor || !target) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Bounded observations distinguish a visible hit from health loss and
		// expose later vtable replacements even when our damage thunk is bypassed.
		if (ShouldSuppressNonHostileCalamityHealthDamage(target, aggressor, nullptr)) {
			static std::atomic_uint32_t logged{ 0u };
			if (logged.fetch_add(1u, std::memory_order_relaxed) < 32u) {
				SKSE::log::info(
					"CalamityAffixes: friendly hit observed (target={:08X}, attacker={:08X}, source={:08X}, health={}, healthHookDirect={}).",
					target->GetFormID(), aggressor->GetFormID(), a_event->source,
					target->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth),
					Hooks::IsHandleHealthDamageHooked(target));
			}
		}

		const auto relation = BuildCombatTriggerContext(target, aggressor);

		if (_loot.debugLog) {
			static std::uint32_t windowCount = 0;
			static auto windowStart = now;
			windowCount += 1;

			if ((now - windowStart) >= std::chrono::seconds(2)) {
				const auto* hitData = a_context.hitData ? &*a_context.hitData : nullptr;
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
		// Safety-net resync on every hit, hook mode included: KID can patch
		// keywords after kDataLoaded and an equip change can be missed. The
		// configured interval is enforced inside. This runs in the deferred
		// event drain on the game thread with _stateMutex held.
		MaybeResyncEquippedAffixes(now);

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
				const auto pairKey =
					(static_cast<std::uint64_t>(aggressor->GetFormID()) << 32) |
					static_cast<std::uint64_t>(target->GetFormID());

				// Outgoing (player-owned).
				if (_configLoaded && relation.attackerIsPlayerOwned && relation.playerOwner &&
					relation.hostileEitherDirection) {
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
						const auto* hitData = a_context.hitData ? &*a_context.hitData : nullptr;
						const bool hasCommittedHitData = detail::IsCommittedFallbackHitData(
							hitData != nullptr,
							hitData && HitDataUtil::HitDataMatchesActors(hitData, target, aggressor),
							hitData && HitDataUtil::HasHitLikeSource(hitData, aggressor));
						const auto hitContent = hasCommittedHitData ? HitDataUtil::MakeHitContentSignature(hitData) : 0u;

						// A spell, breath or enchantment event after a swing reads that
						// swing back as committed HitData: count each content once.
						const bool reusedHitContent =
							hasCommittedHitData && IsReusedFallbackHitData(_combatState.fallbackOutgoingHitContent, pairKey, hitContent);

						// Some weapon types (especially modded weapons with custom
						// animations) raise TESHitEvent before HitData is fully
						// committed to the target actor.  The engine may fire the
						// first event with stale/incomplete hitData (missing or
						// actor-mismatched source data) and a follow-up event with
						// the real data.
						// Reset duplicate tracking so the follow-up is processed.
						// This event comes after the damage, so the killing blow already
						// finds the target dead: only a hit right after the observed
						// death counts (ClassifyDeadTargetHit), and only the first one.
						bool corpseHit = false;
						if (!reusedHitContent && hasCommittedHitData && target->IsDead()) {
							const auto deathIt = _combatState.recentDeathObservedMs.find(target->GetFormID());
							const auto verdict = ClassifyDeadTargetHit(
								deathIt != _combatState.recentDeathObservedMs.end(),
								std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(),
								deathIt != _combatState.recentDeathObservedMs.end() ? deathIt->second : 0);
							corpseHit = verdict == DeadTargetHitVerdict::kCorpseHit ||
								!_combatState.fallbackCorpseHits.AdmitDeadTargetHit(target->GetFormID(), StatusClockNowMs());
						}
						if (reusedHitContent) {
							// Like uncommitted data: the real follow-up event for this
							// pair must not fall into the duplicate window.
							_combatState.lastHitAt = {};
							if (_loot.debugLog) {
								SKSE::log::debug(
									"CalamityAffixes: TESHitEvent fallback skipped reused lastHitData (target={:08X}, source=0x{:X}).",
									target->GetFormID(),
									a_event->source);
							}
						} else if (corpseHit) {
							RecordFallbackHitData(_combatState.fallbackOutgoingHitContent, pairKey, hitContent, now);
							NoteDebugTestEvent("corpse:skipped");
							if (_loot.debugLog) {
								SKSE::log::debug(
									"CalamityAffixes: TESHitEvent fallback skipped procs on a corpse hit (target={:08X}).",
									target->GetFormID());
							}
						} else if (!hasCommittedHitData) {
							_combatState.lastHitAt = {};
						} else {
							RecordFallbackHitData(_combatState.fallbackOutgoingHitContent, pairKey, hitContent, now);
							ProcessTrigger(Trigger::kHit, relation.playerOwner, target, hitData);
							ProcessEchoStrikeHit(aggressor, target, hitData, now);

							if (aggressor->IsPlayerRef()) {
								const auto cocBatch = EvaluateCastOnCrit(aggressor, target, hitData);
								if (cocBatch.count > 0) {
									// Keep the whole fallback cast batch under one recursion guard.
									// Synchronous TESHitEvent reentry is rejected at the top of this handler.
									const ScopedProcDepth procDepthGuard{ _combatState };
									if (auto* magicCaster = aggressor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
										for (std::size_t i = 0; i < cocBatch.count; ++i) {
											const auto& coc = cocBatch.entries[i];
											if (!coc.spell) {
												continue;
											}
											CastHostileOnlySpellImmediate(
												magicCaster,
												coc.spell,
												coc.noHitEffectArt,
												target,
												coc.effectiveness,
												coc.magnitudeOverride,
												aggressor);
											if (coc.riderSpell) {
												CastHostileOnlySpellImmediate(
													magicCaster,
													coc.riderSpell,
													coc.noHitEffectArt,
													target,
													coc.effectiveness,
													0.0f,
													aggressor);
												NoteDebugTestEvent("coc:rider");
											}
										}
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
						const auto* hitData = a_context.hitData ? &*a_context.hitData : nullptr;
						const bool hasCommittedHitData = detail::IsCommittedFallbackHitData(
							hitData != nullptr,
							hitData && HitDataUtil::HitDataMatchesActors(hitData, target, aggressor),
							hitData && HitDataUtil::HasHitLikeSource(hitData, aggressor));

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
							const auto hitContent = HitDataUtil::MakeHitContentSignature(hitData);
							// An enemy's breath or spell after its swing reads that swing back:
							// no second MoM refund or IncomingHit for it. LowHealth still runs.
							const bool reusedHitContent =
								IsReusedFallbackHitData(_combatState.fallbackIncomingHitContent, pairKey, hitContent);
							if (!reusedHitContent) {
								RecordFallbackHitData(_combatState.fallbackIncomingHitContent, pairKey, hitContent, now);
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
											if (auto* avOwner = skyrim_cast<RE::ActorValueOwner*>(target)) {
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
								}
							} else {
								// Like uncommitted data: the real follow-up event for this
								// pair must not fall into the duplicate window.
								_combatState.lastHitAt = {};
							}

							if (!target->IsDead()) {
								// A reused HitData's damage belongs to the old swing: unknown here.
								const float dealt = reusedHitContent ? 0.0f : std::max(
									0.0f,
									hitData->totalDamage - hitData->resistedPhysicalDamage - hitData->resistedTypedDamage);
								ProcessTrigger(Trigger::kLowHealth, target, aggressor, hitData, dealt);
							}
						}
					}
				}
			}
		}

		// Archmage, in both hook and fallback modes: the spell that hit is only
		// visible as TESHitEvent::source (HitData has no cast-spell field), so it
		// needs no committed HitData. Own proc spells are excluded inside.
		ProcessArchmageFromHitEvent(aggressor, target, a_event->source, now);

		// Only forward the hit signal to Papyrus when the C++ HandleHealthDamage
		// hook is NOT active for this target.  When hookRouted is true, all proc
		// processing is handled in C++ (via the hook → OnHealthDamage path).
		// Sending the ModEvent unconditionally caused Papyrus AffixManager to
		// fire a second, duplicate proc spell for every hit.
		bool sendPapyrusHitEvent = false;
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
				// ShouldSuppressPapyrusHitEvent commits the dedup timestamp,
				// so the decision has to be taken while we still hold the lock.
				sendPapyrusHitEvent = !ShouldSuppressPapyrusHitEvent(key, now);
			}
		}

		// Terminal engine re-entry: dispatching the ModEvent reads no
		// EventBridge state and nothing follows it, so hand the lock back
		// before crossing into the Papyrus VM.  Same shape as the
		// ForEachHighActor hand-off in TickTraps.
		lock.unlock();
		if (sendPapyrusHitEvent) {
			SendModEvent("CalamityAffixes_Hit", target);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

}
