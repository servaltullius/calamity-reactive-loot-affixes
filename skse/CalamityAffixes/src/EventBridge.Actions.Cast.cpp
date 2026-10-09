#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/HostileEffectGuard.h"
#include "CalamityAffixes/ImmediateHealthReadback.h"
#include "CalamityAffixes/PluginEditorIds.h"
#include "CalamityAffixes/PointerSafety.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string_view>
#include <vector>


namespace CalamityAffixes
{
	namespace
	{
		// Values whose timed change is a buff. Health, magicka and stamina are
		// instant restores in our spells, not something to keep one of.
		[[nodiscard]] bool IsReplaceableSelfBuffValue(RE::ActorValue a_value) noexcept
		{
			return a_value != RE::ActorValue::kNone && a_value != RE::ActorValue::kHealth &&
				a_value != RE::ActorValue::kMagicka && a_value != RE::ActorValue::kStamina;
		}

		// v2.3.0, Elden Ring's rule: one Calamity self-buff per kind. A new timed
		// buff from one of our spells removes our other timed buffs that raise
		// the same value, so procs from several affixes stop stacking into one
		// runaway bonus. Buffs of different kinds still combine; constant
		// (equipped) effects, other mods' and vanilla buffs are never touched.
		void DispelReplacedCalamitySelfBuffs(RE::Actor* a_owner, const RE::SpellItem* a_spell)
		{
			a_owner = SanitizeObjectPointer(a_owner);
			if (!a_owner || !a_spell) {
				return;
			}
			std::vector<RE::ActorValue> kinds;
			for (const auto* effect : a_spell->effects) {
				if (!effect || !effect->baseEffect || effect->effectItem.duration <= 0) {
					continue;
				}
				const auto value = effect->baseEffect->data.primaryAV;
				if (IsReplaceableSelfBuffValue(value) &&
					std::find(kinds.begin(), kinds.end(), value) == kinds.end()) {
					kinds.push_back(value);
				}
			}
			if (kinds.empty()) {
				return;
			}
			auto* magicTarget = a_owner->AsMagicTarget();
			auto* effects = magicTarget ? magicTarget->GetActiveEffectList() : nullptr;
			if (!effects) {
				return;
			}
			std::vector<RE::ActiveEffect*> replaced;
			for (auto* activeEffect : *effects) {
				activeEffect = SanitizeObjectPointer(activeEffect);
				if (!activeEffect || !activeEffect->spell || activeEffect->spell == a_spell ||
					activeEffect->duration <= 0.0f ||
					activeEffect->flags.any(RE::ActiveEffect::Flag::kDispelled) ||
					!activeEffect->effect || !activeEffect->effect->baseEffect) {
					continue;
				}
				if (!PluginEditorIds::OwnEditorIdOf(activeEffect->spell).starts_with("CAFF_SPEL_")) {
					continue;
				}
				const auto value = activeEffect->effect->baseEffect->data.primaryAV;
				if (std::find(kinds.begin(), kinds.end(), value) != kinds.end()) {
					replaced.push_back(activeEffect);
				}
			}
			// Dispel after the walk: dispelling edits the list being iterated.
			for (auto* activeEffect : replaced) {
				SKSE::log::debug(
					"CalamityAffixes: self-buff replaced (old={}, new={}, value={}).",
					PluginEditorIds::OwnEditorIdOf(activeEffect->spell),
					PluginEditorIds::OwnEditorIdOf(a_spell),
					static_cast<std::uint32_t>(activeEffect->effect->baseEffect->data.primaryAV));
				activeEffect->Dispel(true);
			}
		}

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

		[[nodiscard]] bool HasImmediateHealthValueModifier(const RE::SpellItem* a_spell) noexcept
		{
			if (!a_spell) {
				return false;
			}

			for (const auto* effect : a_spell->effects) {
				if (!effect || !effect->baseEffect || effect->effectItem.duration != 0u) {
					continue;
				}
				if (effect->baseEffect->GetArchetype() == RE::EffectSetting::Archetype::kValueModifier &&
					effect->baseEffect->data.primaryAV == RE::ActorValue::kHealth) {
					return true;
				}
			}

			return false;
		}

		[[nodiscard]] std::optional<float> ReadCurrentHealth(RE::Actor* a_actor) noexcept
		{
			if (!a_actor) {
				return std::nullopt;
			}

			auto* actorValueOwner = skyrim_cast<RE::ActorValueOwner*>(a_actor);
			if (!actorValueOwner) {
				return std::nullopt;
			}

			const float health = actorValueOwner->GetActorValue(RE::ActorValue::kHealth);
			return std::isfinite(health) ? std::optional<float>{ health } : std::nullopt;
		}

		void LogImmediateHealthReadback(
			std::string_view a_lane,
			std::string_view a_affixId,
			const RE::SpellItem* a_spell,
			const RE::Actor* a_target,
			float a_magnitudeOverride,
			const ImmediateHealthReadbackResult& a_result)
		{
			// Same-call sample only: the engine may apply the effect after this call
			// returns, so a 0 delta here is NOT evidence that the damage failed. The
			// authoritative applied-signal is the TESMagicEffectApplyEvent observation.
			if (a_result.healthBefore && a_result.healthAfter && a_result.healthChange) {
				SKSE::log::debug(
					"CalamityAffixes: immediate health readback (same-call sample, not a damage verdict) (affix={}, lane={}, spell={}, target={}, magnitudeOverride={}, healthBefore={}, healthAfter={}, healthChangeAfterMinusBefore={}).",
					a_affixId,
					a_lane,
					a_spell ? a_spell->GetName() : "<none>",
					a_target ? a_target->GetName() : "<none>",
					a_magnitudeOverride,
					*a_result.healthBefore,
					*a_result.healthAfter,
					*a_result.healthChange);
				return;
			}

			SKSE::log::debug(
				"CalamityAffixes: immediate health readback unavailable (same-call sample, not a damage verdict) (affix={}, lane={}, spell={}, target={}, sampledBefore={}, sampledAfter={}).",
				a_affixId,
				a_lane,
				a_spell ? a_spell->GetName() : "<none>",
				a_target ? a_target->GetName() : "<none>",
				a_result.healthBefore.has_value(),
				a_result.healthAfter.has_value());
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
			SKSE::log::debug(
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
		// CommonLibSSE-NG exposes skyrim_cast for RTTI-safe cross-version ActorValueOwner access.
		if (auto* avOwner = skyrim_cast<RE::ActorValueOwner*>(a_analysisTarget)) {
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

		SKSE::log::debug(
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
			SKSE::log::error("CalamityAffixes: ExecuteAction CastSpell skipped (spell is null, affix={}).", a_affix.id);
			return;
		}

		RE::TESObjectREFR* castTarget = ResolveSpellCastTarget(a_action, a_target);
		if (!a_action.applyToSelf && IsSummonLikeSpell(spell)) {
			const auto* targetActor = castTarget ? castTarget->As<RE::Actor>() : nullptr;
			if (!castTarget || (targetActor && targetActor->IsDead())) {
				castTarget = a_owner;
				if (_loot.debugLog) {
					SKSE::log::debug(
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
			SKSE::log::debug(
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

		// Shared statuses: Exposed keeps only the strongest per resistance, and
		// every status our spell applies goes into the ledger after the cast.
		auto* statusTarget = a_action.applyToSelf ? nullptr : (castTarget ? castTarget->As<RE::Actor>() : nullptr);
		if (statusTarget && !PrepareExposureCast(a_action.statusTag, spell, statusTarget, magnitudeOverride)) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: Exposed skipped, a stronger one is active (affix={}, target={}).",
					a_affix.id,
					statusTarget->GetName());
			}
			return;
		}

		auto* healthTarget = _loot.debugLog ?
			(a_action.applyToSelf ? caster : (castTarget ? castTarget->As<RE::Actor>() : nullptr)) :
			nullptr;
		const bool observeImmediateHealth =
			_loot.debugLog && healthTarget && HasImmediateHealthValueModifier(spell);
		const auto healthReadback = ObserveImmediateHealthChange(
			observeImmediateHealth,
			[healthTarget]() { return ReadCurrentHealth(healthTarget); },
			[&]() {
				if (a_action.applyToSelf && !IsSummonLikeSpell(spell)) {
					DispelReplacedCalamitySelfBuffs(caster, spell);
					magicCaster->CastSpellImmediate(
						spell,
						a_action.noHitEffectArt,
						castTarget,
						a_action.effectiveness,
						false,
						magnitudeOverride,
						caster);
				} else {
					CastHostileOnlySpellImmediate(
						magicCaster,
						spell,
						a_action.noHitEffectArt,
						castTarget,
						a_action.effectiveness,
						magnitudeOverride,
						caster);
				}
			});
		if (observeImmediateHealth) {
			LogImmediateHealthReadback(
				"CastSpell",
				a_affix.id,
				spell,
				healthTarget,
				magnitudeOverride,
				healthReadback);
		}
		if (statusTarget) {
			RecordStatusApplication(a_action.statusTag, spell, statusTarget, magnitudeOverride);
			ApplyTaggedBuildUp(a_action, caster, statusTarget, magicCaster);
		}
		PlayActionFeedback(a_action, a_owner, a_target, ActionFeedbackPlayOn::kProc);
	}

	void EventBridge::ExecuteCastSpellAdaptiveElementAction(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		const auto& a_action = a_affix.action;
		auto* caster = a_owner;
		auto* magicCaster = caster->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!magicCaster) {
			SKSE::log::error("CalamityAffixes: ExecuteAction CastSpellAdaptiveElement skipped (magicCaster is null, affix={}).", a_affix.id);
			return;
		}

		const auto primaryKey = ResolvePrimaryEquippedInstanceKey(a_affix.token);
		const InstanceRuntimeState* state = nullptr;
		if (primaryKey) {
			state = FindInstanceRuntimeState(*primaryKey, a_affix.token);
		}

		auto* analysisTarget = ResolveAdaptiveAnalysisTarget(a_action, a_owner, a_target);
		if (!analysisTarget) {
			return;
		}

		AdaptiveCastSelection selection = SelectAdaptiveSpellForTarget(a_action, analysisTarget);
		RE::SpellItem* spell = selection.spell;
		std::uint32_t modeIndex = 0u;
		bool manualModeOverride = false;
		if (a_action.modeCycleEnabled && a_action.modeCycleManualOnly && !a_action.modeCycleSpells.empty()) {
			const auto manualModeCount = static_cast<std::uint32_t>(a_action.modeCycleSpells.size());
			const auto modeCountWithAuto = manualModeCount + 1u;  // index 0 = adaptive auto
			modeIndex = (modeCountWithAuto > 0u) ? ((state ? state->modeIndex : 0u) % modeCountWithAuto) : 0u;
			if (modeIndex > 0u) {
				const auto spellIdx = modeIndex - 1u;
				if (spellIdx < manualModeCount) {
					spell = a_action.modeCycleSpells[spellIdx];
					manualModeOverride = (spell != nullptr);
				}
			}
		}

		if (!spell) {
			return;
		}

		RE::TESObjectREFR* castTarget = ResolveSpellCastTarget(a_action, a_target);
		float magnitudeOverride = ResolveSpellMagnitudeOverride(a_action, spell, a_hitData, false);
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

		if (!manualModeOverride) {
			LogAdaptiveCastSpell(selection, castTarget, magnitudeOverride);
		} else if (_loot.debugLog) {
			const std::uint32_t xp = state ? state->evolutionXp : 0u;
			SKSE::log::debug(
				"CalamityAffixes: CastSpellAdaptiveElementManualOverride (affix={}, spell={}, manualModeIndex={}, magnitudeOverride={}, target={}, evolutionStage={}, evolutionXP={}, evolutionMult={}).",
				a_affix.id,
				spell->GetName(),
				modeIndex,
				magnitudeOverride,
				castTarget ? castTarget->GetName() : "<none>",
				evolutionStage,
				xp,
				evolutionMultiplier);
		}

		// Shared statuses: Exposed keeps only the strongest per resistance, and
		// every status our spell applies goes into the ledger after the cast.
		auto* statusTarget = a_action.applyToSelf ? nullptr : (castTarget ? castTarget->As<RE::Actor>() : nullptr);
		if (statusTarget && !PrepareExposureCast(a_action.statusTag, spell, statusTarget, magnitudeOverride)) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: Exposed skipped, a stronger one is active (affix={}, target={}).",
					a_affix.id,
					statusTarget->GetName());
			}
			return;
		}

		auto* healthTarget = _loot.debugLog ?
			(a_action.applyToSelf ? caster : (castTarget ? castTarget->As<RE::Actor>() : nullptr)) :
			nullptr;
		const bool observeImmediateHealth =
			_loot.debugLog && healthTarget && HasImmediateHealthValueModifier(spell);
		const auto healthReadback = ObserveImmediateHealthChange(
			observeImmediateHealth,
			[healthTarget]() { return ReadCurrentHealth(healthTarget); },
			[&]() {
				if (a_action.applyToSelf) {
					DispelReplacedCalamitySelfBuffs(caster, spell);
					magicCaster->CastSpellImmediate(
						spell,
						a_action.noHitEffectArt,
						castTarget,
						a_action.effectiveness,
						false,
						magnitudeOverride,
						caster);
				} else {
					CastHostileOnlySpellImmediate(
						magicCaster,
						spell,
						a_action.noHitEffectArt,
						castTarget,
						a_action.effectiveness,
						magnitudeOverride,
						caster);
				}
			});
		if (observeImmediateHealth) {
			LogImmediateHealthReadback(
				"CastSpellAdaptiveElement",
				a_affix.id,
				spell,
				healthTarget,
				magnitudeOverride,
				healthReadback);
		}
		if (statusTarget) {
			RecordStatusApplication(a_action.statusTag, spell, statusTarget, magnitudeOverride);
			ApplyTaggedBuildUp(a_action, caster, statusTarget, magicCaster);
		}
		PlayActionFeedback(a_action, a_owner, a_target, ActionFeedbackPlayOn::kProc);
	}
}
