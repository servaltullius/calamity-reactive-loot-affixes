#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include <RE/M/Misc.h>
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

			// NOTE:
			// Actor's multiple-inheritance layout differs across Skyrim versions (notably 1.6.629+),
			// so accessing ActorValueOwner directly through Actor is unsafe for a multi-version DLL.
			// CommonLibSSE-NG provides AsActorValueOwner() to do this cast in a layout-agnostic way.
			if (auto* avOwner = a_analysisTarget->AsActorValueOwner()) {
				selection.resistFire = avOwner->GetActorValue(RE::ActorValue::kResistFire);
				selection.resistFrost = avOwner->GetActorValue(RE::ActorValue::kResistFrost);
				selection.resistShock = avOwner->GetActorValue(RE::ActorValue::kResistShock);
			}
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
			state = FindInstanceRuntimeState(*primaryKey, a_affix.token);
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
