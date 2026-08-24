#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/HostileEffectGuard.h"
#include "CalamityAffixes/TrapCellPolicy.h"

namespace CalamityAffixes
{
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
