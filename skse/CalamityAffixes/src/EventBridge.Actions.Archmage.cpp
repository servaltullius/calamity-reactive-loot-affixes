#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/TriggerGuards.h"
#include <algorithm>
#include <mutex>
#include <string_view>


namespace CalamityAffixes
{
	namespace
	{
		float ResolveSpecialActionProcChancePct_Archmage(float a_configuredChancePct)
		{
			if (a_configuredChancePct <= 0.0f) {
				return 100.0f;
			}

			return std::clamp(a_configuredChancePct, 0.0f, 100.0f);
		}

		bool RollProcChance_Archmage(std::mt19937& a_rng, std::mutex& a_rngMutex, float a_chancePct)
		{
			if (a_chancePct >= 100.0f) {
				return true;
			}
			if (a_chancePct <= 0.0f) {
				return false;
			}

			std::lock_guard<std::mutex> lock(a_rngMutex);
			std::uniform_real_distribution<float> dist(0.0f, 100.0f);
			return dist(a_rng) < a_chancePct;
		}
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

			const float chancePct = ResolveSpecialActionProcChancePct_Archmage(affix.procChancePct * _runtimeProcChanceMult);
			if (!RollProcChance_Archmage(_rng, _rngMutex, chancePct)) {
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
			SKSE::log::debug(
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

		const auto sourceEditorId = SafeCStringView(a_sourceSpell->GetFormEditorID());
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
