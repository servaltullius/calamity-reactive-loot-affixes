#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/HostileEffectGuard.h"
#include "CalamityAffixes/TrapCellPolicy.h"

namespace CalamityAffixes
{
	bool EventBridge::IsZeroScaledHitCast(const Action& a_action, RE::SpellItem* a_spell, const RE::HitData* a_hitData) const
	{
		// Raw (pre-evolution) scaled value, as DoomMark checks it.
		return a_spell && a_hitData &&
		       a_action.magnitudeScaling.source != MagnitudeScaling::Source::kNone &&
		       ResolveSpellMagnitudeOverride(a_action, a_spell, a_hitData, false) <= 0.0f;
	}

	bool EventBridge::CanExecuteAction(
		const AffixRuntime& a_affix,
		RE::Actor* a_owner,
		RE::Actor* a_target,
		const RE::HitData* a_hitData)
	{
		if (!a_owner) {
			return false;
		}

		const auto& action = a_affix.action;
		switch (action.type) {
		case ActionType::kDebugNotify:
			return true;
		case ActionType::kCastSpell:
			if (!a_owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
				return false;
			}
			if (!action.applyToSelf && !IsHostileEffectTarget(a_owner, a_target)) {
				return false;
			}
			if (!action.modeCycleEnabled || action.modeCycleSpells.empty()) {
				// A summon proc fired by the player's own summon would replace that
				// very summon (it shares the player's summon slot): skip it before
				// chance and cooldown.
				if (action.spell && !action.applyToSelf && a_hitData && IsSummonCreatureSpellRecord(action.spell)) {
					const auto aggressorHolder = a_hitData->aggressor.get();
					const auto* aggressor = aggressorHolder.get();
					if (aggressor && aggressor != a_owner && aggressor->IsCommandedActor()) {
						return false;
					}
				}
				// Never push out a summon or thrall the player made themselves. An
				// older Calamity summon may be replaced. Conservative: with Twin
				// Souls the proc also waits while one own summon is out.
				if (action.spell && !action.applyToSelf && IsSummonCreatureSpellRecord(action.spell) &&
					CommandsNonCalamitySummon(a_owner)) {
					NoteDebugTestEvent("summon:own-kept");
					return false;
				}
				// An Exposed that would not beat the one already on the target is
				// skipped here, before its chance and cooldown are spent.
				if (action.spell && !action.applyToSelf && action.statusTag == StatusKind::kExposed &&
					!PlanExposureCast(action.spell, a_target, ResolvePreflightMagnitudeOverride(a_affix, action.spell, a_hitData)).improves) {
					NoteDebugTestEvent("exposed:skipped");
					return false;
				}
				// Likewise a self-buff weaker than the same-kind Calamity buff already on.
				if (action.spell && action.applyToSelf &&
					!PlanSelfBuffCast(a_owner, action.spell, ResolvePreflightMagnitudeOverride(a_affix, action.spell, a_hitData)).cast) {
					NoteDebugTestEvent("buff:stronger");
					return false;
				}
				// A hit-scaled cast on a hit that dealt nothing (blocked, resisted)
				// would land at the record's 1-point base and still spend the
				// cooldown. Same rule as DoomMark; triggers without HitData keep
				// their record magnitude.
				if (IsZeroScaledHitCast(action, action.spell, a_hitData)) {
					return false;
				}
				return action.spell != nullptr;
			}
			for (auto* spell : action.modeCycleSpells) {
				if (spell) {
					return true;
				}
			}
			return false;
		case ActionType::kCastSpellAdaptiveElement: {
			if (!a_owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
				return false;
			}
			if (!action.applyToSelf && !IsHostileEffectTarget(a_owner, a_target)) {
				return false;
			}

			auto* analysisTarget = ResolveAdaptiveAnalysisTarget(action, a_owner, a_target);
			if (!analysisTarget) {
				return false;
			}

			auto* spell = SelectAdaptiveSpellForTarget(action, analysisTarget).spell;
			if (action.modeCycleEnabled && action.modeCycleManualOnly && !action.modeCycleSpells.empty()) {
				const auto primaryKey = ResolvePrimaryEquippedInstanceKey(a_affix.token);
				const auto* state = primaryKey ?
					FindInstanceRuntimeState(*primaryKey, a_affix.token) :
					nullptr;
				const auto manualModeCount = static_cast<std::uint32_t>(action.modeCycleSpells.size());
				const auto modeIndex = (state ? state->modeIndex : 0u) % (manualModeCount + 1u);
				if (modeIndex > 0u) {
					spell = action.modeCycleSpells[modeIndex - 1u];
				}
			}
			if (spell && !action.applyToSelf && action.statusTag == StatusKind::kExposed &&
				!PlanExposureCast(spell, a_target, ResolvePreflightMagnitudeOverride(a_affix, spell, a_hitData)).improves) {
				NoteDebugTestEvent("exposed:skipped");
				return false;
			}
			if (IsZeroScaledHitCast(action, spell, a_hitData)) {
				return false;
			}
			return spell != nullptr;
		}
		case ActionType::kSpawnTrap: {
			RE::Actor* spawnTarget = nullptr;
			if (!SelectSpawnTrapTarget(
					action,
					a_owner,
					a_target,
					a_hitData,
					spawnTarget,
					a_affix.normalWeaponHitProcChancePct > 0.0f) ||
				!spawnTarget) {
				return false;
			}
			auto* cell = spawnTarget->GetParentCell();
			return detail::IsTrapCellUsable(cell != nullptr, cell && cell->IsAttached());
		}
		case ActionType::kEchoStrike:
			// Only the owner's own melee swing opens the window; reject everything
			// else before chance or ICD are committed.
			return action.spell && action.echoStanceSpell &&
			       a_owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant) &&
			       IsHostileEffectTarget(a_owner, a_target) &&
			       IsEchoStrikeActivationHit(action, a_owner, a_hitData);
		case ActionType::kDoomMark:
			// A blocked hit scales to nothing, and a full queue cannot take a new
			// mark: both would spend the cooldown for no burst.
			// A corpse hit still raises a hit event; a mark on it never bursts.
			return action.spell && action.doomMarkSpell && a_target && !a_target->IsDead() &&
			       a_owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant) &&
			       IsHostileEffectTarget(a_owner, a_target) &&
			       ResolveSpellMagnitudeOverride(action, action.spell, a_hitData, false) > 0.0f &&
			       (_combatState.doomQueue.Size() < DoomQueue::kCapacity ||
				       _combatState.doomQueue.Find(a_target->GetFormID()).has_value());
		case ActionType::kSpreadStatus:
			// The victim is already dead; only what it carried matters. A kill with
			// nobody in range would spend the cooldown on nothing, so require one
			// living hostile there (same rules as the spread itself).
			return a_target != nullptr &&
			       action.spreadRadius > 0.0f &&
			       a_owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant) &&
			       HasSpreadableStatus(a_target) &&
			       !CollectNearbyHostiles(a_owner, a_target, action.spreadRadius, 1u).empty();
		default:
			return false;
		}
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
			ExecuteCastSpellAdaptiveElementAction(a_affix, a_owner, a_target, a_hitData);
			break;
		case ActionType::kSpawnTrap:
			ExecuteSpawnTrapAction(a_affix, a_owner, a_target, a_hitData);
			break;
		case ActionType::kEchoStrike:
			ExecuteEchoStrikeActivation(a_affix, a_owner);
			break;
		case ActionType::kDoomMark:
			ExecuteDoomMarkAction(a_affix, a_owner, a_target, a_hitData);
			break;
		case ActionType::kSpreadStatus:
			ExecuteSpreadStatusAction(a_affix, a_owner, a_target);
			break;
		default:
			SKSE::log::warn("CalamityAffixes: DispatchActionByType unhandled ActionType {}.", static_cast<int>(a_action.type));
			break;
		}
	}

	void EventBridge::ExecuteActionWithProcDepthGuard(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		const ScopedProcDepth procDepthGuard{ _combatState };
		(void)procDepthGuard;
		ExecuteAction(a_affix, a_owner, a_target, a_hitData);
	}

	void EventBridge::ExecuteAction(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		if (!a_owner) {
			return;
		}

		DispatchActionByType(a_affix, a_owner, a_target, a_hitData);
	}
}
