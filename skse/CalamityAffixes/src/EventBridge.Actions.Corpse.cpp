#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

#include <RE/M/Misc.h>
#include <RE/P/ProcessLists.h>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		float ResolveSafeCorpseMaxHealth(RE::Actor* a_corpse)
		{
			if (!a_corpse) {
				return 0.0f;
			}

			auto* actorBase = a_corpse->GetActorBase();
			if (!actorBase) {
				return 0.0f;
			}

			return std::max(0.0f, actorBase->GetPermanentActorValue(RE::ActorValue::kHealth));
		}

		RE::TESEffectShader* GetInstantCorpseExplosionShader()
		{
			static RE::TESEffectShader* shader = RE::TESForm::LookupByID<RE::TESEffectShader>(0x0001B212);
			return shader;
		}

		float ResolveSpecialActionProcChancePct(float a_configuredChancePct)
		{
			if (a_configuredChancePct <= 0.0f) {
				// Keep backward compatibility with existing configs:
				// special actions historically used 0% as "always on".
				return 100.0f;
			}

			return std::clamp(a_configuredChancePct, 0.0f, 100.0f);
		}

		bool RollProcChance(std::mt19937& a_rng, float a_chancePct)
		{
			if (a_chancePct >= 100.0f) {
				return true;
			}
			if (a_chancePct <= 0.0f) {
				return false;
			}

			std::uniform_real_distribution<float> dist(0.0f, 100.0f);
			return dist(a_rng) < a_chancePct;
		}
	}
	EventBridge::CorpseExplosionSelection EventBridge::SelectBestCorpseExplosionAffix(
		const std::vector<std::size_t>& a_affixIndices,
		ActionType a_expectedActionType,
		float a_corpseMaxHealth,
		std::chrono::steady_clock::time_point a_now) const
	{
		CorpseExplosionSelection selection{};

		for (const auto idx : a_affixIndices) {
			if (idx >= _affixes.size() || idx >= _activeCounts.size()) {
				continue;
			}

			if (_activeCounts[idx] == 0) {
				continue;
			}

			selection.activeAffixes += 1;

			const auto& affix = _affixes[idx];
			if (affix.action.type != a_expectedActionType) {
				continue;
			}

			if (a_now < affix.nextAllowed) {
				selection.blockedCooldown += 1;
				continue;
			}

			const auto& action = affix.action;
			if (!action.spell) {
				selection.blockedNoSpell += 1;
				continue;
			}

			const float baseDamage =
				action.corpseExplosionFlatDamage +
				(a_corpseMaxHealth * (action.corpseExplosionPctOfCorpseMaxHealth / 100.0f));
			if (baseDamage <= 0.0f) {
				selection.blockedZeroDamage += 1;
				continue;
			}

			if (!selection.bestIdx || baseDamage > selection.bestBaseDamage) {
				selection.bestIdx = idx;
				selection.bestBaseDamage = baseDamage;
			}
		}

		return selection;
	}

	void EventBridge::LogCorpseExplosionSelectionSkipped(
		const CorpseExplosionSelection& a_selection,
		RE::Actor* a_corpse,
		const char* a_actionName,
		std::chrono::steady_clock::time_point a_now) const
	{
		static auto lastInfoAt = std::chrono::steady_clock::time_point{};
		if (lastInfoAt.time_since_epoch().count() != 0 && (a_now - lastInfoAt) <= std::chrono::seconds(2)) {
			return;
		}
		lastInfoAt = a_now;

		if (!_loot.debugLog) {
			return;
		}

		if (a_selection.activeAffixes == 0) {
			spdlog::info(
				"CalamityAffixes: {} skipped (no active affix equipped) (corpse={}).",
				a_actionName ? a_actionName : "CorpseExplosion",
				a_corpse ? a_corpse->GetName() : "<null>");
			return;
		}

		spdlog::info(
			"CalamityAffixes: {} skipped (no eligible affix) (activeAffixes={}, cooldown={}, noSpell={}, zeroDamage={}).",
			a_actionName ? a_actionName : "CorpseExplosion",
			a_selection.activeAffixes,
			a_selection.blockedCooldown,
			a_selection.blockedNoSpell,
			a_selection.blockedZeroDamage);
	}

	const char* EventBridge::DescribeCorpseExplosionDenyReason(CorpseExplosionBudgetDenyReason a_reason)
	{
		switch (a_reason) {
		case CorpseExplosionBudgetDenyReason::kDuplicateCorpse:
			return "DuplicateCorpse";
		case CorpseExplosionBudgetDenyReason::kRateLimited:
			return "RateLimited";
		case CorpseExplosionBudgetDenyReason::kChainDepthExceeded:
			return "ChainDepthExceeded";
		case CorpseExplosionBudgetDenyReason::kNone:
			return "None";
		default:
			return "Unknown";
		}
	}

	void EventBridge::LogCorpseExplosionBudgetDenied(
		CorpseExplosionBudgetDenyReason a_reason,
		const AffixRuntime& a_affix,
		RE::Actor* a_corpse,
		const char* a_actionName,
		std::chrono::steady_clock::time_point a_now) const
	{
		static auto lastInfoAt = std::chrono::steady_clock::time_point{};
		if (lastInfoAt.time_since_epoch().count() != 0 && (a_now - lastInfoAt) <= std::chrono::seconds(1)) {
			return;
		}
		lastInfoAt = a_now;

		if (!_loot.debugLog) {
			return;
		}

		spdlog::info(
			"CalamityAffixes: {} denied by budget (reason={}, affixId={}, corpse=0x{:X}).",
			a_actionName ? a_actionName : "CorpseExplosion",
			DescribeCorpseExplosionDenyReason(a_reason),
			a_affix.id,
			a_corpse ? a_corpse->GetFormID() : 0u);
	}

	void EventBridge::NotifyCorpseExplosionFired(
		const AffixRuntime& a_affix,
		std::uint32_t a_targetsHit,
		float a_finalDamage,
		const char* a_actionName,
		std::uint32_t a_chainDepth) const
	{
		std::string note = "CAFF ";
		note += (a_actionName && *a_actionName) ? a_actionName : "CorpseExplosion";
		note += ": ";
		note += a_affix.id;
		note += " (targets=";
		note += std::to_string(a_targetsHit);
		note += ", dmg=";
		note += std::to_string(a_finalDamage);
		note += ", chain=";
		note += std::to_string(a_chainDepth);
		note += ")";
		RE::DebugNotification(note.c_str());
	}

	void EventBridge::ProcessCorpseExplosionKill(RE::Actor* a_owner, RE::Actor* a_corpse)
	{
		ProcessCorpseExplosionAction(
			a_owner,
			a_corpse,
			_corpseExplosionAffixIndices,
			ActionType::kCorpseExplosion,
			"CorpseExplosion",
			false);
	}

	void EventBridge::ProcessSummonCorpseExplosionDeath(RE::Actor* a_owner, RE::Actor* a_corpse)
	{
		ProcessCorpseExplosionAction(
			a_owner,
			a_corpse,
			_summonCorpseExplosionAffixIndices,
			ActionType::kSummonCorpseExplosion,
			"SummonCorpseExplosion",
			true);
	}

	void EventBridge::ProcessCorpseExplosionAction(
		RE::Actor* a_owner,
		RE::Actor* a_corpse,
		const std::vector<std::size_t>& a_affixIndices,
		ActionType a_expectedActionType,
		const char* a_actionName,
		bool a_summonMode)
	{
		if (!_configLoaded || !_runtimeEnabled || !a_owner || !a_corpse) {
			return;
		}
		if (a_affixIndices.empty()) {
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		const float corpseMaxHealth = ResolveSafeCorpseMaxHealth(a_corpse);

		const auto selection = SelectBestCorpseExplosionAffix(
			a_affixIndices,
			a_expectedActionType,
			corpseMaxHealth,
			now);
		if (!selection.bestIdx) {
			LogCorpseExplosionSelectionSkipped(selection, a_corpse, a_actionName, now);
			return;
		}

		auto& bestAffix = _affixes[*selection.bestIdx];

		if (!PassesRecentlyGates(bestAffix, a_owner, now)) {
			return;
		}

		PerTargetCooldownKey perTargetKey{};
		const bool usesPerTargetIcd = (bestAffix.perTargetIcd.count() > 0 && a_corpse && bestAffix.token != 0u);
		if (IsPerTargetCooldownBlocked(bestAffix, a_corpse, now, &perTargetKey)) {
			return;
		}

		const float chancePct = ResolveSpecialActionProcChancePct(bestAffix.procChancePct * _runtimeProcChanceMult);
		if (!RollProcChance(_rng, chancePct)) {
			return;
		}

		if (bestAffix.icd.count() > 0) {
			bestAffix.nextAllowed = now + bestAffix.icd;
		}

		std::uint32_t chainDepth = 0;
		float chainMultiplier = 1.0f;
		CorpseExplosionBudgetDenyReason denyReason{};
		if (!TryConsumeCorpseExplosionBudget(
				a_corpse->GetFormID(),
				bestAffix.action,
				now,
				chainDepth,
				chainMultiplier,
				&denyReason,
				a_summonMode)) {
			LogCorpseExplosionBudgetDenied(denyReason, bestAffix, a_corpse, a_actionName, now);
			return;
		}

		const float finalDamage = selection.bestBaseDamage * chainMultiplier;
		if (finalDamage <= 0.0f) {
			return;
		}

		if (_loot.debugLog) {
			spdlog::debug(
				"CalamityAffixes: {} (corpse={}, maxHealth={}, baseDamage={}, chainDepth={}, chainMult={})",
				a_actionName ? a_actionName : "CorpseExplosion",
				a_corpse->GetName(),
				corpseMaxHealth,
				selection.bestBaseDamage,
				chainDepth,
				chainMultiplier);
		}

		_procDepth += 1;
		const std::uint32_t targetsHit = ExecuteCorpseExplosion(bestAffix.action, a_owner, a_corpse, finalDamage);
		_procDepth -= 1;
		if (usesPerTargetIcd && targetsHit > 0u) {
			// Consume per-target ICD only when the explosion actually hit at least one target.
			CommitPerTargetCooldown(perTargetKey, bestAffix.perTargetIcd, now);
		}

		if (_loot.debugLog) {
			spdlog::info(
				"CalamityAffixes: {} fired (affixId={}, corpse={}, targets={}, dmg={}, radius={}, chain={}).",
				a_actionName ? a_actionName : "CorpseExplosion",
				bestAffix.id,
				a_corpse->GetName(),
				targetsHit,
				finalDamage,
				bestAffix.action.corpseExplosionRadius,
				chainDepth);
		}

		if (bestAffix.action.debugNotify || _loot.debugLog) {
			NotifyCorpseExplosionFired(bestAffix, targetsHit, finalDamage, a_actionName, chainDepth);
		}
	}

	bool EventBridge::TryConsumeCorpseExplosionBudget(
		RE::FormID a_corpseFormID,
		const Action& a_action,
		std::chrono::steady_clock::time_point a_now,
		std::uint32_t& a_outChainDepth,
		float& a_outChainMultiplier,
		CorpseExplosionBudgetDenyReason* a_outDenyReason,
		bool a_summonMode)
	{
		a_outChainDepth = 0;
		a_outChainMultiplier = 1.0f;
		if (a_outDenyReason) {
			*a_outDenyReason = CorpseExplosionBudgetDenyReason::kNone;
		}

		auto& explosionState = a_summonMode ? _summonCorpseExplosionState : _corpseExplosionState;
		auto& seenCorpses = a_summonMode ? _summonCorpseExplosionSeenCorpses : _corpseExplosionSeenCorpses;

		// Per-corpse guard: avoid double-processing the same dying reference.
		static constexpr auto corpseTTL = std::chrono::seconds(60);
		if (const auto it = seenCorpses.find(a_corpseFormID); it != seenCorpses.end()) {
			if ((a_now - it->second) < corpseTTL) {
				if (a_outDenyReason) {
					*a_outDenyReason = CorpseExplosionBudgetDenyReason::kDuplicateCorpse;
				}
				return false;
			}
		}

		// Rate limit window (hard safety cap).
		if (a_action.corpseExplosionRateLimitWindow.count() > 0 && a_action.corpseExplosionMaxExplosionsPerWindow > 0u) {
			if (explosionState.rateWindowStartAt.time_since_epoch().count() == 0 ||
				(a_now - explosionState.rateWindowStartAt) > a_action.corpseExplosionRateLimitWindow) {
				explosionState.rateWindowStartAt = a_now;
				explosionState.explosionsInWindow = 0;
			}

			if (explosionState.explosionsInWindow >= a_action.corpseExplosionMaxExplosionsPerWindow) {
				if (a_outDenyReason) {
					*a_outDenyReason = CorpseExplosionBudgetDenyReason::kRateLimited;
				}
				return false;
			}
		}

		// Chain depth + falloff.
		std::uint32_t depth = 0;
		if (a_action.corpseExplosionChainWindow.count() > 0 &&
			explosionState.lastExplosionAt.time_since_epoch().count() != 0 &&
			(a_now - explosionState.lastExplosionAt) <= a_action.corpseExplosionChainWindow) {
			depth = explosionState.chainDepth + 1;
			if (depth >= a_action.corpseExplosionMaxChainDepth) {
				if (a_outDenyReason) {
					*a_outDenyReason = CorpseExplosionBudgetDenyReason::kChainDepthExceeded;
				}
				return false;
			}
		} else {
			depth = 0;
			explosionState.chainAnchorAt = a_now;
		}

		float mult = 1.0f;
		if (depth > 0 && a_action.corpseExplosionChainFalloff > 0.0f && a_action.corpseExplosionChainFalloff <= 1.0f) {
			mult = static_cast<float>(std::pow(static_cast<double>(a_action.corpseExplosionChainFalloff), static_cast<double>(depth)));
		}

		explosionState.lastExplosionAt = a_now;
		explosionState.chainDepth = depth;

		if (a_action.corpseExplosionRateLimitWindow.count() > 0 && a_action.corpseExplosionMaxExplosionsPerWindow > 0u) {
			explosionState.explosionsInWindow += 1;
		}

		seenCorpses.insert_or_assign(a_corpseFormID, a_now);

		// Prevent unbounded growth (kills are low-rate; a linear prune is fine).
		if (seenCorpses.size() > 512u) {
			for (auto it = seenCorpses.begin(); it != seenCorpses.end();) {
				if ((a_now - it->second) > corpseTTL) {
					it = seenCorpses.erase(it);
				} else {
					++it;
				}
			}
		}

		a_outChainDepth = depth;
		a_outChainMultiplier = mult;
		return true;
	}

	std::uint32_t EventBridge::ExecuteCorpseExplosion(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_corpse, float a_baseDamage)
	{
		if (!a_owner || !a_corpse || !a_action.spell) {
			return 0;
		}
		if (a_baseDamage <= 0.0f) {
			return 0;
		}

		auto* magicCaster = a_owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!magicCaster) {
			return 0;
		}

		auto* processLists = RE::ProcessLists::GetSingleton();
		if (!processLists) {
			return 0;
		}

		const auto origin = a_corpse->GetPosition();
		const float radius = a_action.corpseExplosionRadius;
		const float radiusSq = radius * radius;
		const auto configuredMaxTargets = static_cast<std::size_t>(a_action.corpseExplosionMaxTargets);
		const std::size_t targetCap = (configuredMaxTargets > 0u) ? configuredMaxTargets : static_cast<std::size_t>(48u);
		if (targetCap == 0u) {
			return 0;
		}

		if (auto* shader = GetInstantCorpseExplosionShader()) {
			a_corpse->InstantiateHitShader(
				shader,
				0.35f,
				nullptr,
				false,
				false,
				nullptr,
				false);
		}

		struct Target
		{
			RE::Actor* actor{ nullptr };
			float distanceSq{ 0.0f };
		};

		std::vector<Target> targets;
		targets.reserve(std::min<std::size_t>(targetCap, 48u));
		processLists->ForEachHighActor([&](RE::Actor& a) {
			if (&a == a_owner || &a == a_corpse) {
				return RE::BSContainer::ForEachResult::kContinue;
			}

			if (a.IsDead()) {
				return RE::BSContainer::ForEachResult::kContinue;
			}

			if (!a_owner->IsHostileToActor(std::addressof(a))) {
				return RE::BSContainer::ForEachResult::kContinue;
			}

			const auto targetPos = a.GetPosition();
			const float dx = targetPos.x - origin.x;
			const float dy = targetPos.y - origin.y;
			const float dz = targetPos.z - origin.z;
			const float distSq = (dx * dx) + (dy * dy) + (dz * dz);
			if (distSq > radiusSq) {
				return RE::BSContainer::ForEachResult::kContinue;
			}

			if (targets.size() < targetCap) {
				targets.push_back(Target{ std::addressof(a), distSq });
				return RE::BSContainer::ForEachResult::kContinue;
			}

			// Keep the closest N actors only to avoid sort pressure in crowded fights.
			auto farthestIt = std::max_element(
				targets.begin(),
				targets.end(),
				[](const Target& lhs, const Target& rhs) {
					return lhs.distanceSq < rhs.distanceSq;
				});
			if (farthestIt != targets.end() && distSq < farthestIt->distanceSq) {
				*farthestIt = Target{ std::addressof(a), distSq };
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});

		if (targets.empty()) {
			return 0;
		}
		std::sort(targets.begin(), targets.end(), [](const Target& a, const Target& b) {
			return a.distanceSq < b.distanceSq;
		});

		if (_loot.debugLog) {
			spdlog::debug(
				"CalamityAffixes: CorpseExplosion applying damage (targets={}, radius={}, damage={}).",
				targets.size(),
				radius,
				a_baseDamage);
		}

		std::uint32_t hitCount = 0;
		for (const auto& t : targets) {
			if (!t.actor || t.actor->IsDead()) {
				continue;
			}

			magicCaster->CastSpellImmediate(
				a_action.spell,
				a_action.noHitEffectArt,
				t.actor,
				a_action.effectiveness,
				false,
				a_baseDamage,
				a_owner);
			hitCount += 1;
		}

		return hitCount;
	}


}
