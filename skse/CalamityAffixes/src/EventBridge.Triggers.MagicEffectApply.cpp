#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/PlayerOwnership.h"
#include "EventBridge.Triggers.Events.Detail.h"

#include <chrono>
#include <cstdint>

namespace CalamityAffixes
{
	using namespace TriggersDetail;

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESMagicEffectApplyEvent* a_event,
		RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*)
	{
		const auto now = std::chrono::steady_clock::now();
		const std::scoped_lock lock(_stateMutex);
		MaybeFlushRuntimeUserSettings(now, false);

		if (!a_event || !a_event->caster || !a_event->target || !a_event->magicEffect) {
			return RE::BSEventNotifyControl::kContinue;
		}
		if (!_runtimeSettings.enabled) {
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

		// Treat DoT as "apply/refresh", not per-tick. Filter by keyword.
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

		const RE::FormID observedMgefFormId = mgef->GetFormID();
		if (_combatState.dotObservedMagicEffects.size() < kDotObservedMagicEffectsMaxEntries ||
			_combatState.dotObservedMagicEffects.contains(observedMgefFormId)) {
			_combatState.dotObservedMagicEffects.insert(observedMgefFormId);
		} else if (!_combatState.dotObservedMagicEffectsCapWarned) {
			_combatState.dotObservedMagicEffectsCapWarned = true;
			SKSE::log::warn(
				"CalamityAffixes: DotApply observed-magic-effect set reached hard cap ({}). Additional IDs will be ignored until reset.",
				kDotObservedMagicEffectsMaxEntries);
		}
		const auto observedDotEffectCount = _combatState.dotObservedMagicEffects.size();
		if (_loot.dotTagSafetyUniqueEffectThreshold > 0u &&
			observedDotEffectCount > _loot.dotTagSafetyUniqueEffectThreshold) {
			if (!_combatState.dotTagSafetyWarned) {
				_combatState.dotTagSafetyWarned = true;
				SKSE::log::warn(
					"CalamityAffixes: DotApply safety warning (unique tagged magic effects={}, threshold={}, autoDisable={}).",
					observedDotEffectCount,
					_loot.dotTagSafetyUniqueEffectThreshold,
					_loot.dotTagSafetyAutoDisable);
				EmitHudNotification("Calamity: DotApply safety warning (tag spread broad)");
			}

			if (_loot.dotTagSafetyAutoDisable) {
				if (!_combatState.dotTagSafetySuppressed) {
					_combatState.dotTagSafetySuppressed = true;
					SKSE::log::error(
						"CalamityAffixes: DotApply safety auto-disabled (unique tagged magic effects={}, threshold={}).",
						observedDotEffectCount,
						_loot.dotTagSafetyUniqueEffectThreshold);
					EmitHudNotification("Calamity: DotApply safety auto-disabled");
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		}

		if (_combatState.dotTagSafetySuppressed) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Safety: avoid turning broad KID tagging into "any spell cast" proc storms.
		// Treat DotApply as "harmful duration effect apply/refresh", not instant hits or buffs.
		if (!mgef->IsHostile() || mgef->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kNoDuration)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const std::uint64_t key =
			(static_cast<std::uint64_t>(target->GetFormID()) << 32) |
			static_cast<std::uint64_t>(mgef->GetFormID());

		if (const auto it = _combatState.dotCooldowns.find(key); it != _combatState.dotCooldowns.end()) {
			if (now - it->second < kDotApplyICD) {
				return RE::BSEventNotifyControl::kContinue;
			}
		}

		_combatState.dotCooldowns[key] = now;

		if (_combatState.dotCooldowns.size() > kDotCooldownMaxEntries) {
			if (_combatState.dotCooldownsLastPruneAt.time_since_epoch().count() == 0 ||
				(now - _combatState.dotCooldownsLastPruneAt) > kDotCooldownPruneInterval) {
				_combatState.dotCooldownsLastPruneAt = now;

				for (auto it = _combatState.dotCooldowns.begin(); it != _combatState.dotCooldowns.end();) {
					if ((now - it->second) > kDotCooldownTtl) {
						it = _combatState.dotCooldowns.erase(it);
					} else {
						++it;
					}
				}
			}
		}

		if (_configLoaded) {
			ProcessTrigger(Trigger::kDotApply, GetPlayerOwner(caster), target, nullptr);
		}

		// Forward a lightweight "player-owned DoT apply/refresh" signal to Papyrus
		// only when the C++ config is not loaded (fallback mode).  When _configLoaded
		// is true, ProcessTrigger above already handled the proc; sending the ModEvent
		// would cause Papyrus AffixManager to fire a duplicate proc.
		if (!_configLoaded) {
			SendModEvent("CalamityAffixes_DotApply", target);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

}
