#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/CombatContext.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>

namespace CalamityAffixes
{
	namespace
	{
		constexpr auto kHealthDamageSignatureStaleWindow = std::chrono::milliseconds(300);
		constexpr auto kOutgoingPerTargetWindow = std::chrono::milliseconds(400);

		[[nodiscard]] std::uint32_t FloatBits(float a_value) noexcept
		{
			return std::bit_cast<std::uint32_t>(a_value);
		}

		[[nodiscard]] std::uint64_t MakeHealthDamageSignature(
			const RE::Actor* a_target,
			const RE::Actor* a_attacker,
			const RE::HitData* a_hitData,
			RE::FormID a_sourceFormID,
			float a_damage) noexcept
		{
			std::uint64_t hash = 14695981039346656037ull;
			auto mix = [&](std::uint64_t v) {
				hash ^= v;
				hash *= 1099511628211ull;
			};

			mix(static_cast<std::uint64_t>(a_attacker ? a_attacker->GetFormID() : 0u));
			mix(static_cast<std::uint64_t>(a_target ? a_target->GetFormID() : 0u));
			mix(static_cast<std::uint64_t>(a_sourceFormID));
			mix(static_cast<std::uint64_t>(FloatBits(a_damage)));

			if (a_hitData) {
				mix(static_cast<std::uint64_t>(a_hitData->flags.underlying()));
				mix(static_cast<std::uint64_t>(FloatBits(a_hitData->hitPosition.x)));
				mix(static_cast<std::uint64_t>(FloatBits(a_hitData->hitPosition.y)));
				mix(static_cast<std::uint64_t>(FloatBits(a_hitData->hitPosition.z)));
				mix(static_cast<std::uint64_t>(FloatBits(a_hitData->hitDirection.x)));
				mix(static_cast<std::uint64_t>(FloatBits(a_hitData->hitDirection.y)));
				mix(static_cast<std::uint64_t>(FloatBits(a_hitData->hitDirection.z)));
			}

			return hash;
		}

		[[nodiscard]] std::uint64_t ToElapsedMilliseconds(std::chrono::steady_clock::duration a_duration) noexcept
		{
			const auto count = std::chrono::duration_cast<std::chrono::milliseconds>(a_duration).count();
			return count > 0 ? static_cast<std::uint64_t>(count) : 0u;
		}
	}

	bool EventBridge::RouteHealthDamageAsHit(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData,
		RE::FormID a_sourceFormID,
		float a_damage,
		std::chrono::steady_clock::time_point a_now)
	{
		const bool hitDataMatches = HitDataUtil::HitDataMatchesActors(a_hitData, a_target, a_attacker);
		const bool hasHitLikeSource = HitDataUtil::HasHitLikeSource(a_hitData, a_attacker);

		bool routeAsHit = (a_hitData != nullptr) && hitDataMatches && hasHitLikeSource;
		if (!routeAsHit) {
			return false;
		}

		const float expectedDealt = std::max(
			0.0f,
			a_hitData->totalDamage - a_hitData->resistedPhysicalDamage - a_hitData->resistedTypedDamage);
		const float absDamage = std::abs(a_damage);
		if (ShouldSuppressHealthDamageStaleLeak(expectedDealt, absDamage)) {
			routeAsHit = false;
		}

		const auto sig = MakeHealthDamageSignature(a_target, a_attacker, a_hitData, a_sourceFormID, a_damage);
		const bool hasRecentSignature = _combatState.lastHealthDamageSignatureAt.time_since_epoch().count() != 0;
		const auto elapsedMs = hasRecentSignature ? ToElapsedMilliseconds(a_now - _combatState.lastHealthDamageSignatureAt) : 0u;
		if (ShouldSuppressDuplicateHealthDamageSignature(
				sig,
				_combatState.lastHealthDamageSignature,
				elapsedMs,
				0u,
				static_cast<std::uint64_t>(kHealthDamageSignatureStaleWindow.count()))) {
			routeAsHit = false;
		}

		if (routeAsHit) {
			_combatState.lastHealthDamageSignature = sig;
			_combatState.lastHealthDamageSignatureAt = a_now;
		} else if (_loot.debugLog) {
			static std::uint32_t suppressed = 0;
			suppressed += 1;
			if (suppressed <= 3) {
				SKSE::log::debug(
					"CalamityAffixes: suppressed HandleHealthDamage routing (damage={}, expectedDealt={}, target={}, attacker={}, source=0x{:X}).",
					a_damage,
					expectedDealt,
					a_target->GetName(),
					a_attacker ? a_attacker->GetName() : "<none>",
					a_sourceFormID);
			}
		}

		return routeAsHit;
	}

	void EventBridge::ProcessOutgoingHealthDamageHit(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData,
		RE::FormID a_sourceFormID,
		std::chrono::steady_clock::time_point a_now)
	{
		const auto context = BuildCombatTriggerContext(a_target, a_attacker);
		if (!context.attackerIsPlayerOwned || !context.playerOwner) {
			return;
		}
		const bool allowNeutralOutgoing =
			ShouldResolveNonHostileOutgoingFirstHitAllowance(
				context.hasPlayerOwner,
				context.targetIsPlayer,
				AllowsNonHostilePlayerOwnedOutgoingProcs()) &&
			ResolveNonHostileOutgoingFirstHitAllowance(
				context.playerOwner,
				a_target,
				context.hostileEitherDirection,
				a_now);
		if (!(context.hostileEitherDirection || allowNeutralOutgoing)) {
			return;
		}

		const RE::FormID targetFormID = a_target->GetFormID();
		if (const auto it = _combatState.outgoingHitPerTargetLastAt.find(targetFormID);
			it != _combatState.outgoingHitPerTargetLastAt.end()) {
			const auto elapsedMs = ToElapsedMilliseconds(a_now - it->second);
			if (ShouldSuppressPerTargetRepeatWindow(
					elapsedMs,
					static_cast<std::uint64_t>(kOutgoingPerTargetWindow.count()))) {
				return;
			}
		}

		const LastHitKey key{
			.outgoing = true,
			.aggressor = a_attacker->GetFormID(),
			.target = targetFormID,
			.source = a_sourceFormID,
		};
		if (ShouldSuppressDuplicateHit(key, a_now)) {
			return;
		}

		_combatState.outgoingHitPerTargetLastAt[targetFormID] = a_now;
		if (_combatState.outgoingHitPerTargetLastAt.size() > 256) {
			for (auto it = _combatState.outgoingHitPerTargetLastAt.begin();
			     it != _combatState.outgoingHitPerTargetLastAt.end();) {
				if ((a_now - it->second) > std::chrono::seconds(5)) {
					it = _combatState.outgoingHitPerTargetLastAt.erase(it);
				} else {
					++it;
				}
			}
		}

		ProcessTrigger(Trigger::kHit, context.playerOwner, a_target, a_hitData);

		if (a_attacker->IsPlayerRef() && a_hitData && a_hitData->attackDataSpell && !_affixSpecialActions.archmageAffixIndices.empty()) {
			ProcessArchmageSpellHit(a_attacker, a_target, a_hitData->attackDataSpell, a_hitData);
		}
	}

	void EventBridge::ProcessIncomingHealthDamageHit(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData,
		RE::FormID a_sourceFormID,
		std::chrono::steady_clock::time_point a_now)
	{
		if (!_configLoaded || !a_target->IsPlayerRef() || !a_attacker) {
			return;
		}
		if (!IsHostileEitherDirection(a_target, a_attacker)) {
			return;
		}

		const LastHitKey key{
			.outgoing = false,
			.aggressor = a_attacker->GetFormID(),
			.target = a_target->GetFormID(),
			.source = a_sourceFormID,
		};

		if (!ShouldSuppressDuplicateHit(key, a_now)) {
			ProcessTrigger(Trigger::kIncomingHit, a_target, a_attacker, a_hitData);
		}
	}

	void EventBridge::ProcessImmediateCorpseExplosionFromLethalHit(
		RE::Actor* a_target,
		RE::Actor* a_attacker)
	{
		const auto context = BuildCombatTriggerContext(a_target, a_attacker);
		if (a_target->IsPlayerRef() ||
			!a_target->IsDead() ||
			!context.attackerIsPlayerOwned ||
			_affixSpecialActions.corpseExplosionAffixIndices.empty()) {
			return;
		}
		if (!context.playerOwner || !context.hostileEitherDirection) {
			return;
		}

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: immediate corpse explosion path (target={}, attacker={}).",
				a_target->GetName(),
				a_attacker->GetName());
		}
		ProcessCorpseExplosionKill(context.playerOwner, a_target);
	}

	void EventBridge::ProcessLowHealthTriggerFromHealthDamage(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData)
	{
		if (!_configLoaded || !_runtimeSettings.enabled || !a_target || _affixRegistry.lowHealthTriggerAffixIndices.empty()) {
			return;
		}
		if (!a_target->IsPlayerRef()) {
			return;
		}

		ProcessTrigger(Trigger::kLowHealth, a_target, a_attacker, a_hitData);
	}
}
