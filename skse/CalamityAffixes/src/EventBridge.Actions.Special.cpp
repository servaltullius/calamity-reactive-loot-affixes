#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/CombatContext.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/ProcChanceUtil.h"
#include "CalamityAffixes/SpecialActionSafetyPolicy.h"
#include "CalamityAffixes/TriggerGuards.h"
#include <algorithm>
#include <string_view>
#include <vector>


namespace CalamityAffixes
{
	namespace
	{
		float GetCritCastDirectDamageMagnitude(const RE::SpellItem* a_spell)
		{
			if (!a_spell) {
				return 0.0f;
			}

			DirectElementalDamageMagnitudeSelector selector;
			for (const auto* effect : a_spell->effects) {
				if (!effect || !effect->baseEffect) {
					continue;
				}

				const auto archetype = effect->baseEffect->GetArchetype();
				const bool isValueModifier =
					archetype == RE::EffectSetting::Archetype::kValueModifier ||
					archetype == RE::EffectSetting::Archetype::kDualValueModifier;
				const auto primaryAV = effect->baseEffect->data.primaryAV;
				const auto resistAV = effect->baseEffect->data.resistVariable;
				const bool usesElementalResistance =
					resistAV == RE::ActorValue::kResistFire ||
					resistAV == RE::ActorValue::kResistFrost ||
					resistAV == RE::ActorValue::kResistShock;

				selector.Consider(
					effect->effectItem.magnitude,
					isValueModifier,
					primaryAV == RE::ActorValue::kHealth,
					usesElementalResistance,
					effect->effectItem.duration);
			}
			return selector.Resolve();
		}
	}

	EventBridge::ConversionResults EventBridge::EvaluateConversion(
		RE::Actor* a_attacker,
		RE::Actor* a_target,
		const RE::HitData* a_hitData,
		float& a_inOutDamage,
		bool a_allowResync)
	{
		const std::scoped_lock lock(_stateMutex);

		if (!_configLoaded || !_runtimeSettings.enabled.load(std::memory_order_relaxed) || _affixSpecialActions.convertAffixIndices.empty()) {
			return {};
		}

		if (!a_attacker || !a_target) {
			return {};
		}

		if (!a_attacker->IsPlayerRef()) {
			return {};
		}
		const auto now = std::chrono::steady_clock::now();
		const bool hostileEitherDirection = IsHostileEitherDirection(a_attacker, a_target);
		const bool allowNeutralOutgoing =
			ShouldResolveNonHostileOutgoingFirstHitAllowance(
				true,
				a_target->IsPlayerRef(),
				AllowsNonHostilePlayerOwnedOutgoingProcs()) &&
			ResolveNonHostileOutgoingFirstHitAllowance(a_attacker, a_target, hostileEitherDirection, now);
		if (!(hostileEitherDirection || allowNeutralOutgoing)) {
			return {};
		}
		if (!HitDataUtil::IsWeaponLikeHit(a_hitData, a_attacker)) {
			return {};
		}

		if (a_allowResync) {
			MaybeResyncEquippedAffixes(now);
		}

		bool hasAnyConversion = false;
		for (const auto idx : _affixSpecialActions.convertAffixIndices) {
			if (idx < _affixRuntimeState.activeCounts.size() && _affixRuntimeState.activeCounts[idx] > 0) {
				hasAnyConversion = true;
				break;
			}
		}
		if (!hasAnyConversion) {
			return {};
		}

		// --- Phase 1: Collect best candidate per element ---
		struct ConversionCandidate
		{
			AffixRuntime* affix{ nullptr };
			const Action* action{ nullptr };
			PerTargetCooldownKey perTargetKey{};
			bool usesPerTargetIcd{ false };
			float pct{ 0.0f };
		};

		// Index: 0=Fire, 1=Frost, 2=Shock (matching Element enum values minus 1)
		std::array<ConversionCandidate, kMaxConversionsPerHit> candidates{};

		for (const auto idx : _affixSpecialActions.convertAffixIndices) {
			if (idx >= _affixRuntimeState.affixes.size() || idx >= _affixRuntimeState.activeCounts.size()) {
				continue;
			}
			if (_affixRuntimeState.activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixRuntimeState.affixes[idx];
			const auto& candidate = affix.action;
			if (candidate.type != ActionType::kConvertDamage || !candidate.spell || candidate.convertPct <= 0.0f) {
				continue;
			}

			const auto elemIdx = static_cast<std::size_t>(candidate.element);
			if (elemIdx < 1u || elemIdx > kMaxConversionsPerHit) {
				continue;
			}

			if (now < affix.nextAllowed) {
				continue;
			}

			if (!PassesRecentlyGates(affix, a_attacker, now)) {
				continue;
			}

			if (!PassesLuckyHitGate(affix, Trigger::kHit, a_hitData, now)) {
				continue;
			}

			PerTargetCooldownKey perTargetKey{};
			const bool usesPerTargetIcd = (affix.perTargetIcd.count() > 0 && a_target && affix.token != 0u);
			if (IsPerTargetCooldownBlocked(affix, a_target, now, &perTargetKey)) {
				continue;
			}

			const float chancePct = detail::ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeSettings.procChanceMult);
			if (!RollProcChance(_rng, _rngMutex, chancePct)) {
				continue;
			}

			// Same element: keep higher pct only
			auto& slot = candidates[elemIdx - 1u];
			if (candidate.convertPct > slot.pct) {
				slot.affix = std::addressof(affix);
				slot.action = std::addressof(candidate);
				slot.perTargetKey = perTargetKey;
				slot.usesPerTargetIcd = usesPerTargetIcd;
				slot.pct = candidate.convertPct;
			}
		}

		// --- Phase 2: Compute total and normalize ---
		float totalPct = 0.0f;
		for (const auto& c : candidates) {
			totalPct += c.pct;
		}
		if (totalPct <= 0.0f) {
			return {};
		}

		const float physicalDealt = std::max(0.0f, a_hitData->physicalDamage - a_hitData->resistedPhysicalDamage);
		if (physicalDealt <= 0.0f) {
			return {};
		}

		if (a_inOutDamage <= 0.0f) {
			return {};
		}

		const bool needsNormalization = (totalPct > 100.0f);

		// --- Phase 3: Apply each candidate, deducting from remaining damage ---
		ConversionResults results{};
		const auto* hitWeapon = _loot.debugLog ? HitDataUtil::ResolveHitWeapon(a_hitData, a_attacker) : nullptr;

		for (std::size_t i = 0; i < kMaxConversionsPerHit; ++i) {
			const auto& c = candidates[i];
			if (!c.action || !c.affix) {
				continue;
			}

			const float normalizedPct = needsNormalization
				? (c.pct / totalPct * 100.0f)
				: c.pct;

			float converted = physicalDealt * (normalizedPct / 100.0f);
			if (converted <= 0.0f) {
				continue;
			}

			if (a_inOutDamage <= 0.0f) {
				break;
			}

			if (converted > a_inOutDamage) {
				converted = a_inOutDamage;
			}

			a_inOutDamage -= converted;
			if (a_inOutDamage < 0.0f) {
				a_inOutDamage = 0.0f;
			}

			results.entries[results.count++] = ConversionResult{
				.spell = c.action->spell,
				.convertedDamage = converted,
				.effectiveness = c.action->effectiveness,
				.noHitEffectArt = c.action->noHitEffectArt,
			};

			// Commit ICD
			if (c.affix->icd.count() > 0) {
				c.affix->nextAllowed = now + c.affix->icd;
			}
			if (c.usesPerTargetIcd) {
				CommitPerTargetCooldown(c.perTargetKey, c.affix->perTargetIcd, now);
			}

			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: ConvertDamage (weapon={}, physicalDealt={:.1f}, element={}, rawPct={:.0f}%, normalizedPct={:.0f}%, converted={:.1f}, remainingPhys={:.1f})",
					hitWeapon ? hitWeapon->GetName() : "<unknown>",
					physicalDealt,
					static_cast<int>(c.action->element),
					c.pct,
					normalizedPct,
					converted,
					a_inOutDamage);
			}
		}

		return results;
	}

	EventBridge::MindOverMatterResult EventBridge::EvaluateMindOverMatter(
		RE::Actor* a_target,
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData,
		float& a_inOutDamage,
		bool a_allowResync)
	{
		const std::scoped_lock lock(_stateMutex);

		MindOverMatterResult result{};
		if (!_configLoaded || !_runtimeSettings.enabled.load(std::memory_order_relaxed) || _affixSpecialActions.mindOverMatterAffixIndices.empty()) {
			return result;
		}
		if (!a_target || !a_attacker || !a_target->IsPlayerRef()) {
			return result;
		}
		if (!IsHostileEitherDirection(a_target, a_attacker)) {
			return result;
		}
		if (!a_hitData || a_inOutDamage <= 0.0f) {
			return result;
		}

		const float physicalTaken = std::max(0.0f, a_hitData->physicalDamage - a_hitData->resistedPhysicalDamage);
		if (physicalTaken <= 0.0f) {
			return result;
		}

		if (a_allowResync) {
			MaybeResyncEquippedAffixes(std::chrono::steady_clock::now());
		}

		bool hasAnyMindOverMatter = false;
		for (const auto idx : _affixSpecialActions.mindOverMatterAffixIndices) {
			if (idx < _affixRuntimeState.activeCounts.size() && _affixRuntimeState.activeCounts[idx] > 0) {
				hasAnyMindOverMatter = true;
				break;
			}
		}
		if (!hasAnyMindOverMatter) {
			return result;
		}

		const auto now = std::chrono::steady_clock::now();
		AffixRuntime* selectedAffix = nullptr;
		PerTargetCooldownKey selectedPerTargetKey{};
		bool selectedUsesPerTargetIcd = false;
		const Action* action = nullptr;
		float bestRedirectPct = 0.0f;

		for (const auto idx : _affixSpecialActions.mindOverMatterAffixIndices) {
			if (idx >= _affixRuntimeState.affixes.size() || idx >= _affixRuntimeState.activeCounts.size()) {
				continue;
			}
			if (_affixRuntimeState.activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixRuntimeState.affixes[idx];
			const auto& candidate = affix.action;
			if (candidate.type != ActionType::kMindOverMatter || candidate.mindOverMatterDamageToMagickaPct <= 0.0f) {
				continue;
			}

			if (now < affix.nextAllowed) {
				continue;
			}
			if (!PassesRecentlyGates(affix, a_target, now)) {
				continue;
			}
			if (!PassesLuckyHitGate(affix, Trigger::kIncomingHit, a_hitData, now)) {
				continue;
			}

			PerTargetCooldownKey perTargetKey{};
			const bool usesPerTargetIcd = (affix.perTargetIcd.count() > 0 && a_attacker && affix.token != 0u);
			if (IsPerTargetCooldownBlocked(affix, a_attacker, now, &perTargetKey)) {
				continue;
			}

			const float chancePct = detail::ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeSettings.procChanceMult);
			if (!RollProcChance(_rng, _rngMutex, chancePct)) {
				continue;
			}

			if (!action || candidate.mindOverMatterDamageToMagickaPct > bestRedirectPct) {
				action = std::addressof(candidate);
				selectedAffix = std::addressof(affix);
				selectedPerTargetKey = perTargetKey;
				selectedUsesPerTargetIcd = usesPerTargetIcd;
				bestRedirectPct = candidate.mindOverMatterDamageToMagickaPct;
			}
		}

		if (!action || action->mindOverMatterDamageToMagickaPct <= 0.0f) {
			return result;
		}

		auto* avOwner = skyrim_cast<RE::ActorValueOwner*>(a_target);
		if (!avOwner) {
			return result;
		}

		const float currentMagicka = std::max(0.0f, avOwner->GetActorValue(RE::ActorValue::kMagicka));
		if (currentMagicka <= 0.0f) {
			return result;
		}

		float redirect = physicalTaken * (action->mindOverMatterDamageToMagickaPct / 100.0f);
		if (redirect <= 0.0f) {
			return result;
		}
		redirect = std::min(redirect, a_inOutDamage);
		if (action->mindOverMatterMaxRedirectPerHit > 0.0f) {
			redirect = std::min(redirect, action->mindOverMatterMaxRedirectPerHit);
		}
		redirect = std::min(redirect, currentMagicka);
		if (redirect <= 0.0f) {
			return result;
		}

		avOwner->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -redirect);
		a_inOutDamage -= redirect;
		if (a_inOutDamage < 0.0f) {
			a_inOutDamage = 0.0f;
		}

		if (selectedAffix && selectedAffix->icd.count() > 0) {
			selectedAffix->nextAllowed = now + selectedAffix->icd;
		}
		if (selectedUsesPerTargetIcd && selectedAffix) {
			CommitPerTargetCooldown(selectedPerTargetKey, selectedAffix->perTargetIcd, now);
		}

		result.redirectedDamage = redirect;
		result.consumedMagicka = redirect;
		result.redirectPct = action->mindOverMatterDamageToMagickaPct;

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: MindOverMatter (physicalTaken={}, redirectPct={}%, redirected={}, remainingHealthDamage={}).",
				physicalTaken,
				result.redirectPct,
				result.redirectedDamage,
				a_inOutDamage);
		}

		return result;
	}

	EventBridge::CastOnCritResults EventBridge::EvaluateCastOnCrit(
		RE::Actor* a_attacker,
		RE::Actor* a_target,
		const RE::HitData* a_hitData,
		bool a_allowResync)
	{
		static_assert(kMaxCastOnCritPerHit == 2u);
		const std::scoped_lock lock(_stateMutex);

		if (!_configLoaded || !_runtimeSettings.enabled.load(std::memory_order_relaxed) || _affixSpecialActions.castOnCritAffixIndices.empty()) {
			return {};
		}

		if (!a_attacker || !a_target || !a_hitData) {
			return {};
		}

		if (!a_attacker->IsPlayerRef()) {
			return {};
		}

		if (a_allowResync) {
			MaybeResyncEquippedAffixes(std::chrono::steady_clock::now());
		}

		// Bow/crossbow projectile HitData may omit the direct weapon pointer.
		// Resolve only from weapon-origin evidence carried by this hit; the
		// resolver deliberately rejects spell HitData and never falls back to a
		// merely equipped bow, preventing proc-on-proc spell projectile chains.
		const auto* hitWeapon = HitDataUtil::ResolveCastOnCritHitWeapon(a_hitData, a_attacker);
		if (!hitWeapon) {
			return {};
		}

		const bool isCrit = a_hitData->flags.any(RE::HitData::Flag::kCritical);
		const bool isPowerAttack = a_hitData->flags.any(RE::HitData::Flag::kPowerAttack);

		// Bow/crossbow: kPowerAttack is never set and kCritical is rare, so keep
		// the existing procChancePct path and its one-result limit.  A normal melee
		// hit instead selects one eligible candidate first, then rolls that
		// candidate's dedicated normal-hit chance exactly once.
		const bool isRangedWeapon = HitDataUtil::IsBowOrCrossbow(hitWeapon);
		const bool isNormalMeleeHit = !isRangedWeapon && detail::IsEligibleNormalWeaponHitFlags(
			isCrit,
			isPowerAttack,
			a_hitData->attackDataSpell != nullptr,
			a_hitData->flags.any(RE::HitData::Flag::kBash),
			a_hitData->flags.any(RE::HitData::Flag::kTimedBash),
			a_hitData->flags.any(RE::HitData::Flag::kExplosion));
		if (!isRangedWeapon && !isCrit && !isPowerAttack && !isNormalMeleeHit) {
			return {};
		}

		// Avoid friendly-fire spam.
		const auto now = std::chrono::steady_clock::now();
		const bool hostileEitherDirection = IsHostileEitherDirection(a_attacker, a_target);
		const bool allowNeutralOutgoing =
			ShouldResolveNonHostileOutgoingFirstHitAllowance(
				true,
				a_target->IsPlayerRef(),
				AllowsNonHostilePlayerOwnedOutgoingProcs()) &&
			ResolveNonHostileOutgoingFirstHitAllowance(a_attacker, a_target, hostileEitherDirection, now);
		if (!(hostileEitherDirection || allowNeutralOutgoing)) {
			return {};
		}
		if (now < _combatState.castOnCritNextAllowed) {
			return {};
		}

		std::vector<std::size_t> pool;
		pool.reserve(_affixSpecialActions.castOnCritAffixIndices.size());

		for (const auto idx : _affixSpecialActions.castOnCritAffixIndices) {
			if (idx >= _affixRuntimeState.affixes.size() || idx >= _affixRuntimeState.activeCounts.size()) {
				continue;
			}

			if (_affixRuntimeState.activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixRuntimeState.affixes[idx];
			const auto& action = affix.action;
			if (action.type != ActionType::kCastOnCrit || !action.spell) {
				continue;
			}
			if (isNormalMeleeHit && affix.normalWeaponHitProcChancePct <= 0.0f) {
				continue;
			}

			if (now < affix.nextAllowed) {
				continue;
			}

			if (!PassesRecentlyGates(affix, a_attacker, now)) {
				continue;
			}

			if (!PassesLuckyHitGate(affix, Trigger::kHit, a_hitData, now)) {
				continue;
			}

			PerTargetCooldownKey perTargetKey{};
			if (IsPerTargetCooldownBlocked(affix, a_target, now, &perTargetKey)) {
				continue;
			}
			if (!isNormalMeleeHit) {
				const float chancePct = detail::ResolveSpecialActionProcChancePct(
					affix.procChancePct * _runtimeSettings.procChanceMult);
				if (!RollProcChance(_rng, _rngMutex, chancePct)) {
					continue;
				}
			}

			pool.push_back(idx);
		}

		if (pool.empty()) {
			return {};
		}

		std::array<std::size_t, kMaxCastOnCritPerHit> selectedIndices{};
		std::size_t selectedCount = 0;

		if (isNormalMeleeHit) {
			// Advance even on a failed roll so every eligible affix receives an equal
			// opportunity over repeated normal hits.  Only the selected candidate's
			// normal-hit chance is rolled, avoiding N independent rolls on one hit.
			const auto pickedIdx = pool[detail::ResolveCyclicCandidateIndex(
				pool.size(),
				_combatState.castOnCritCycleCursor)];
			_combatState.castOnCritCycleCursor += 1;
			const auto& pickedAffix = _affixRuntimeState.affixes[pickedIdx];
			const float normalHitChancePct = detail::ResolveSpecialActionProcChancePct(
				pickedAffix.normalWeaponHitProcChancePct * _runtimeSettings.procChanceMult);
			if (!RollProcChance(_rng, _rngMutex, normalHitChancePct)) {
				return {};
			}
			selectedIndices[selectedCount++] = pickedIdx;
		} else {
			selectedCount = detail::ResolveCastOnCritSelectionCount(
				pool.size(),
				isRangedWeapon,
				isNormalMeleeHit);
			for (std::size_t offset = 0; offset < selectedCount; ++offset) {
				selectedIndices[offset] = pool[detail::ResolveCyclicCandidateIndex(
					pool.size(),
					_combatState.castOnCritCycleCursor,
					offset)];
			}
			_combatState.castOnCritCycleCursor += selectedCount;
		}

		if (selectedCount == 0) {
			return {};
		}

		// The 150 ms limiter applies once to the original-hit batch.  Individual
		// and per-target cooldowns are consumed only by candidates actually selected.
		_combatState.castOnCritNextAllowed = now + kCastOnCritICD;
		CastOnCritResults results{};
		for (std::size_t selected = 0; selected < selectedCount; ++selected) {
			auto& pickedAffix = _affixRuntimeState.affixes[selectedIndices[selected]];
			const auto& pick = pickedAffix.action;

			if (pickedAffix.icd.count() > 0) {
				pickedAffix.nextAllowed = now + pickedAffix.icd;
			}
			if (pickedAffix.perTargetIcd.count() > 0 && a_target && pickedAffix.token != 0u) {
				const PerTargetCooldownKey perTargetKey{
					.token = pickedAffix.token,
					.target = a_target->GetFormID()
				};
				CommitPerTargetCooldown(perTargetKey, pickedAffix.perTargetIcd, now);
			}

			float magnitudeOverride = pick.magnitudeOverride;
			if (pick.magnitudeScaling.source != MagnitudeScaling::Source::kNone) {
				const float hitPhysicalDealt = std::max(
					0.0f,
					a_hitData->physicalDamage - a_hitData->resistedPhysicalDamage);
				const float hitTotalDealt = std::max(
					0.0f,
					a_hitData->totalDamage - a_hitData->resistedPhysicalDamage - a_hitData->resistedTypedDamage);
				const float spellBaseMagnitude = GetCritCastDirectDamageMagnitude(pick.spell);
				magnitudeOverride = ResolveMagnitudeOverride(
					pick.magnitudeOverride,
					spellBaseMagnitude,
					hitPhysicalDealt,
					hitTotalDealt,
					pick.magnitudeScaling);
			}

			results.entries[results.count++] = CastOnCritResult{
				.spell = pick.spell,
				.effectiveness = pick.effectiveness,
				.magnitudeOverride = magnitudeOverride,
				.noHitEffectArt = pick.noHitEffectArt,
			};
		}

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: CastOnCrit batch triggered (crit={}, powerAttack={}, ranged={}, candidates={}, selected={}).",
				isCrit,
				isPowerAttack,
				isRangedWeapon,
				pool.size(),
				results.count);
		}

		return results;
	}

	float EventBridge::GetCritDamageMultiplier(
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData) const
	{
		const std::scoped_lock lock(_stateMutex);

		if (!_configLoaded || !_runtimeSettings.enabled.load(std::memory_order_relaxed) || _affixRuntimeState.activeCritDamageBonusPct <= 0.0f) {
			return 1.0f;
		}

		if (!a_attacker || !a_attacker->IsPlayerRef()) {
			return 1.0f;
		}

		if (!a_hitData || !a_hitData->flags.any(RE::HitData::Flag::kCritical)) {
			return 1.0f;
		}

		const float mult = 1.0f + (_affixRuntimeState.activeCritDamageBonusPct / 100.0f);
		if (_loot.debugLog) {
			SKSE::log::debug("CalamityAffixes: crit damage bonus {:.0f}% -> multiplier {:.2f}", _affixRuntimeState.activeCritDamageBonusPct, mult);
		}
		return mult;
	}

}
