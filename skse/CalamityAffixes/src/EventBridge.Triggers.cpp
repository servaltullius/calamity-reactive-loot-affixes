#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/PrismaTooltip.h"
#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
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

			void RecordRecentEventTimestamp(
				std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point>& a_store,
				RE::FormID a_ownerFormId,
				std::chrono::steady_clock::time_point a_now)
			{
				if (a_ownerFormId == 0u) {
					return;
				}

				a_store[a_ownerFormId] = a_now;

				// Player-owned actor population is tiny, but keep this bounded for long sessions.
				static constexpr auto kTtl = std::chrono::seconds(90);
				static constexpr std::size_t kHardCap = 256;
				if (a_store.size() <= kHardCap) {
					return;
				}

				for (auto it = a_store.begin(); it != a_store.end();) {
					if ((a_now - it->second) > kTtl) {
						it = a_store.erase(it);
					} else {
						++it;
					}
				}
			}

			constexpr auto kZeroIcdProcSafetyGuard = std::chrono::milliseconds(120);

			[[nodiscard]] float ResolveActorHealthPercent(RE::Actor* a_actor) noexcept
			{
				if (!a_actor) {
					return 100.0f;
				}

				auto* avOwner = a_actor->AsActorValueOwner();
				if (!avOwner) {
					return 100.0f;
				}

				const float maxHealth = std::max(0.0f, avOwner->GetPermanentActorValue(RE::ActorValue::kHealth));
				if (maxHealth <= 0.0f) {
					return 100.0f;
				}

				const float currentHealth = std::clamp(avOwner->GetActorValue(RE::ActorValue::kHealth), 0.0f, maxHealth);
				return (currentHealth / maxHealth) * 100.0f;
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

	bool EventBridge::TryConsumeTriggerProcBudget(std::chrono::steady_clock::time_point a_now) noexcept
	{
		return TryConsumeFixedWindowBudget(
			ToNonNegativeMilliseconds(a_now),
			static_cast<std::uint64_t>(_loot.triggerProcBudgetWindowMs),
			_loot.triggerProcBudgetPerWindow,
			_triggerProcBudgetWindowStartMs,
			_triggerProcBudgetConsumed);
	}

		void EventBridge::RecordRecentCombatEvent(
			Trigger a_trigger,
			RE::Actor* a_owner,
			std::chrono::steady_clock::time_point a_now)
	{
		if (!a_owner) {
			return;
		}

		const auto ownerFormId = a_owner->GetFormID();
		switch (a_trigger) {
		case Trigger::kHit:
			RecordRecentEventTimestamp(_recentOwnerHitAt, ownerFormId, a_now);
			break;
		case Trigger::kKill:
			RecordRecentEventTimestamp(_recentOwnerKillAt, ownerFormId, a_now);
			break;
		case Trigger::kIncomingHit:
			RecordRecentEventTimestamp(_recentOwnerIncomingHitAt, ownerFormId, a_now);
			break;
			case Trigger::kDotApply:
			case Trigger::kLowHealth:
			default:
				break;
			}
		}

	bool EventBridge::PassesRecentlyGates(
		const AffixRuntime& a_affix,
		RE::Actor* a_owner,
		std::chrono::steady_clock::time_point a_now) const
	{
		if (!a_owner) {
			return false;
		}

		const auto ownerFormId = a_owner->GetFormID();
		if (ownerFormId == 0u) {
			return false;
		}

		const auto nowMs = ToNonNegativeMilliseconds(a_now);
		auto findLastMs = [&](const std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point>& a_store) -> std::uint64_t {
			const auto it = a_store.find(ownerFormId);
			if (it == a_store.end()) {
				return 0u;
			}
			return ToNonNegativeMilliseconds(it->second);
		};

		const auto recentHitWindowMs = static_cast<std::uint64_t>(std::max<std::int64_t>(0, a_affix.requireRecentlyHit.count()));
		const auto recentKillWindowMs = static_cast<std::uint64_t>(std::max<std::int64_t>(0, a_affix.requireRecentlyKill.count()));
		const auto notHitWindowMs = static_cast<std::uint64_t>(std::max<std::int64_t>(0, a_affix.requireNotHitRecently.count()));

		if (!IsWithinRecentlyWindowMs(nowMs, findLastMs(_recentOwnerHitAt), recentHitWindowMs)) {
			return false;
		}
		if (!IsWithinRecentlyWindowMs(nowMs, findLastMs(_recentOwnerKillAt), recentKillWindowMs)) {
			return false;
		}
		if (!IsOutsideRecentlyWindowMs(nowMs, findLastMs(_recentOwnerIncomingHitAt), notHitWindowMs)) {
			return false;
		}

		return true;
	}

		bool EventBridge::PassesLuckyHitGate(
			const AffixRuntime& a_affix,
		Trigger a_trigger,
		const RE::HitData* a_hitData,
		std::chrono::steady_clock::time_point)
	{
		if (a_affix.luckyHitChancePct <= 0.0f) {
			return true;
		}

		// Lucky-hit semantics only apply to direct hit style triggers.
		if (a_trigger != Trigger::kHit && a_trigger != Trigger::kIncomingHit) {
			return true;
		}

		if (!a_hitData) {
			return false;
		}

		const float luckyChancePct = ResolveLuckyHitEffectiveChancePct(
			a_affix.luckyHitChancePct,
			a_affix.luckyHitProcCoefficient);
		if (luckyChancePct <= 0.0f) {
			return false;
		}
		if (luckyChancePct >= 100.0f) {
			return true;
		}

			std::uniform_real_distribution<float> dist(0.0f, 100.0f);
			return dist(_rng) < luckyChancePct;
		}

		bool EventBridge::PassesLowHealthTriggerGate(
			const AffixRuntime& a_affix,
			RE::Actor* a_owner,
			float a_previousHealthPct,
			float a_currentHealthPct)
		{
			if (a_affix.trigger != Trigger::kLowHealth) {
				return true;
			}
			if (!a_owner || a_affix.token == 0u) {
				return false;
			}

			const RE::FormID ownerFormID = a_owner->GetFormID();
			if (ownerFormID == 0u) {
				return false;
			}

			const float thresholdPct = std::clamp(a_affix.lowHealthThresholdPct, 1.0f, 95.0f);
			const float rearmPct = std::clamp(a_affix.lowHealthRearmPct, thresholdPct + 1.0f, 100.0f);
			const float healthPct = std::clamp(a_currentHealthPct, 0.0f, 100.0f);
			const float previousHealthPct = std::clamp(a_previousHealthPct, 0.0f, 100.0f);

			LowHealthTriggerKey key{};
			key.token = a_affix.token;
			key.owner = ownerFormID;
			bool consumed = false;
				if (const auto it = _lowHealthTriggerConsumed.find(key); it != _lowHealthTriggerConsumed.end()) {
					consumed = it->second;
				}

				if (previousHealthPct >= rearmPct && healthPct <= thresholdPct) {
					// If HP dropped across the rearm band in one hit, allow this fresh low-health window.
					consumed = false;
					_lowHealthTriggerConsumed[key] = false;
				}

				if (healthPct >= rearmPct) {
					_lowHealthTriggerConsumed[key] = false;
					return false;
				}
			if (healthPct > thresholdPct) {
				return false;
			}

			return !consumed;
		}

	void EventBridge::MarkLowHealthTriggerConsumed(
		const AffixRuntime& a_affix,
		RE::Actor* a_owner)
	{
			if (a_affix.trigger != Trigger::kLowHealth || !a_owner || a_affix.token == 0u) {
				return;
			}
			const RE::FormID ownerFormID = a_owner->GetFormID();
			if (ownerFormID == 0u) {
				return;
			}
			LowHealthTriggerKey key{};
			key.token = a_affix.token;
			key.owner = ownerFormID;
		_lowHealthTriggerConsumed[key] = true;
	}

	void EventBridge::RebuildActiveTriggerIndexCaches()
	{
		const auto rebuildFor = [&](const std::vector<std::size_t>& a_source, std::vector<std::size_t>& a_out) {
			a_out.clear();
			a_out.reserve(a_source.size());
			for (const auto idx : a_source) {
				if (idx >= _activeCounts.size() || _activeCounts[idx] == 0) {
					continue;
				}
				a_out.push_back(idx);
			}
		};

		rebuildFor(_hitTriggerAffixIndices, _activeHitTriggerAffixIndices);
		rebuildFor(_incomingHitTriggerAffixIndices, _activeIncomingHitTriggerAffixIndices);
		rebuildFor(_dotApplyTriggerAffixIndices, _activeDotApplyTriggerAffixIndices);
		rebuildFor(_killTriggerAffixIndices, _activeKillTriggerAffixIndices);
		rebuildFor(_lowHealthTriggerAffixIndices, _activeLowHealthTriggerAffixIndices);
	}

	const std::vector<std::size_t>* EventBridge::ResolveActiveTriggerIndices(Trigger a_trigger) const noexcept
	{
		switch (a_trigger) {
		case Trigger::kHit:
			return &_activeHitTriggerAffixIndices;
		case Trigger::kIncomingHit:
			return &_activeIncomingHitTriggerAffixIndices;
		case Trigger::kDotApply:
			return &_activeDotApplyTriggerAffixIndices;
		case Trigger::kKill:
			return &_activeKillTriggerAffixIndices;
		case Trigger::kLowHealth:
			return &_activeLowHealthTriggerAffixIndices;
		default:
			return nullptr;
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
		const RE::FormID lowHealthOwnerFormID =
			(a_trigger == Trigger::kLowHealth && a_owner) ? a_owner->GetFormID() : 0u;
		const bool hasLowHealthSnapshot = lowHealthOwnerFormID != 0u;
		const float lowHealthCurrentPct = hasLowHealthSnapshot ? ResolveActorHealthPercent(a_owner) : 100.0f;
		const float lowHealthPreviousPct = [&]() {
			if (!hasLowHealthSnapshot) {
				return 100.0f;
			}
			if (const auto it = _lowHealthLastObservedPct.find(lowHealthOwnerFormID); it != _lowHealthLastObservedPct.end()) {
				return it->second;
			}
			return 100.0f;
		}();

		const auto* indices = ResolveActiveTriggerIndices(a_trigger);

		if (!indices || indices->empty()) {
			RecordRecentCombatEvent(a_trigger, a_owner, now);
			return;
		}

		bool loggedProcBudgetDenied = false;
		for (const auto i : *indices) {
			if (i >= _affixes.size() || i >= _activeCounts.size() || _activeCounts[i] == 0) {
				continue;
			}

			auto& affix = _affixes[i];
			if (now < affix.nextAllowed) {
				continue;
			}

			if (!PassesRecentlyGates(affix, a_owner, now)) {
				continue;
			}

				if (!PassesLuckyHitGate(affix, a_trigger, a_hitData, now)) {
					continue;
				}

				if (!PassesLowHealthTriggerGate(affix, a_owner, lowHealthPreviousPct, lowHealthCurrentPct)) {
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

			if (!TryConsumeTriggerProcBudget(now)) {
				if (_loot.debugLog && !loggedProcBudgetDenied) {
					spdlog::debug(
						"CalamityAffixes: trigger proc budget exhausted (budget={} / windowMs={}).",
						_loot.triggerProcBudgetPerWindow,
						_loot.triggerProcBudgetWindowMs);
					loggedProcBudgetDenied = true;
				}
				continue;
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
					MarkLowHealthTriggerConsumed(affix, a_owner);
				}

		if (hasLowHealthSnapshot) {
			// Update once per trigger pass so all low-health affixes evaluate against the same health snapshot.
			_lowHealthLastObservedPct[lowHealthOwnerFormID] = lowHealthCurrentPct;
		}

		RecordRecentCombatEvent(a_trigger, a_owner, now);
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

	bool EventBridge::ShouldSuppressPapyrusHitEvent(const LastHitKey& a_key, std::chrono::steady_clock::time_point a_now) noexcept
	{
		const DuplicateHitKey current{
			.outgoing = a_key.outgoing,
			.aggressor = a_key.aggressor,
			.target = a_key.target,
			.source = a_key.source
		};
		const DuplicateHitKey last{
			.outgoing = _lastPapyrusHit.outgoing,
			.aggressor = _lastPapyrusHit.aggressor,
			.target = _lastPapyrusHit.target,
			.source = _lastPapyrusHit.source
		};
		if (ShouldSuppressDuplicateHitWindow(
				current,
				last,
				ToNonNegativeMilliseconds(a_now),
				ToNonNegativeMilliseconds(_lastPapyrusHitEventAt),
				static_cast<std::uint64_t>(kPapyrusHitEventWindow.count()))) {
			return true;
		}

		_lastPapyrusHitEventAt = a_now;
		_lastPapyrusHit = a_key;
		return false;
	}
}
