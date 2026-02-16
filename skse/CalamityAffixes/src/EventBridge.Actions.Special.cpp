#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/CombatContext.h"
#include "CalamityAffixes/TriggerGuards.h"
#include <algorithm>
#include <string_view>
#include <vector>

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
	EventBridge::ConversionResult EventBridge::EvaluateConversion(
		RE::Actor* a_attacker,
		RE::Actor* a_target,
		const RE::HitData* a_hitData,
		float& a_inOutDamage)
	{
		if (!_configLoaded || !_runtimeEnabled || _convertAffixIndices.empty()) {
			return {};
		}

		if (!a_attacker || !a_target) {
			return {};
		}

			if (!a_attacker->IsPlayerRef()) {
				return {};
			}
			const auto now = std::chrono::steady_clock::now();
			const bool hostileEitherDirection = IsHostileEitherDirection(a_attacker, a_target);
			const bool allowNeutralOutgoing =
				ShouldResolveNonHostileOutgoingFirstHitAllowance(
					true,
					a_target->IsPlayerRef(),
					AllowsNonHostilePlayerOwnedOutgoingProcs()) &&
				ResolveNonHostileOutgoingFirstHitAllowance(a_attacker, a_target, hostileEitherDirection, now);
			if (!(hostileEitherDirection || allowNeutralOutgoing)) {
				return {};
			}
			if (!a_hitData || !a_hitData->weapon) {
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

			if (!PassesRecentlyGates(affix, a_attacker, now)) {
				continue;
			}

			if (!PassesLuckyHitGate(affix, Trigger::kHit, a_hitData, now)) {
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

		if (_loot.debugLog) {
			spdlog::debug(
				"CalamityAffixes: ConvertDamage (weapon={}, physicalDealt={}, convertPct={}%, converted={}, inOutDamage={})",
				a_hitData->weapon->GetName(),
				physicalDealt,
				action->convertPct,
				converted,
				a_inOutDamage);
		}

		return ConversionResult{
			.spell = action->spell,
			.convertedDamage = converted,
			.effectiveness = action->effectiveness,
			.noHitEffectArt = action->noHitEffectArt,
		};
	}

	EventBridge::MindOverMatterResult EventBridge::EvaluateMindOverMatter(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData,
		float& a_inOutDamage)
	{
		MindOverMatterResult result{};
		if (!_configLoaded || !_runtimeEnabled || _mindOverMatterAffixIndices.empty()) {
			return result;
		}
		if (!a_target || !a_attacker || !a_target->IsPlayerRef()) {
			return result;
		}
		if (!IsHostileEitherDirection(a_target, a_attacker)) {
			return result;
		}
		if (!a_hitData || a_inOutDamage <= 0.0f) {
			return result;
		}

		const float physicalTaken = std::max(0.0f, a_hitData->physicalDamage - a_hitData->resistedPhysicalDamage);
		if (physicalTaken <= 0.0f) {
			return result;
		}

		MaybeResyncEquippedAffixes(std::chrono::steady_clock::now());

		bool hasAnyMindOverMatter = false;
		for (const auto idx : _mindOverMatterAffixIndices) {
			if (idx < _activeCounts.size() && _activeCounts[idx] > 0) {
				hasAnyMindOverMatter = true;
				break;
			}
		}
		if (!hasAnyMindOverMatter) {
			return result;
		}

		const auto now = std::chrono::steady_clock::now();
		AffixRuntime* selectedAffix = nullptr;
		PerTargetCooldownKey selectedPerTargetKey{};
		bool selectedUsesPerTargetIcd = false;
		const Action* action = nullptr;
		float bestRedirectPct = 0.0f;

		for (const auto idx : _mindOverMatterAffixIndices) {
			if (idx >= _affixes.size() || idx >= _activeCounts.size()) {
				continue;
			}
			if (_activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixes[idx];
			const auto& candidate = affix.action;
			if (candidate.type != ActionType::kMindOverMatter || candidate.mindOverMatterDamageToMagickaPct <= 0.0f) {
				continue;
			}

			if (now < affix.nextAllowed) {
				continue;
			}
			if (!PassesRecentlyGates(affix, a_target, now)) {
				continue;
			}
			if (!PassesLuckyHitGate(affix, Trigger::kIncomingHit, a_hitData, now)) {
				continue;
			}

			PerTargetCooldownKey perTargetKey{};
			const bool usesPerTargetIcd = (affix.perTargetIcd.count() > 0 && a_attacker && affix.token != 0u);
			if (IsPerTargetCooldownBlocked(affix, a_attacker, now, &perTargetKey)) {
				continue;
			}

			const float chancePct = ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeProcChanceMult);
			if (!RollProcChance(_rng, chancePct)) {
				continue;
			}

			if (!action || candidate.mindOverMatterDamageToMagickaPct > bestRedirectPct) {
				action = std::addressof(candidate);
				selectedAffix = std::addressof(affix);
				selectedPerTargetKey = perTargetKey;
				selectedUsesPerTargetIcd = usesPerTargetIcd;
				bestRedirectPct = candidate.mindOverMatterDamageToMagickaPct;
			}
		}

		if (!action || action->mindOverMatterDamageToMagickaPct <= 0.0f) {
			return result;
		}

		auto* avOwner = a_target->AsActorValueOwner();
		if (!avOwner) {
			return result;
		}

		const float currentMagicka = std::max(0.0f, avOwner->GetActorValue(RE::ActorValue::kMagicka));
		if (currentMagicka <= 0.0f) {
			return result;
		}

		float redirect = physicalTaken * (action->mindOverMatterDamageToMagickaPct / 100.0f);
		if (redirect <= 0.0f) {
			return result;
		}
		redirect = std::min(redirect, a_inOutDamage);
		if (action->mindOverMatterMaxRedirectPerHit > 0.0f) {
			redirect = std::min(redirect, action->mindOverMatterMaxRedirectPerHit);
		}
		redirect = std::min(redirect, currentMagicka);
		if (redirect <= 0.0f) {
			return result;
		}

		avOwner->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -redirect);
		a_inOutDamage -= redirect;
		if (a_inOutDamage < 0.0f) {
			a_inOutDamage = 0.0f;
		}

		if (selectedAffix && selectedAffix->icd.count() > 0) {
			selectedAffix->nextAllowed = now + selectedAffix->icd;
		}
		if (selectedUsesPerTargetIcd && selectedAffix) {
			CommitPerTargetCooldown(selectedPerTargetKey, selectedAffix->perTargetIcd, now);
		}

		result.redirectedDamage = redirect;
		result.consumedMagicka = redirect;
		result.redirectPct = action->mindOverMatterDamageToMagickaPct;

		if (_loot.debugLog) {
			spdlog::debug(
				"CalamityAffixes: MindOverMatter (physicalTaken={}, redirectPct={}%, redirected={}, remainingHealthDamage={}).",
				physicalTaken,
				result.redirectPct,
				result.redirectedDamage,
				a_inOutDamage);
		}

		return result;
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
			const auto now = std::chrono::steady_clock::now();
			const bool hostileEitherDirection = IsHostileEitherDirection(a_attacker, a_target);
			const bool allowNeutralOutgoing =
				ShouldResolveNonHostileOutgoingFirstHitAllowance(
					true,
					a_target->IsPlayerRef(),
					AllowsNonHostilePlayerOwnedOutgoingProcs()) &&
				ResolveNonHostileOutgoingFirstHitAllowance(a_attacker, a_target, hostileEitherDirection, now);
			if (!(hostileEitherDirection || allowNeutralOutgoing)) {
				return {};
			}
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

			if (!PassesRecentlyGates(affix, a_attacker, now)) {
				continue;
			}

			if (!PassesLuckyHitGate(affix, Trigger::kHit, a_hitData, now)) {
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

		if (_loot.debugLog) {
			spdlog::debug(
				"CalamityAffixes: CastOnCrit triggered (crit={}, powerAttack={}, spells={}, picked={}).",
				isCrit,
				isPowerAttack,
				pool.size(),
				pick->spell->GetName());
		}

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

	float EventBridge::GetCritDamageMultiplier(
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData) const
	{
		if (!_configLoaded || !_runtimeEnabled || _activeCritDamageBonusPct <= 0.0f) {
			return 1.0f;
		}

		if (!a_attacker || !a_attacker->IsPlayerRef()) {
			return 1.0f;
		}

		if (!a_hitData || !a_hitData->flags.any(RE::HitData::Flag::kCritical)) {
			return 1.0f;
		}

		const float mult = 1.0f + (_activeCritDamageBonusPct / 100.0f);
		if (_loot.debugLog) {
			spdlog::debug("CalamityAffixes: crit damage bonus {:.0f}% -> multiplier {:.2f}", _activeCritDamageBonusPct, mult);
		}
		return mult;
	}

	EventBridge::ArchmageSelection EventBridge::SelectBestArchmageAction(
		std::chrono::steady_clock::time_point a_now,
		RE::Actor* a_owner,
		RE::Actor* a_target,
		const RE::HitData* a_hitData)
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
			if (!PassesRecentlyGates(affix, a_owner, a_now)) {
				continue;
			}
			if (IsPerTargetCooldownBlocked(affix, a_target, a_now, nullptr)) {
				continue;
			}
			if (!PassesLuckyHitGate(affix, Trigger::kHit, a_hitData, a_now)) {
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

			auto* avOwner = a_caster->AsActorValueOwner();
			if (!avOwner) {
				return false;
			}

			a_outMaxMagicka = std::max(0.0f, avOwner->GetPermanentActorValue(RE::ActorValue::kMagicka));
			const float currentMagicka = std::max(0.0f, avOwner->GetActorValue(RE::ActorValue::kMagicka));

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

			auto* avOwner = a_caster ? a_caster->AsActorValueOwner() : nullptr;
			if (!avOwner) {
				return false;
			}
			avOwner->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -a_extraCost);

		if (_loot.debugLog) {
			spdlog::debug(
				"CalamityAffixes: Archmage (source={}, maxMagicka={}, extraCost={}, extraDamage={})",
				a_sourceEditorId,
				a_maxMagicka,
				a_extraCost,
				a_extraDamage);
		}

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

	void EventBridge::ProcessArchmageSpellHit(RE::Actor* a_caster, RE::Actor* a_target, RE::SpellItem* a_sourceSpell, const RE::HitData* a_hitData)
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
		const auto selection = SelectBestArchmageAction(now, a_caster, a_target, a_hitData);
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


}
