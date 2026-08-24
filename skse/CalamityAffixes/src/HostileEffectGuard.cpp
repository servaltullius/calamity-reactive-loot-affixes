#include "CalamityAffixes/HostileEffectGuard.h"

#include "CalamityAffixes/HostileEffectDamagePolicy.h"
#include "CalamityAffixes/PlayerOwnership.h"
#include "CalamityAffixes/PointerSafety.h"

#include <SKSE/SKSE.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
		constexpr std::size_t kMaxRegisteredSummons = 64u;
		constexpr std::size_t kMaxPendingSummonCasts = 32u;
		constexpr std::size_t kMaxExplosionDamageWindows = 256u;
		constexpr std::uint8_t kMaxSummonRefreshAttempts = 40u;
		constexpr auto kSummonRefreshInterval = std::chrono::milliseconds(25);
		constexpr auto kExplosionDamageWindow = std::chrono::milliseconds(150);

		struct RegisteredSummon
		{
			RE::ActorHandle handle{};
			RE::FormID formID{ 0u };
		};

		struct SummonEffectSnapshot
		{
			RE::ActorHandle target{};
			RE::ActorHandle caster{};
			RE::FormID spellFormID{ 0u };
			std::uintptr_t effectAddress{ 0u };
			std::uint16_t uniqueID{ 0u };
		};

		struct PendingSummonCast
		{
			RE::ActorHandle caster{};
			RE::ObjectRefHandle target{};
			RE::FormID spellFormID{ 0u };
			std::vector<SummonEffectSnapshot> effectsBefore;
			std::uint8_t attemptsRemaining{ kMaxSummonRefreshAttempts };
		};

		struct ExplosionDamageWindow
		{
			RE::ObjectRefHandle source{};
			RE::ActorHandle target{};
			std::chrono::steady_clock::time_point acceptUntil{};
		};

		std::vector<RegisteredSummon> g_registeredSummons;
		std::vector<PendingSummonCast> g_pendingSummonCasts;
		std::vector<ExplosionDamageWindow> g_explosionDamageWindows;
		std::mutex g_summonStateMutex;
		std::atomic_uint64_t g_summonRuntimeGeneration{ 1u };
		bool g_summonRefreshScheduled = false;
		std::once_flag g_summonRefreshWorkerOnce;
		std::jthread g_summonRefreshWorker;
		thread_local std::uint32_t g_hostileOnlyCastDepth = 0u;
		thread_local RE::ActorHandle g_hostileOnlyCastPlayerOwner{};

		class HostileOnlyCastScope
		{
		public:
			explicit HostileOnlyCastScope(RE::Actor* a_playerOwner) noexcept :
				previousPlayerOwner(g_hostileOnlyCastPlayerOwner)
			{
				++g_hostileOnlyCastDepth;
				a_playerOwner = SanitizeObjectPointer(a_playerOwner);
				if (a_playerOwner) {
					g_hostileOnlyCastPlayerOwner = a_playerOwner->GetHandle();
				}
			}

			~HostileOnlyCastScope()
			{
				if (g_hostileOnlyCastDepth > 0u) {
					--g_hostileOnlyCastDepth;
				}
				g_hostileOnlyCastPlayerOwner = previousPlayerOwner;
			}

			HostileOnlyCastScope(const HostileOnlyCastScope&) = delete;
			HostileOnlyCastScope& operator=(const HostileOnlyCastScope&) = delete;

		private:
			RE::ActorHandle previousPlayerOwner{};
		};

		[[nodiscard]] RE::Actor* ResolveScopedPlayerOwner() noexcept
		{
			if (g_hostileOnlyCastDepth == 0u || !g_hostileOnlyCastPlayerOwner) {
				return nullptr;
			}

			const auto ownerHolder = g_hostileOnlyCastPlayerOwner.get();
			auto* owner = SanitizeObjectPointer(ownerHolder.get());
			return owner && owner->IsPlayerRef() ? owner : nullptr;
		}

		[[nodiscard]] bool SameHandle(RE::ActorHandle a_lhs, RE::ActorHandle a_rhs) noexcept
		{
			return a_lhs == a_rhs;
		}

		[[nodiscard]] bool RegisterSummonActor(RE::ActorHandle a_handle) noexcept
		{
			if (!a_handle) {
				return false;
			}

			const auto actorHolder = a_handle.get();
			auto* actor = SanitizeObjectPointer(actorHolder.get());
			if (!actor) {
				return false;
			}

			try {
				const std::scoped_lock lock(g_summonStateMutex);
				const auto existing = std::find_if(
					g_registeredSummons.begin(),
					g_registeredSummons.end(),
					[&](const RegisteredSummon& a_entry) {
						return SameHandle(a_entry.handle, a_handle);
					});
				if (existing != g_registeredSummons.end()) {
					existing->formID = actor->GetFormID();
					return true;
				}

				if (g_registeredSummons.size() >= kMaxRegisteredSummons) {
					g_registeredSummons.erase(g_registeredSummons.begin());
				}
				g_registeredSummons.push_back({ a_handle, actor->GetFormID() });
				return true;
			} catch (...) {
				// Exact summon attribution is defensive and must never abort a cast.
				return false;
			}
		}

		[[nodiscard]] bool IsRegisteredSummonHandle(RE::ActorHandle a_handle) noexcept
		{
			if (!a_handle) {
				return false;
			}

			try {
				const auto actorHolder = a_handle.get();
				auto* actor = SanitizeObjectPointer(actorHolder.get());
				const std::scoped_lock lock(g_summonStateMutex);
				const auto existing = std::find_if(
					g_registeredSummons.begin(),
					g_registeredSummons.end(),
					[&](const RegisteredSummon& a_entry) {
						return SameHandle(a_entry.handle, a_handle);
					});
				if (existing == g_registeredSummons.end()) {
					return false;
				}

				// A native handle includes an age, but also compare the live reference's
				// FormID so a very old entry cannot match after an age wrap. A dead
				// summon may no longer resolve while its explosion is being processed;
				// in that case the exact original handle remains the best evidence.
				return !actor || actor->GetFormID() == existing->formID;
			} catch (...) {
				return false;
			}
		}

		[[nodiscard]] bool IsRegisteredSummon(RE::Actor* a_actor) noexcept
		{
			a_actor = SanitizeObjectPointer(a_actor);
			return a_actor && IsRegisteredSummonHandle(a_actor->GetHandle());
		}

		[[nodiscard]] bool IsSummonCreatureSpell(const RE::SpellItem* a_spell) noexcept
		{
			a_spell = SanitizeObjectPointer(a_spell);
			if (!a_spell) {
				return false;
			}

			for (auto* effect : a_spell->effects) {
				effect = SanitizeObjectPointer(effect);
				auto* baseEffect = effect ? SanitizeObjectPointer(effect->baseEffect) : nullptr;
				if (baseEffect && baseEffect->HasArchetype(
						RE::EffectArchetypes::ArchetypeID::kSummonCreature)) {
					return true;
				}
			}
			return false;
		}

		void AppendSummonEffectSnapshots(
			RE::Actor* a_effectTarget,
			RE::ActorHandle a_expectedCaster,
			RE::SpellItem* a_expectedSpell,
			std::vector<SummonEffectSnapshot>& a_out)
		{
			a_effectTarget = SanitizeObjectPointer(a_effectTarget);
			a_expectedSpell = SanitizeObjectPointer(a_expectedSpell);
			if (!a_effectTarget || !a_expectedCaster || !a_expectedSpell) {
				return;
			}

			auto* magicTarget = a_effectTarget->AsMagicTarget();
			auto* effects = magicTarget ? magicTarget->GetActiveEffectList() : nullptr;
			if (!effects) {
				return;
			}

			const auto targetHandle = a_effectTarget->GetHandle();
			for (auto* activeEffect : *effects) {
				activeEffect = SanitizeObjectPointer(activeEffect);
				auto* summonEffect = activeEffect ?
					skyrim_cast<RE::SummonCreatureEffect*>(activeEffect) :
					nullptr;
				if (!summonEffect || summonEffect->spell != a_expectedSpell ||
					!SameHandle(summonEffect->caster, a_expectedCaster)) {
					continue;
				}

				a_out.push_back({
					.target = targetHandle,
					.caster = a_expectedCaster,
					.spellFormID = a_expectedSpell->GetFormID(),
					.effectAddress = reinterpret_cast<std::uintptr_t>(summonEffect),
					.uniqueID = summonEffect->usUniqueID
				});
			}
		}

		[[nodiscard]] std::vector<SummonEffectSnapshot> CaptureSummonEffectSnapshots(
			RE::Actor* a_caster,
			RE::TESObjectREFR* a_target,
			RE::SpellItem* a_spell)
		{
			std::vector<SummonEffectSnapshot> snapshots;
			if (!a_caster || !a_target || !a_spell) {
				return snapshots;
			}

			const auto casterHandle = a_caster->GetHandle();
			AppendSummonEffectSnapshots(a_caster, casterHandle, a_spell, snapshots);
			auto* targetActor = SanitizeObjectPointer(a_target->As<RE::Actor>());
			if (targetActor && targetActor != a_caster) {
				AppendSummonEffectSnapshots(targetActor, casterHandle, a_spell, snapshots);
			}
			return snapshots;
		}

		[[nodiscard]] bool WasPresentBefore(
			const SummonEffectSnapshot& a_effect,
			const std::vector<SummonEffectSnapshot>& a_before) noexcept
		{
			return std::any_of(
				a_before.begin(),
				a_before.end(),
				[&](const SummonEffectSnapshot& a_existing) {
					return a_existing.effectAddress == a_effect.effectAddress &&
					       a_existing.uniqueID == a_effect.uniqueID &&
					       SameHandle(a_existing.target, a_effect.target);
				});
		}

		[[nodiscard]] bool TryResolveSummonEffect(const SummonEffectSnapshot& a_effect) noexcept
		{
			const auto targetHolder = a_effect.target.get();
			auto* target = SanitizeObjectPointer(targetHolder.get());
			if (!target) {
				return false;
			}

			auto* magicTarget = target->AsMagicTarget();
			auto* effects = magicTarget ? magicTarget->GetActiveEffectList() : nullptr;
			if (!effects) {
				return false;
			}

			for (auto* activeEffect : *effects) {
				activeEffect = SanitizeObjectPointer(activeEffect);
				auto* summonEffect = activeEffect ?
					skyrim_cast<RE::SummonCreatureEffect*>(activeEffect) :
					nullptr;
				if (!summonEffect ||
					reinterpret_cast<std::uintptr_t>(summonEffect) != a_effect.effectAddress ||
					summonEffect->usUniqueID != a_effect.uniqueID ||
					!SameHandle(summonEffect->caster, a_effect.caster) ||
					!summonEffect->spell || summonEffect->spell->GetFormID() != a_effect.spellFormID) {
					continue;
				}

				if (!summonEffect->commandedActor) {
					return false;
				}
				return RegisterSummonActor(summonEffect->commandedActor);
			}

			return false;
		}

		[[nodiscard]] bool TryResolveSummonCast(const PendingSummonCast& a_cast) noexcept
		{
			const auto casterHolder = a_cast.caster.get();
			auto* caster = SanitizeObjectPointer(casterHolder.get());
			if (!caster) {
				return false;
			}

			const auto targetHolder = a_cast.target.get();
			auto* target = SanitizeObjectPointer(targetHolder.get());
			if (!target) {
				target = caster;
			}

			auto* spell = SanitizeObjectPointer(RE::TESForm::LookupByID<RE::SpellItem>(a_cast.spellFormID));
			if (!spell) {
				return false;
			}

			try {
				const auto after = CaptureSummonEffectSnapshots(caster, target, spell);
				bool foundNewEffect = false;
				bool allNewEffectsResolved = true;
				for (const auto& summonEffect : after) {
					if (WasPresentBefore(summonEffect, a_cast.effectsBefore)) {
						continue;
					}
					foundNewEffect = true;
					if (!TryResolveSummonEffect(summonEffect)) {
						allNewEffectsResolved = false;
					}
				}
				return foundNewEffect && allNewEffectsResolved;
			} catch (...) {
				return false;
			}
		}

		void RefreshPendingSummonCasts(std::uint64_t a_generation) noexcept;

		void ScheduleSummonRefreshIfNeeded() noexcept
		{
			bool shouldSchedule = false;
			std::uint64_t generation = 0u;
			try {
				const std::scoped_lock lock(g_summonStateMutex);
				if (!g_pendingSummonCasts.empty() && !g_summonRefreshScheduled) {
					g_summonRefreshScheduled = true;
					generation = g_summonRuntimeGeneration.load(std::memory_order_relaxed);
					shouldSchedule = true;
				}
			} catch (...) {
				return;
			}

			if (!shouldSchedule) {
				return;
			}

			try {
				if (auto* tasks = SKSE::GetTaskInterface()) {
					tasks->AddTask([generation]() {
						RefreshPendingSummonCasts(generation);
					});
					return;
				}
			} catch (...) {
				// Fall through and release the scheduled flag below.
			}

			try {
				const std::scoped_lock lock(g_summonStateMutex);
				if (generation == g_summonRuntimeGeneration.load(std::memory_order_relaxed)) {
					g_summonRefreshScheduled = false;
				}
			} catch (...) {
				// Runtime provenance tracking is best-effort and noexcept.
			}
		}

		void StartSummonRefreshWorker() noexcept
		{
			try {
				std::call_once(g_summonRefreshWorkerOnce, []() {
					g_summonRefreshWorker = std::jthread([](std::stop_token a_stopToken) {
						while (!a_stopToken.stop_requested()) {
							// A task queued from inside BSTaskPool::ProcessTasks can run again
							// during the same queue drain.  A real-time worker interval keeps
							// retries from collapsing into one frame while every RE access
							// remains on the game thread.
							std::this_thread::sleep_for(kSummonRefreshInterval);
							if (a_stopToken.stop_requested()) {
								break;
							}
							ScheduleSummonRefreshIfNeeded();
						}
					});
				});
			} catch (...) {
				// The immediate post-cast scan still provides a safe best-effort path.
			}
		}

		void QueuePendingSummonCast(PendingSummonCast a_cast) noexcept
		{
			try {
				const std::scoped_lock lock(g_summonStateMutex);
				if (g_pendingSummonCasts.size() >= kMaxPendingSummonCasts) {
					g_pendingSummonCasts.erase(g_pendingSummonCasts.begin());
				}
				g_pendingSummonCasts.push_back(std::move(a_cast));
			} catch (...) {
				return;
			}
			StartSummonRefreshWorker();
		}

		void RefreshPendingSummonCasts(std::uint64_t a_generation) noexcept
		{
			if (a_generation != g_summonRuntimeGeneration.load(std::memory_order_relaxed)) {
				return;
			}

			std::vector<PendingSummonCast> pending;
			try {
				const std::scoped_lock lock(g_summonStateMutex);
				if (a_generation != g_summonRuntimeGeneration.load(std::memory_order_relaxed)) {
					return;
				}
				g_summonRefreshScheduled = false;
				pending.swap(g_pendingSummonCasts);
			} catch (...) {
				return;
			}

			std::vector<PendingSummonCast> retry;
			try {
				for (auto& candidate : pending) {
					if (TryResolveSummonCast(candidate)) {
						continue;
					}
					if (candidate.attemptsRemaining > 1u) {
						--candidate.attemptsRemaining;
						retry.push_back(std::move(candidate));
					}
				}

				if (!retry.empty()) {
					const std::scoped_lock lock(g_summonStateMutex);
					if (a_generation == g_summonRuntimeGeneration.load(std::memory_order_relaxed)) {
						for (auto& candidate : retry) {
							if (g_pendingSummonCasts.size() >= kMaxPendingSummonCasts) {
								g_pendingSummonCasts.erase(g_pendingSummonCasts.begin());
							}
							g_pendingSummonCasts.push_back(std::move(candidate));
						}
					}
				}
			} catch (...) {
				// A failed refresh must not affect combat processing.
			}

		}

		void ResolveOrQueueSummonCast(PendingSummonCast a_cast) noexcept
		{
			if (!TryResolveSummonCast(a_cast)) {
				QueuePendingSummonCast(std::move(a_cast));
			}
		}

		[[nodiscard]] bool IsFreshExplosionDamageFromRegisteredSummon(
			RE::Actor* a_target,
			RE::Actor* a_attacker,
			const RE::HitData* a_hitData) noexcept
		{
			a_target = SanitizeObjectPointer(a_target);
			a_attacker = SanitizeObjectPointer(a_attacker);
			a_hitData = SanitizeObjectPointer(a_hitData);
			if (!a_target || !a_hitData ||
				!a_hitData->flags.any(RE::HitData::Flag::kExplosion)) {
				return false;
			}

			const auto hitTargetHolder = a_hitData->target.get();
			auto* hitTarget = SanitizeObjectPointer(hitTargetHolder.get());
			const bool hitTargetMatches = hitTarget == a_target;

			const auto hitAggressorHolder = a_hitData->aggressor.get();
			auto* hitAggressor = SanitizeObjectPointer(hitAggressorHolder.get());
			const bool hitAggressorCompatible = a_attacker ?
				(!hitAggressor || hitAggressor == a_attacker) :
				(!hitAggressor || IsRegisteredSummon(hitAggressor));

			const auto sourceHolder = a_hitData->sourceRef.get();
			auto* sourceRef = SanitizeObjectPointer(sourceHolder.get());
			auto* explosion = sourceRef ? SanitizeObjectPointer(sourceRef->AsExplosion()) : nullptr;
			if (!explosion) {
				return false;
			}

			const auto& runtime = explosion->GetExplosionRuntimeData();
			bool sourceOwnedByRegisteredSummon = IsRegisteredSummonHandle(runtime.actorOwner);
			auto* actorCause = SanitizeObjectPointer(runtime.actorCause.get());
			if (!sourceOwnedByRegisteredSummon && actorCause) {
				sourceOwnedByRegisteredSummon = IsRegisteredSummonHandle(actorCause->actor);
			}

			const auto sourceHandle = explosion->GetHandle();
			const auto targetHandle = a_target->GetHandle();
			if (!sourceHandle || !targetHandle) {
				return false;
			}

			const auto now = std::chrono::steady_clock::now();
			try {
				const std::scoped_lock lock(g_summonStateMutex);
				const auto existing = std::find_if(
					g_explosionDamageWindows.begin(),
					g_explosionDamageWindows.end(),
					[&](const ExplosionDamageWindow& a_entry) {
						return a_entry.source == sourceHandle && a_entry.target == targetHandle;
					});
				const bool windowSeen = existing != g_explosionDamageWindows.end();
				const bool windowActive = windowSeen && now <= existing->acceptUntil;
				const bool accept = detail::ShouldAcceptRegisteredSummonExplosionDamage({
					.hitDataIsExplosion = true,
					.hitTargetMatches = hitTargetMatches,
					.hitAggressorCompatible = hitAggressorCompatible,
					.sourceExplosionOwnedByRegisteredSummon = sourceOwnedByRegisteredSummon,
					.sourceTargetWindowSeen = windowSeen,
					.sourceTargetWindowActive = windowActive
				});
				if (!accept || windowSeen) {
					return accept;
				}

				if (g_explosionDamageWindows.size() >= kMaxExplosionDamageWindows) {
					g_explosionDamageWindows.erase(g_explosionDamageWindows.begin());
				}
				g_explosionDamageWindows.push_back({
					.source = sourceHandle,
					.target = targetHandle,
					.acceptUntil = now + kExplosionDamageWindow
				});
				return true;
			} catch (...) {
				return false;
			}
		}
	}

	bool IsHostileEffectTarget(RE::Actor* a_owner, RE::Actor* a_target) noexcept
	{
		a_owner = SanitizeObjectPointer(a_owner);
		a_target = SanitizeObjectPointer(a_target);
		return detail::IsHostileOnlyEffectTargetAllowed(
			a_owner != nullptr,
			a_target != nullptr,
			a_owner && a_target && a_owner == a_target,
			a_target && (a_target->IsPlayerRef() || a_target->IsPlayerTeammate() ||
				ResolvePlayerOwnerActor(a_target) != nullptr),
			a_owner && a_target && a_owner->IsHostileToActor(a_target),
			a_owner && a_target && a_target->IsHostileToActor(a_owner));
	}

	void CastHostileOnlySpellImmediate(
		RE::MagicCaster* a_magicCaster,
		RE::SpellItem* a_spell,
		bool a_noHitArt,
		RE::TESObjectREFR* a_target,
		float a_effectiveness,
		float a_magnitudeOverride,
		RE::Actor* a_caster) noexcept
	{
		a_magicCaster = SanitizeObjectPointer(a_magicCaster);
		a_spell = SanitizeObjectPointer(a_spell);
		a_target = SanitizeObjectPointer(a_target);
		a_caster = SanitizeObjectPointer(a_caster);
		if (!a_magicCaster || !a_spell || !a_target || !a_caster) {
			return;
		}

		auto* playerOwner = ResolvePlayerOwnerActor(a_caster);
		const bool isSummonCreatureSpell = IsSummonCreatureSpell(a_spell);
		auto* targetActor = SanitizeObjectPointer(a_target->As<RE::Actor>());
		if (!isSummonCreatureSpell && targetActor &&
			!IsHostileEffectTarget(playerOwner ? playerOwner : a_caster, targetActor)) {
			return;
		}

		bool trackSummonCast = playerOwner && isSummonCreatureSpell;
		PendingSummonCast summonCast;
		if (trackSummonCast) {
			try {
				summonCast.caster = a_caster->GetHandle();
				summonCast.target = a_target->GetHandle();
				summonCast.spellFormID = a_spell->GetFormID();
				summonCast.effectsBefore = CaptureSummonEffectSnapshots(a_caster, a_target, a_spell);
				trackSummonCast = summonCast.caster && summonCast.target &&
				                  summonCast.spellFormID != 0u;
			} catch (...) {
				trackSummonCast = false;
			}
		}

		{
			const HostileOnlyCastScope scope(playerOwner);
			a_magicCaster->CastSpellImmediate(
				a_spell,
				a_noHitArt,
				a_target,
				a_effectiveness,
				false,
				a_magnitudeOverride,
				a_caster);
		}

		if (trackSummonCast) {
			ResolveOrQueueSummonCast(std::move(summonCast));
		}
	}

	bool ShouldSuppressNonHostileCalamityHealthDamage(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData) noexcept
	{
		a_target = SanitizeObjectPointer(a_target);
		a_attacker = SanitizeObjectPointer(a_attacker);

		const bool attackerIsRegisteredSummon = IsRegisteredSummon(a_attacker);
		const bool sourceExplosionOwnedByRegisteredSummon =
			IsFreshExplosionDamageFromRegisteredSummon(a_target, a_attacker, a_hitData);

		auto* playerOwner = ResolvePlayerOwnerActor(a_attacker);
		if (!playerOwner) {
			playerOwner = ResolveScopedPlayerOwner();
		}
		if (!playerOwner &&
			(attackerIsRegisteredSummon || sourceExplosionOwnedByRegisteredSummon)) {
			playerOwner = SanitizeObjectPointer(RE::PlayerCharacter::GetSingleton());
		}

		return detail::ShouldSuppressNonHostileCalamityHealthDamage({
			.hasTarget = a_target != nullptr,
			.hasAttacker = a_attacker != nullptr,
			.hasPlayerOwner = playerOwner != nullptr,
			.targetIsHostileToPlayerOwner = IsHostileEffectTarget(playerOwner, a_target),
			.hostileOnlyCastScopeActive = g_hostileOnlyCastDepth > 0u,
			.attackerIsRegisteredCalamitySummon = attackerIsRegisteredSummon,
			.sourceExplosionOwnedByRegisteredCalamitySummon =
				sourceExplosionOwnedByRegisteredSummon
		});
	}

	void ClearHostileEffectGuardRuntimeState() noexcept
	{
		g_summonRuntimeGeneration.fetch_add(1u, std::memory_order_relaxed);
		g_hostileOnlyCastDepth = 0u;
		g_hostileOnlyCastPlayerOwner.reset();
		try {
			const std::scoped_lock lock(g_summonStateMutex);
			g_registeredSummons.clear();
			g_pendingSummonCasts.clear();
			g_explosionDamageWindows.clear();
			g_summonRefreshScheduled = false;
		} catch (...) {
			// Runtime teardown must remain best-effort and noexcept.
		}
	}
}
