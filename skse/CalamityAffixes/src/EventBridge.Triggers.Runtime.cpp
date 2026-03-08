#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <chrono>
#include <cstdint>

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

			static constexpr auto kTtl = std::chrono::seconds(90);
			static constexpr std::size_t kHardCap = 256;
			for (auto it = a_store.begin(); it != a_store.end();) {
				if ((a_now - it->second) > kTtl) {
					it = a_store.erase(it);
				} else {
					++it;
				}
			}

			while (a_store.size() > kHardCap) {
				a_store.erase(a_store.begin());
			}
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

		return _combatState.perTargetCooldownStore.IsBlocked(key.token, key.target, a_now);
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

		_combatState.perTargetCooldownStore.Commit(a_key.token, a_key.target, a_perTargetIcd, a_now);
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
		return _combatState.nonHostileFirstHitGate.Resolve(
			a_owner->GetFormID(),
			a_target->GetFormID(),
			_runtimeSettings.allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed),
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
			_combatState.triggerProcBudgetWindowStartMs,
			_combatState.triggerProcBudgetConsumed);
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
			RecordRecentEventTimestamp(_combatState.recentOwnerHitAt, ownerFormId, a_now);
			break;
		case Trigger::kKill:
			RecordRecentEventTimestamp(_combatState.recentOwnerKillAt, ownerFormId, a_now);
			break;
		case Trigger::kIncomingHit:
			RecordRecentEventTimestamp(_combatState.recentOwnerIncomingHitAt, ownerFormId, a_now);
			break;
		case Trigger::kDotApply:
		case Trigger::kLowHealth:
		default:
			break;
		}
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

		rebuildFor(_affixRegistry.hitTriggerAffixIndices, _activeHitTriggerAffixIndices);
		rebuildFor(_affixRegistry.incomingHitTriggerAffixIndices, _activeIncomingHitTriggerAffixIndices);
		rebuildFor(_affixRegistry.dotApplyTriggerAffixIndices, _activeDotApplyTriggerAffixIndices);
		rebuildFor(_affixRegistry.killTriggerAffixIndices, _activeKillTriggerAffixIndices);
		rebuildFor(_affixRegistry.lowHealthTriggerAffixIndices, _activeLowHealthTriggerAffixIndices);
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

	void EventBridge::MarkPlayerCombatEvidence(
		std::chrono::steady_clock::time_point a_now,
		PlayerCombatEvidenceSource a_source,
		const RE::Actor* a_player,
		const RE::Actor* a_other) noexcept
	{
		const auto describeSource = [](PlayerCombatEvidenceSource a_value) noexcept -> const char* {
			switch (a_value) {
			case PlayerCombatEvidenceSource::kHealthDamageRoutedHit:
				return "HealthDamage";
			case PlayerCombatEvidenceSource::kTesHitOutgoing:
				return "TESHitOutgoing";
			case PlayerCombatEvidenceSource::kTesHitIncoming:
				return "TESHitIncoming";
			default:
				return "Unknown";
			}
		};

		constexpr auto kPlayerCombatEvidenceWindow = std::chrono::seconds(20);
		const auto newExpiry = a_now + kPlayerCombatEvidenceWindow;

		if (_runtimeSettings.disableCombatEvidenceLease) {
			if (_runtimeSettings.combatDebugLog) {
				static auto nextLogAt = std::chrono::steady_clock::time_point{};
				if (nextLogAt.time_since_epoch().count() == 0 || a_now >= nextLogAt) {
					nextLogAt = a_now + std::chrono::seconds(2);
					SKSE::log::info(
						"CalamityAffixes: combat evidence lease suppressed (source={}, player={}({:08X}), other={}({:08X})).",
						describeSource(a_source),
						a_player ? a_player->GetName() : "<none>",
						a_player ? a_player->GetFormID() : 0u,
						a_other ? a_other->GetName() : "<none>",
						a_other ? a_other->GetFormID() : 0u);
				}
			}
			return;
		}

		_combatState.playerCombatEvidenceExpiresAt = newExpiry;

		if (!_runtimeSettings.combatDebugLog) {
			return;
		}

		static auto nextLogAt = std::chrono::steady_clock::time_point{};
		if (nextLogAt.time_since_epoch().count() != 0 && a_now < nextLogAt) {
			return;
		}
		nextLogAt = a_now + std::chrono::seconds(1);

		const auto expiresInMs = static_cast<std::int64_t>(
			std::chrono::duration_cast<std::chrono::milliseconds>(newExpiry - a_now).count());
		SKSE::log::info(
			"CalamityAffixes: combat evidence marked (source={}, player={}({:08X}) inCombat={}, other={}({:08X}), expiresIn={}ms).",
			describeSource(a_source),
			a_player ? a_player->GetName() : "<none>",
			a_player ? a_player->GetFormID() : 0u,
			a_player ? a_player->IsInCombat() : false,
			a_other ? a_other->GetName() : "<none>",
			a_other ? a_other->GetFormID() : 0u,
			expiresInMs);
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
			.outgoing = _combatState.lastHit.outgoing,
			.aggressor = _combatState.lastHit.aggressor,
			.target = _combatState.lastHit.target,
			.source = _combatState.lastHit.source
		};
		if (ShouldSuppressDuplicateHitWindow(
				current,
				last,
				ToNonNegativeMilliseconds(a_now),
				ToNonNegativeMilliseconds(_combatState.lastHitAt),
				static_cast<std::uint64_t>(kDuplicateHitWindow.count()))) {
			return true;
		}

		_combatState.lastHitAt = a_now;
		_combatState.lastHit = a_key;
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
			.outgoing = _combatState.lastPapyrusHit.outgoing,
			.aggressor = _combatState.lastPapyrusHit.aggressor,
			.target = _combatState.lastPapyrusHit.target,
			.source = _combatState.lastPapyrusHit.source
		};
		if (ShouldSuppressDuplicateHitWindow(
				current,
				last,
				ToNonNegativeMilliseconds(a_now),
				ToNonNegativeMilliseconds(_combatState.lastPapyrusHitEventAt),
				static_cast<std::uint64_t>(kPapyrusHitEventWindow.count()))) {
			return true;
		}

		_combatState.lastPapyrusHitEventAt = a_now;
		_combatState.lastPapyrusHit = a_key;
		return false;
	}
}
