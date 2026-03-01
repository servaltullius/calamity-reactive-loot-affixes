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
		if (!_runtimeEnabled) {
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
		if (_dotObservedMagicEffects.size() < kDotObservedMagicEffectsMaxEntries ||
			_dotObservedMagicEffects.contains(observedMgefFormId)) {
			_dotObservedMagicEffects.insert(observedMgefFormId);
		} else if (!_dotObservedMagicEffectsCapWarned) {
			_dotObservedMagicEffectsCapWarned = true;
			SKSE::log::warn(
				"CalamityAffixes: DotApply observed-magic-effect set reached hard cap ({}). Additional IDs will be ignored until reset.",
				kDotObservedMagicEffectsMaxEntries);
		}
		const auto observedDotEffectCount = _dotObservedMagicEffects.size();
		if (_loot.dotTagSafetyUniqueEffectThreshold > 0u &&
			observedDotEffectCount > _loot.dotTagSafetyUniqueEffectThreshold) {
			if (!_dotTagSafetyWarned) {
				_dotTagSafetyWarned = true;
				SKSE::log::warn(
					"CalamityAffixes: DotApply safety warning (unique tagged magic effects={}, threshold={}, autoDisable={}).",
					observedDotEffectCount,
					_loot.dotTagSafetyUniqueEffectThreshold,
					_loot.dotTagSafetyAutoDisable);
				RE::DebugNotification("Calamity: DotApply safety warning (tag spread broad)");
			}

			if (_loot.dotTagSafetyAutoDisable) {
				if (!_dotTagSafetySuppressed) {
					_dotTagSafetySuppressed = true;
					SKSE::log::error(
						"CalamityAffixes: DotApply safety auto-disabled (unique tagged magic effects={}, threshold={}).",
						observedDotEffectCount,
						_loot.dotTagSafetyUniqueEffectThreshold);
					RE::DebugNotification("Calamity: DotApply safety auto-disabled");
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		}

		if (_dotTagSafetySuppressed) {
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

		if (const auto it = _dotCooldowns.find(key); it != _dotCooldowns.end()) {
			if (now - it->second < kDotApplyICD) {
				return RE::BSEventNotifyControl::kContinue;
			}
		}

		_dotCooldowns[key] = now;

		if (_dotCooldowns.size() > kDotCooldownMaxEntries) {
			if (_dotCooldownsLastPruneAt.time_since_epoch().count() == 0 ||
				(now - _dotCooldownsLastPruneAt) > kDotCooldownPruneInterval) {
				_dotCooldownsLastPruneAt = now;

				for (auto it = _dotCooldowns.begin(); it != _dotCooldowns.end();) {
					if ((now - it->second) > kDotCooldownTtl) {
						it = _dotCooldowns.erase(it);
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
