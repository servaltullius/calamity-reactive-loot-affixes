#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/HostileEffectGuard.h"
#include "CalamityAffixes/Hooks.h"

#include <chrono>
#include <vector>

// Shadow Boxer (EchoStrike): a melee hit (optionally only a power attack)
// opens a window, and every melee swing inside it is repeated by a shadow a
// moment later for a share of that swing's damage.
namespace CalamityAffixes
{
	bool EventBridge::IsEchoStrikeEligibleHit(RE::Actor* a_owner, const RE::HitData* a_hitData)
	{
		if (!a_owner || !a_hitData) {
			return false;
		}

		const auto aggressor = a_hitData->aggressor.get();
		// Only the swing's own record counts. Falling back to the weapon the owner
		// holds would let a spell or hazard tick pass as a melee swing.
		auto* directWeapon = SanitizeObjectPointer(a_hitData->weapon);
		const bool hasMeleeEvidence =
			(directWeapon && !HitDataUtil::IsBowOrCrossbow(directWeapon)) ||
			a_hitData->flags.any(RE::HitData::Flag::kMeleeAttack);
		return detail::IsEchoStrikeEligibleHit(
			aggressor.get() == a_owner,
			hasMeleeEvidence,
			HitDataUtil::IsBowOrCrossbow(HitDataUtil::ResolveCastOnCritHitWeapon(a_hitData, a_owner)),
			a_hitData->attackDataSpell != nullptr,
			a_hitData->flags.any(RE::HitData::Flag::kBash, RE::HitData::Flag::kTimedBash),
			a_hitData->flags.any(RE::HitData::Flag::kExplosion));
	}

	bool EventBridge::IsEchoStrikeActivationHit(
		const Action& a_action,
		RE::Actor* a_owner,
		const RE::HitData* a_hitData)
	{
		return detail::IsEchoStrikeActivationHit(
			IsEchoStrikeEligibleHit(a_owner, a_hitData),
			a_hitData && a_hitData->flags.any(RE::HitData::Flag::kPowerAttack),
			a_action.echoRequirePowerAttack);
	}

	void EventBridge::ExecuteEchoStrikeActivation(const AffixRuntime& a_affix, RE::Actor* a_owner)
	{
		const auto& action = a_affix.action;
		auto* magicCaster = a_owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!magicCaster || !action.echoStanceSpell) {
			return;
		}

		// The stance spell carries the shadow shader and lasts as long as the window.
		magicCaster->CastSpellImmediate(action.echoStanceSpell, false, a_owner, 1.0f, false, 0.0f, a_owner);

		const auto now = std::chrono::steady_clock::now();
		_combatState.echoStrike.OpenWindow(a_affix.token, now + action.echoWindow);

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: EchoStrike window opened (affix={}, windowMs={}, delayMs={}).",
				a_affix.id,
				action.echoWindow.count(),
				action.echoDelay.count());
		}
	}

	void EventBridge::ProcessEchoStrikeHit(
		RE::Actor* a_attacker,
		RE::Actor* a_target,
		const RE::HitData* a_hitData,
		std::chrono::steady_clock::time_point a_now)
	{
		auto& state = _combatState.echoStrike;
		if (!state.IsWindowOpen(a_now)) {
			return;
		}
		if (!a_attacker || !a_attacker->IsPlayerRef() || !a_target || a_target->IsDead()) {
			return;
		}

		const auto idxIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(state.windowToken);
		if (idxIt == _affixRuntimeState.affixRegistry.affixIndexByToken.end() ||
			idxIt->second >= _affixRuntimeState.affixes.size() ||
			idxIt->second >= _affixRuntimeState.activeCounts.size() ||
			_affixRuntimeState.activeCounts[idxIt->second] == 0) {
			// The item carrying the window was unequipped.
			state.CloseWindow();
			return;
		}

		const auto& affix = _affixRuntimeState.affixes[idxIt->second];
		const auto& action = affix.action;
		if (action.type != ActionType::kEchoStrike || !action.spell ||
			!IsEchoStrikeEligibleHit(a_attacker, a_hitData) ||
			!IsHostileEffectTarget(a_attacker, a_target)) {
			return;
		}

		if (!state.TryRememberSource(HitDataUtil::MakeHitContentSignature(a_hitData))) {
			return;
		}

		const float magnitude = ResolveSpellMagnitudeOverride(action, action.spell, a_hitData, false);
		if (magnitude <= 0.0f) {
			return;
		}

		const bool queued = state.Enqueue({
			.affixToken = affix.token,
			.targetFormID = a_target->GetFormID(),
			.magnitude = magnitude,
			.fireAt = a_now + action.echoDelay,
		});
		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: EchoStrike {} (affix={}, target={}, magnitude={}, pending={}).",
				queued ? "queued" : "dropped (queue full)",
				affix.id,
				a_target->GetName(),
				magnitude,
				state.pending.size());
		}
	}

	void EventBridge::TickEchoStrikes()
	{
		const std::scoped_lock lock(_stateMutex);
		auto& state = _combatState.echoStrike;
		if (!_configLoaded || !_runtimeSettings.enabled.load(std::memory_order_relaxed)) {
			state.Reset();
			return;
		}

		std::vector<EchoStrikePending> due;
		if (state.TakeDue(std::chrono::steady_clock::now(), due) == 0u) {
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* magicCaster = player ? player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant) : nullptr;
		if (!magicCaster) {
			return;
		}

		for (const auto& echo : due) {
			const auto idxIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(echo.affixToken);
			if (idxIt == _affixRuntimeState.affixRegistry.affixIndexByToken.end() ||
				idxIt->second >= _affixRuntimeState.affixes.size()) {
				continue;
			}
			const auto& action = _affixRuntimeState.affixes[idxIt->second].action;
			auto* target = RE::TESForm::LookupByID<RE::Actor>(echo.targetFormID);
			if (!action.spell || !target || target->IsDead() || !target->Is3DLoaded()) {
				continue;
			}

			Hooks::ExpectEchoStrikeDamage(target, player, echo.magnitude);
			{
				const ScopedProcDepth procDepthGuard{ _combatState };
				CastHostileOnlySpellImmediate(
					magicCaster,
					action.spell,
					action.noHitEffectArt,
					target,
					action.effectiveness,
					echo.magnitude,
					player);
			}
			PlayActionFeedback(action, player, target, ActionFeedbackPlayOn::kProc);
		}
	}
}
