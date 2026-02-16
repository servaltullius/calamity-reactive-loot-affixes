#include "CalamityAffixes/EventBridge.h"

#include <algorithm>

#include <RE/P/ProcessLists.h>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	void EventBridge::TickTraps()
	{
		if (_traps.empty()) {
			_hasActiveTraps.store(false, std::memory_order_relaxed);
			return;
		}

		const auto now = std::chrono::steady_clock::now();

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
			return;
		}

		auto* processLists = RE::ProcessLists::GetSingleton();
		if (!processLists) {
			return;
		}

		const std::size_t trapCastBudgetPerTick = static_cast<std::size_t>(_loot.trapCastBudgetPerTick);
		std::size_t trapCastsConsumed = 0u;
		bool loggedBudgetExhausted = false;
		const auto hasTrapCastBudget = [&](std::size_t a_cost = 1u) noexcept {
			return trapCastBudgetPerTick == 0u || (trapCastsConsumed + a_cost) <= trapCastBudgetPerTick;
		};

		for (std::size_t i = 0; i < _traps.size();) {
			if (!hasTrapCastBudget()) {
				if (_loot.debugLog && !loggedBudgetExhausted) {
					spdlog::debug(
						"CalamityAffixes: trap cast budget exhausted for this tick (budget={}).",
						trapCastBudgetPerTick);
					loggedBudgetExhausted = true;
				}
				break;
			}

			auto& trap = _traps[i];
			if (now < trap.armedAt) {
				++i;
				continue;
			}

			auto* ownerForm = RE::TESForm::LookupByID<RE::TESForm>(trap.ownerFormID);
			auto* owner = ownerForm ? ownerForm->As<RE::Actor>() : nullptr;
			if (!owner) {
				_traps.erase(_traps.begin() + static_cast<std::ptrdiff_t>(i));
				continue;
			}

			auto* magicCaster = owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
			if (!magicCaster) {
				++i;
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
					_traps.erase(_traps.begin() + static_cast<std::ptrdiff_t>(i));
					continue;
				}

				trap.armedAt = now + trap.rearmDelay;
				++i;
				continue;
			}

			++i;
		}
	}
}
