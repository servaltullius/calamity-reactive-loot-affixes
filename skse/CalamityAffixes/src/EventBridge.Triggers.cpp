#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/PrismaTooltip.h"

#include <algorithm>
#include <bit>
#include <chrono>
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
			constexpr std::size_t kDotCooldownMaxEntries = 4096;
			constexpr auto kDotCooldownTtl = std::chrono::seconds(30);
			constexpr auto kDotCooldownPruneInterval = std::chrono::seconds(10);

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
				if (a_now < it->second) {
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
			if (a_perTargetIcd.count() <= 0 || a_key.token == 0u || a_key.target == 0u) {
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

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESHitEvent* a_event,
		RE::BSTEventSource<RE::TESHitEvent>*)
	{
		if (!a_event || !a_event->cause || !a_event->target) {
			return RE::BSEventNotifyControl::kContinue;
		}
		if (!_runtimeEnabled) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* aggressor = a_event->cause->As<RE::Actor>();
		auto* target = a_event->target->As<RE::Actor>();
		if (!aggressor || !target) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto now = std::chrono::steady_clock::now();

		if (_loot.debugLog) {
			static std::uint32_t windowCount = 0;
			static auto windowStart = now;
			windowCount += 1;

			if ((now - windowStart) >= std::chrono::seconds(2)) {
				const auto* hitData = HitDataUtil::GetLastHitData(target);
				const auto sourceFormID = hitData && hitData->weapon ?
					hitData->weapon->GetFormID() :
					(hitData && hitData->attackDataSpell ? hitData->attackDataSpell->GetFormID() : 0u);

				SKSE::log::info(
					"CalamityAffixes: TESHitEvent seen (count={}, cause={}, target={}, source=0x{:X}, hasHitData={}, hasWeapon={}, hasSpell={}).",
					windowCount,
					aggressor->GetName(),
					target->GetName(),
					sourceFormID,
					(hitData != nullptr),
					(hitData && hitData->weapon != nullptr),
					(hitData && hitData->attackDataSpell != nullptr));

				windowCount = 0;
				windowStart = now;
			}
		}

		// KID can patch keywords after kDataLoaded; re-sync equipped counts occasionally so procs become active.
		MaybeResyncEquippedAffixes(now);

		// NOTE: Some modlists can override vfunc hooks. If HandleHealthDamage is not routed through our hook
		// for this actor, fall back to TESHitEvent-driven procs so affixes still work.
		const bool hookRouted = CalamityAffixes::Hooks::IsHandleHealthDamageHooked(target);

		if (!hookRouted) {
			if (_loot.debugLog) {
				static bool loggedOnce = false;
				if (!loggedOnce) {
					loggedOnce = true;
					SKSE::log::debug("CalamityAffixes: HandleHealthDamage hook not routed; enabling TESHitEvent fallback.");
				}
			}

			// Outgoing (player-owned).
			if (_configLoaded && IsPlayerOwned(aggressor)) {
				const LastHitKey key{
					.outgoing = true,
					.aggressor = aggressor->GetFormID(),
					.target = target->GetFormID(),
					.source = a_event->source
				};

				if (!ShouldSuppressDuplicateHit(key, now)) {
					const auto* hitData = HitDataUtil::GetLastHitData(target);
					ProcessTrigger(Trigger::kHit, GetPlayerOwner(aggressor), target, hitData);

					if (aggressor->IsPlayerRef() && !_archmageAffixIndices.empty()) {
						auto* source = RE::TESForm::LookupByID<RE::TESForm>(a_event->source);
						auto* spell = source ? source->As<RE::SpellItem>() : nullptr;
						if (spell) {
							ProcessArchmageSpellHit(aggressor, target, spell);
						}
					}
				}
			}

			// Incoming (player hit).
			if (_configLoaded && target->IsPlayerRef()) {
				const LastHitKey key{
					.outgoing = false,
					.aggressor = aggressor->GetFormID(),
					.target = target->GetFormID(),
					.source = a_event->source
				};

				if (!ShouldSuppressDuplicateHit(key, now)) {
					const auto* hitData = HitDataUtil::GetLastHitData(target);
					ProcessTrigger(Trigger::kIncomingHit, target, aggressor, hitData);
				}
			}
		}

		if (IsPlayerOwned(aggressor)) {
			SendModEvent("CalamityAffixes_Hit", target);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESDeathEvent* a_event,
		RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		if (!_configLoaded || !_runtimeEnabled || !a_event || !a_event->dead) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto now = std::chrono::steady_clock::now();

		// Keep event references alive for the entire handler. Using raw pointers directly from
		// TESDeathEvent can race with reference release under heavy combat churn.
		const auto dyingRefHolder = a_event->actorDying;
		const auto killerRefHolder = a_event->actorKiller;

		auto* dyingRef = dyingRefHolder.get();
		auto* killerRef = killerRefHolder.get();
		auto* dying = dyingRef ? dyingRef->As<RE::Actor>() : nullptr;
		if (!dying) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MaybeResyncEquippedAffixes(now);

		// Independent path: player-owned summoned actor death can trigger dedicated summon corpse explosion.
		if (!_summonCorpseExplosionAffixIndices.empty() &&
			dying->IsSummoned() &&
			IsPlayerOwned(dying)) {
			if (auto* summonOwner = GetPlayerOwner(dying); summonOwner) {
				ProcessSummonCorpseExplosionDeath(summonOwner, dying);
			}
		}

		if (!killerRef) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// NOTE:
		// TESDeathEvent::actorKiller is a TESObjectREFR, which can be a Projectile/Hazard/etc (not always an Actor).
		// Attribute kills through ActorCause / shooter so Kill-triggered affixes work for ranged/magic too.
		RE::Actor* killer = killerRef->As<RE::Actor>();
		if (!killer) {
			if (auto* cause = killerRef->GetActorCause()) {
				auto actorAny = cause->actor.get();
				killer = actorAny.get();
			}
		}
		if (!killer) {
			if (auto* proj = killerRef->As<RE::Projectile>()) {
				const auto& rt = proj->GetProjectileRuntimeData();
				if (auto shooterAny = rt.shooter.get(); shooterAny) {
					killer = shooterAny.get()->As<RE::Actor>();
				}
				if (!killer && rt.actorCause) {
					auto actorAny = rt.actorCause->actor.get();
					killer = actorAny.get();
				}
			}
		}

		if (!killer) {
			static auto lastWarnAt = std::chrono::steady_clock::time_point{};
			if (lastWarnAt.time_since_epoch().count() == 0 || (now - lastWarnAt) > std::chrono::seconds(2)) {
				lastWarnAt = now;
				if (_loot.debugLog) {
					spdlog::info(
						"CalamityAffixes: TESDeathEvent killer attribution failed (dying={}, killerRef=0x{:X}); skipping Kill triggers.",
						dying->GetName(),
						killerRef->GetFormID());
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!IsPlayerOwned(killer)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* owner = GetPlayerOwner(killer);
		if (!owner) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Only react to hostile kills (avoid friendly-fire weirdness).
		const bool hostile = owner->IsHostileToActor(dying) || dying->IsHostileToActor(owner);
		if (!hostile) {
			static auto lastInfoAt = std::chrono::steady_clock::time_point{};
			if (lastInfoAt.time_since_epoch().count() == 0 || (now - lastInfoAt) > std::chrono::seconds(2)) {
				lastInfoAt = now;
				if (_loot.debugLog) {
					spdlog::info(
						"CalamityAffixes: Kill ignored (non-hostile) (dying={}, killer={}).",
						dying->GetName(),
						killer->GetName());
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		ProcessTrigger(Trigger::kKill, owner, dying, nullptr);

		if (!_corpseExplosionAffixIndices.empty()) {
			ProcessCorpseExplosionKill(owner, dying);
		}

		SendModEvent("CalamityAffixes_Kill", dying);

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESEquipEvent* a_event,
		RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		if (!_configLoaded || !_runtimeEnabled || _affixes.empty()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!a_event || !a_event->actor) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* actor = a_event->actor.get() ? a_event->actor.get()->As<RE::Actor>() : nullptr;
		if (!actor || !actor->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* item = RE::TESForm::LookupByID<RE::TESForm>(a_event->baseObject);
		if (!item) {
			return RE::BSEventNotifyControl::kContinue;
		}

		RebuildActiveCounts();
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESMagicEffectApplyEvent* a_event,
		RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*)
	{
		if (!a_event || !a_event->caster || !a_event->target || !a_event->magicEffect) {
			return RE::BSEventNotifyControl::kContinue;
		}
		if (!_runtimeEnabled) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* caster = a_event->caster->As<RE::Actor>();
		auto* target = a_event->target->As<RE::Actor>();
		if (!caster || !target) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!IsPlayerOwned(caster)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Treat DoT as “apply/refresh”, not per-tick. Filter by keyword.
		auto* mgef = RE::TESForm::LookupByID<RE::EffectSetting>(a_event->magicEffect);
		if (!mgef) {
			return RE::BSEventNotifyControl::kContinue;
		}

		static RE::BGSKeyword* dotKeyword = nullptr;
		if (!dotKeyword) {
			dotKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(kDotKeywordEditorID);
		}

		if (!dotKeyword || !mgef->HasKeyword(dotKeyword)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		_dotObservedMagicEffects.insert(mgef->GetFormID());
		const auto observedDotEffectCount = _dotObservedMagicEffects.size();
		if (_loot.dotTagSafetyUniqueEffectThreshold > 0u &&
			observedDotEffectCount > _loot.dotTagSafetyUniqueEffectThreshold) {
			if (!_dotTagSafetyWarned) {
				_dotTagSafetyWarned = true;
				SKSE::log::warn(
					"CalamityAffixes: DotApply safety warning (unique tagged magic effects={}, threshold={}, autoDisable={}).",
					observedDotEffectCount,
					_loot.dotTagSafetyUniqueEffectThreshold,
					_loot.dotTagSafetyAutoDisable);
				RE::DebugNotification("Calamity: DotApply safety warning (tag spread broad)");
			}

			if (_loot.dotTagSafetyAutoDisable) {
				if (!_dotTagSafetySuppressed) {
					_dotTagSafetySuppressed = true;
					SKSE::log::error(
						"CalamityAffixes: DotApply safety auto-disabled (unique tagged magic effects={}, threshold={}).",
						observedDotEffectCount,
						_loot.dotTagSafetyUniqueEffectThreshold);
					RE::DebugNotification("Calamity: DotApply safety auto-disabled");
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		}

		if (_dotTagSafetySuppressed) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Safety: avoid turning broad KID tagging into "any spell cast" proc storms.
		// Treat DotApply as "harmful duration effect apply/refresh", not instant hits or buffs.
		if (!mgef->IsHostile() || mgef->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kNoDuration)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto now = std::chrono::steady_clock::now();
		const std::uint64_t key =
			(static_cast<std::uint64_t>(target->GetFormID()) << 32) |
			static_cast<std::uint64_t>(mgef->GetFormID());

		if (const auto it = _dotCooldowns.find(key); it != _dotCooldowns.end()) {
			if (now - it->second < kDotApplyICD) {
				return RE::BSEventNotifyControl::kContinue;
			}
		}

		_dotCooldowns[key] = now;

		if (_dotCooldowns.size() > kDotCooldownMaxEntries) {
			if (_dotCooldownsLastPruneAt.time_since_epoch().count() == 0 ||
				(now - _dotCooldownsLastPruneAt) > kDotCooldownPruneInterval) {
				_dotCooldownsLastPruneAt = now;

				for (auto it = _dotCooldowns.begin(); it != _dotCooldowns.end();) {
					if ((now - it->second) > kDotCooldownTtl) {
						it = _dotCooldowns.erase(it);
					} else {
						++it;
					}
				}
			}
		}

		if (_configLoaded) {
			ProcessTrigger(Trigger::kDotApply, GetPlayerOwner(caster), target, nullptr);
		}

		// Forward a lightweight “player-owned DoT apply/refresh” signal to Papyrus.
		SendModEvent("CalamityAffixes_DotApply", target);

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const SKSE::ModCallbackEvent* a_event,
		RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const char* eventNameRaw = a_event->eventName.c_str();
		if (!eventNameRaw || !*eventNameRaw) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const std::string_view eventName(eventNameRaw);
		if (eventName == kUiTogglePanelEvent) {
			PrismaTooltip::ToggleControlPanel();
			RE::DebugNotification("Calamity: Prisma panel toggled");
			return RE::BSEventNotifyControl::kContinue;
		}

		if (eventName == kUiSetPanelEvent) {
			const bool open = a_event->numArg > 0.5f;
			PrismaTooltip::SetControlPanelOpen(open);
			RE::DebugNotification(open ? "Calamity: Prisma panel ON" : "Calamity: Prisma panel OFF");
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!_configLoaded) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const char* filterRaw = a_event->strArg.c_str();
		const std::string_view affixFilter = (filterRaw && *filterRaw) ? std::string_view(filterRaw) : std::string_view{};

			if (eventName == kMcmSetEnabledEvent) {
				_runtimeEnabled = (a_event->numArg > 0.5f);
				RE::DebugNotification(_runtimeEnabled ? "Calamity: enabled" : "Calamity: disabled");
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetDebugNotificationsEvent) {
				_loot.debugLog = (a_event->numArg > 0.5f);
				spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
				spdlog::flush_on(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
				RE::DebugNotification(_loot.debugLog ? "Calamity: debug notifications ON" : "Calamity: debug notifications OFF");
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetValidationIntervalEvent) {
				const float seconds = std::clamp(a_event->numArg, 0.0f, 30.0f);
				_equipResync.intervalMs = (seconds <= 0.0f) ? 0u : static_cast<std::uint64_t>(seconds * 1000.0f);
				_equipResync.nextAtMs = 0;
				if (seconds <= 0.0f) {
					RE::DebugNotification("Calamity: periodic validation OFF");
				} else {
					std::string note = "Calamity: validation interval ";
					note += std::to_string(static_cast<int>(seconds));
					note += "s";
					RE::DebugNotification(note.c_str());
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetProcChanceMultEvent) {
				_runtimeProcChanceMult = std::clamp(a_event->numArg, 0.0f, 3.0f);
				std::string note = "Calamity: proc x";
				note += std::to_string(_runtimeProcChanceMult);
				RE::DebugNotification(note.c_str());
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetDotSafetyAutoDisableEvent) {
				_loot.dotTagSafetyAutoDisable = (a_event->numArg > 0.5f);
				if (!_loot.dotTagSafetyAutoDisable) {
					_dotTagSafetySuppressed = false;
					RE::DebugNotification("Calamity: DotApply auto-disable OFF (warn only)");
				} else {
					if (_loot.dotTagSafetyUniqueEffectThreshold > 0u &&
						_dotObservedMagicEffects.size() > _loot.dotTagSafetyUniqueEffectThreshold) {
						_dotTagSafetySuppressed = true;
						RE::DebugNotification("Calamity: DotApply auto-disabled (safety)");
					} else {
						RE::DebugNotification("Calamity: DotApply auto-disable ON");
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSpawnTestItemEvent) {
				auto* player = RE::PlayerCharacter::GetSingleton();
				auto* ironSword = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00012EB7);
				if (player && ironSword) {
					player->AddObjectToContainer(ironSword, nullptr, 1, nullptr);
					RE::DebugNotification("Calamity: spawned test item (Iron Sword)");
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!_runtimeEnabled) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kManualModeCycleNextEvent) {
				CycleManualModeForEquippedAffixes(+1, affixFilter);
			} else if (eventName == kManualModeCyclePrevEvent) {
				CycleManualModeForEquippedAffixes(-1, affixFilter);
			} else if (eventName == kRunewordBaseNextEvent) {
				CycleRunewordBase(+1);
			} else if (eventName == kRunewordBasePrevEvent) {
				CycleRunewordBase(-1);
			} else if (eventName == kRunewordRecipeNextEvent) {
				CycleRunewordRecipe(+1);
			} else if (eventName == kRunewordRecipePrevEvent) {
				CycleRunewordRecipe(-1);
			} else if (eventName == kRunewordInsertRuneEvent) {
				InsertRunewordRuneIntoSelectedBase();
			} else if (eventName == kRunewordStatusEvent) {
				LogRunewordStatus();
			} else if (eventName == kRunewordGrantNextRuneEvent) {
				const auto amount = (a_event->numArg > 0.0f) ? static_cast<std::uint32_t>(a_event->numArg) : 1u;
				GrantNextRequiredRuneFragment(amount);
			} else if (eventName == kRunewordGrantRecipeSetEvent) {
				const auto amount = (a_event->numArg > 0.0f) ? static_cast<std::uint32_t>(a_event->numArg) : 1u;
				GrantCurrentRecipeRuneSet(amount);
			}

		return RE::BSEventNotifyControl::kContinue;
	}

	bool EventBridge::IsPlayerOwned(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		if (a_actor->IsPlayerRef()) {
			return true;
		}

		// Summon/proxy attribution:
		// If the actor is commanded by the player, treat it as "player-owned".
		auto commandingAny = a_actor->GetCommandingActor();
		auto* commanding = commandingAny.get();

		return commanding && commanding->IsPlayerRef();
	}

	RE::Actor* EventBridge::GetPlayerOwner(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return nullptr;
		}

		if (a_actor->IsPlayerRef()) {
			return a_actor;
		}

		auto commandingAny = a_actor->GetCommandingActor();
		auto* commanding = commandingAny.get();

		return (commanding && commanding->IsPlayerRef()) ? commanding : nullptr;
	}

	void EventBridge::SendModEvent(std::string_view a_eventName, RE::TESForm* a_sender)
	{
		auto* source = SKSE::GetModCallbackEventSource();
		if (!source) {
			return;
		}

		SKSE::ModCallbackEvent event{
			RE::BSFixedString(a_eventName.data()),
			RE::BSFixedString(""),
			0.0f,
			a_sender
		};

		source->SendEvent(&event);
	}

	void EventBridge::MaybeResyncEquippedAffixes(std::chrono::steady_clock::time_point a_now)
	{
		if (!_configLoaded || !_runtimeEnabled) {
			return;
		}

		if (_equipResync.intervalMs == 0u) {
			return;
		}

		const auto nowMs =
			static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(a_now.time_since_epoch()).count());

		if (!_equipResync.ShouldRun(nowMs)) {
			return;
		}

		RebuildActiveCounts();
	}

	void EventBridge::RebuildActiveCounts()
	{
		if (!_configLoaded) {
			return;
		}

		_activeCounts.assign(_affixes.size(), 0);

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		auto* changes = player->GetInventoryChanges();
		if (!changes || !changes->entryList) {
			return;
		}

		for (auto* entry : *changes->entryList) {
			if (!entry || !entry->extraLists) {
				continue;
			}

			for (auto* xList : *entry->extraLists) {
				if (!xList) {
					continue;
				}

				auto* uid = xList->GetByType<RE::ExtraUniqueID>();
				if (!uid) {
					continue;
				}

				const auto it = _instanceAffixes.find(MakeInstanceKey(uid->baseID, uid->uniqueID));
				if (it == _instanceAffixes.end()) {
					continue;
				}
				EnsureInstanceRuntimeState(MakeInstanceKey(uid->baseID, uid->uniqueID));

				const auto idxIt = _affixIndexByToken.find(it->second);
				if (idxIt == _affixIndexByToken.end()) {
					continue;
				}

				if (idxIt->second < _affixes.size()) {
					EnsureAffixDisplayName(entry, xList, _affixes[idxIt->second]);
				}

				const bool worn = xList->HasType<RE::ExtraWorn>() || xList->HasType<RE::ExtraWornLeft>();
				if (!worn) {
					continue;
				}

				if (idxIt->second < _activeCounts.size()) {
					_activeCounts[idxIt->second] += 1;
				}
			}
		}

		if (_loot.debugLog) {
			std::uint32_t shown = 0;
			for (std::size_t i = 0; i < _affixes.size() && i < _activeCounts.size(); i++) {
				if (_activeCounts[i] == 0) {
					continue;
				}
				spdlog::debug("CalamityAffixes: active affix (id={}, count={})", _affixes[i].id, _activeCounts[i]);
				shown += 1;
				if (shown >= 50) {
					break;
				}
			}
			if (shown == 0) {
				spdlog::debug("CalamityAffixes: active affix list is empty (no equipped affix instances detected).");
			}
		}
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
		if ((a_now - _lastHitAt) < kDuplicateHitWindow) {
			if (a_key.outgoing == _lastHit.outgoing &&
				a_key.aggressor == _lastHit.aggressor &&
				a_key.target == _lastHit.target &&
				a_key.source == _lastHit.source) {
				return true;
			}
		}

		_lastHitAt = a_now;
		_lastHit = a_key;
		return false;
	}
}
