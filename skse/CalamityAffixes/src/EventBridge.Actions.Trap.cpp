#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/ProcFeedback.h"

#include <algorithm>
#include <format>
#include <mutex>
#include <random>
#include <string_view>


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
		RE::Actor*& a_outSpawnTarget,
		const char** a_outFailureReason)
	{
		const auto setFailureReason = [&](const char* a_reason) {
			if (a_outFailureReason) {
				*a_outFailureReason = a_reason;
			}
		};

		if (!a_action.spell || a_action.trapRadius <= 0.0f || a_action.trapTtl.count() <= 0) {
			setFailureReason("invalid trap config");
			return false;
		}

		if (a_action.trapRequireWeaponHit && !HitDataUtil::IsWeaponLikeHit(a_hitData, a_owner)) {
			setFailureReason("needs weapon hit");
			return false;
		}

		if (a_action.trapRequireCritOrPowerAttack) {
			if (!a_hitData) {
				setFailureReason("missing hit data");
				return false;
			}

			const bool isCrit = a_hitData->flags.any(RE::HitData::Flag::kCritical);
			const bool isPowerAttack = a_hitData->flags.any(RE::HitData::Flag::kPowerAttack);
			if (!isCrit && !isPowerAttack) {
				setFailureReason("needs crit/power attack");
				return false;
			}
		}

		if (a_target && !a_owner->IsHostileToActor(a_target)) {
			setFailureReason("target not hostile");
			return false;
		}

		a_outSpawnTarget = nullptr;
		if (a_action.trapSpawnAt == TrapSpawnAt::kOwnerFeet) {
			a_outSpawnTarget = a_owner;
		} else if (a_target) {
			a_outSpawnTarget = a_target;
		}

		if (!a_outSpawnTarget) {
			setFailureReason("target unavailable");
			return false;
		}

		setFailureReason("unknown");
		return true;
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
		auto& activeTraps = _trapState.activeTraps;

		// Enforce per-affix cap (drops the oldest trap if the cap is reached).
		if (a_action.trapMaxActive == 0u) {
			return;
		}

		while (true) {
			std::size_t count = 0;
			auto oldestIt = activeTraps.end();
			for (auto it = activeTraps.begin(); it != activeTraps.end(); ++it) {
				if (it->sourceToken != a_action.sourceToken) {
					continue;
				}
				count += 1;
				if (oldestIt == activeTraps.end() || it->createdAt < oldestIt->createdAt) {
					oldestIt = it;
				}
			}

			if (count < static_cast<std::size_t>(a_action.trapMaxActive)) {
				break;
			}

			if (oldestIt != activeTraps.end()) {
				activeTraps.erase(oldestIt);
			} else {
				break;
			}
		}
	}

	void EventBridge::EnforceGlobalTrapCap()
	{
		auto& activeTraps = _trapState.activeTraps;

		// Global safety cap: prevents trap-heavy affix pools from degrading performance.
		// When the cap is reached, drop the oldest traps first.
		const std::size_t globalMax = static_cast<std::size_t>(_loot.trapGlobalMaxActive);
		if (globalMax == 0 || activeTraps.size() < globalMax) {
			return;
		}

		const std::size_t before = activeTraps.size();
		while (activeTraps.size() >= globalMax) {
			auto oldestIt = activeTraps.end();
			for (auto it = activeTraps.begin(); it != activeTraps.end(); ++it) {
				if (oldestIt == activeTraps.end() || it->createdAt < oldestIt->createdAt) {
					oldestIt = it;
				}
			}

			if (oldestIt == activeTraps.end()) {
				break;
			}

			activeTraps.erase(oldestIt);
		}

		if (_loot.debugLog && before != activeTraps.size()) {
			SKSE::log::debug(
				"CalamityAffixes: global trap cap enforced (max={}, before={}, after={}).",
				globalMax,
				before,
				activeTraps.size());
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
			{
				std::lock_guard<std::mutex> lock(_rngMutex);
				trap.extraSpell = a_action.trapExtraSpells[pick(_rng)];
			}
		}
		trap.armedAt = a_now + a_action.trapArmDelay;
		trap.expiresAt = a_now + a_action.trapTtl;
		trap.createdAt = a_now;
		trap.rearmDelay = a_action.trapRearmDelay;
		trap.maxTriggers = a_action.trapMaxTriggers;
		trap.triggeredCount = 0;
		trap.maxTargetsPerTrigger = a_action.trapMaxTargetsPerTrigger;
		return trap;
	}

	void EventBridge::LogSpawnTrapCreated(const TrapInstance& a_trap, const Action& a_action) const
	{
		if (!_loot.debugLog) {
			return;
		}

		const char* extraName = (a_trap.extraSpell && a_trap.extraSpell->GetName()) ? a_trap.extraSpell->GetName() : "<none>";
		SKSE::log::debug(
			"CalamityAffixes: trap spawned (token={}, radius={}, ttlMs={}, armDelayMs={}, rearmDelayMs={}, maxTriggers={}, maxTargetsPerTrigger={}, extra={}, pos=({}, {}, {})).",
			a_trap.sourceToken,
			a_trap.radius,
			a_action.trapTtl.count(),
			a_action.trapArmDelay.count(),
			a_action.trapRearmDelay.count(),
			a_action.trapMaxTriggers,
			a_action.trapMaxTargetsPerTrigger,
			extraName,
			a_trap.position.x,
			a_trap.position.y,
			a_trap.position.z);
	}

	void EventBridge::ExecuteSpawnTrapAction(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		auto& trapState = _trapState;

		RE::Actor* spawnTarget = nullptr;
		const char* failureReason = "unknown";
		if (!SelectSpawnTrapTarget(a_action, a_owner, a_target, a_hitData, spawnTarget, &failureReason)) {
			if (_loot.debugLog && ProcFeedback::IsBloomProcSpell(a_action.spell)) {
				const auto note = std::format(
					"Calamity: {} skipped ({})",
					ProcFeedback::ResolveBloomProcDebugLabel(a_action.spell),
					failureReason);
				RE::DebugNotification(note.c_str());
			}
			return;
		}

		const float magnitudeOverride = ResolveSpawnTrapMagnitudeOverride(a_action, a_hitData);
		const auto now = std::chrono::steady_clock::now();

		EnforcePerAffixTrapCap(a_action);
		EnforceGlobalTrapCap();

		auto trap = BuildSpawnTrapInstance(a_action, a_owner, spawnTarget, magnitudeOverride, now);
		trapState.activeTraps.push_back(trap);
		trapState.hasActiveTraps.store(true, std::memory_order_relaxed);
		const auto armDelaySeconds = static_cast<float>(a_action.trapArmDelay.count()) / 1000.0f;
		ProcFeedback::PlayBloomProcFeedback(
			spawnTarget,
			a_action.spell,
			std::clamp(armDelaySeconds, 0.18f, 0.75f),
			true);
		if (_loot.debugLog && ProcFeedback::IsBloomProcSpell(a_action.spell)) {
			const auto note = std::format("Calamity: {} planted", ProcFeedback::ResolveBloomProcDebugLabel(a_action.spell));
			RE::DebugNotification(note.c_str());
		}
		LogSpawnTrapCreated(trap, a_action);
	}
}
