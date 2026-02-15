#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/PrismaTooltip.h"
#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <chrono>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <RE/M/Misc.h>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
		namespace
		{
			[[nodiscard]] std::uint64_t ToNonNegativeMilliseconds(std::chrono::steady_clock::time_point a_value) noexcept
			{
				const auto count = std::chrono::duration_cast<std::chrono::milliseconds>(a_value.time_since_epoch()).count();
				return count > 0 ? static_cast<std::uint64_t>(count) : 0u;
			}

			constexpr auto kZeroIcdProcSafetyGuard = std::chrono::milliseconds(120);
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

				return _perTargetCooldownStore.IsBlocked(key.token, key.target, a_now);
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

				_perTargetCooldownStore.Commit(a_key.token, a_key.target, a_perTargetIcd, a_now);
				}

			bool EventBridge::ResolveNonHostileOutgoingFirstHitAllowance(
				RE::Actor* a_owner,
				RE::Actor* a_target,
				bool a_hostileEitherDirection,
				std::chrono::steady_clock::time_point a_now)
			{
				if (!a_owner || !a_target) {
					return false;
				}
				return _nonHostileFirstHitGate.Resolve(
					a_owner->GetFormID(),
					a_target->GetFormID(),
					_allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed),
					a_hostileEitherDirection,
					a_target->IsPlayerRef(),
					a_now);
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

			const float penalty = (i < _activeSlotPenalty.size() && _activeSlotPenalty[i] > 0.0f) ? _activeSlotPenalty[i] : 1.0f;
			const float chance = std::clamp(affix.procChancePct * _runtimeProcChanceMult * penalty, 0.0f, 100.0f);
			if (chance <= 0.0f) {
				continue;
			}

			if (chance < 100.0f) {
				const float roll = dist(_rng);
				if (roll >= chance) {
					continue;
				}
			}

			const auto effectiveIcdMs = ResolveTriggerProcCooldownMs(
				affix.icd.count(),
				usesPerTargetIcd,
				chance,
				kZeroIcdProcSafetyGuard.count());
			if (effectiveIcdMs > 0) {
				affix.nextAllowed = now + std::chrono::milliseconds(effectiveIcdMs);
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
