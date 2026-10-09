#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/ActorTierQueries.h"
#include "CalamityAffixes/HostileEffectGuard.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/PluginEditorIds.h"
#include "CalamityAffixes/PointerSafety.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

// Shared statuses (v2.3.0). Exposed and Doom are tracked in the runtime's own
// ledger as our spells apply them; Exploit Weakness counts them and Contagion
// copies a dying enemy's statuses to its neighbours. Freeze, Bleed and Shock fill
// per-target meters that pay off when full, and Burning stacks.
namespace CalamityAffixes
{
	namespace
	{
		[[nodiscard]] float EffectMagnitude(const RE::Effect& a_effect, float a_magnitudeOverride) noexcept
		{
			return a_magnitudeOverride > 0.0f ? a_magnitudeOverride : a_effect.effectItem.magnitude;
		}

		// Dispels the target's active effects from a_spell that change a_actorValue.
		void DispelSpellEffectsOn(RE::Actor* a_target, RE::FormID a_spellFormID, RE::ActorValue a_actorValue)
		{
			auto* magicTarget = a_target ? a_target->AsMagicTarget() : nullptr;
			auto* effects = magicTarget ? magicTarget->GetActiveEffectList() : nullptr;
			if (!effects) {
				return;
			}
			std::vector<RE::ActiveEffect*> matched;
			for (auto* activeEffect : *effects) {
				activeEffect = SanitizeObjectPointer(activeEffect);
				if (!activeEffect || !activeEffect->spell || activeEffect->spell->GetFormID() != a_spellFormID ||
					activeEffect->flags.any(RE::ActiveEffect::Flag::kDispelled) || !activeEffect->effect ||
					!activeEffect->effect->baseEffect ||
					activeEffect->effect->baseEffect->data.primaryAV != a_actorValue) {
					continue;
				}
				matched.push_back(activeEffect);
			}
			for (auto* activeEffect : matched) {
				activeEffect->Dispel(true);
			}
		}

		// Freeze lasts as long as its spell; Bleed and Shock pay off at once, and the
		// short lock only keeps one hit from paying twice.
		constexpr std::int64_t kFreezeLockMs = 3000;
		constexpr std::int64_t kBleedLockMs = 1000;
		constexpr std::int64_t kShockLockMs = 500;
		constexpr float kBleedMaxHealthShare = 0.08f;
		constexpr float kBleedToughMaxHealthShare = 0.03f;
		constexpr float kBleedMinDamage = 10.0f;
		constexpr float kBleedMaxDamage = 400.0f;
		constexpr float kShockArcRadius = 300.0f;
		constexpr std::size_t kShockArcTargets = 2u;
		constexpr float kBurningDamagePerStack = 4.0f;

		// Ledger key for a build-up status: one entry per target and kind, never a
		// real spell, so Contagion copies it from the meters instead of re-casting.
		[[nodiscard]] constexpr std::uint32_t BuildUpLedgerValue(StatusKind a_kind) noexcept
		{
			return 0x10000u + static_cast<std::uint32_t>(a_kind);
		}

		[[nodiscard]] RE::SpellItem* StatusPayoffSpell(StatusKind a_kind)
		{
			switch (a_kind) {
			case StatusKind::kFreeze:
				return PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_STATUS_FREEZE");
			case StatusKind::kBleed:
				return PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_STATUS_BLEED_BURST");
			case StatusKind::kShock:
				return PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_STATUS_SHOCK_DISCHARGE");
			case StatusKind::kBurning:
				return PluginEditorIds::Lookup<RE::SpellItem>("CAFF_SPEL_STATUS_BURNING");
			default:
				return nullptr;
			}
		}

		[[nodiscard]] std::int64_t BuildUpLockMs(StatusKind a_kind) noexcept
		{
			switch (a_kind) {
			case StatusKind::kFreeze:
				return kFreezeLockMs;
			case StatusKind::kBleed:
				return kBleedLockMs;
			default:
				return kShockLockMs;
			}
		}

		// A payoff gets its own vanilla sound and a short art on the target, so it
		// reads in a busy fight: the meter itself is invisible.
		struct PayoffCue
		{
			RE::FormID art{ 0u };    // Skyrim.esm ArtObject, 0 for none
			RE::FormID sound{ 0u };  // Skyrim.esm sound descriptor
			float artSeconds{ 0.0f };
		};

		[[nodiscard]] constexpr PayoffCue PayoffCueFor(StatusKind a_kind) noexcept
		{
			switch (a_kind) {
			case StatusKind::kFreeze:
				return { 0x0005E990u, 0x000A7247u, 0.8f };  // IceWraithExplosion01Object, VOCShoutImpactIceForm
			case StatusKind::kBleed:
				return { 0u, 0x000DAB82u, 0.0f };  // NPCKillGore; the burst MGEF carries the blood spray
			case StatusKind::kShock:
				return { 0x000592D6u, 0x0003F20Du, 0.6f };  // LightningStormCastBodyFX, MAGShockImpactSD
			default:
				return {};
			}
		}

		[[nodiscard]] std::string_view StatusNameKo(StatusKind a_kind) noexcept
		{
			switch (a_kind) {
			case StatusKind::kFreeze:
				return "빙결";
			case StatusKind::kBleed:
				return "출혈";
			case StatusKind::kShock:
				return "감전";
			default:
				return "?";
			}
		}

		[[nodiscard]] std::string_view StatusName(StatusKind a_kind) noexcept
		{
			switch (a_kind) {
			case StatusKind::kFreeze:
				return "Freeze";
			case StatusKind::kBleed:
				return "Bleed";
			case StatusKind::kShock:
				return "Shock";
			case StatusKind::kBurning:
				return "Burning";
			default:
				return "?";
			}
		}
	}

	bool EventBridge::PrepareExposureCast(
		StatusKind a_kind, const RE::SpellItem* a_spell, RE::Actor* a_target, float a_magnitudeOverride)
	{
		if (a_kind != StatusKind::kExposed || !a_spell || !a_target) {
			return true;
		}
		const auto now = StatusClockNowMs();
		const auto target = a_target->GetFormID();
		bool improves = false;
		for (const auto* effect : a_spell->effects) {
			if (!effect || !effect->baseEffect) {
				continue;
			}
			const auto value = effect->baseEffect->data.primaryAV;
			const float magnitude = EffectMagnitude(*effect, a_magnitudeOverride);
			// The same spell re-applied is a refresh, never compared with itself.
			auto best = _combatState.statusLedger.Strongest(
				target, StatusKind::kExposed, static_cast<std::uint32_t>(value), now, a_spell->GetFormID());
			if (!best || magnitude > best->magnitude) {
				improves = true;
			}
			// Replace every weaker Exposed on this value, so only the new one counts.
			while (best && best->magnitude < magnitude) {
				DispelSpellEffectsOn(a_target, best->spellFormID, value);
				_combatState.statusLedger.EraseSpell(target, best->spellFormID, best->actorValue);
				best = _combatState.statusLedger.Strongest(
					target, StatusKind::kExposed, static_cast<std::uint32_t>(value), now, a_spell->GetFormID());
			}
		}
		return improves;
	}

	void EventBridge::RecordStatusApplication(
		StatusKind a_kind, const RE::SpellItem* a_spell, RE::Actor* a_target, float a_magnitudeOverride)
	{
		// Build-up statuses and Burning are recorded by their meters, not by the
		// spell that fed them.
		if (a_kind == StatusKind::kNone || a_kind == StatusKind::kBurning || IsBuildUpStatus(a_kind) || !a_spell ||
			!a_target) {
			return;
		}
		const auto now = StatusClockNowMs();
		for (const auto* effect : a_spell->effects) {
			if (!effect || !effect->baseEffect || effect->effectItem.duration <= 0) {
				continue;
			}
			_combatState.statusLedger.Record({
				.target = a_target->GetFormID(),
				.kind = a_kind,
				.spellFormID = a_spell->GetFormID(),
				.actorValue = static_cast<std::uint32_t>(effect->baseEffect->data.primaryAV),
				.magnitude = EffectMagnitude(*effect, a_magnitudeOverride),
				.expiresAtMs = now + static_cast<std::int64_t>(effect->effectItem.duration) * 1000,
			});
		}
	}

	void EventBridge::ApplyTaggedBuildUp(
		const Action& a_action, RE::Actor* a_owner, RE::Actor* a_target, RE::MagicCaster* a_magicCaster)
	{
		// A stray hit on a follower still procs the cast (the hostile-only cast
		// then does nothing); it must not fill the follower's meters either.
		if (!IsHostileEffectTarget(a_owner, a_target)) {
			return;
		}
		if (a_action.statusTag == StatusKind::kBurning) {
			ApplyBurningStacks(static_cast<std::uint8_t>(a_action.statusAmount), a_owner, a_target, a_magicCaster);
		} else if (IsBuildUpStatus(a_action.statusTag)) {
			ApplyBuildUp(a_action.statusTag, a_action.statusAmount, a_owner, a_target, a_magicCaster);
		}
	}

	void EventBridge::ApplyBuildUp(
		StatusKind a_kind, float a_amount, RE::Actor* a_owner, RE::Actor* a_target, RE::MagicCaster* a_magicCaster)
	{
		if (!IsBuildUpStatus(a_kind) || a_amount <= 0.0f || !a_owner || !a_target || !a_magicCaster || a_target->IsDead()) {
			return;
		}
		const auto now = StatusClockNowMs();
		const auto target = a_target->GetFormID();
		const bool tough = IsToughActor(a_target);
		const auto lockMs = BuildUpLockMs(a_kind);
		const auto gain = _combatState.statusMeters.Gain(target, a_kind, a_amount, now, tough, lockMs);
		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: {} build-up (target={}, +{}, value={}, threshold={}, applied={}).",
				StatusName(a_kind),
				a_target->GetName(),
				a_amount,
				gain.paidOff ? gain.threshold : gain.value,
				gain.threshold,
				gain.applied);
		}
		if (!gain.applied) {
			return;
		}
		// While the meter holds anything (or its payoff lasts), the target counts
		// as carrying the status.
		_combatState.statusLedger.Record({
			.target = target,
			.kind = a_kind,
			.spellFormID = 0u,
			.actorValue = BuildUpLedgerValue(a_kind),
			.magnitude = gain.value,
			.expiresAtMs = gain.paidOff ? now + lockMs : BuildUpEmptiesAtMs(gain.value, now),
		});
		if (!gain.paidOff) {
			return;
		}

		auto* payoff = StatusPayoffSpell(a_kind);
		if (!payoff) {
			SKSE::log::error("CalamityAffixes: {} payoff spell missing from the plugin.", StatusName(a_kind));
			return;
		}
		switch (a_kind) {
		case StatusKind::kFreeze:
			CastHostileOnlySpellImmediate(a_magicCaster, payoff, false, a_target, 1.0f, 0.0f, a_owner);
			break;
		case StatusKind::kBleed:
			{
				auto* avOwner = a_target->AsActorValueOwner();
				const float maxHealth = avOwner ? std::max(0.0f, avOwner->GetPermanentActorValue(RE::ActorValue::kHealth)) : 0.0f;
				const float share = tough ? kBleedToughMaxHealthShare : kBleedMaxHealthShare;
				const float damage = std::clamp(maxHealth * share, kBleedMinDamage, kBleedMaxDamage);
				CastHostileOnlySpellImmediate(a_magicCaster, payoff, false, a_target, 1.0f, damage, a_owner);
			}
			break;
		case StatusKind::kShock:
			{
				CastHostileOnlySpellImmediate(a_magicCaster, payoff, false, a_target, 1.0f, 0.0f, a_owner);
				const auto arcs = CollectNearbyHostiles(a_owner, a_target, kShockArcRadius, kShockArcTargets);
				for (const auto& arc : arcs) {
					if (arc && !arc->IsDead()) {
						CastHostileOnlySpellImmediate(a_magicCaster, payoff, false, arc.get(), 1.0f, 0.0f, a_owner);
					}
				}
			}
			break;
		default:
			break;
		}
		PlayStatusPayoffCue(a_kind, a_target);
		if (_loot.debugHudNotifications) {
			// Payoffs are hard to tell apart in a busy fight; name them on the HUD.
			std::string note = "CAFF 상태: ";
			note.append(StatusNameKo(a_kind)).append(" → ").append(a_target->GetName());
			EmitDebugHudNotification(note.c_str());
		}
		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: {} payoff (target={}, threshold={}, tough={}).",
				StatusName(a_kind),
				a_target->GetName(),
				gain.threshold,
				tough);
		}
	}

	void EventBridge::PlayStatusPayoffCue(StatusKind a_kind, RE::Actor* a_target) const
	{
		const auto cue = PayoffCueFor(a_kind);
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler || !a_target) {
			return;
		}
		if (cue.art != 0u && a_target->Is3DLoaded()) {
			if (auto* art = dataHandler->LookupForm<RE::BGSArtObject>(cue.art, "Skyrim.esm")) {
				a_target->InstantiateHitArt(art, cue.artSeconds, nullptr, false, false, nullptr, false);
			}
		}
		if (cue.sound != 0u) {
			if (auto* sound = dataHandler->LookupForm<RE::BGSSoundDescriptorForm>(cue.sound, "Skyrim.esm")) {
				PlaySpatialSound(sound, a_target->GetPosition());
			}
		}
	}

	void EventBridge::ApplyBurningStacks(
		std::uint8_t a_stacks, RE::Actor* a_owner, RE::Actor* a_target, RE::MagicCaster* a_magicCaster)
	{
		if (a_stacks == 0u || !a_owner || !a_target || !a_magicCaster || a_target->IsDead()) {
			return;
		}
		auto* burning = StatusPayoffSpell(StatusKind::kBurning);
		if (!burning) {
			SKSE::log::error("CalamityAffixes: Burning spell missing from the plugin.");
			return;
		}
		const auto now = StatusClockNowMs();
		const auto target = a_target->GetFormID();
		const auto stacks = _combatState.statusMeters.AddBurning(target, a_stacks, now);
		// Re-casting the same spell replaces the running burn, so the damage per
		// second always matches the current stacks and the timer restarts.
		CastHostileOnlySpellImmediate(
			a_magicCaster, burning, false, a_target, 1.0f, kBurningDamagePerStack * static_cast<float>(stacks), a_owner);
		_combatState.statusLedger.Record({
			.target = target,
			.kind = StatusKind::kBurning,
			.spellFormID = 0u,
			.actorValue = BuildUpLedgerValue(StatusKind::kBurning),
			.magnitude = static_cast<float>(stacks),
			.expiresAtMs = now + BuildUpRules::kBurningDurationMs,
		});
		if (_loot.debugLog) {
			SKSE::log::debug("CalamityAffixes: Burning stacks (target={}, stacks={}).", a_target->GetName(), stacks);
		}
	}

	std::vector<RE::NiPointer<RE::Actor>> EventBridge::CollectNearbyHostiles(
		RE::Actor* a_owner, RE::Actor* a_center, float a_radius, std::size_t a_maxTargets) const
	{
		std::vector<RE::NiPointer<RE::Actor>> result;
		auto* processLists = RE::ProcessLists::GetSingleton();
		if (!processLists || !a_owner || !a_center || a_maxTargets == 0u || a_radius <= 0.0f) {
			return result;
		}
		struct Candidate
		{
			RE::NiPointer<RE::Actor> actor{};
			float distanceSq{ 0.0f };
		};
		std::vector<Candidate> candidates;
		const auto origin = a_center->GetPosition();
		const float radiusSq = a_radius * a_radius;
		// Diagnostics for an empty result: how many actors were seen and how far
		// the nearest living hostile one was.
		std::size_t scanned = 0u;
		std::size_t hostile = 0u;
		float nearestHostileSq = -1.0f;
		processLists->ForEachHighActor([&](RE::Actor& a) {
			++scanned;
			if (&a == a_owner || &a == a_center || a.IsDead() || !IsHostileEffectTarget(a_owner, std::addressof(a))) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			const auto pos = a.GetPosition();
			const float dx = pos.x - origin.x;
			const float dy = pos.y - origin.y;
			const float dz = pos.z - origin.z;
			const float distSq = dx * dx + dy * dy + dz * dz;
			++hostile;
			if (nearestHostileSq < 0.0f || distSq < nearestHostileSq) {
				nearestHostileSq = distSq;
			}
			if (distSq > radiusSq) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			if (candidates.size() < a_maxTargets) {
				candidates.push_back({ RE::NiPointer<RE::Actor>{ std::addressof(a) }, distSq });
			} else {
				auto farthest = std::max_element(candidates.begin(), candidates.end(),
					[](const Candidate& l, const Candidate& r) { return l.distanceSq < r.distanceSq; });
				if (farthest != candidates.end() && distSq < farthest->distanceSq) {
					*farthest = { RE::NiPointer<RE::Actor>{ std::addressof(a) }, distSq };
				}
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});
		std::sort(candidates.begin(), candidates.end(),
			[](const Candidate& l, const Candidate& r) { return l.distanceSq < r.distanceSq; });
		if (_loot.debugLog && candidates.empty()) {
			SKSE::log::debug(
				"CalamityAffixes: no nearby hostiles (center={}, radius={}, scanned={}, livingHostile={}, nearest={}).",
				a_center->GetName(),
				a_radius,
				scanned,
				hostile,
				nearestHostileSq < 0.0f ? -1.0f : std::sqrt(nearestHostileSq));
		}
		result.reserve(candidates.size());
		for (auto& candidate : candidates) {
			result.push_back(std::move(candidate.actor));
		}
		return result;
	}

	void EventBridge::ExecuteDoomMarkAction(
		const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		const auto& action = a_affix.action;
		auto* magicCaster = a_owner ? a_owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant) : nullptr;
		if (!magicCaster || !a_target || !action.spell || !action.doomMarkSpell) {
			return;
		}
		const float magnitude = ResolveSpellMagnitudeOverride(action, action.spell, a_hitData, false);
		if (magnitude <= 0.0f) {
			return;
		}
		const auto now = StatusClockNowMs();
		const auto fireAt = now + action.doomDelay.count();
		if (!_combatState.doomQueue.Mark({ a_affix.token, a_target->GetFormID(), magnitude, fireAt })) {
			return;
		}
		_combatState.hasPendingDoom.store(true, std::memory_order_release);

		// The mark is the visible shadow on the target and lasts until the burst.
		CastHostileOnlySpellImmediate(
			magicCaster, action.doomMarkSpell, true, a_target, 1.0f, 0.0f, a_owner);
		_combatState.statusLedger.Record({
			.target = a_target->GetFormID(),
			.kind = StatusKind::kDoom,
			.spellFormID = action.doomMarkSpell->GetFormID(),
			.actorValue = 0u,
			.magnitude = magnitude,
			.expiresAtMs = fireAt,
		});
		PlayActionFeedback(action, a_owner, a_target, ActionFeedbackPlayOn::kProc);
		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: Doom marked (affix={}, target={}, magnitude={}, delayMs={}).",
				a_affix.id,
				a_target->GetName(),
				magnitude,
				action.doomDelay.count());
		}
	}

	void EventBridge::TickDoomBursts(RE::PlayerCharacter* a_player, RE::MagicCaster* a_magicCaster)
	{
		std::array<DoomPending, DoomQueue::kCapacity> due{};
		const auto count = _combatState.doomQueue.TakeDue(StatusClockNowMs(), due);
		_combatState.hasPendingDoom.store(!_combatState.doomQueue.Empty(), std::memory_order_release);
		for (std::size_t i = 0; i < count; ++i) {
			const auto& doom = due[i];
			const auto idxIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(doom.affixToken);
			if (idxIt == _affixRuntimeState.affixRegistry.affixIndexByToken.end() ||
				idxIt->second >= _affixRuntimeState.affixes.size()) {
				continue;
			}
			const auto& action = _affixRuntimeState.affixes[idxIt->second].action;
			auto* target = RE::TESForm::LookupByID<RE::Actor>(doom.target);
			if (action.doomMarkSpell) {
				_combatState.statusLedger.EraseSpell(doom.target, action.doomMarkSpell->GetFormID(), 0u);
			}
			if (!action.spell || !target || target->IsDead() || !target->Is3DLoaded()) {
				continue;
			}
			// The mark's spell lasts whole seconds; clear the shadow as it bursts.
			if (action.doomMarkSpell) {
				DispelSpellEffectsOn(target, action.doomMarkSpell->GetFormID(), RE::ActorValue::kNone);
			}
			// Same guard as the shadow echo: the burst is not a new hit, so it never
			// fires on-hit affixes, and a kill it causes does not start Kill chains.
			Hooks::ExpectEchoStrikeDamage(target, a_player);
			{
				const ScopedProcDepth procDepthGuard{ _combatState };
				CastHostileOnlySpellImmediate(
					a_magicCaster,
					action.spell,
					action.noHitEffectArt,
					target,
					action.effectiveness,
					doom.magnitude,
					a_player);
			}
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: Doom burst (target={}, magnitude={}).", target->GetName(), doom.magnitude);
			}
		}
	}

	void EventBridge::ExecuteSpreadStatusAction(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_victim)
	{
		const auto& action = a_affix.action;
		auto* magicCaster = a_owner ? a_owner->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant) : nullptr;
		if (!magicCaster || !a_victim) {
			return;
		}
		const auto now = StatusClockNowMs();
		const auto victim = a_victim->GetFormID();
		std::array<StatusEntry, 16> statuses{};
		const auto statusCount = _combatState.statusLedger.Collect(victim, now, statuses);
		const auto pendingDoom = _combatState.doomQueue.Find(victim);
		const auto meters = _combatState.statusMeters.Take(victim, now);
		_combatState.statusLedger.EraseTarget(victim);
		_combatState.doomQueue.Remove(victim);
		if (statusCount == 0u) {
			return;
		}

		const auto targets = CollectNearbyHostiles(a_owner, a_victim, action.spreadRadius, action.spreadMaxTargets);
		for (const auto& targetPtr : targets) {
			auto* target = targetPtr.get();
			if (!target || target->IsDead()) {
				continue;
			}
			// Meters and Burning pass on at the victim's current amount.
			for (const auto kind : { StatusKind::kFreeze, StatusKind::kBleed, StatusKind::kShock }) {
				const float value = meters.values[*BuildUpIndex(kind)];
				if (value > 0.0f) {
					ApplyBuildUp(kind, value, a_owner, target, magicCaster);
				}
			}
			ApplyBurningStacks(meters.burningStacks, a_owner, target, magicCaster);

			std::array<RE::FormID, 16> castSpells{};
			std::size_t castCount = 0u;
			for (std::size_t i = 0; i < statusCount; ++i) {
				const auto& status = statuses[i];
				if (status.kind == StatusKind::kBurning || IsBuildUpStatus(status.kind)) {
					continue;
				}
				if (std::find(castSpells.begin(), castSpells.begin() + castCount, status.spellFormID) !=
					castSpells.begin() + castCount) {
					continue;  // one cast per spell; entries are per changed value
				}
				castSpells[castCount++] = status.spellFormID;
				auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(status.spellFormID);
				if (!spell) {
					continue;
				}
				if (status.kind == StatusKind::kDoom) {
					if (!pendingDoom) {
						continue;
					}
					std::int64_t delayMs = 1500;
					if (const auto it = _affixRuntimeState.affixRegistry.affixIndexByToken.find(pendingDoom->affixToken);
						it != _affixRuntimeState.affixRegistry.affixIndexByToken.end() &&
						it->second < _affixRuntimeState.affixes.size()) {
						delayMs = _affixRuntimeState.affixes[it->second].action.doomDelay.count();
					}
					if (_combatState.doomQueue.Mark({ pendingDoom->affixToken, target->GetFormID(), pendingDoom->magnitude, now + delayMs })) {
						_combatState.hasPendingDoom.store(true, std::memory_order_release);
						CastHostileOnlySpellImmediate(magicCaster, spell, true, target, 1.0f, 0.0f, a_owner);
						_combatState.statusLedger.Record({
							.target = target->GetFormID(),
							.kind = StatusKind::kDoom,
							.spellFormID = status.spellFormID,
							.actorValue = 0u,
							.magnitude = pendingDoom->magnitude,
							.expiresAtMs = now + delayMs,
						});
					}
					continue;
				}
				// Keep the victim's strength for single-effect spells (scaled casts carry
				// an override). An override would flatten every effect of a multi-effect
				// spell to one value, so those are cast at their own magnitudes.
				const float overrideMagnitude = spell->effects.size() == 1u ? status.magnitude : 0.0f;
				if (!PrepareExposureCast(status.kind, spell, target, overrideMagnitude)) {
					continue;
				}
				CastHostileOnlySpellImmediate(magicCaster, spell, true, target, 1.0f, overrideMagnitude, a_owner);
				RecordStatusApplication(status.kind, spell, target, overrideMagnitude);
			}
			PlayActionFeedback(action, a_owner, target, ActionFeedbackPlayOn::kProc);
		}
		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: Contagion spread (affix={}, statuses={}, targets={}).",
				a_affix.id,
				statusCount,
				targets.size());
		}
	}
}
