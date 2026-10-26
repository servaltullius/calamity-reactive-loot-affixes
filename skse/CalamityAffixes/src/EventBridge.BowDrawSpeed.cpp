#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/BowDrawSpeed.h"
#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/PluginEditorIds.h"

#include <array>
#include <cmath>
#include <string_view>

namespace CalamityAffixes
{
	namespace
	{
		// The no-op effects that carry a bow draw speed bonus in percent. Their
		// editor IDs predate the suffix's rename and stay for FormID stability.
		constexpr std::array<std::string_view, 4> kBowDrawSpeedCarriers{
			"CAFF_MGEF_SUFFIX_SHOUT_COOLDOWN_T1",
			"CAFF_MGEF_SUFFIX_SHOUT_COOLDOWN_T2",
			"CAFF_MGEF_SUFFIX_SHOUT_COOLDOWN_T3",
			"CAFF_MGEF_RW_ZEPHYR_RUSH",
		};

		[[nodiscard]] bool IsBowDrawSpeedCarrier(const RE::EffectSetting* a_effect) noexcept
		{
			if (!a_effect) {
				return false;
			}
			const auto editorId = PluginEditorIds::OwnEditorIdOf(a_effect);
			for (const auto carrier : kBowDrawSpeedCarriers) {
				if (editorId == carrier) {
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] bool SpellCarriesBowDrawSpeed(const RE::SpellItem* a_spell) noexcept
		{
			if (!a_spell) {
				return false;
			}
			for (const auto* effect : a_spell->effects) {
				if (effect && IsBowDrawSpeedCarrier(effect->baseEffect)) {
					return true;
				}
			}
			return false;
		}

		// The bonus as a fraction, from the carrier effects active on the actor.
		[[nodiscard]] float ActiveBowDrawSpeedBonus(RE::Actor* a_actor)
		{
			auto* magicTarget = a_actor ? a_actor->AsMagicTarget() : nullptr;
			auto* effects = magicTarget ? magicTarget->GetActiveEffectList() : nullptr;
			if (!effects) {
				return 0.0f;
			}
			float percent = 0.0f;
			for (auto* activeEffect : *effects) {
				activeEffect = SanitizeObjectPointer(activeEffect);
				if (!activeEffect || activeEffect->flags.any(RE::ActiveEffect::Flag::kDispelled, RE::ActiveEffect::Flag::kInactive) ||
					!activeEffect->effect || !IsBowDrawSpeedCarrier(activeEffect->effect->baseEffect)) {
					continue;
				}
				percent += activeEffect->magnitude;
			}
			return percent > 0.0f ? percent / 100.0f : 0.0f;
		}

		[[nodiscard]] bool HasBowDrawn(RE::Actor* a_actor)
		{
			if (!a_actor) {
				return false;
			}
			// A bow takes both hands; check either slot the engine keeps it in.
			for (const bool leftHand : { false, true }) {
				auto* object = a_actor->GetEquippedObject(leftHand);
				if (HitDataUtil::IsBowOrCrossbow(object ? object->As<RE::TESObjectWEAP>() : nullptr)) {
					return true;
				}
			}
			return false;
		}
	}

	void EventBridge::RefreshBowDrawSpeedWanted()
	{
		bool wanted = false;
		for (std::size_t i = 0; i < _affixRuntimeState.affixes.size() && !wanted; ++i) {
			if (i >= _affixRuntimeState.activeCounts.size() || _affixRuntimeState.activeCounts[i] == 0) {
				continue;
			}
			const auto& affix = _affixRuntimeState.affixes[i];
			wanted = SpellCarriesBowDrawSpeed(affix.passiveSpell) || SpellCarriesBowDrawSpeed(affix.action.spell);
		}
		_bowDrawSpeed.wanted.store(wanted, std::memory_order_relaxed);
	}

	void EventBridge::SyncBowDrawSpeed()
	{
		const std::scoped_lock lock(_stateMutex);
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}
		const bool enabled = _configLoaded && _runtimeSettings.enabled.load(std::memory_order_relaxed);
		const float bonus = enabled ? ActiveBowDrawSpeedBonus(player) : 0.0f;
		auto* avOwner = player->AsActorValueOwner();
		if (!avOwner) {
			return;
		}
		const float applied = _bowDrawSpeed.applied;
		// Assumes everything else changes WeaponSpeedMult additively (ModAV,
		// enchantments, abilities, slows). An external ForceActorValue that
		// overwrites it erases our part without telling us, so this baseline and
		// the pre-save removal can drift by that amount until the next load.
		// Deliberately not "reset applied when the value moved": that cannot
		// tell an overwrite from an additive change and would leak our bonus
		// into saves in the common case.
		const float withoutOurs = avOwner->GetActorValue(RE::ActorValue::kWeaponSpeedMult) - applied;
		const float desired = ResolveBowDrawSpeedDelta(withoutOurs, bonus, enabled && HasBowDrawn(player));
		if (std::abs(desired - applied) < kBowDrawSpeedEpsilon) {
			return;
		}
		avOwner->ModActorValue(RE::ActorValue::kWeaponSpeedMult, desired - applied);
		_bowDrawSpeed.applied = desired;
		_bowDrawSpeed.active.store(desired != 0.0f, std::memory_order_relaxed);
		if (desired > 0.0f) {
			NoteDebugTestEvent("bowspeed:applied");
		}
		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: bow draw speed (bonus={:.2f}, weaponSpeedMultWithout={:.3f}, added={:.3f}).",
				bonus,
				withoutOurs,
				desired);
		}
	}

	void EventBridge::RemoveBowDrawSpeed()
	{
		// Before a save: what the runtime added never goes into the save, so a
		// load or an uninstall cannot leave a bow permanently faster.
		const std::scoped_lock lock(_stateMutex);
		if (_bowDrawSpeed.applied != 0.0f) {
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				if (auto* avOwner = player->AsActorValueOwner()) {
					avOwner->ModActorValue(RE::ActorValue::kWeaponSpeedMult, -_bowDrawSpeed.applied);
				}
			}
		}
		_bowDrawSpeed.applied = 0.0f;
		_bowDrawSpeed.active.store(false, std::memory_order_relaxed);
	}

	void EventBridge::ForgetBowDrawSpeed() noexcept
	{
		// A load replaces the player with the saved one, which never carries it.
		_bowDrawSpeed.applied = 0.0f;
		_bowDrawSpeed.active.store(false, std::memory_order_relaxed);
	}
}
