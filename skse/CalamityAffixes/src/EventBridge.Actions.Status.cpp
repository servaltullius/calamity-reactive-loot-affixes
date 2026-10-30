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

	ExposurePlan EventBridge::PlanExposureCast(
		const RE::SpellItem* a_spell, RE::Actor* a_target, float a_magnitudeOverride) const
	{
		ExposurePlan plan{};
		plan.improves = true;
		if (!a_spell || !a_target) {
			return plan;
		}
		// Only timed values are statuses; an instant part (Mist's magicka burn)
		// never reaches the ledger and must not make the cast look like a gain.
		std::array<ExposureValue, ExposurePlan::kMaxValues> values{};
		std::size_t count = 0u;
		for (const auto* effect : a_spell->effects) {
			if (!effect || !effect->baseEffect || effect->effectItem.duration <= 0 || count >= values.size()) {
				continue;
			}
			values[count++] = {
				static_cast<std::uint32_t>(effect->baseEffect->data.primaryAV),
				EffectMagnitude(*effect, a_magnitudeOverride),
			};
		}
		if (count == 0u) {
			return plan;
		}
		return PlanExposure(
			_combatState.statusLedger, a_target->GetFormID(), a_spell->GetFormID(), values, count, StatusClockNowMs());
	}

	EventBridge::SpellEffectSnapshot EventBridge::SnapshotSpellEffectsOn(RE::Actor* a_target, const RE::SpellItem* a_spell)
	{
		SpellEffectSnapshot snapshot{};
		auto* magicTarget = a_target && a_spell ? a_target->AsMagicTarget() : nullptr;
		auto* effects = magicTarget ? magicTarget->GetActiveEffectList() : nullptr;
		if (!effects) {
			return snapshot;
		}
		for (auto* activeEffect : *effects) {
			activeEffect = SanitizeObjectPointer(activeEffect);
			if (activeEffect && activeEffect->spell == a_spell) {
				snapshot.entries.push_back({ activeEffect, activeEffect->elapsedSeconds, activeEffect->magnitude });
			}
		}
		return snapshot;
	}

	namespace
	{
		[[nodiscard]] bool HasInstantEffect(const RE::SpellItem* a_spell) noexcept
		{
			for (const auto* effect : a_spell->effects) {
				if (effect && effect->effectItem.duration <= 0) {
					return true;
				}
			}
			return false;
		}

		// Whether the cast left something on the target: an instance that was not
		// there before, or one the cast refreshed. An older cast of the same spell
		// still running does not count.
		// (A template so the bridge's private snapshot type can be passed in.)
		template <class Snapshot>
		[[nodiscard]] bool SpellLandedSince(RE::Actor* a_target, const RE::SpellItem* a_spell, const Snapshot& a_before)
		{
			auto* magicTarget = a_target ? a_target->AsMagicTarget() : nullptr;
			auto* effects = magicTarget ? magicTarget->GetActiveEffectList() : nullptr;
			if (!effects) {
				return false;
			}
			for (auto* activeEffect : *effects) {
				activeEffect = SanitizeObjectPointer(activeEffect);
				if (!activeEffect || activeEffect->spell != a_spell ||
					activeEffect->flags.any(RE::ActiveEffect::Flag::kDispelled)) {
					continue;
				}
				const auto before = std::find_if(a_before.entries.begin(), a_before.entries.end(), [&](const auto& a_entry) {
					return a_entry.effect == activeEffect;
				});
				if (before == a_before.entries.end() || activeEffect->elapsedSeconds < before->elapsed ||
					activeEffect->magnitude != before->magnitude) {
					return true;
				}
			}
			return false;
		}
	}

	void EventBridge::FinishStatusCast(
		StatusKind a_kind, const ExposurePlan& a_plan, const SpellEffectSnapshot& a_before, const RE::SpellItem* a_spell,
		RE::Actor* a_target, float a_magnitudeOverride)
	{
		if (a_kind != StatusKind::kExposed) {
			RecordStatusApplication(a_kind, a_spell, a_target, a_magnitudeOverride);
			return;
		}
		if (!a_spell || !a_target) {
			return;
		}
		// Absorbed, reflected or fully resisted: nothing landed, so the weaker
		// Exposed already there stays and nothing is recorded.
		if (!SpellLandedSince(a_target, a_spell, a_before)) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: Exposed did not land (spell={}, target={}).", a_spell->GetName(), a_target->GetName());
			}
			return;
		}
		const auto target = a_target->GetFormID();
		for (std::size_t i = 0; i < a_plan.replacedCount; ++i) {
			const auto& old = a_plan.replaced[i];
			DispelSpellEffectsOn(a_target, old.spellFormID, static_cast<RE::ActorValue>(old.actorValue));
			_combatState.statusLedger.EraseSpell(target, old.spellFormID, old.actorValue);
		}
		const auto keptStronger = [&](std::uint32_t a_value) {
			return std::find(a_plan.keepStronger.begin(), a_plan.keepStronger.begin() + a_plan.keepStrongerCount, a_value) !=
			       a_plan.keepStronger.begin() + a_plan.keepStrongerCount;
		};
		for (std::size_t i = 0; i < a_plan.keepStrongerCount; ++i) {
			// A stronger Exposed stays on this value: drop the new spell's part.
			DispelSpellEffectsOn(a_target, a_spell->GetFormID(), static_cast<RE::ActorValue>(a_plan.keepStronger[i]));
		}
		const auto now = StatusClockNowMs();
		for (const auto* effect : a_spell->effects) {
			if (!effect || !effect->baseEffect || effect->effectItem.duration <= 0) {
				continue;
			}
			const auto value = static_cast<std::uint32_t>(effect->baseEffect->data.primaryAV);
			if (keptStronger(value)) {
				continue;
			}
			_combatState.statusLedger.Record({
				.target = target,
				.kind = StatusKind::kExposed,
				.spellFormID = a_spell->GetFormID(),
				.actorValue = value,
				.magnitude = EffectMagnitude(*effect, a_magnitudeOverride),
				.expiresAtMs = now + static_cast<std::int64_t>(effect->effectItem.duration) * 1000,
			});
		}
	}

	void EventBridge::RebuildExposureLedgerFromLiveEffects()
	{
		// The ledger is runtime memory, but the engine saves the Exposed effects
		// themselves. After a load, record what is still on each nearby actor so a
		// weaker cut cannot stack on a stronger one that survived the save.
		std::vector<const RE::SpellItem*> exposedSpells;
		for (const auto& affix : _affixRuntimeState.affixes) {
			if (affix.action.statusTag != StatusKind::kExposed) {
				continue;
			}
			for (const auto* spell : { affix.action.spell, affix.action.adaptiveFire, affix.action.adaptiveFrost, affix.action.adaptiveShock }) {
				if (spell && std::find(exposedSpells.begin(), exposedSpells.end(), spell) == exposedSpells.end()) {
					exposedSpells.push_back(spell);
				}
			}
		}
		auto* processLists = RE::ProcessLists::GetSingleton();
		if (exposedSpells.empty() || !processLists) {
			return;
		}
		const auto now = StatusClockNowMs();
		std::size_t recorded = 0u;
		processLists->ForEachHighActor([&](RE::Actor& a_actor) {
			auto* magicTarget = a_actor.IsDead() ? nullptr : a_actor.AsMagicTarget();
			auto* effects = magicTarget ? magicTarget->GetActiveEffectList() : nullptr;
			if (!effects) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			for (auto* activeEffect : *effects) {
				activeEffect = SanitizeObjectPointer(activeEffect);
				if (!activeEffect || !activeEffect->spell || activeEffect->duration <= 0.0f ||
					activeEffect->flags.any(RE::ActiveEffect::Flag::kDispelled) || !activeEffect->effect ||
					!activeEffect->effect->baseEffect ||
					std::find(exposedSpells.begin(), exposedSpells.end(), activeEffect->spell) == exposedSpells.end()) {
					continue;
				}
				const float remaining = activeEffect->duration - activeEffect->elapsedSeconds;
				if (remaining <= 0.0f) {
					continue;
				}
				_combatState.statusLedger.Record({
					.target = a_actor.GetFormID(),
					.kind = StatusKind::kExposed,
					.spellFormID = activeEffect->spell->GetFormID(),
					.actorValue = static_cast<std::uint32_t>(activeEffect->effect->baseEffect->data.primaryAV),
					.magnitude = std::abs(activeEffect->magnitude),
					.expiresAtMs = now + static_cast<std::int64_t>(remaining * 1000.0f),
				});
				++recorded;
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});
		if (_loot.debugLog && recorded > 0u) {
			SKSE::log::debug("CalamityAffixes: restored {} Exposed entries from live effects after load.", recorded);
		}
	}

	void EventBridge::NoteDebugTestEvent(std::string_view a_key) const
	{
		if (_loot.debugLog || _loot.debugHudNotifications) {
			if (_debugTestEvents.find(a_key) == _debugTestEvents.end()) {
				_debugTestEvents.emplace(a_key);
			}
		}
	}

	void EventBridge::NoteDebugTestEvent(std::string_view a_prefix, std::string_view a_name) const
	{
		// Builds the key only when it is recorded: proc paths call this on every
		// cast, and debug logging is normally off.
		if (_loot.debugLog || _loot.debugHudNotifications) {
			std::string key;
			key.reserve(a_prefix.size() + a_name.size());
			key.append(a_prefix).append(a_name);
			NoteDebugTestEvent(key);
		}
	}

	void EventBridge::NoteDebugTestEventLocked(std::string_view a_key)
	{
		const std::scoped_lock lock(_stateMutex);
		NoteDebugTestEvent(a_key);
	}

	void EventBridge::NoteCorpseHitSkipped()
	{
		const std::scoped_lock lock(_stateMutex);
		NoteDebugTestEvent("corpse:skipped");
	}

	void EventBridge::KeepDyingTargetDoom(RE::FormID a_target)
	{
		const auto pending = _combatState.doomQueue.Find(a_target);
		if (!pending) {
			return;
		}
		std::uint32_t markSpellFormID = 0u;
		std::array<StatusEntry, 16> statuses{};
		const auto now = StatusClockNowMs();
		const auto count = _combatState.statusLedger.Collect(a_target, now, statuses);
		for (std::size_t i = 0; i < count; ++i) {
			if (statuses[i].kind == StatusKind::kDoom) {
				markSpellFormID = statuses[i].spellFormID;
				break;
			}
		}
		if (markSpellFormID == 0u) {
			if (const auto idxIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(pending->affixToken);
				idxIt != _affixRuntimeState.affixRegistry.affixIndexByToken.end() &&
				idxIt->second < _affixRuntimeState.affixes.size()) {
				if (auto* mark = _affixRuntimeState.affixes[idxIt->second].action.doomMarkSpell) {
					markSpellFormID = mark->GetFormID();
				}
			}
		}
		_combatState.dyingDoom.Keep(*pending, markSpellFormID, now);
		_combatState.doomQueue.Remove(a_target);
		_combatState.hasPendingDoom.store(!_combatState.doomQueue.Empty(), std::memory_order_release);
	}

	bool EventBridge::HasSpreadableStatus(RE::Actor* a_victim) const
	{
		if (!a_victim) {
			return false;
		}
		const auto victim = a_victim->GetFormID();
		const auto now = StatusClockNowMs();
		const bool hasDoom = _combatState.doomQueue.Find(victim).has_value() ||
			_combatState.dyingDoom.Find(victim, now).has_value();
		if (hasDoom) {
			return true;
		}
		// Same rules as ExecuteSpreadStatusAction: Doom passes only while a burst
		// waits, and a spell with an instant part (Mist) is never passed on.
		std::array<StatusEntry, 16> statuses{};
		const auto count = _combatState.statusLedger.Collect(victim, now, statuses);
		for (std::size_t i = 0; i < count; ++i) {
			const auto kind = statuses[i].kind;
			if (kind == StatusKind::kBurning || IsBuildUpStatus(kind) || kind == StatusKind::kDoom) {
				continue;
			}
			const auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(statuses[i].spellFormID);
			if (!spell || HasInstantEffect(spell)) {
				continue;
			}
			return true;
		}
		// A meter that just paid off is empty: nothing of it is left to pass on.
		for (const auto kind : { StatusKind::kFreeze, StatusKind::kBleed, StatusKind::kShock }) {
			if (_combatState.statusMeters.Value(victim, kind, now) > 0.0f) {
				return true;
			}
		}
		return _combatState.statusMeters.BurningStacks(victim, now) > 0u;
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
			// A DLL newer than the plugin: say it once, not on every payoff.
			static bool logged = false;
			if (!logged) {
				logged = true;
				SKSE::log::error("CalamityAffixes: {} payoff spell missing from the plugin.", StatusName(a_kind));
			}
			return;
		}
		switch (a_kind) {
		case StatusKind::kFreeze:
			CastHostileOnlySpellImmediate(a_magicCaster, payoff, false, a_target, 1.0f, 0.0f, a_owner);
			break;
		case StatusKind::kBleed:
			{
				const float maxHealth = ResolveActorValueMax(a_target, RE::ActorValue::kHealth);
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
		NoteDebugTestEvent(std::string("payoff:").append(StatusName(a_kind)));
		if (_loot.debugHudNotifications) {
			// Payoffs are hard to tell apart in a busy fight; name them on the HUD.
			// HUD text stays English: the English HUD font has no Hangul.
			std::string note = "CAFF status: ";
			note.append(StatusName(a_kind)).append(" -> ").append(a_target->GetName());
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
			static bool logged = false;
			if (!logged) {
				logged = true;
				SKSE::log::error("CalamityAffixes: Burning spell missing from the plugin.");
			}
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
			// fires on-hit affixes. The guard only covers synchronous re-entry; a
			// kill the burst causes still arrives later as TESDeathEvent(dead=true)
			// and counts as a player kill (like DoT and trap kills).
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
		auto statusCount = _combatState.statusLedger.Collect(victim, now, statuses);
		auto pendingDoom = _combatState.doomQueue.Find(victim);
		// Killed before the burst: the Doom kept at dead=false (its ledger entry
		// may have expired with the burst time by now).
		if (const auto dying = _combatState.dyingDoom.Find(victim, now)) {
			if (!pendingDoom) {
				pendingDoom = dying->doom;
			}
			const bool hasDoomEntry = std::any_of(statuses.begin(), statuses.begin() + statusCount, [](const StatusEntry& a_entry) {
				return a_entry.kind == StatusKind::kDoom;
			});
			if (!hasDoomEntry && dying->markSpellFormID != 0u && statusCount < statuses.size()) {
				statuses[statusCount++] = StatusEntry{
					.target = victim,
					.kind = StatusKind::kDoom,
					.spellFormID = dying->markSpellFormID,
					.actorValue = 0u,
					.magnitude = dying->doom.magnitude,
					.expiresAtMs = now + 1,
				};
			}
		}
		const auto meters = _combatState.statusMeters.Take(victim, now);
		_combatState.statusLedger.EraseTarget(victim);
		_combatState.doomQueue.Remove(victim);
		_combatState.dyingDoom.Forget(victim);
		const bool hasMeters = meters.burningStacks > 0u ||
			std::ranges::any_of(meters.values, [](float a_value) { return a_value > 0.0f; });
		if (statusCount == 0u && !hasMeters) {
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
				// Contagion passes on statuses, not hits: a spell with an instant part
				// (Mist's magicka burn) would land that part again on every neighbour.
				if (HasInstantEffect(spell)) {
					continue;
				}
				// Keep the victim's strength for single-effect spells (scaled casts carry
				// an override). An override would flatten every effect of a multi-effect
				// spell to one value, so those are cast at their own magnitudes.
				const float overrideMagnitude = spell->effects.size() == 1u ? status.magnitude : 0.0f;
				ExposurePlan plan{};
				plan.improves = true;
				if (status.kind == StatusKind::kExposed) {
					plan = PlanExposureCast(spell, target, overrideMagnitude);
					if (!plan.improves) {
						continue;
					}
				}
				const auto before = status.kind == StatusKind::kExposed ? SnapshotSpellEffectsOn(target, spell) : SpellEffectSnapshot{};
				CastHostileOnlySpellImmediate(magicCaster, spell, true, target, 1.0f, overrideMagnitude, a_owner);
				FinishStatusCast(status.kind, plan, before, spell, target, overrideMagnitude);
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
