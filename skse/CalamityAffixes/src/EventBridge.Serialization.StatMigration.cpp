#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/PluginEditorIds.h"
#include "CalamityAffixes/PointerSafety.h"
#include "CalamityAffixes/StaleStatMigration.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
		namespace Av = StaleStatMigration::Av;
		static_assert(Av::kMagickaRate == static_cast<std::uint32_t>(RE::ActorValue::kMagickaRate));
		static_assert(Av::kBowSpeedBonus == static_cast<std::uint32_t>(RE::ActorValue::kBowSpeedBonus));
		static_assert(Av::kWeaponSpeedMult == static_cast<std::uint32_t>(RE::ActorValue::kWeaponSpeedMult));
		static_assert(Av::kShoutRecoveryMult == static_cast<std::uint32_t>(RE::ActorValue::kShoutRecoveryMult));
		static_assert(Av::kOneHandedModifier == static_cast<std::uint32_t>(RE::ActorValue::kOneHandedModifier));
		static_assert(Av::kTwoHandedModifier == static_cast<std::uint32_t>(RE::ActorValue::kTwoHandedModifier));
		static_assert(Av::kBlockModifier == static_cast<std::uint32_t>(RE::ActorValue::kBlockModifier));
		static_assert(Av::kHeavyArmorModifier == static_cast<std::uint32_t>(RE::ActorValue::kHeavyArmorModifier));
		static_assert(Av::kAlterationModifier == static_cast<std::uint32_t>(RE::ActorValue::kAlterationModifier));
		static_assert(Av::kConjurationModifier == static_cast<std::uint32_t>(RE::ActorValue::kConjurationModifier));
		static_assert(Av::kDestructionModifier == static_cast<std::uint32_t>(RE::ActorValue::kDestructionModifier));
		static_assert(Av::kIllusionModifier == static_cast<std::uint32_t>(RE::ActorValue::kIllusionModifier));
		static_assert(Av::kRestorationModifier == static_cast<std::uint32_t>(RE::ActorValue::kRestorationModifier));
		static_assert(Av::kEnchantingModifier == static_cast<std::uint32_t>(RE::ActorValue::kEnchantingModifier));
		static_assert(Av::kAttackDamageMult == static_cast<std::uint32_t>(RE::ActorValue::kAttackDamageMult));

		struct FoundEffect
		{
			RE::ActiveEffect* effect{ nullptr };
			const StaleStatMigration::Entry* entry{ nullptr };
			bool inactive{ false };
			// The base effect as loaded now still modifies the old actor value.
			bool baseOnOldActorValue{ false };
			// Its old amount counts toward the old actor value's removal.
			bool oldFormat{ false };
		};

		[[nodiscard]] bool IsValueModifierArchetype(RE::EffectSetting::Archetype a_archetype) noexcept
		{
			using Archetype = RE::EffectSetting::Archetype;
			return a_archetype == Archetype::kValueModifier || a_archetype == Archetype::kPeakValueModifier ||
			       a_archetype == Archetype::kDualValueModifier;
		}

		// What a live effect of this base effect adds to a_actorValue per point
		// of magnitude (0 when it does not modify it).
		[[nodiscard]] float ActorValueWeight(const RE::EffectSetting* a_baseEffect, RE::ActorValue a_actorValue) noexcept
		{
			if (!a_baseEffect || !IsValueModifierArchetype(a_baseEffect->GetArchetype())) {
				return 0.0f;
			}
			float weight = 0.0f;
			if (a_baseEffect->data.primaryAV == a_actorValue) {
				weight += 1.0f;
			}
			if (a_baseEffect->GetArchetype() == RE::EffectSetting::Archetype::kDualValueModifier &&
				a_baseEffect->data.secondaryAV == a_actorValue) {
				weight += a_baseEffect->data.secondAVWeight;
			}
			return a_baseEffect->IsDetrimental() ? -weight : weight;
		}

		// What the active value-modifier effects on the player account for in
		// a_actorValue's modifiers right now.
		[[nodiscard]] float ExplainedByLiveEffects(RE::BSSimpleList<RE::ActiveEffect*>& a_effects, RE::ActorValue a_actorValue)
		{
			float explained = 0.0f;
			for (auto* activeEffect : a_effects) {
				activeEffect = SanitizeObjectPointer(activeEffect);
				if (!activeEffect ||
					activeEffect->flags.any(RE::ActiveEffect::Flag::kDispelled, RE::ActiveEffect::Flag::kInactive) ||
					!activeEffect->effect || !activeEffect->effect->baseEffect) {
					continue;
				}
				explained += activeEffect->magnitude * ActorValueWeight(activeEffect->effect->baseEffect, a_actorValue);
			}
			return explained;
		}

		[[nodiscard]] RE::ACTOR_VALUE_MODIFIER ToModifier(StaleStatMigration::Slot a_slot) noexcept
		{
			return a_slot == StaleStatMigration::Slot::kPermanent ? RE::ACTOR_VALUE_MODIFIER::kPermanent :
			                                                        RE::ACTOR_VALUE_MODIFIER::kTemporary;
		}

		[[nodiscard]] const char* SlotName(StaleStatMigration::Slot a_slot) noexcept
		{
			switch (a_slot) {
			case StaleStatMigration::Slot::kTemporary:
				return "temporary";
			case StaleStatMigration::Slot::kPermanent:
				return "permanent";
			default:
				return "none";
			}
		}

		// Zero what the loaded effect would take off when it finishes: it never
		// added anything to the actor value it points at now.
		void NeutralizeLoadedEffect(RE::ActiveEffect* a_effect)
		{
			a_effect->magnitude = 0.0f;
			if (auto* valueEffect = skyrim_cast<RE::ValueModifierEffect*>(a_effect)) {
				valueEffect->value = 0.0f;
			}
		}
	}

	void EventBridge::MaybeMigrateStaleStatModifiers()
	{
		if (_staleStatModifiersMigrated || !_configLoaded) {
			// Without a config the runtime would not add back the abilities this
			// removes; nothing else touches them then, so wait for a load that has one.
			return;
		}
		if (!PluginEditorIds::IsIndexed()) {
			// Without the index nothing can be recognized; try again on a later load.
			SKSE::log::warn("CalamityAffixes: stale stat migration postponed ({} is not indexed).", PluginEditorIds::kPluginName);
			return;
		}
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* magicTarget = player ? player->AsMagicTarget() : nullptr;
		auto* effects = magicTarget ? magicTarget->GetActiveEffectList() : nullptr;
		if (!player || !effects) {
			return;
		}

		std::vector<FoundEffect> found;
		std::unordered_set<std::string_view> foundEditorIds;
		for (auto* activeEffect : *effects) {
			activeEffect = SanitizeObjectPointer(activeEffect);
			if (!activeEffect || activeEffect->flags.any(RE::ActiveEffect::Flag::kDispelled) ||
				!activeEffect->effect || !activeEffect->effect->baseEffect) {
				continue;
			}
			const auto editorId = PluginEditorIds::OwnEditorIdOf(activeEffect->effect->baseEffect);
			if (editorId.empty()) {
				continue;
			}
			if (const auto* entry = StaleStatMigration::FindEntry(editorId, activeEffect->magnitude)) {
				found.push_back(FoundEffect{
					.effect = activeEffect,
					.entry = entry,
					.inactive = activeEffect->flags.any(RE::ActiveEffect::Flag::kInactive),
					.baseOnOldActorValue = ActorValueWeight(
											   activeEffect->effect->baseEffect,
											   static_cast<RE::ActorValue>(entry->oldActorValue)) != 0.0f,
				});
				foundEditorIds.insert(entry->magicEffectEditorId);
			}
		}

		// Which found effects are old ones whose amount is still on the old
		// actor value. Entries whose magnitude did not change need the old actor
		// value to hold the amount beyond what live effects explain.
		for (auto& item : found) {
			const auto& entry = *item.entry;
			if (entry.handling == StaleStatMigration::Handling::kRefresh) {
				continue;
			}
			const auto actorValue = static_cast<RE::ActorValue>(entry.oldActorValue);
			const float total = player->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, actorValue) +
			                    player->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kPermanent, actorValue);
			const float explained = entry.magnitudeUnchanged ? ExplainedByLiveEffects(*effects, actorValue) : 0.0f;
			item.oldFormat = StaleStatMigration::IsClearlyOldFormat(entry, item.baseOnOldActorValue, total, explained);
			if (!item.oldFormat) {
				SKSE::log::info(
					"CalamityAffixes: stale stat migration will not take {} off actor value {} for {} ({}; modifiers {:.3f}, live effects {:.3f}).",
					entry.oldMagnitude,
					entry.oldActorValue,
					entry.magicEffectEditorId,
					item.baseOnOldActorValue ? "its base effect still modifies it" : "not clearly an old effect",
					total,
					explained);
			}
		}

		struct Pending
		{
			float expected{ 0.0f };
			float orphan{ 0.0f };
		};
		std::map<std::uint32_t, Pending> pendingByActorValue;
		for (const auto& item : found) {
			if (item.entry->handling != StaleStatMigration::Handling::kRefresh && item.oldFormat) {
				pendingByActorValue[item.entry->oldActorValue].expected += item.entry->oldMagnitude;
			}
		}
		for (const auto& entry : StaleStatMigration::kEntries) {
			if (entry.orphanSweep && !foundEditorIds.contains(entry.magicEffectEditorId)) {
				pendingByActorValue[entry.oldActorValue].orphan += entry.oldMagnitude;
			}
		}

		// Decide per old actor value first. A value whose stale amount is not
		// there is left alone: something else already moved it, and guessing
		// would take off more than the old effect added. The effects themselves
		// are still neutralized and refreshed below.
		std::map<std::uint32_t, StaleStatMigration::Removal> removals;
		for (const auto& [actorValueIndex, pending] : pendingByActorValue) {
			const auto actorValue = static_cast<RE::ActorValue>(actorValueIndex);
			const StaleStatMigration::SlotValues slots{
				.temporaryModifier = player->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, actorValue),
				.permanentModifier = player->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kPermanent, actorValue),
			};
			const auto removal = StaleStatMigration::ResolveRemoval(slots, pending.expected, pending.orphan);
			if (removal.slot == StaleStatMigration::Slot::kNone) {
				if (pending.expected > 0.0f) {
					SKSE::log::warn(
						"CalamityAffixes: stale stat migration left actor value {} alone (expected {:.3f}, temporary {:.3f}, permanent {:.3f}).",
						actorValueIndex,
						pending.expected,
						slots.temporaryModifier,
						slots.permanentModifier);
				}
				continue;
			}
			removals.emplace(actorValueIndex, removal);
		}

		// Two passes: zero every effect first, then remove. Removing an ability
		// can free its other effects, so nothing found may be touched after the
		// first removal.
		std::vector<RE::ActiveEffect*> toRemove;
		std::uint32_t refreshed = 0u;
		std::uint32_t neutralized = 0u;
		for (const auto& item : found) {
			const auto& entry = *item.entry;
			if (entry.handling == StaleStatMigration::Handling::kRefresh || item.baseOnOldActorValue) {
				// Same actor value: its own Finish takes the old amount off.
				toRemove.push_back(item.effect);
				++refreshed;
				SKSE::log::info(
					"CalamityAffixes: stale stat migration refreshes {} (old magnitude {:.3f}).",
					entry.magicEffectEditorId,
					entry.oldMagnitude);
				continue;
			}
			// A moved effect's Finish would take its amount off the new actor
			// value, which it never added to: always zero it and let the runtime
			// add the current one, whether or not the old amount comes off.
			if (entry.handling == StaleStatMigration::Handling::kMovedValue) {
				NeutralizeLoadedEffect(item.effect);
				toRemove.push_back(item.effect);
				++neutralized;
			}
			SKSE::log::info(
				"CalamityAffixes: stale stat migration found {} (old actor value {}, old magnitude {:.3f}{}{}{}).",
				entry.magicEffectEditorId,
				entry.oldActorValue,
				entry.oldMagnitude,
				entry.handling == StaleStatMigration::Handling::kMovedValue ? ", removed" : ", kept as carrier",
				item.oldFormat && removals.contains(entry.oldActorValue) ? "" : ", old actor value left alone",
				item.inactive ? ", was inactive" : "");
		}

		// Resolve the spells while every effect is still alive, then remove.
		std::vector<RE::ActiveEffect*> timedEffects;
		std::vector<RE::SpellItem*> abilities;
		for (auto* effect : toRemove) {
			auto* spell = effect->spell ? effect->spell->As<RE::SpellItem>() : nullptr;
			if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kAbility) {
				if (std::find(abilities.begin(), abilities.end(), spell) == abilities.end()) {
					abilities.push_back(spell);
				}
			} else {
				timedEffects.push_back(effect);
			}
		}
		for (auto* effect : timedEffects) {
			effect->Dispel(true);
		}
		for (auto* spell : abilities) {
			// The runtime adds it back with the current data at RebuildActiveCounts.
			player->RemoveSpell(spell);
		}

		auto* avOwner = player->AsActorValueOwner();
		for (const auto& [actorValueIndex, removal] : removals) {
			if (!avOwner) {
				break;
			}
			const auto actorValue = static_cast<RE::ActorValue>(actorValueIndex);
			const auto modifier = ToModifier(removal.slot);
			const float before = player->GetActorValueModifier(modifier, actorValue);
			avOwner->RestoreActorValue(modifier, actorValue, -removal.amount);
			const float after = player->GetActorValueModifier(modifier, actorValue);
			SKSE::log::info(
				"CalamityAffixes: stale stat migration took {:.3f} off actor value {} ({} modifier {:.3f} -> {:.3f}{}).",
				removal.amount,
				actorValueIndex,
				SlotName(removal.slot),
				before,
				after,
				removal.includesOrphan ? ", includes a dropped timed buff" : "");
			if (StaleStatMigration::Abs((before - after) - removal.amount) > 0.01f) {
				SKSE::log::warn(
					"CalamityAffixes: stale stat migration on actor value {} changed it by {:.3f} instead of {:.3f}.",
					actorValueIndex,
					before - after,
					removal.amount);
			}
		}

		_staleStatModifiersMigrated = true;
		SKSE::log::info(
			"CalamityAffixes: stale stat migration done ({} old effects, {} removed, {} refreshed, {} actor values cleaned).",
			found.size(),
			neutralized,
			refreshed,
			removals.size());
	}
}
