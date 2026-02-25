#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/CombatContext.h"

#include <algorithm>

#include <RE/P/ProcessLists.h>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	void EventBridge::TickTraps()
	{
		const std::scoped_lock lock(_stateMutex);
		const auto now = std::chrono::steady_clock::now();
		if (_disableTrapSystemTick) {
			_hasActiveTraps.store(false, std::memory_order_relaxed);
			if (_combatDebugLog) {
				static auto nextLogAt = std::chrono::steady_clock::time_point{};
				if (nextLogAt.time_since_epoch().count() == 0 || now >= nextLogAt) {
					nextLogAt = now + std::chrono::seconds(2);
					spdlog::info("CalamityAffixes: TickTraps disabled by runtime setting.");
				}
			}
			return;
		}

		auto tryResolveStalePlayerCombat = [&]() {
			static auto lastClearAt = std::chrono::steady_clock::time_point{};
			static auto lastHardClearAt = std::chrono::steady_clock::time_point{};
			static std::uint32_t clearConfidence = 0u;
			constexpr auto kClearCooldown = std::chrono::seconds(5);
			constexpr auto kHardClearCooldown = std::chrono::seconds(20);
			constexpr std::uint32_t kRequiredConfidence = 8u;

			auto logStaleCombat = [&](const char* a_message) {
				if (!_combatDebugLog) {
					return;
				}
				static auto nextLogAt = std::chrono::steady_clock::time_point{};
				if (nextLogAt.time_since_epoch().count() != 0 && now < nextLogAt) {
					return;
				}
				nextLogAt = now + std::chrono::seconds(2);
				const bool hasLease = _playerCombatEvidenceExpiresAt.time_since_epoch().count() != 0;
				const bool leaseActive = hasLease && now <= _playerCombatEvidenceExpiresAt;
				const auto leaseRemainingMs = leaseActive
					? static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(_playerCombatEvidenceExpiresAt - now).count())
					: 0;
				auto* player = RE::PlayerCharacter::GetSingleton();
				spdlog::info(
					"CalamityAffixes: stale combat check: {} (inCombat={}, leaseActive={}, leaseRemainingMs={}, confidence={}, traps={}).",
					a_message,
					player ? player->IsInCombat() : false,
					leaseActive,
					leaseRemainingMs,
					clearConfidence,
					_traps.size());
			};

			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* processLists = RE::ProcessLists::GetSingleton();
			const auto runtimeCombatTargetHandle =
				player ? player->GetActorRuntimeData().currentCombatTarget.native_handle() : 0u;
			auto* combatController = player ? player->GetActorRuntimeData().combatController : nullptr;
			const auto controllerTargetHandle =
				combatController ? combatController->targetHandle.native_handle() : 0u;
			const bool murderAlarmBit =
				player ? player->GetActorRuntimeData().boolBits.any(RE::Actor::BOOL_BITS::kMurderAlarm) : false;
			const bool controllerStarted = combatController ? combatController->startedCombat : false;
			const bool controllerStopped = combatController ? combatController->stoppedCombat : false;
			if (_combatDebugLog) {
				static auto nextHeartbeatLogAt = std::chrono::steady_clock::time_point{};
				if (nextHeartbeatLogAt.time_since_epoch().count() == 0 || now >= nextHeartbeatLogAt) {
					nextHeartbeatLogAt = now + std::chrono::seconds(2);
					const bool hasLease = _playerCombatEvidenceExpiresAt.time_since_epoch().count() != 0;
					const bool leaseActive = hasLease && now <= _playerCombatEvidenceExpiresAt;
					const auto leaseRemainingMs = leaseActive
						? static_cast<std::int64_t>(
							std::chrono::duration_cast<std::chrono::milliseconds>(_playerCombatEvidenceExpiresAt - now).count())
						: 0;
					spdlog::info(
						"CalamityAffixes: stale combat heartbeat (hasPlayer={}, inCombat={}, leaseActive={}, leaseRemainingMs={}, traps={}, runtimeTarget={:08X}, controllerTarget={:08X}, murderAlarm={}, ctrlStarted={}, ctrlStopped={}).",
						player != nullptr,
						player ? player->IsInCombat() : false,
						leaseActive,
						leaseRemainingMs,
						_traps.size(),
						runtimeCombatTargetHandle,
						controllerTargetHandle,
						murderAlarmBit,
						controllerStarted,
						controllerStopped);
				}
			}

			if (_forceStopAlarmPulse && player && processLists) {
				static auto lastPulseAt = std::chrono::steady_clock::time_point{};
				constexpr auto kPulseCooldown = std::chrono::milliseconds(300);
				if (lastPulseAt.time_since_epoch().count() == 0 || (now - lastPulseAt) >= kPulseCooldown) {
					lastPulseAt = now;
					player->StopAlarmOnActor();
					player->StopCombat();
					processLists->StopCombatAndAlarmOnActor(player, true);
					processLists->ClearCachedFactionFightReactions();
					if (_combatDebugLog) {
						spdlog::info(
							"CalamityAffixes: forceStopAlarmPulse fired (runtimeTarget={:08X}, controllerTarget={:08X}, murderAlarm={}, ctrlStarted={}, ctrlStopped={}, inCombat={}).",
							runtimeCombatTargetHandle,
							controllerTargetHandle,
							murderAlarmBit,
							controllerStarted,
							controllerStopped,
							player->IsInCombat());
					}
				}
			}
			if (!player) {
				clearConfidence = 0u;
				return;
			}

			if (!player->IsInCombat()) {
				clearConfidence = 0u;
				return;
			}

			const auto playerFormID = player->GetFormID();
			if (playerFormID == 0u) {
				return;
			}

			if (_playerCombatEvidenceExpiresAt.time_since_epoch().count() != 0 &&
				now <= _playerCombatEvidenceExpiresAt) {
				clearConfidence = 0u;
				logStaleCombat("skip: combat evidence lease active");
				return;
			}

			if (!processLists) {
				clearConfidence = 0u;
				logStaleCombat("skip: ProcessLists unavailable");
				return;
			}

			bool hostileNearby = false;
			RE::FormID hostileFormID = 0u;
			std::string hostileName{};
			float hostileDistSq = 0.0f;
			const auto playerPos = player->GetPosition();
			constexpr float kCombatHoldRadiusSq = 2000.0f * 2000.0f;
			processLists->ForEachHighActor([&](RE::Actor& a_actor) {
				if (&a_actor == player || a_actor.IsDead() || !a_actor.IsInCombat()) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				if (!a_actor.IsHostileToActor(player) && !player->IsHostileToActor(&a_actor)) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				if (!IsCombatTargetEitherDirection(player, &a_actor)) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				const auto actorPos = a_actor.GetPosition();
				const float dx = actorPos.x - playerPos.x;
				const float dy = actorPos.y - playerPos.y;
				const float dz = actorPos.z - playerPos.z;
				const float distSq = (dx * dx) + (dy * dy) + (dz * dz);
				if (distSq > kCombatHoldRadiusSq) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				
				hostileNearby = true;
				hostileFormID = a_actor.GetFormID();
				hostileName = a_actor.GetName();
				hostileDistSq = distSq;
				return RE::BSContainer::ForEachResult::kStop;
			});

			if (hostileNearby) {
				clearConfidence = 0u;
				if (_combatDebugLog && hostileFormID != 0u) {
					spdlog::info(
						"CalamityAffixes: stale combat check: skip: hostile nearby (actor={}({:08X}), distSq={:.1f}).",
						hostileName.empty() ? "<unnamed>" : hostileName,
						hostileFormID,
						hostileDistSq);
				} else {
					logStaleCombat("skip: hostile nearby");
				}
				return;
			}

			if (lastClearAt.time_since_epoch().count() != 0 && (now - lastClearAt) < kClearCooldown) {
				logStaleCombat("skip: clear cooldown");
				return;
			}

			clearConfidence += 1u;
			if (clearConfidence < kRequiredConfidence) {
				logStaleCombat("accumulating confidence");
				return;
			}
			clearConfidence = 0u;
			lastClearAt = now;
			logStaleCombat("attempting StopCombat");

			player->StopAlarmOnActor();
			player->StopCombat();
			if (player->IsInCombat()) {
				if (lastHardClearAt.time_since_epoch().count() == 0 ||
					(now - lastHardClearAt) >= kHardClearCooldown) {
					processLists->StopCombatAndAlarmOnActor(player, true);
					lastHardClearAt = now;
					spdlog::info("CalamityAffixes: escalated stale combat clear with StopCombatAndAlarmOnActor.");
				}
			}
			_recentOwnerHitAt.erase(playerFormID);
			_recentOwnerKillAt.erase(playerFormID);
			_recentOwnerIncomingHitAt.erase(playerFormID);
			_playerCombatEvidenceExpiresAt = {};
			_nonHostileFirstHitGate.Clear();
			if (_loot.debugLog) {
				spdlog::info("CalamityAffixes: cleared stale player combat state (no hostile actor nearby).");
			}
		};

		if (_traps.empty()) {
			_hasActiveTraps.store(false, std::memory_order_relaxed);
			tryResolveStalePlayerCombat();
			return;
		}

		// Prune expired/invalid traps.
		for (auto it = _traps.begin(); it != _traps.end();) {
			if (!it->spell || it->radius <= 0.0f || now >= it->expiresAt) {
				it = _traps.erase(it);
			} else {
				++it;
			}
		}

		if (_traps.empty()) {
			_hasActiveTraps.store(false, std::memory_order_relaxed);
			tryResolveStalePlayerCombat();
			return;
		}

		if (_disableTrapCasts) {
			_hasActiveTraps.store(!_traps.empty(), std::memory_order_relaxed);
			if (_combatDebugLog) {
				static auto nextLogAt = std::chrono::steady_clock::time_point{};
				if (nextLogAt.time_since_epoch().count() == 0 || now >= nextLogAt) {
					nextLogAt = now + std::chrono::seconds(2);
					spdlog::info(
						"CalamityAffixes: trap casts disabled by runtime setting (activeTraps={}).",
						_traps.size());
				}
			}
			tryResolveStalePlayerCombat();
			return;
		}

		auto* processLists = RE::ProcessLists::GetSingleton();
		if (!processLists) {
			tryResolveStalePlayerCombat();
			return;
		}

		const std::size_t trapCastBudgetPerTick = static_cast<std::size_t>(_loot.trapCastBudgetPerTick);
		std::size_t trapCastsConsumed = 0u;
		bool loggedBudgetExhausted = false;
		const auto hasTrapCastBudget = [&](std::size_t a_cost = 1u) noexcept {
			return trapCastBudgetPerTick == 0u || (trapCastsConsumed + a_cost) <= trapCastBudgetPerTick;
		};

		if (_trapTickCursor >= _traps.size()) {
			_trapTickCursor = 0;
		}
		const std::size_t trapVisitBudgetPerTick = std::min<std::size_t>(
			_traps.size(),
			(trapCastBudgetPerTick == 0u)
				? static_cast<std::size_t>(8u)
				: std::max<std::size_t>(1u, trapCastBudgetPerTick * 2u));
		std::size_t visitedTraps = 0u;

		while (!_traps.empty() && visitedTraps < trapVisitBudgetPerTick) {
			if (!hasTrapCastBudget()) {
				if (_loot.debugLog && !loggedBudgetExhausted) {
					spdlog::debug(
						"CalamityAffixes: trap cast budget exhausted for this tick (budget={}).",
						trapCastBudgetPerTick);
					loggedBudgetExhausted = true;
				}
				break;
			}

			if (_trapTickCursor >= _traps.size()) {
				_trapTickCursor = 0;
			}
			const auto trapIndex = _trapTickCursor;
			++visitedTraps;
			auto& trap = _traps[trapIndex];
			if (now < trap.armedAt) {
				if (!_traps.empty()) {
					_trapTickCursor = (_trapTickCursor + 1u) % _traps.size();
				}
				continue;
			}

			auto* ownerForm = RE::TESForm::LookupByID<RE::TESForm>(trap.ownerFormID);
			auto* owner = ownerForm ? ownerForm->As<RE::Actor>() : nullptr;
			if (!owner) {
				_traps.erase(_traps.begin() + static_cast<std::ptrdiff_t>(trapIndex));
				if (_trapTickCursor >= _traps.size()) {
					_trapTickCursor = 0;
				}
				continue;
			}

			auto* magicCaster = owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
			if (!magicCaster) {
				if (!_traps.empty()) {
					_trapTickCursor = (_trapTickCursor + 1u) % _traps.size();
				}
				continue;
			}

			bool triggered = false;
			RE::Actor* triggeredTarget = nullptr;
			std::uint32_t triggeredTargets = 0u;
			const std::uint32_t maxTargetsPerTrigger = std::max(1u, trap.maxTargetsPerTrigger);
			const float radiusSq = trap.radius * trap.radius;

			processLists->ForEachHighActor([&](RE::Actor& a) {
				if (!hasTrapCastBudget()) {
					return RE::BSContainer::ForEachResult::kStop;
				}

				if (&a == owner) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				if (a.IsDead()) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				if (!owner->IsHostileToActor(std::addressof(a))) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				const auto targetPos = a.GetPosition();
				const float dx = targetPos.x - trap.position.x;
				const float dy = targetPos.y - trap.position.y;
				const float dz = targetPos.z - trap.position.z;
				const float distSq = (dx * dx) + (dy * dy) + (dz * dz);
				if (distSq > radiusSq) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				if (!hasTrapCastBudget()) {
					return RE::BSContainer::ForEachResult::kStop;
				}
				magicCaster->CastSpellImmediate(
					trap.spell,
					trap.noHitEffectArt,
					std::addressof(a),
					trap.effectiveness,
					false,
					trap.magnitudeOverride,
					owner);
				trapCastsConsumed += 1u;

				if (trap.extraSpell && hasTrapCastBudget()) {
					magicCaster->CastSpellImmediate(
						trap.extraSpell,
						false,
						std::addressof(a),
						1.0f,
						false,
						0.0f,
						owner);
					trapCastsConsumed += 1u;
				}

				if (!triggered) {
					triggeredTarget = std::addressof(a);
				}
				triggered = true;
				triggeredTargets += 1u;

				if (triggeredTargets >= maxTargetsPerTrigger) {
					return RE::BSContainer::ForEachResult::kStop;
				}
				if (!hasTrapCastBudget()) {
					return RE::BSContainer::ForEachResult::kStop;
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});

			if (triggered) {
				if (_loot.debugLog) {
					spdlog::debug(
						"CalamityAffixes: trap triggered (token={}, target={}, targetsHit={}, count={} / max={}).",
						trap.sourceToken,
						triggeredTarget ? triggeredTarget->GetName() : "<none>",
						triggeredTargets,
						trap.triggeredCount + 1,
						trap.maxTriggers);
				}

				trap.triggeredCount += 1;

				const bool unlimited = (trap.maxTriggers == 0u);
				const bool outOfTriggers = (!unlimited && trap.triggeredCount >= trap.maxTriggers);
				const bool canRearm = (trap.rearmDelay.count() > 0);

				if (!canRearm || outOfTriggers) {
					_traps.erase(_traps.begin() + static_cast<std::ptrdiff_t>(trapIndex));
					if (_trapTickCursor >= _traps.size()) {
						_trapTickCursor = 0;
					}
					continue;
				}

				trap.armedAt = now + trap.rearmDelay;
				if (!_traps.empty()) {
					_trapTickCursor = (_trapTickCursor + 1u) % _traps.size();
				}
				continue;
			}

			if (!_traps.empty()) {
				_trapTickCursor = (_trapTickCursor + 1u) % _traps.size();
			}
		}

	if (_traps.empty()) {
		_hasActiveTraps.store(false, std::memory_order_relaxed);
	}

	tryResolveStalePlayerCombat();
}
}
