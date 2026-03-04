#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/CombatContext.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/ProcChanceUtil.h"
#include "CalamityAffixes/TriggerGuards.h"
#include <algorithm>
#include <string_view>
#include <vector>


namespace CalamityAffixes
{
	namespace
	{
		float GetSpellBaseMagnitude(const RE::SpellItem* a_spell)
		{
			if (!a_spell) {
				return 0.0f;
			}

			float maxMagnitude = 0.0f;
			for (const auto* effect : a_spell->effects) {
				if (!effect) {
					continue;
				}
				maxMagnitude = std::max(maxMagnitude, effect->effectItem.magnitude);
			}
			return maxMagnitude;
		}

		float ResolveSpecialActionProcChancePct(float a_configuredChancePct)
		{
			if (a_configuredChancePct <= 0.0f) {
				// Keep backward compatibility with existing configs:
				// special actions historically used 0% as "always on".
				return 100.0f;
			}

			return std::clamp(a_configuredChancePct, 0.0f, 100.0f);
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

		if (!_configLoaded || !_runtimeEnabled || _convertAffixIndices.empty()) {
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
		for (const auto idx : _convertAffixIndices) {
			if (idx < _activeCounts.size() && _activeCounts[idx] > 0) {
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

		for (const auto idx : _convertAffixIndices) {
			if (idx >= _affixes.size() || idx >= _activeCounts.size()) {
				continue;
			}
			if (_activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixes[idx];
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

			const float chancePct = ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeProcChanceMult);
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
		if (!_configLoaded || !_runtimeEnabled || _mindOverMatterAffixIndices.empty()) {
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
		for (const auto idx : _mindOverMatterAffixIndices) {
			if (idx < _activeCounts.size() && _activeCounts[idx] > 0) {
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

		for (const auto idx : _mindOverMatterAffixIndices) {
			if (idx >= _affixes.size() || idx >= _activeCounts.size()) {
				continue;
			}
			if (_activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixes[idx];
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

			const float chancePct = ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeProcChanceMult);
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

		auto* avOwner = a_target->AsActorValueOwner();
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

	EventBridge::CastOnCritResult EventBridge::EvaluateCastOnCrit(
		RE::Actor* a_attacker,
		RE::Actor* a_target,
		const RE::HitData* a_hitData,
		bool a_allowResync)
	{
		const std::scoped_lock lock(_stateMutex);

		if (!_configLoaded || !_runtimeEnabled || _castOnCritAffixIndices.empty()) {
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

		// CoC requires a DIRECT weapon reference in the hitData.
		// Do NOT use ResolveHitWeapon (which falls back to equipped weapon),
		// because CoC spell projectile hits (Fire Bolt, Ice Spike, etc.) have
		// hitData->weapon == NULL, and the fallback would return the equipped
		// bow — causing an infinite proc-on-proc chain for ranged weapons.
		const auto* hitWeapon = a_hitData->weapon
			? SanitizeObjectPointer(a_hitData->weapon)
			: nullptr;
		if (!hitWeapon) {
			return {};
		}

		const bool isCrit = a_hitData->flags.any(RE::HitData::Flag::kCritical);
		const bool isPowerAttack = a_hitData->flags.any(RE::HitData::Flag::kPowerAttack);

		// Bow/crossbow: kPowerAttack is never set and kCritical is rare,
		// so skip the crit/power gate and let procChancePct control activation.
		const bool isRangedWeapon =
			(hitWeapon->GetWeaponType() == RE::WEAPON_TYPE::kBow ||
			 hitWeapon->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow);

		if (!isRangedWeapon && !isCrit && !isPowerAttack) {
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
			if (now < _castOnCritNextAllowed) {
				return {};
			}

		std::vector<std::size_t> pool;
		pool.reserve(_castOnCritAffixIndices.size());

		for (const auto idx : _castOnCritAffixIndices) {
			if (idx >= _affixes.size() || idx >= _activeCounts.size()) {
				continue;
			}

			if (_activeCounts[idx] == 0) {
				continue;
			}

			auto& affix = _affixes[idx];
			const auto& action = affix.action;
			if (action.type != ActionType::kCastOnCrit || !action.spell) {
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

			const float chancePct = ResolveSpecialActionProcChancePct(affix.procChancePct * _runtimeProcChanceMult);
			if (!RollProcChance(_rng, _rngMutex, chancePct)) {
				continue;
			}

			pool.push_back(idx);
		}

		if (pool.empty()) {
			return {};
		}

		const auto pickedIdx = pool[_castOnCritCycleCursor % pool.size()];
		auto& pickedAffix = _affixes[pickedIdx];
		const auto* pick = std::addressof(pickedAffix.action);
		_castOnCritCycleCursor += 1;
		_castOnCritNextAllowed = now + kCastOnCritICD;
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

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: CastOnCrit triggered (crit={}, powerAttack={}, spells={}, picked={}).",
				isCrit,
				isPowerAttack,
				pool.size(),
				pick->spell->GetName());
		}

		float magnitudeOverride = pick->magnitudeOverride;
		if (pick->magnitudeScaling.source != MagnitudeScaling::Source::kNone) {
			const float hitPhysicalDealt = std::max(0.0f, a_hitData->physicalDamage - a_hitData->resistedPhysicalDamage);
			const float hitTotalDealt = std::max(0.0f, a_hitData->totalDamage - a_hitData->resistedPhysicalDamage - a_hitData->resistedTypedDamage);
			const float spellBaseMagnitude = GetSpellBaseMagnitude(pick->spell);
			magnitudeOverride = ResolveMagnitudeOverride(
				pick->magnitudeOverride,
				spellBaseMagnitude,
				hitPhysicalDealt,
				hitTotalDealt,
				pick->magnitudeScaling);
		}

		return CastOnCritResult{
			.spell = pick->spell,
			.effectiveness = pick->effectiveness,
			.magnitudeOverride = magnitudeOverride,
			.noHitEffectArt = pick->noHitEffectArt,
		};
	}

	float EventBridge::GetCritDamageMultiplier(
		RE::Actor* a_attacker,
		const RE::HitData* a_hitData) const
	{
		const std::scoped_lock lock(_stateMutex);

		if (!_configLoaded || !_runtimeEnabled || _activeCritDamageBonusPct <= 0.0f) {
			return 1.0f;
		}

		if (!a_attacker || !a_attacker->IsPlayerRef()) {
			return 1.0f;
		}

		if (!a_hitData || !a_hitData->flags.any(RE::HitData::Flag::kCritical)) {
			return 1.0f;
		}

		const float mult = 1.0f + (_activeCritDamageBonusPct / 100.0f);
		if (_loot.debugLog) {
			SKSE::log::debug("CalamityAffixes: crit damage bonus {:.0f}% -> multiplier {:.2f}", _activeCritDamageBonusPct, mult);
		}
		return mult;
	}

}
