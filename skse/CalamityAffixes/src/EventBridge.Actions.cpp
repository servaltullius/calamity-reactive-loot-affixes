#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

#include <RE/M/Misc.h>
#include <RE/P/ProcessLists.h>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		float GetSpellBaseMagnitude(const RE::SpellItem* a_spell)
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

		bool IsSummonLikeSpell(const RE::SpellItem* a_spell)
		{
			if (!a_spell) {
				return false;
			}

			for (const auto* effect : a_spell->effects) {
				if (!effect || !effect->baseEffect) {
					continue;
				}

				const auto archetype = effect->baseEffect->GetArchetype();
				if (archetype == RE::EffectSetting::Archetype::kSummonCreature ||
					archetype == RE::EffectSetting::Archetype::kReanimate) {
					return true;
				}
			}

			return false;
		}
	}

	EventBridge::ConversionResult EventBridge::EvaluateConversion(
		RE::Actor* a_attacker,
		RE::Actor* a_target,
		const RE::HitData* a_hitData,
		float& a_inOutDamage)
	{
		if (!_configLoaded || !_runtimeEnabled || _convertAffixIndices.empty()) {
			return {};
		}

		if (!a_attacker || !a_target || !a_hitData || !a_hitData->weapon) {
			return {};
		}

		if (!a_attacker->IsPlayerRef()) {
			return {};
		}

		MaybeResyncEquippedAffixes(std::chrono::steady_clock::now());

		bool hasAnyConversion = false;
		for (const auto idx : _convertAffixIndices) {
			if (idx < _activeCounts.size() && _activeCounts[idx] > 0) {
				hasAnyConversion = true;
				break;
			}
		}
		if (!hasAnyConversion) {
			return {};
		}

		const auto now = std::chrono::steady_clock::now();

		AffixRuntime* selectedAffix = nullptr;
		PerTargetCooldownKey selectedPerTargetKey{};
		bool selectedUsesPerTargetIcd = false;
		const Action* action = nullptr;
		float bestConvertPct = 0.0f;
		for (const auto idx : _convertAffixIndices) {
			if (idx >= _affixes.size() || idx >= _activeCounts.size()) {
				continue;
			}
			if (_activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixes[idx];
			const auto& candidate = affix.action;
			if (candidate.type != ActionType::kConvertDamage || !candidate.spell || candidate.convertPct <= 0.0f) {
				continue;
			}

			if (now < affix.nextAllowed) {
				continue;
			}

			PerTargetCooldownKey perTargetKey{};
			const bool usesPerTargetIcd = (affix.perTargetIcd.count() > 0 && a_target && affix.token != 0u);
			if (IsPerTargetCooldownBlocked(affix, a_target, now, &perTargetKey)) {
				continue;
			}

			const float chancePct = ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeProcChanceMult);
			if (!RollProcChance(_rng, chancePct)) {
				continue;
			}

			if (!action || candidate.convertPct > bestConvertPct) {
				action = std::addressof(candidate);
				selectedAffix = std::addressof(affix);
				selectedPerTargetKey = perTargetKey;
				selectedUsesPerTargetIcd = usesPerTargetIcd;
				bestConvertPct = candidate.convertPct;
			}
		}
		if (!action || !action->spell || action->convertPct <= 0.0f) {
			return {};
		}

		const float physicalDealt = std::max(0.0f, a_hitData->physicalDamage - a_hitData->resistedPhysicalDamage);
		if (physicalDealt <= 0.0f) {
			return {};
		}

		float converted = physicalDealt * (action->convertPct / 100.0f);
		if (converted <= 0.0f) {
			return {};
		}

		if (a_inOutDamage <= 0.0f) {
			return {};
		}

		if (converted > a_inOutDamage) {
			converted = a_inOutDamage;
		}

		a_inOutDamage -= converted;
		if (a_inOutDamage < 0.0f) {
			a_inOutDamage = 0.0f;
		}

		if (selectedAffix && selectedAffix->icd.count() > 0) {
			selectedAffix->nextAllowed = now + selectedAffix->icd;
		}
		if (selectedUsesPerTargetIcd && selectedAffix) {
			CommitPerTargetCooldown(selectedPerTargetKey, selectedAffix->perTargetIcd, now);
		}

		spdlog::debug(
			"CalamityAffixes: ConvertDamage (weapon={}, physicalDealt={}, convertPct={}%, converted={}, inOutDamage={})",
			a_hitData->weapon->GetName(),
			physicalDealt,
			action->convertPct,
			converted,
			a_inOutDamage);

		return ConversionResult{
			.spell = action->spell,
			.convertedDamage = converted,
			.effectiveness = action->effectiveness,
			.noHitEffectArt = action->noHitEffectArt,
		};
	}

	EventBridge::CastOnCritResult EventBridge::EvaluateCastOnCrit(
		RE::Actor* a_attacker,
		RE::Actor* a_target,
		const RE::HitData* a_hitData)
	{
		if (!_configLoaded || !_runtimeEnabled || _castOnCritAffixIndices.empty()) {
			return {};
		}

		if (!a_attacker || !a_target || !a_hitData) {
			return {};
		}

		if (!a_attacker->IsPlayerRef()) {
			return {};
		}

		MaybeResyncEquippedAffixes(std::chrono::steady_clock::now());

		// CoC is attack-triggered (not spell hit).
		if (!a_hitData->weapon) {
			return {};
		}

		const bool isCrit = a_hitData->flags.any(RE::HitData::Flag::kCritical);
		const bool isPowerAttack = a_hitData->flags.any(RE::HitData::Flag::kPowerAttack);

		// User choice (#3): trigger on Crit OR PowerAttack (do not treat SneakAttack as a trigger).
		if (!isCrit && !isPowerAttack) {
			return {};
		}

		// Avoid friendly-fire spam.
		if (!a_attacker->IsHostileToActor(a_target)) {
			return {};
		}

		const auto now = std::chrono::steady_clock::now();
		if (now < _castOnCritNextAllowed) {
			return {};
		}

		std::vector<std::size_t> pool;
		pool.reserve(_castOnCritAffixIndices.size());

		for (const auto idx : _castOnCritAffixIndices) {
			if (idx >= _affixes.size() || idx >= _activeCounts.size()) {
				continue;
			}

			if (_activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixes[idx];
			const auto& action = affix.action;
			if (action.type != ActionType::kCastOnCrit || !action.spell) {
				continue;
			}

			if (now < affix.nextAllowed) {
				continue;
			}

			PerTargetCooldownKey perTargetKey{};
			if (IsPerTargetCooldownBlocked(affix, a_target, now, &perTargetKey)) {
				continue;
			}

			const float chancePct = ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeProcChanceMult);
			if (!RollProcChance(_rng, chancePct)) {
				continue;
			}

			pool.push_back(idx);
		}

		if (pool.empty()) {
			return {};
		}

		const auto pickedIdx = pool[_castOnCritCycleCursor % pool.size()];
		auto& pickedAffix = _affixes[pickedIdx];
		const auto* pick = std::addressof(pickedAffix.action);
		_castOnCritCycleCursor += 1;
		_castOnCritNextAllowed = now + kCastOnCritICD;
		if (pickedAffix.icd.count() > 0) {
			pickedAffix.nextAllowed = now + pickedAffix.icd;
		}
		if (pickedAffix.perTargetIcd.count() > 0 && a_target && pickedAffix.token != 0u) {
			const PerTargetCooldownKey perTargetKey{
				.token = pickedAffix.token,
				.target = a_target->GetFormID()
			};
			CommitPerTargetCooldown(perTargetKey, pickedAffix.perTargetIcd, now);
		}

		spdlog::debug(
			"CalamityAffixes: CastOnCrit triggered (crit={}, powerAttack={}, spells={}, picked={}).",
			isCrit,
			isPowerAttack,
			pool.size(),
			pick->spell->GetName());

		float magnitudeOverride = pick->magnitudeOverride;
		if (pick->magnitudeScaling.source != MagnitudeScaling::Source::kNone) {
			const float hitPhysicalDealt = std::max(0.0f, a_hitData->physicalDamage - a_hitData->resistedPhysicalDamage);
			const float hitTotalDealt = std::max(0.0f, a_hitData->totalDamage - a_hitData->resistedPhysicalDamage - a_hitData->resistedTypedDamage);
			const float spellBaseMagnitude = GetSpellBaseMagnitude(pick->spell);
			magnitudeOverride = ResolveMagnitudeOverride(
				pick->magnitudeOverride,
				spellBaseMagnitude,
				hitPhysicalDealt,
				hitTotalDealt,
				pick->magnitudeScaling);
		}

		return CastOnCritResult{
			.spell = pick->spell,
			.effectiveness = pick->effectiveness,
			.magnitudeOverride = magnitudeOverride,
			.noHitEffectArt = pick->noHitEffectArt,
		};
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

		spdlog::debug(
			"CalamityAffixes: {} (corpse={}, maxHealth={}, baseDamage={}, chainDepth={}, chainMult={})",
			a_actionName ? a_actionName : "CorpseExplosion",
			a_corpse->GetName(),
			corpseMaxHealth,
			selection.bestBaseDamage,
			chainDepth,
			chainMultiplier);

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
			float distance{ 0.0f };
		};

		std::vector<Target> targets;
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

			const float dist = origin.GetDistance(a.GetPosition());
			if (dist > radius) {
				return RE::BSContainer::ForEachResult::kContinue;
			}

			targets.push_back(Target{ std::addressof(a), dist });
			return RE::BSContainer::ForEachResult::kContinue;
		});

		if (targets.empty()) {
			return 0;
		}

		std::sort(targets.begin(), targets.end(), [](const Target& a, const Target& b) {
			return a.distance < b.distance;
		});

		const auto maxTargets = static_cast<std::size_t>(a_action.corpseExplosionMaxTargets);
		if (maxTargets > 0 && targets.size() > maxTargets) {
			targets.resize(maxTargets);
		}

		spdlog::debug(
			"CalamityAffixes: CorpseExplosion applying damage (targets={}, radius={}, damage={}).",
			targets.size(),
			radius,
			a_baseDamage);

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

	EventBridge::ArchmageSelection EventBridge::SelectBestArchmageAction(
		std::chrono::steady_clock::time_point a_now,
		RE::Actor* a_target)
	{
		ArchmageSelection selection{};

		for (const auto idx : _archmageAffixIndices) {
			if (idx >= _affixes.size() || idx >= _activeCounts.size()) {
				continue;
			}
			if (_activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixes[idx];
			if (a_now < affix.nextAllowed) {
				continue;
			}
			if (IsPerTargetCooldownBlocked(affix, a_target, a_now, nullptr)) {
				continue;
			}

			const float chancePct = ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeProcChanceMult);
			if (!RollProcChance(_rng, chancePct)) {
				continue;
			}

			const auto& action = affix.action;
			if (action.type != ActionType::kArchmage) {
				continue;
			}

			if (action.archmageDamagePctOfMaxMagicka > selection.bestDamagePct) {
				selection.bestIdx = idx;
				selection.bestDamagePct = action.archmageDamagePctOfMaxMagicka;
				selection.bestCostPct = action.archmageCostPctOfMaxMagicka;
				selection.bestAction = std::addressof(action);
			}
		}

		return selection;
	}

	bool EventBridge::ResolveArchmageResourceUsage(
		RE::Actor* a_caster,
		float a_damagePct,
		float a_costPct,
		float& a_outMaxMagicka,
		float& a_outExtraCost,
		float& a_outExtraDamage) const
	{
		if (!a_caster) {
			return false;
		}

		a_outMaxMagicka = std::max(0.0f, a_caster->GetPermanentActorValue(RE::ActorValue::kMagicka));
		const float currentMagicka = std::max(0.0f, a_caster->GetActorValue(RE::ActorValue::kMagicka));

		a_outExtraCost = a_outMaxMagicka * (a_costPct / 100.0f);
		if (a_outExtraCost <= 0.0f || currentMagicka < a_outExtraCost) {
			return false;
		}

		a_outExtraDamage = a_outMaxMagicka * (a_damagePct / 100.0f);
		if (a_outExtraDamage <= 0.0f) {
			return false;
		}

		return true;
	}

	bool EventBridge::ExecuteArchmageCast(
		const Action& a_action,
		RE::Actor* a_caster,
		RE::Actor* a_target,
		std::string_view a_sourceEditorId,
		float a_maxMagicka,
		float a_extraCost,
		float a_extraDamage)
	{
		auto* magicCaster = a_caster ? a_caster->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant) : nullptr;
		if (!magicCaster) {
			return false;
		}

		a_caster->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -a_extraCost);

		spdlog::debug(
			"CalamityAffixes: Archmage (source={}, maxMagicka={}, extraCost={}, extraDamage={})",
			a_sourceEditorId,
			a_maxMagicka,
			a_extraCost,
			a_extraDamage);

		magicCaster->CastSpellImmediate(
			a_action.spell,
			a_action.noHitEffectArt,
			a_target,
			a_action.effectiveness,
			false,
			a_extraDamage,
			a_caster);
		return true;
	}

	void EventBridge::ProcessArchmageSpellHit(RE::Actor* a_caster, RE::Actor* a_target, RE::SpellItem* a_sourceSpell)
	{
		if (!_configLoaded || !_runtimeEnabled || !a_caster || !a_target || !a_sourceSpell) {
			return;
		}
		if (!a_caster->IsPlayerRef()) {
			return;
		}
		if (_procDepth > 0) {
			return;
		}

		const auto sourceEditorId = std::string_view(a_sourceSpell->GetFormEditorID());
		if (sourceEditorId.starts_with("CAFF_")) {
			// Avoid recursive stacking on our own proc spells.
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		const auto selection = SelectBestArchmageAction(now, a_target);
		if (!selection.bestIdx || !selection.bestAction || !selection.bestAction->spell || selection.bestDamagePct <= 0.0f || selection.bestCostPct <= 0.0f) {
			return;
		}

		float maxMagicka = 0.0f;
		float extraCost = 0.0f;
		float extraDamage = 0.0f;
		if (!ResolveArchmageResourceUsage(a_caster, selection.bestDamagePct, selection.bestCostPct, maxMagicka, extraCost, extraDamage)) {
			return;
		}

		if (!ExecuteArchmageCast(*selection.bestAction, a_caster, a_target, sourceEditorId, maxMagicka, extraCost, extraDamage)) {
			return;
		}

		auto& affix = _affixes[*selection.bestIdx];
		if (affix.icd.count() > 0) {
			affix.nextAllowed = now + affix.icd;
		}
		if (affix.perTargetIcd.count() > 0 && a_target && affix.token != 0u) {
			const PerTargetCooldownKey perTargetKey{
				.token = affix.token,
				.target = a_target->GetFormID()
			};
			CommitPerTargetCooldown(perTargetKey, affix.perTargetIcd, now);
		}
	}

	void EventBridge::ExecuteDebugNotifyAction(const Action& a_action)
	{
		const std::string msg = a_action.text.empty() ? "CalamityAffixes proc" : ("CalamityAffixes proc: " + a_action.text);
		if (auto* console = RE::ConsoleLog::GetSingleton()) {
			console->Print(msg.c_str());
		}
	}

	RE::TESObjectREFR* EventBridge::ResolveSpellCastTarget(const Action& a_action, RE::Actor* a_target) const
	{
		if (!a_action.applyToSelf && a_target) {
			return a_target;
		}
		return nullptr;
	}

	float EventBridge::ResolveSpellMagnitudeOverride(
		const Action& a_action,
		RE::SpellItem* a_spell,
		const RE::HitData* a_hitData,
		bool a_logWithoutHitData) const
	{
		if (a_action.magnitudeScaling.source == MagnitudeScaling::Source::kNone) {
			return a_action.magnitudeOverride;
		}

		float hitPhysicalDealt = 0.0f;
		float hitTotalDealt = 0.0f;
		if (a_hitData) {
			hitPhysicalDealt = std::max(0.0f, a_hitData->physicalDamage - a_hitData->resistedPhysicalDamage);
			hitTotalDealt = std::max(0.0f, a_hitData->totalDamage - a_hitData->resistedPhysicalDamage - a_hitData->resistedTypedDamage);
		}

		const float spellBaseMagnitude = GetSpellBaseMagnitude(a_spell);
		const float magnitudeOverride = ResolveMagnitudeOverride(
			a_action.magnitudeOverride,
			spellBaseMagnitude,
			hitPhysicalDealt,
			hitTotalDealt,
			a_action.magnitudeScaling);

		if (a_logWithoutHitData && _loot.debugLog && !a_hitData) {
			spdlog::debug(
				"CalamityAffixes: CastSpell computed magnitudeOverride without HitData (spell={}, baseMag={}, outMag={}).",
				a_spell ? a_spell->GetName() : "<null>",
				spellBaseMagnitude,
				magnitudeOverride);
		}

		return magnitudeOverride;
	}

	RE::Actor* EventBridge::ResolveAdaptiveAnalysisTarget(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_target) const
	{
		return a_action.applyToSelf ? a_owner : a_target;
	}

	EventBridge::AdaptiveCastSelection EventBridge::SelectAdaptiveSpellForTarget(const Action& a_action, RE::Actor* a_analysisTarget) const
	{
		AdaptiveCastSelection selection{};
		if (!a_analysisTarget) {
			selection.pick = AdaptiveElement::kFire;
			selection.spell = nullptr;
			return selection;
		}

		selection.resistFire = a_analysisTarget->GetActorValue(RE::ActorValue::kResistFire);
		selection.resistFrost = a_analysisTarget->GetActorValue(RE::ActorValue::kResistFrost);
		selection.resistShock = a_analysisTarget->GetActorValue(RE::ActorValue::kResistShock);
		selection.pick = PickAdaptiveElement(selection.resistFire, selection.resistFrost, selection.resistShock, a_action.adaptiveMode);

		switch (selection.pick) {
		case AdaptiveElement::kFire:
			selection.spell = a_action.adaptiveFire;
			break;
		case AdaptiveElement::kFrost:
			selection.spell = a_action.adaptiveFrost;
			break;
		case AdaptiveElement::kShock:
			selection.spell = a_action.adaptiveShock;
			break;
		}

		// Fallback: pick any configured spell (misconfigured element slot shouldn't hard-disable the affix).
		if (!selection.spell) {
			selection.spell = a_action.adaptiveFire ? a_action.adaptiveFire : (a_action.adaptiveFrost ? a_action.adaptiveFrost : a_action.adaptiveShock);
		}

		return selection;
	}

	void EventBridge::LogAdaptiveCastSpell(
		const AdaptiveCastSelection& a_selection,
		RE::TESObjectREFR* a_castTarget,
		float a_magnitudeOverride) const
	{
		if (!_loot.debugLog || !a_selection.spell) {
			return;
		}

		spdlog::debug(
			"CalamityAffixes: CastSpellAdaptiveElement (pick={}, spell={}, magnitudeOverride={}, target={}, rFire={}, rFrost={}, rShock={}).",
			static_cast<std::uint32_t>(a_selection.pick),
			a_selection.spell->GetName(),
			a_magnitudeOverride,
			a_castTarget ? a_castTarget->GetName() : "<self>",
			a_selection.resistFire,
			a_selection.resistFrost,
			a_selection.resistShock);
	}

	void EventBridge::ExecuteCastSpellAction(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		const auto& a_action = a_affix.action;

		auto* caster = a_owner;
		auto* magicCaster = caster->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!magicCaster) {
			SKSE::log::error("CalamityAffixes: ExecuteAction CastSpell skipped (magicCaster is null).");
			return;
		}

		const auto primaryKey = ResolvePrimaryEquippedInstanceKey(a_affix.token);
		const InstanceRuntimeState* state = nullptr;
		if (primaryKey) {
			state = FindInstanceRuntimeState(*primaryKey);
		}

		RE::SpellItem* spell = a_action.spell;
		std::uint32_t modeIndex = 0u;
		if (a_action.modeCycleEnabled && !a_action.modeCycleSpells.empty()) {
			const auto modeCount = static_cast<std::uint32_t>(a_action.modeCycleSpells.size());
			modeIndex = (modeCount > 0u) ? ((state ? state->modeIndex : 0u) % modeCount) : 0u;
			spell = a_action.modeCycleSpells[modeIndex];

			// Fallback: recover from partially empty mode lists by finding the first valid spell.
			if (!spell) {
				for (auto* candidate : a_action.modeCycleSpells) {
					if (candidate) {
						spell = candidate;
						break;
					}
				}
			}
		}

		if (!spell) {
			SKSE::log::error("CalamityAffixes: ExecuteAction CastSpell skipped (spell is null).");
			return;
		}

		RE::TESObjectREFR* castTarget = ResolveSpellCastTarget(a_action, a_target);
		if (!a_action.applyToSelf && IsSummonLikeSpell(spell)) {
			const auto* targetActor = castTarget ? castTarget->As<RE::Actor>() : nullptr;
			if (!castTarget || (targetActor && targetActor->IsDead())) {
				castTarget = a_owner;
				if (_loot.debugLog) {
					spdlog::debug(
						"CalamityAffixes: CastSpell summon fallback (spell={}, originalTarget={}, fallback=self).",
						spell->GetName(),
						targetActor ? targetActor->GetName() : "<none>");
				}
			}
		}

		float magnitudeOverride = ResolveSpellMagnitudeOverride(a_action, spell, a_hitData, true);
		float evolutionMultiplier = 1.0f;
		std::size_t evolutionStage = 0;
		if (a_action.evolutionEnabled) {
			evolutionMultiplier = ResolveEvolutionMultiplier(a_action, state);
			evolutionStage = ResolveEvolutionStageIndex(a_action, state);
			if (magnitudeOverride > 0.0f) {
				magnitudeOverride *= evolutionMultiplier;
			} else if (evolutionMultiplier != 1.0f) {
				const float baseMagnitude = GetSpellBaseMagnitude(spell);
				if (baseMagnitude > 0.0f) {
					magnitudeOverride = baseMagnitude * evolutionMultiplier;
				}
			}
		}

		if (_loot.debugLog) {
			const std::uint32_t xp = state ? state->evolutionXp : 0u;
			spdlog::debug(
				"CalamityAffixes: CastSpellImmediate (affix={}, spell={}, magnitudeOverride={}, target={}, evolutionStage={}, evolutionXP={}, evolutionMult={}, modeIndex={}).",
				a_affix.id,
				spell->GetName(),
				magnitudeOverride,
				castTarget ? castTarget->GetName() : "<none>",
				evolutionStage,
				xp,
				evolutionMultiplier,
				modeIndex);
		}

		magicCaster->CastSpellImmediate(
			spell,
			a_action.noHitEffectArt,
			castTarget,
			a_action.effectiveness,
			false,
			magnitudeOverride,
			caster);
	}

	void EventBridge::ExecuteCastSpellAdaptiveElementAction(const Action& a_action, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		auto* caster = a_owner;
		auto* magicCaster = caster->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!magicCaster) {
			SKSE::log::error("CalamityAffixes: ExecuteAction CastSpellAdaptiveElement skipped (magicCaster is null).");
			return;
		}

		auto* analysisTarget = ResolveAdaptiveAnalysisTarget(a_action, a_owner, a_target);
		if (!analysisTarget) {
			return;
		}

		const auto selection = SelectAdaptiveSpellForTarget(a_action, analysisTarget);
		if (!selection.spell) {
			return;
		}

		RE::TESObjectREFR* castTarget = ResolveSpellCastTarget(a_action, a_target);
		const float magnitudeOverride = ResolveSpellMagnitudeOverride(a_action, selection.spell, a_hitData, false);
		LogAdaptiveCastSpell(selection, castTarget, magnitudeOverride);

		magicCaster->CastSpellImmediate(
			selection.spell,
			a_action.noHitEffectArt,
			castTarget,
			a_action.effectiveness,
			false,
			magnitudeOverride,
			caster);
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
			const float spellBaseMagnitude = GetSpellBaseMagnitude(a_action.spell);
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

	void EventBridge::DispatchActionByType(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		const auto& a_action = a_affix.action;
		switch (a_action.type) {
		case ActionType::kDebugNotify:
			ExecuteDebugNotifyAction(a_action);
			break;
		case ActionType::kCastSpell:
			ExecuteCastSpellAction(a_affix, a_owner, a_target, a_hitData);
			break;
		case ActionType::kCastSpellAdaptiveElement:
			ExecuteCastSpellAdaptiveElementAction(a_action, a_owner, a_target, a_hitData);
			break;
		case ActionType::kSpawnTrap:
			ExecuteSpawnTrapAction(a_action, a_owner, a_target, a_hitData);
			break;
		default:
			break;
		}
	}

	void EventBridge::ExecuteActionWithProcDepthGuard(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		_procDepth += 1;
		ExecuteAction(a_affix, a_owner, a_target, a_hitData);
		_procDepth -= 1;
	}

	void EventBridge::ExecuteAction(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		if (!a_owner) {
			return;
		}

		DispatchActionByType(a_affix, a_owner, a_target, a_hitData);
	}
}
