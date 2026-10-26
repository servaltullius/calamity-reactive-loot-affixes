#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LowHealthRearmPolicy.h"
#include "CalamityAffixes/ProcChancePolicy.h"

#include "CalamityAffixes/TriggerGuards.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <random>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
		constexpr auto kZeroIcdProcSafetyGuard = std::chrono::milliseconds(120);

		[[nodiscard]] std::uint64_t ToNonNegativeMilliseconds(std::chrono::steady_clock::time_point a_value) noexcept
		{
			const auto count = std::chrono::duration_cast<std::chrono::milliseconds>(a_value.time_since_epoch()).count();
			return count > 0 ? static_cast<std::uint64_t>(count) : 0u;
		}

		[[nodiscard]] float ResolveActorHealthPercent(RE::Actor* a_actor) noexcept
		{
			if (!a_actor) {
				return 100.0f;
			}

			auto* avOwner = skyrim_cast<RE::ActorValueOwner*>(a_actor);
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

		if (!IsWithinRecentlyWindowMs(nowMs, findLastMs(_combatState.recentOwnerHitAt), recentHitWindowMs)) {
			return false;
		}
		if (!IsWithinRecentlyWindowMs(nowMs, findLastMs(_combatState.recentOwnerKillAt), recentKillWindowMs)) {
			return false;
		}
		if (!IsOutsideRecentlyWindowMs(nowMs, findLastMs(_combatState.recentOwnerIncomingHitAt), notHitWindowMs)) {
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

		float roll;
		{
			std::lock_guard<std::mutex> lock(_rngMutex);
			std::uniform_real_distribution<float> dist(0.0f, 100.0f);
			roll = dist(_rng);
		}
		return roll < luckyChancePct;
	}

	bool EventBridge::PassesLowHealthTriggerGate(
		const AffixRuntime& a_affix,
		RE::Actor* a_owner,
		[[maybe_unused]] float a_previousHealthPct,
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

		// Rearm already ran for this observation (RearmLowHealthTriggers, before
		// the ICD gate); this only decides whether it fires now.
		const auto bands = detail::ResolveLowHealthBands(a_affix.lowHealthThresholdPct, a_affix.lowHealthRearmPct);
		LowHealthTriggerKey key{};
		key.token = a_affix.token;
		key.owner = ownerFormID;
		bool consumed = false;
		if (const auto it = _combatState.lowHealthTriggerConsumed.find(key); it != _combatState.lowHealthTriggerConsumed.end()) {
			consumed = it->second;
		}

		return detail::PassesLowHealthFireGate(a_currentHealthPct, consumed, bands);
	}

	void EventBridge::RearmLowHealthTriggers(
		const std::vector<std::size_t>& a_affixIndices,
		RE::Actor* a_owner,
		float a_preHitHealthPct,
		float a_currentHealthPct)
	{
		if (!a_owner || a_owner->GetFormID() == 0u) {
			return;
		}
		for (const auto idx : a_affixIndices) {
			if (idx >= _affixRuntimeState.affixes.size()) {
				continue;
			}
			const auto& affix = _affixRuntimeState.affixes[idx];
			if (affix.trigger != Trigger::kLowHealth || affix.token == 0u) {
				continue;
			}
			const auto bands = detail::ResolveLowHealthBands(affix.lowHealthThresholdPct, affix.lowHealthRearmPct);
			if (!detail::ShouldRearmLowHealthTrigger(a_preHitHealthPct, a_currentHealthPct, bands)) {
				continue;
			}
			LowHealthTriggerKey key{};
			key.token = affix.token;
			key.owner = a_owner->GetFormID();
			if (const auto it = _combatState.lowHealthTriggerConsumed.find(key);
				it != _combatState.lowHealthTriggerConsumed.end()) {
				it->second = false;
			}
		}
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
		_combatState.lowHealthTriggerConsumed[key] = true;
	}

	bool EventBridge::PassesTriggerProcPreconditions(
		const AffixRuntime& a_affix,
		Trigger a_trigger,
		RE::Actor* a_owner,
		RE::Actor* a_target,
		const RE::HitData* a_hitData,
		std::chrono::steady_clock::time_point a_now,
		float a_lowHealthPreviousPct,
		float a_lowHealthCurrentPct,
		PerTargetCooldownKey* a_outPerTargetKey)
	{
		if (a_outPerTargetKey) {
			*a_outPerTargetKey = {};
		}

		if (a_now < a_affix.nextAllowed) {
			return false;
		}

		if (!PassesRecentlyGates(a_affix, a_owner, a_now)) {
			return false;
		}

		// Exploit Weakness and other status payoffs: the target must carry enough
		// distinct Calamity statuses before chance or cooldown are spent.
		if (a_affix.requireTargetStatusCount > 0u &&
			(!a_target ||
				_combatState.statusLedger.CountDistinctKinds(a_target->GetFormID(), StatusClockNowMs()) <
					a_affix.requireTargetStatusCount)) {
			return false;
		}

		if (!PassesLuckyHitGate(a_affix, a_trigger, a_hitData, a_now)) {
			return false;
		}

		if (!PassesLowHealthTriggerGate(a_affix, a_owner, a_lowHealthPreviousPct, a_lowHealthCurrentPct)) {
			return false;
		}

		if (IsPerTargetCooldownBlocked(a_affix, a_target, a_now, a_outPerTargetKey)) {
			return false;
		}

		return true;
	}

	float EventBridge::ResolveTriggerProcChancePct(
		const AffixRuntime& a_affix,
		std::size_t a_affixIndex) const noexcept
	{
		return ResolveTriggerProcChancePctFromBase(a_affix, a_affixIndex, a_affix.procChancePct);
	}

	float EventBridge::ResolveTriggerProcChancePctFromBase(
		const AffixRuntime&,
		std::size_t a_affixIndex,
		float a_baseChancePct) const noexcept
	{
		if (a_affixIndex >= _lootState.activeProcSlotPenalties.size()) {
			return ResolveStackedProcChancePct(a_baseChancePct, _runtimeSettings.procChanceMult, {});
		}

		const auto& penalties = _lootState.activeProcSlotPenalties[a_affixIndex];
		return ResolveStackedProcChancePct(a_baseChancePct, _runtimeSettings.procChanceMult, penalties);
	}

	bool EventBridge::RollTriggerProcChance(float a_chancePct)
	{
		if (a_chancePct <= 0.0f) {
			return false;
		}
		if (a_chancePct >= 100.0f) {
			return true;
		}

		float roll;
		{
			std::lock_guard<std::mutex> lock(_rngMutex);
			std::uniform_real_distribution<float> dist(0.0f, 100.0f);
			roll = dist(_rng);
		}
		return roll < a_chancePct;
	}

	void EventBridge::CommitTriggerProcRuntime(
		AffixRuntime& a_affix,
		const PerTargetCooldownKey& a_perTargetKey,
		bool a_usesPerTargetIcd,
		float a_chancePct,
		std::chrono::steady_clock::time_point a_now)
	{
		const auto effectiveIcdMs = ResolveTriggerProcCooldownMs(
			a_affix.icd.count(),
			a_usesPerTargetIcd,
			a_chancePct,
			kZeroIcdProcSafetyGuard.count());
		if (effectiveIcdMs > 0) {
			a_affix.nextAllowed = a_now + std::chrono::milliseconds(effectiveIcdMs);
		}

		if (a_usesPerTargetIcd) {
			CommitPerTargetCooldown(a_perTargetKey, a_affix.perTargetIcd, a_now);
		}
	}

	float EventBridge::ResolveLowHealthTriggerCurrentPct(
		Trigger a_trigger,
		RE::Actor* a_owner) const noexcept
	{
		if (a_trigger != Trigger::kLowHealth) {
			return 100.0f;
		}

		return ResolveActorHealthPercent(a_owner);
	}

	float EventBridge::ResolveLowHealthTriggerDamagePct(
		RE::Actor* a_owner,
		float a_damageTaken) const noexcept
	{
		if (!a_owner || !(a_damageTaken > 0.0f)) {
			return 0.0f;
		}
		auto* avOwner = skyrim_cast<RE::ActorValueOwner*>(a_owner);
		if (!avOwner) {
			return 0.0f;
		}
		const float maxHealth = avOwner->GetPermanentActorValue(RE::ActorValue::kHealth);
		if (!(maxHealth > 0.0f)) {
			return 0.0f;
		}
		return std::clamp((a_damageTaken / maxHealth) * 100.0f, 0.0f, 100.0f);
	}

	float EventBridge::ResolveLowHealthTriggerPreviousPct(RE::FormID a_ownerFormID) const noexcept
	{
		if (a_ownerFormID == 0u) {
			return 100.0f;
		}

		if (const auto it = _combatState.lowHealthLastObservedPct.find(a_ownerFormID); it != _combatState.lowHealthLastObservedPct.end()) {
			return it->second;
		}
		return 100.0f;
	}

	void EventBridge::StoreLowHealthTriggerSnapshot(
		RE::FormID a_ownerFormID,
		float a_currentHealthPct) noexcept
	{
		if (a_ownerFormID == 0u) {
			return;
		}

		_combatState.lowHealthLastObservedPct[a_ownerFormID] = a_currentHealthPct;
	}
}
