#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/HostileEffectGuard.h"

#include "CalamityAffixes/ArchmageSpellHitPolicy.h"
#include "CalamityAffixes/CombatContext.h"

#include "CalamityAffixes/PluginEditorIds.h"
#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/ProcChanceUtil.h"
#include "CalamityAffixes/SpecialActionSafetyPolicy.h"
#include "CalamityAffixes/TriggerGuards.h"
#include <algorithm>
#include <cstdint>
#include <string_view>


namespace CalamityAffixes
{
	EventBridge::ArchmageSelection EventBridge::SelectBestArchmageAction(
		std::chrono::steady_clock::time_point a_now,
		RE::Actor* a_owner,
		RE::Actor* a_target,
		const RE::HitData* a_hitData)
	{
		ArchmageSelection selection{};

		for (const auto idx : _affixSpecialActions.archmageAffixIndices) {
			if (idx >= _affixRuntimeState.affixes.size() || idx >= _affixRuntimeState.activeCounts.size()) {
				continue;
			}
			if (_affixRuntimeState.activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixRuntimeState.affixes[idx];
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

			const float chancePct = detail::ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeSettings.procChanceMult);
			if (!RollProcChance(_rng, _rngMutex, chancePct)) {
				continue;
			}

			const auto& action = affix.action;
			if (action.type != ActionType::kArchmage) {
				continue;
			}

			float maxMagicka = 0.0f;
			float extraCost = 0.0f;
			float extraDamage = 0.0f;
			if (!ResolveArchmageResourceUsage(
					a_owner,
					action.archmageDamagePctOfMaxMagicka,
					action.archmageCostPctOfMaxMagicka,
					maxMagicka,
					extraCost,
					extraDamage)) {
				continue;
			}

			if (action.archmageDamagePctOfMaxMagicka > selection.bestDamagePct) {
				selection.bestIdx = idx;
				selection.bestDamagePct = action.archmageDamagePctOfMaxMagicka;
				selection.bestCostPct = action.archmageCostPctOfMaxMagicka;
				selection.bestMaxMagicka = maxMagicka;
				selection.bestExtraCost = extraCost;
				selection.bestExtraDamage = extraDamage;
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

			auto* avOwner = skyrim_cast<RE::ActorValueOwner*>(a_caster);
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
		if (!IsHostileEffectTarget(a_caster, a_target)) {
			return false;
		}

		auto* magicCaster = a_caster ? a_caster->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant) : nullptr;
			if (!magicCaster) {
				return false;
			}

			auto* avOwner = a_caster ? skyrim_cast<RE::ActorValueOwner*>(a_caster) : nullptr;
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

		CastHostileOnlySpellImmediate(
			magicCaster,
			a_action.spell,
			a_action.noHitEffectArt,
			a_target,
			a_action.effectiveness,
			a_extraDamage,
			a_caster);
		return true;
	}

	void EventBridge::ProcessArchmageSpellHit(RE::Actor* a_caster, RE::Actor* a_target, RE::SpellItem* a_sourceSpell, const RE::HitData* a_hitData)
	{
		if (!_configLoaded || !_runtimeSettings.enabled.load(std::memory_order_relaxed) || !a_caster || !a_target || !a_sourceSpell) {
			return;
		}
		if (!a_caster->IsPlayerRef()) {
			return;
		}
		if (_combatState.procDepth > 0) {
			return;
		}

		const auto sourceEditorId = PluginEditorIds::OwnEditorIdOf(a_sourceSpell);
		if (detail::IsCalamityProcSource(sourceEditorId)) {
			// Avoid recursive stacking on our own proc spells.
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		const auto selection = SelectBestArchmageAction(now, a_caster, a_target, a_hitData);
		if (!selection.bestIdx || !selection.bestAction || !selection.bestAction->spell || selection.bestDamagePct <= 0.0f || selection.bestCostPct <= 0.0f) {
			return;
		}

		if (!ExecuteArchmageCast(
				*selection.bestAction,
				a_caster,
				a_target,
				sourceEditorId,
				selection.bestMaxMagicka,
				selection.bestExtraCost,
				selection.bestExtraDamage)) {
			return;
		}
		NoteDebugTestEvent("archmage:spell");

		auto& affix = _affixRuntimeState.affixes[*selection.bestIdx];
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


	// Runs from the deferred TESHitEvent drain (game thread, _stateMutex held,
	// procOrigin/procDepth already rejected by HandleHitEvent). Both hook and
	// fallback modes come here: HandleHealthDamage never sees which spell hit.
	void EventBridge::ProcessArchmageFromHitEvent(
		RE::Actor* a_caster,
		RE::Actor* a_target,
		RE::FormID a_sourceFormID,
		std::chrono::steady_clock::time_point a_now)
	{
		if (!_configLoaded || _affixSpecialActions.archmageAffixIndices.empty() || a_sourceFormID == 0u) {
			return;
		}
		if (!a_caster || !a_target || !a_caster->IsPlayerRef() || a_target->IsPlayerRef() || a_target->IsDead()) {
			return;
		}
		if (_combatState.procDepth > 0) {
			return;
		}
		if (!BuildCombatTriggerContext(a_target, a_caster).hostileEitherDirection) {
			return;
		}

		auto* source = RE::TESForm::LookupByID<RE::TESForm>(a_sourceFormID);
		auto* spell = source ? source->As<RE::SpellItem>() : nullptr;
		if (!spell) {
			return;
		}
		const auto spellType = spell->GetSpellType();
		if (!detail::IsArchmageSourceSpellType(
				spellType == RE::MagicSystem::SpellType::kSpell,
				spellType == RE::MagicSystem::SpellType::kScroll)) {
			return;
		}
		if (detail::IsCalamityProcSource(PluginEditorIds::OwnEditorIdOf(spell))) {
			return;
		}

		const auto key = (static_cast<std::uint64_t>(a_target->GetFormID()) << 32) |
		                 static_cast<std::uint64_t>(a_sourceFormID);
		const auto windowMs = detail::ResolveArchmageSpellHitDedupWindowMs(
			spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration);
		auto& seen = _combatState.archmageSpellHitAt;
		if (const auto it = seen.find(key); it != seen.end()) {
			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(a_now - it->second).count();
			if (detail::ShouldSuppressArchmageSpellHit(
					true,
					elapsed > 0 ? static_cast<std::uint64_t>(elapsed) : 0u,
					windowMs)) {
				return;
			}
		}
		seen[key] = a_now;
		if (seen.size() > 256u) {
			std::erase_if(seen, [&](const auto& a_entry) {
				return (a_now - a_entry.second) >=
				       std::chrono::milliseconds(detail::kArchmageConcentrationHitDedupWindowMs);
			});
		}

		ProcessArchmageSpellHit(a_caster, a_target, spell, nullptr);
	}

}
