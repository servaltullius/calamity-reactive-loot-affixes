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

		for (std::size_t i = 0; i < _traps.size();) {
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

			processLists->ForEachHighActor([&](RE::Actor& a) {
				if (&a == owner) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				if (a.IsDead()) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				if (!owner->IsHostileToActor(std::addressof(a))) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				const float dist = trap.position.GetDistance(a.GetPosition());
				if (dist > trap.radius) {
					return RE::BSContainer::ForEachResult::kContinue;
				}

				magicCaster->CastSpellImmediate(
					trap.spell,
					trap.noHitEffectArt,
					std::addressof(a),
					trap.effectiveness,
					false,
					trap.magnitudeOverride,
					owner);

				if (trap.extraSpell) {
					magicCaster->CastSpellImmediate(
						trap.extraSpell,
						false,
						std::addressof(a),
						1.0f,
						false,
						0.0f,
						owner);
				}

				triggered = true;
				triggeredTarget = std::addressof(a);
				return RE::BSContainer::ForEachResult::kStop;
			});

			if (triggered) {
				if (_loot.debugLog) {
					spdlog::debug(
						"CalamityAffixes: trap triggered (token={}, target={}, count={} / max={}).",
						trap.sourceToken,
						triggeredTarget ? triggeredTarget->GetName() : "<none>",
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
