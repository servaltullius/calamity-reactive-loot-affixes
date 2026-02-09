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

		[[nodiscard]] std::uint64_t ToNonNegativeMilliseconds(std::chrono::steady_clock::time_point a_value) noexcept
		{
			const auto count = std::chrono::duration_cast<std::chrono::milliseconds>(a_value.time_since_epoch()).count();
			return count > 0 ? static_cast<std::uint64_t>(count) : 0u;
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

		bool EventBridge::IsPerTargetCooldownBlocked(
			const AffixRuntime& a_affix,
			RE::Actor* a_target,
			std::chrono::steady_clock::time_point a_now,
			PerTargetCooldownKey* a_outKey) const
		{
			if (a_outKey) {
				*a_outKey = {};
			}

			if (a_affix.perTargetIcd.count() <= 0 || !a_target || a_affix.token == 0u) {
				return false;
			}

			const PerTargetCooldownKey key{
				.token = a_affix.token,
				.target = a_target->GetFormID()
			};
			if (a_outKey) {
				*a_outKey = key;
			}

			if (const auto it = _perTargetCooldowns.find(key); it != _perTargetCooldowns.end()) {
				const auto nowMs = ToNonNegativeMilliseconds(a_now);
				const auto nextAllowedMs = ToNonNegativeMilliseconds(it->second);
				if (IsPerTargetCooldownBlockedMs(nowMs, nextAllowedMs)) {
					return true;
				}
			}

			return false;
		}

		void EventBridge::CommitPerTargetCooldown(
			const PerTargetCooldownKey& a_key,
			std::chrono::milliseconds a_perTargetIcd,
			std::chrono::steady_clock::time_point a_now)
		{
			const auto nowMs = ToNonNegativeMilliseconds(a_now);
			if (!ComputePerTargetCooldownNextAllowedMs(nowMs, a_perTargetIcd.count(), a_key.token, a_key.target).has_value()) {
				return;
			}

			_perTargetCooldowns[a_key] = a_now + a_perTargetIcd;

			if (_perTargetCooldowns.size() > kPerTargetCooldownMaxEntries) {
				if (_perTargetCooldownsLastPruneAt.time_since_epoch().count() == 0 ||
					(a_now - _perTargetCooldownsLastPruneAt) > kPerTargetCooldownPruneInterval) {
					_perTargetCooldownsLastPruneAt = a_now;

					for (auto it = _perTargetCooldowns.begin(); it != _perTargetCooldowns.end();) {
						if (a_now >= it->second) {
							it = _perTargetCooldowns.erase(it);
						} else {
							++it;
						}
					}
				}
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

	void EventBridge::ProcessTrigger(Trigger a_trigger, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		if (!_configLoaded || !_runtimeEnabled || !a_owner) {
			return;
		}

		if (_procDepth > 0) {
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		std::uniform_real_distribution<float> dist(0.0f, 100.0f);

		const std::vector<std::size_t>* indices = nullptr;
		switch (a_trigger) {
		case Trigger::kHit:
			indices = &_hitTriggerAffixIndices;
			break;
		case Trigger::kIncomingHit:
			indices = &_incomingHitTriggerAffixIndices;
			break;
		case Trigger::kDotApply:
			indices = &_dotApplyTriggerAffixIndices;
			break;
		case Trigger::kKill:
			indices = &_killTriggerAffixIndices;
			break;
		}

		if (!indices || indices->empty()) {
			return;
		}

		for (const auto i : *indices) {
			if (i >= _affixes.size() || i >= _activeCounts.size() || _activeCounts[i] == 0) {
				continue;
			}

			auto& affix = _affixes[i];
			if (now < affix.nextAllowed) {
				continue;
			}

				PerTargetCooldownKey perTargetKey{};
				const bool usesPerTargetIcd = (affix.perTargetIcd.count() > 0 && a_target && affix.token != 0u);
				if (IsPerTargetCooldownBlocked(affix, a_target, now, &perTargetKey)) {
					continue;
				}

			const float chance = std::clamp(affix.procChancePct * _runtimeProcChanceMult, 0.0f, 100.0f);
			if (chance <= 0.0f) {
				continue;
			}

			if (chance < 100.0f) {
				const float roll = dist(_rng);
				if (roll >= chance) {
					continue;
				}
			}

			if (affix.icd.count() > 0) {
				affix.nextAllowed = now + affix.icd;
			}

				if (usesPerTargetIcd) {
					CommitPerTargetCooldown(perTargetKey, affix.perTargetIcd, now);
				}

			if (_loot.debugLog) {
				spdlog::debug(
					"CalamityAffixes: proc (affixId={}, trigger={}, chancePct={}, target={}, hasHitData={})",
					affix.id,
					static_cast<std::uint32_t>(a_trigger),
					chance,
					a_target ? a_target->GetName() : "<none>",
					(a_hitData != nullptr));

				const std::string note = std::string("CAFF proc: ") + affix.id;
				RE::DebugNotification(note.c_str());
			}

				AdvanceRuntimeStateForAffixProc(affix);
				ExecuteActionWithProcDepthGuard(affix, a_owner, a_target, a_hitData);
			}
		}

	bool EventBridge::ShouldSuppressDuplicateHit(const LastHitKey& a_key, std::chrono::steady_clock::time_point a_now) noexcept
	{
		const DuplicateHitKey current{
			.outgoing = a_key.outgoing,
			.aggressor = a_key.aggressor,
			.target = a_key.target,
			.source = a_key.source
		};
		const DuplicateHitKey last{
			.outgoing = _lastHit.outgoing,
			.aggressor = _lastHit.aggressor,
			.target = _lastHit.target,
			.source = _lastHit.source
		};
		if (ShouldSuppressDuplicateHitWindow(
				current,
				last,
				ToNonNegativeMilliseconds(a_now),
				ToNonNegativeMilliseconds(_lastHitAt),
				static_cast<std::uint64_t>(kDuplicateHitWindow.count()))) {
			return true;
		}

		_lastHitAt = a_now;
		_lastHit = a_key;
		return false;
	}
}
