#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <random>

#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		float GetTrapSpellBaseMagnitude(const RE::SpellItem* a_spell)
		{
			if (!a_spell) {
				return 0.0f;
			}

			float maxMagnitude = 0.0f;
			for (const auto* effect : a_spell->effects) {
				if (!effect) {
					continue;
				}
				maxMagnitude = std::max(maxMagnitude, effect->effectItem.magnitude);
			}
			return maxMagnitude;
		}
	}

	bool EventBridge::SelectSpawnTrapTarget(
		const Action& a_action,
		RE::Actor* a_owner,
		RE::Actor* a_target,
		const RE::HitData* a_hitData,
		RE::Actor*& a_outSpawnTarget)
	{
		if (!a_action.spell || a_action.trapRadius <= 0.0f || a_action.trapTtl.count() <= 0) {
			return false;
		}

		if (a_action.trapRequireWeaponHit) {
			if (!a_hitData || !a_hitData->weapon) {
				return false;
			}
		}

		if (a_action.trapRequireCritOrPowerAttack) {
			if (!a_hitData) {
				return false;
			}

			const bool isCrit = a_hitData->flags.any(RE::HitData::Flag::kCritical);
			const bool isPowerAttack = a_hitData->flags.any(RE::HitData::Flag::kPowerAttack);
			if (!isCrit && !isPowerAttack) {
				return false;
			}
		}

		// Avoid friendly-fire weirdness (placing traps on friends feels bad).
		if (a_target && !a_owner->IsHostileToActor(a_target)) {
			return false;
		}

		a_outSpawnTarget = nullptr;
		if (a_action.trapSpawnAt == TrapSpawnAt::kOwnerFeet) {
			a_outSpawnTarget = a_owner;
		} else if (a_target) {
			a_outSpawnTarget = a_target;
		}

		return a_outSpawnTarget != nullptr;
	}

	float EventBridge::ResolveSpawnTrapMagnitudeOverride(const Action& a_action, const RE::HitData* a_hitData) const
	{
		float magnitudeOverride = a_action.magnitudeOverride;
		if (a_action.magnitudeScaling.source != MagnitudeScaling::Source::kNone && a_hitData) {
			const float hitPhysicalDealt = std::max(0.0f, a_hitData->physicalDamage - a_hitData->resistedPhysicalDamage);
			const float hitTotalDealt = std::max(0.0f, a_hitData->totalDamage - a_hitData->resistedPhysicalDamage - a_hitData->resistedTypedDamage);
			const float spellBaseMagnitude = GetTrapSpellBaseMagnitude(a_action.spell);
			magnitudeOverride = ResolveMagnitudeOverride(
				a_action.magnitudeOverride,
				spellBaseMagnitude,
				hitPhysicalDealt,
				hitTotalDealt,
				a_action.magnitudeScaling);
		}
		return magnitudeOverride;
	}

	void EventBridge::EnforcePerAffixTrapCap(const Action& a_action)
	{
		// Enforce per-affix cap (drops the oldest trap if the cap is reached).
		if (a_action.trapMaxActive == 0u) {
			return;
		}

		while (true) {
			std::size_t count = 0;
			auto oldestIt = _traps.end();
			for (auto it = _traps.begin(); it != _traps.end(); ++it) {
				if (it->sourceToken != a_action.sourceToken) {
					continue;
				}
				count += 1;
				if (oldestIt == _traps.end() || it->createdAt < oldestIt->createdAt) {
					oldestIt = it;
				}
			}

			if (count < static_cast<std::size_t>(a_action.trapMaxActive)) {
				break;
			}

			if (oldestIt != _traps.end()) {
				_traps.erase(oldestIt);
			} else {
				break;
			}
		}
	}

	void EventBridge::EnforceGlobalTrapCap()
	{
		// Global safety cap: prevents trap-heavy affix pools from degrading performance.
		// When the cap is reached, drop the oldest traps first.
		const std::size_t globalMax = static_cast<std::size_t>(_loot.trapGlobalMaxActive);
		if (globalMax == 0 || _traps.size() < globalMax) {
			return;
		}

		const std::size_t before = _traps.size();
		while (_traps.size() >= globalMax) {
			auto oldestIt = _traps.end();
			for (auto it = _traps.begin(); it != _traps.end(); ++it) {
				if (oldestIt == _traps.end() || it->createdAt < oldestIt->createdAt) {
					oldestIt = it;
				}
			}

			if (oldestIt == _traps.end()) {
				break;
			}

			_traps.erase(oldestIt);
		}

		if (_loot.debugLog && before != _traps.size()) {
			spdlog::debug(
				"CalamityAffixes: global trap cap enforced (max={}, before={}, after={}).",
				globalMax,
				before,
				_traps.size());
		}
	}

	EventBridge::TrapInstance EventBridge::BuildSpawnTrapInstance(
		const Action& a_action,
		RE::Actor* a_owner,
		RE::Actor* a_spawnTarget,
		float a_magnitudeOverride,
		std::chrono::steady_clock::time_point a_now)
	{
		TrapInstance trap{};
		trap.sourceToken = a_action.sourceToken;
		trap.ownerFormID = a_owner->GetFormID();
		trap.position = a_spawnTarget->GetPosition();
		trap.radius = a_action.trapRadius;
		trap.spell = a_action.spell;
		trap.effectiveness = a_action.effectiveness;
		trap.magnitudeOverride = a_magnitudeOverride;
		trap.noHitEffectArt = a_action.noHitEffectArt;
		if (!a_action.trapExtraSpells.empty()) {
			std::uniform_int_distribution<std::size_t> pick(0, a_action.trapExtraSpells.size() - 1);
			trap.extraSpell = a_action.trapExtraSpells[pick(_rng)];
		}
		trap.armedAt = a_now + a_action.trapArmDelay;
		trap.expiresAt = a_now + a_action.trapTtl;
		trap.createdAt = a_now;
		trap.rearmDelay = a_action.trapRearmDelay;
		trap.maxTriggers = a_action.trapMaxTriggers;
		trap.triggeredCount = 0;
		return trap;
	}

	void EventBridge::LogSpawnTrapCreated(const TrapInstance& a_trap, const Action& a_action) const
	{
		if (!_loot.debugLog) {
			return;
		}

		const char* extraName = (a_trap.extraSpell && a_trap.extraSpell->GetName()) ? a_trap.extraSpell->GetName() : "<none>";
		spdlog::debug(
			"CalamityAffixes: trap spawned (token={}, radius={}, ttlMs={}, armDelayMs={}, rearmDelayMs={}, maxTriggers={}, extra={}, pos=({}, {}, {})).",
			a_trap.sourceToken,
			a_trap.radius,
			a_action.trapTtl.count(),
			a_action.trapArmDelay.count(),
			a_action.trapRearmDelay.count(),
			a_action.trapMaxTriggers,
			extraName,
			a_trap.position.x,
			a_trap.position.y,
			a_trap.position.z);
	}

	void EventBridge::ExecuteSpawnTrapAction(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		RE::Actor* spawnTarget = nullptr;
		if (!SelectSpawnTrapTarget(a_action, a_owner, a_target, a_hitData, spawnTarget)) {
			return;
		}

		const float magnitudeOverride = ResolveSpawnTrapMagnitudeOverride(a_action, a_hitData);
		const auto now = std::chrono::steady_clock::now();

		EnforcePerAffixTrapCap(a_action);
		EnforceGlobalTrapCap();

		auto trap = BuildSpawnTrapInstance(a_action, a_owner, spawnTarget, magnitudeOverride, now);
		_traps.push_back(trap);
		_hasActiveTraps.store(true, std::memory_order_relaxed);
		LogSpawnTrapCreated(trap, a_action);
	}
}
