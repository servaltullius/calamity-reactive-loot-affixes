#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/PrismaTooltip.h"
#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <RE/M/Misc.h>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		constexpr auto kHealthDamageSignatureStaleWindow = std::chrono::milliseconds(300);

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
			// Fast, non-cryptographic signature to detect "stale HitData" reuse (e.g. DoT ticks).
			// We mix in hit position/direction so repeated real hits are unlikely to collide.
			std::uint64_t hash = 14695981039346656037ull;  // FNV-1a offset basis
			auto mix = [&](std::uint64_t v) {
				hash ^= v;
				hash *= 1099511628211ull;  // FNV prime
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

		[[nodiscard]] bool HitDataMatchesActors(
			const RE::HitData* a_hitData,
			const RE::Actor* a_target,
			const RE::Actor* a_attacker) noexcept
		{
			if (!a_hitData) {
				return false;
			}

			const auto hitTarget = a_hitData->target.get().get();
			if (hitTarget && hitTarget != a_target) {
				return false;
			}

			const auto hitAggressor = a_hitData->aggressor.get().get();
			if (a_attacker) {
				if (hitAggressor && hitAggressor != a_attacker) {
					return false;
				}
			} else {
				if (hitAggressor) {
					return false;
				}
			}

			return true;
		}

		[[nodiscard]] bool HasHitLikeSource(const RE::HitData* a_hitData) noexcept
		{
			if (!a_hitData) {
				return false;
			}

			if (a_hitData->weapon || a_hitData->attackDataSpell) {
				return true;
			}

			if (a_hitData->flags.any(RE::HitData::Flag::kMeleeAttack) ||
				a_hitData->flags.any(RE::HitData::Flag::kExplosion)) {
				return true;
			}

			return false;
		}
	}

	void EventBridge::OnHealthDamage(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData,
		float a_damage)
	{
		if (!_configLoaded || !_runtimeEnabled || !a_target) {
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		MaybeResyncEquippedAffixes(now);

		_healthDamageHookSeen = true;
		_healthDamageHookLastAt = now;

		static bool loggedHookIsActive = false;
		if (!loggedHookIsActive) {
			loggedHookIsActive = true;
			SKSE::log::info("CalamityAffixes: HandleHealthDamage callback is active (hit procs routed here).");
		}

		if (_loot.debugLog) {
			static std::uint32_t windowCount = 0;
			static auto windowStart = now;
			windowCount += 1;

			if ((now - windowStart) >= std::chrono::seconds(2)) {
				SKSE::log::info(
					"CalamityAffixes: HandleHealthDamage seen (count={}, target={}, attacker={}, hasHitData={}, hasWeapon={}, hasSpell={}).",
					windowCount,
					a_target->GetName(),
					a_attacker ? a_attacker->GetName() : "<none>",
					(a_hitData != nullptr),
					(a_hitData && a_hitData->weapon != nullptr),
					(a_hitData && a_hitData->attackDataSpell != nullptr));

				windowCount = 0;
				windowStart = now;
			}
		}

		const auto sourceFormID = HitDataUtil::GetHitSourceFormID(a_hitData);
		if (!RouteHealthDamageAsHit(a_target, a_attacker, a_hitData, sourceFormID, a_damage, now)) {
			return;
		}

		ProcessOutgoingHealthDamageHit(a_target, a_attacker, a_hitData, sourceFormID, now);
		ProcessIncomingHealthDamageHit(a_target, a_attacker, a_hitData, sourceFormID, now);
		ProcessImmediateCorpseExplosionFromLethalHit(a_target, a_attacker);
	}

	bool EventBridge::RouteHealthDamageAsHit(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData,
		RE::FormID a_sourceFormID,
		float a_damage,
		std::chrono::steady_clock::time_point a_now)
	{
		const bool hitDataMatches = HitDataMatchesActors(a_hitData, a_target, a_attacker);
		const bool hasHitLikeSource = HasHitLikeSource(a_hitData);

		bool routeAsHit = (a_hitData != nullptr) && hitDataMatches && hasHitLikeSource;
		if (!routeAsHit) {
			return false;
		}

		// Guard against stale HitData reuse (common when non-hit damage flows through HandleHealthDamage).
		const float expectedDealt = std::max(
			0.0f,
			a_hitData->totalDamage - a_hitData->resistedPhysicalDamage - a_hitData->resistedTypedDamage);

		// If the incoming damage is far smaller than the last-hit snapshot, treat it as periodic and ignore it.
		if (expectedDealt >= 5.0f && a_damage > 0.0f && (a_damage < (expectedDealt * 0.25f))) {
			routeAsHit = false;
		}

		const auto sig = MakeHealthDamageSignature(a_target, a_attacker, a_hitData, a_sourceFormID, a_damage);
		if (sig == _lastHealthDamageSignature &&
			_lastHealthDamageSignatureAt.time_since_epoch().count() != 0 &&
			(a_now - _lastHealthDamageSignatureAt) > kHealthDamageSignatureStaleWindow) {
			routeAsHit = false;
		}

		if (routeAsHit) {
			_lastHealthDamageSignature = sig;
			_lastHealthDamageSignatureAt = a_now;
		} else if (_loot.debugLog) {
			static std::uint32_t suppressed = 0;
			suppressed += 1;
			if (suppressed <= 3) {
				spdlog::debug(
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
		if (!a_attacker || !IsPlayerOwned(a_attacker)) {
			return;
		}

		auto* owner = GetPlayerOwner(a_attacker);
		if (!owner) {
			return;
		}

		const LastHitKey key{
			.outgoing = true,
			.aggressor = a_attacker->GetFormID(),
			.target = a_target->GetFormID(),
			.source = a_sourceFormID,
		};

		if (ShouldSuppressDuplicateHit(key, a_now)) {
			return;
		}

		ProcessTrigger(Trigger::kHit, owner, a_target, a_hitData);

		if (a_attacker->IsPlayerRef() && a_hitData && a_hitData->attackDataSpell && !_archmageAffixIndices.empty()) {
			ProcessArchmageSpellHit(a_attacker, a_target, a_hitData->attackDataSpell);
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
		// For hit-like lethal damage, fire corpse explosion immediately instead of waiting for TESDeathEvent dispatch.
		// Duplicate explosions are still blocked by corpse-budget dedupe in ProcessCorpseExplosionKill.
		if (a_target->IsPlayerRef() ||
			!a_target->IsDead() ||
			!a_attacker ||
			!IsPlayerOwned(a_attacker) ||
			_corpseExplosionAffixIndices.empty()) {
			return;
		}

		auto* owner = GetPlayerOwner(a_attacker);
		if (!owner) {
			return;
		}

		const bool hostile = owner->IsHostileToActor(a_target) || a_target->IsHostileToActor(owner);
		if (!hostile) {
			return;
		}

		if (_loot.debugLog) {
			spdlog::debug(
				"CalamityAffixes: immediate corpse explosion path (target={}, attacker={}).",
				a_target->GetName(),
				a_attacker->GetName());
		}
		ProcessCorpseExplosionKill(owner, a_target);
	}
}

