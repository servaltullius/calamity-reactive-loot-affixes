#include "CalamityAffixes/EventBridge.h"

#include "CalamityAffixes/HitDataUtil.h"
#include "CalamityAffixes/Hooks.h"
#include "CalamityAffixes/PrismaTooltip.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <RE/M/Misc.h>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
namespace
{
constexpr std::size_t kDotCooldownMaxEntries = 4096;
constexpr auto kDotCooldownTtl = std::chrono::seconds(30);
constexpr auto kDotCooldownPruneInterval = std::chrono::seconds(10);
}
	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESHitEvent* a_event,
		RE::BSTEventSource<RE::TESHitEvent>*)
	{
		if (!a_event || !a_event->cause || !a_event->target) {
			return RE::BSEventNotifyControl::kContinue;
		}
		if (!_runtimeEnabled) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* aggressor = a_event->cause->As<RE::Actor>();
		auto* target = a_event->target->As<RE::Actor>();
		if (!aggressor || !target) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto now = std::chrono::steady_clock::now();

		if (_loot.debugLog) {
			static std::uint32_t windowCount = 0;
			static auto windowStart = now;
			windowCount += 1;

			if ((now - windowStart) >= std::chrono::seconds(2)) {
				const auto* hitData = HitDataUtil::GetLastHitData(target);
				const auto sourceFormID = hitData && hitData->weapon ?
					hitData->weapon->GetFormID() :
					(hitData && hitData->attackDataSpell ? hitData->attackDataSpell->GetFormID() : 0u);

				SKSE::log::info(
					"CalamityAffixes: TESHitEvent seen (count={}, cause={}, target={}, source=0x{:X}, hasHitData={}, hasWeapon={}, hasSpell={}).",
					windowCount,
					aggressor->GetName(),
					target->GetName(),
					sourceFormID,
					(hitData != nullptr),
					(hitData && hitData->weapon != nullptr),
					(hitData && hitData->attackDataSpell != nullptr));

				windowCount = 0;
				windowStart = now;
			}
		}

		// KID can patch keywords after kDataLoaded; re-sync equipped counts occasionally so procs become active.
		MaybeResyncEquippedAffixes(now);

		// NOTE: Some modlists can override vfunc hooks. If HandleHealthDamage is not routed through our hook
		// for this actor, fall back to TESHitEvent-driven procs so affixes still work.
		const bool hookRouted = CalamityAffixes::Hooks::IsHandleHealthDamageHooked(target);

		if (!hookRouted) {
			if (_loot.debugLog) {
				static bool loggedOnce = false;
				if (!loggedOnce) {
					loggedOnce = true;
					SKSE::log::debug("CalamityAffixes: HandleHealthDamage hook not routed; enabling TESHitEvent fallback.");
				}
			}

			// Outgoing (player-owned).
			if (_configLoaded && IsPlayerOwned(aggressor)) {
				const LastHitKey key{
					.outgoing = true,
					.aggressor = aggressor->GetFormID(),
					.target = target->GetFormID(),
					.source = a_event->source
				};

				if (!ShouldSuppressDuplicateHit(key, now)) {
					const auto* hitData = HitDataUtil::GetLastHitData(target);
					ProcessTrigger(Trigger::kHit, GetPlayerOwner(aggressor), target, hitData);

					if (aggressor->IsPlayerRef() && !_archmageAffixIndices.empty()) {
						auto* source = RE::TESForm::LookupByID<RE::TESForm>(a_event->source);
						auto* spell = source ? source->As<RE::SpellItem>() : nullptr;
						if (spell) {
							ProcessArchmageSpellHit(aggressor, target, spell);
						}
					}
				}
			}

			// Incoming (player hit).
			if (_configLoaded && target->IsPlayerRef()) {
				const LastHitKey key{
					.outgoing = false,
					.aggressor = aggressor->GetFormID(),
					.target = target->GetFormID(),
					.source = a_event->source
				};

				if (!ShouldSuppressDuplicateHit(key, now)) {
					const auto* hitData = HitDataUtil::GetLastHitData(target);
					ProcessTrigger(Trigger::kIncomingHit, target, aggressor, hitData);
				}
			}
		}

		if (IsPlayerOwned(aggressor)) {
			SendModEvent("CalamityAffixes_Hit", target);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESDeathEvent* a_event,
		RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		if (!_configLoaded || !_runtimeEnabled || !a_event || !a_event->dead) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto now = std::chrono::steady_clock::now();

		// Keep event references alive for the entire handler. Using raw pointers directly from
		// TESDeathEvent can race with reference release under heavy combat churn.
		const auto dyingRefHolder = a_event->actorDying;
		const auto killerRefHolder = a_event->actorKiller;

		auto* dyingRef = dyingRefHolder.get();
		auto* killerRef = killerRefHolder.get();
		auto* dying = dyingRef ? dyingRef->As<RE::Actor>() : nullptr;
		if (!dying) {
			return RE::BSEventNotifyControl::kContinue;
		}

		MaybeResyncEquippedAffixes(now);

		// Independent path: player-owned summoned actor death can trigger dedicated summon corpse explosion.
		if (!_summonCorpseExplosionAffixIndices.empty() &&
			dying->IsSummoned() &&
			IsPlayerOwned(dying)) {
			if (auto* summonOwner = GetPlayerOwner(dying); summonOwner) {
				ProcessSummonCorpseExplosionDeath(summonOwner, dying);
			}
		}

		if (!killerRef) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// NOTE:
		// TESDeathEvent::actorKiller is a TESObjectREFR, which can be a Projectile/Hazard/etc (not always an Actor).
		// Attribute kills through ActorCause / shooter so Kill-triggered affixes work for ranged/magic too.
		RE::Actor* killer = killerRef->As<RE::Actor>();
		if (!killer) {
			if (auto* cause = killerRef->GetActorCause()) {
				auto actorAny = cause->actor.get();
				killer = actorAny.get();
			}
		}
		if (!killer) {
			if (auto* proj = killerRef->As<RE::Projectile>()) {
				const auto& rt = proj->GetProjectileRuntimeData();
				if (auto shooterAny = rt.shooter.get(); shooterAny) {
					killer = shooterAny.get()->As<RE::Actor>();
				}
				if (!killer && rt.actorCause) {
					auto actorAny = rt.actorCause->actor.get();
					killer = actorAny.get();
				}
			}
		}

		if (!killer) {
			static auto lastWarnAt = std::chrono::steady_clock::time_point{};
			if (lastWarnAt.time_since_epoch().count() == 0 || (now - lastWarnAt) > std::chrono::seconds(2)) {
				lastWarnAt = now;
				if (_loot.debugLog) {
					spdlog::info(
						"CalamityAffixes: TESDeathEvent killer attribution failed (dying={}, killerRef=0x{:X}); skipping Kill triggers.",
						dying->GetName(),
						killerRef->GetFormID());
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!IsPlayerOwned(killer)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* owner = GetPlayerOwner(killer);
		if (!owner) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Only react to hostile kills (avoid friendly-fire weirdness).
		const bool hostile = owner->IsHostileToActor(dying) || dying->IsHostileToActor(owner);
		if (!hostile) {
			static auto lastInfoAt = std::chrono::steady_clock::time_point{};
			if (lastInfoAt.time_since_epoch().count() == 0 || (now - lastInfoAt) > std::chrono::seconds(2)) {
				lastInfoAt = now;
				if (_loot.debugLog) {
					spdlog::info(
						"CalamityAffixes: Kill ignored (non-hostile) (dying={}, killer={}).",
						dying->GetName(),
						killer->GetName());
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		ProcessTrigger(Trigger::kKill, owner, dying, nullptr);

		if (!_corpseExplosionAffixIndices.empty()) {
			ProcessCorpseExplosionKill(owner, dying);
		}

		SendModEvent("CalamityAffixes_Kill", dying);

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESEquipEvent* a_event,
		RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		if (!_configLoaded || !_runtimeEnabled || _affixes.empty()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!a_event || !a_event->actor) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* actor = a_event->actor.get() ? a_event->actor.get()->As<RE::Actor>() : nullptr;
		if (!actor || !actor->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* item = RE::TESForm::LookupByID<RE::TESForm>(a_event->baseObject);
		if (!item) {
			return RE::BSEventNotifyControl::kContinue;
		}

		RebuildActiveCounts();
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const RE::TESMagicEffectApplyEvent* a_event,
		RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*)
	{
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

		// Treat DoT as “apply/refresh”, not per-tick. Filter by keyword.
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

		_dotObservedMagicEffects.insert(mgef->GetFormID());
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

		const auto now = std::chrono::steady_clock::now();
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

		// Forward a lightweight “player-owned DoT apply/refresh” signal to Papyrus.
		SendModEvent("CalamityAffixes_DotApply", target);

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EventBridge::ProcessEvent(
		const SKSE::ModCallbackEvent* a_event,
		RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const char* eventNameRaw = a_event->eventName.c_str();
		if (!eventNameRaw || !*eventNameRaw) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const std::string_view eventName(eventNameRaw);
		if (eventName == kUiTogglePanelEvent) {
			PrismaTooltip::ToggleControlPanel();
			RE::DebugNotification("Calamity: Prisma panel toggled");
			return RE::BSEventNotifyControl::kContinue;
		}

		if (eventName == kUiSetPanelEvent) {
			const bool open = a_event->numArg > 0.5f;
			PrismaTooltip::SetControlPanelOpen(open);
			RE::DebugNotification(open ? "Calamity: Prisma panel ON" : "Calamity: Prisma panel OFF");
			return RE::BSEventNotifyControl::kContinue;
		}

		if (!_configLoaded) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const char* filterRaw = a_event->strArg.c_str();
		const std::string_view affixFilter = (filterRaw && *filterRaw) ? std::string_view(filterRaw) : std::string_view{};

			if (eventName == kMcmSetEnabledEvent) {
				_runtimeEnabled = (a_event->numArg > 0.5f);
				RE::DebugNotification(_runtimeEnabled ? "Calamity: enabled" : "Calamity: disabled");
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetDebugNotificationsEvent) {
				_loot.debugLog = (a_event->numArg > 0.5f);
				spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
				spdlog::flush_on(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
				RE::DebugNotification(_loot.debugLog ? "Calamity: debug notifications ON" : "Calamity: debug notifications OFF");
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetValidationIntervalEvent) {
				const float seconds = std::clamp(a_event->numArg, 0.0f, 30.0f);
				_equipResync.intervalMs = (seconds <= 0.0f) ? 0u : static_cast<std::uint64_t>(seconds * 1000.0f);
				_equipResync.nextAtMs = 0;
				if (seconds <= 0.0f) {
					RE::DebugNotification("Calamity: periodic validation OFF");
				} else {
					std::string note = "Calamity: validation interval ";
					note += std::to_string(static_cast<int>(seconds));
					note += "s";
					RE::DebugNotification(note.c_str());
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetProcChanceMultEvent) {
				_runtimeProcChanceMult = std::clamp(a_event->numArg, 0.0f, 3.0f);
				std::string note = "Calamity: proc x";
				note += std::to_string(_runtimeProcChanceMult);
				RE::DebugNotification(note.c_str());
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSetDotSafetyAutoDisableEvent) {
				_loot.dotTagSafetyAutoDisable = (a_event->numArg > 0.5f);
				if (!_loot.dotTagSafetyAutoDisable) {
					_dotTagSafetySuppressed = false;
					RE::DebugNotification("Calamity: DotApply auto-disable OFF (warn only)");
				} else {
					if (_loot.dotTagSafetyUniqueEffectThreshold > 0u &&
						_dotObservedMagicEffects.size() > _loot.dotTagSafetyUniqueEffectThreshold) {
						_dotTagSafetySuppressed = true;
						RE::DebugNotification("Calamity: DotApply auto-disabled (safety)");
					} else {
						RE::DebugNotification("Calamity: DotApply auto-disable ON");
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kMcmSpawnTestItemEvent) {
				auto* player = RE::PlayerCharacter::GetSingleton();
				auto* ironSword = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00012EB7);
				if (player && ironSword) {
					player->AddObjectToContainer(ironSword, nullptr, 1, nullptr);
					RE::DebugNotification("Calamity: spawned test item (Iron Sword)");
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!_runtimeEnabled) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (eventName == kManualModeCycleNextEvent) {
				CycleManualModeForEquippedAffixes(+1, affixFilter);
			} else if (eventName == kManualModeCyclePrevEvent) {
				CycleManualModeForEquippedAffixes(-1, affixFilter);
			} else if (eventName == kRunewordBaseNextEvent) {
				CycleRunewordBase(+1);
			} else if (eventName == kRunewordBasePrevEvent) {
				CycleRunewordBase(-1);
			} else if (eventName == kRunewordRecipeNextEvent) {
				CycleRunewordRecipe(+1);
			} else if (eventName == kRunewordRecipePrevEvent) {
				CycleRunewordRecipe(-1);
			} else if (eventName == kRunewordInsertRuneEvent) {
				InsertRunewordRuneIntoSelectedBase();
			} else if (eventName == kRunewordStatusEvent) {
				LogRunewordStatus();
			} else if (eventName == kRunewordGrantNextRuneEvent) {
				const auto amount = (a_event->numArg > 0.0f) ? static_cast<std::uint32_t>(a_event->numArg) : 1u;
				GrantNextRequiredRuneFragment(amount);
			} else if (eventName == kRunewordGrantRecipeSetEvent) {
				const auto amount = (a_event->numArg > 0.0f) ? static_cast<std::uint32_t>(a_event->numArg) : 1u;
				GrantCurrentRecipeRuneSet(amount);
			}

		return RE::BSEventNotifyControl::kContinue;
	}

	bool EventBridge::IsPlayerOwned(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		if (a_actor->IsPlayerRef()) {
			return true;
		}

		// Summon/proxy attribution:
		// If the actor is commanded by the player, treat it as "player-owned".
		auto commandingAny = a_actor->GetCommandingActor();
		auto* commanding = commandingAny.get();

		return commanding && commanding->IsPlayerRef();
	}

	RE::Actor* EventBridge::GetPlayerOwner(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return nullptr;
		}

		if (a_actor->IsPlayerRef()) {
			return a_actor;
		}

		auto commandingAny = a_actor->GetCommandingActor();
		auto* commanding = commandingAny.get();

		return (commanding && commanding->IsPlayerRef()) ? commanding : nullptr;
	}

	void EventBridge::SendModEvent(std::string_view a_eventName, RE::TESForm* a_sender)
	{
		auto* source = SKSE::GetModCallbackEventSource();
		if (!source) {
			return;
		}

		SKSE::ModCallbackEvent event{
			RE::BSFixedString(a_eventName.data()),
			RE::BSFixedString(""),
			0.0f,
			a_sender
		};

		source->SendEvent(&event);
	}

	void EventBridge::MaybeResyncEquippedAffixes(std::chrono::steady_clock::time_point a_now)
	{
		if (!_configLoaded || !_runtimeEnabled) {
			return;
		}

		if (_equipResync.intervalMs == 0u) {
			return;
		}

		const auto nowMs =
			static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(a_now.time_since_epoch()).count());

		if (!_equipResync.ShouldRun(nowMs)) {
			return;
		}

		RebuildActiveCounts();
	}

	void EventBridge::RebuildActiveCounts()
	{
		if (!_configLoaded) {
			return;
		}

		_activeCounts.assign(_affixes.size(), 0);
		_activeSlotPenalty.assign(_affixes.size(), 0.0f);
		_activeCritDamageBonusPct = 0.0f;
		std::unordered_set<RE::SpellItem*> desiredPassives;

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		auto* changes = player->GetInventoryChanges();
		if (!changes || !changes->entryList) {
			return;
		}

		for (auto* entry : *changes->entryList) {
			if (!entry || !entry->extraLists) {
				continue;
			}

			for (auto* xList : *entry->extraLists) {
				if (!xList) {
					continue;
				}

				auto* uid = xList->GetByType<RE::ExtraUniqueID>();
				if (!uid) {
					continue;
				}

				const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
				const auto it = _instanceAffixes.find(key);
				if (it == _instanceAffixes.end()) {
					continue;
				}

				const auto& slots = it->second;
				for (std::uint8_t slot = 0; slot < slots.count; ++slot) {
					if (slots.tokens[slot] != 0u) {
						EnsureInstanceRuntimeState(key, slots.tokens[slot]);
					}
				}

				// Use primary affix for display name
				if (slots.count > 0) {
					if (const auto idxIt = _affixIndexByToken.find(slots.tokens[0]); idxIt != _affixIndexByToken.end() && idxIt->second < _affixes.size()) {
						EnsureMultiAffixDisplayName(entry, xList, slots);
					}
				}

				const bool worn = xList->HasType<RE::ExtraWorn>() || xList->HasType<RE::ExtraWornLeft>();
				if (!worn) {
					continue;
				}

				// Count each unique affix token as active, track best slot penalty
				for (std::uint8_t slot = 0; slot < slots.count; ++slot) {
					const auto idxIt = _affixIndexByToken.find(slots.tokens[slot]);
					if (idxIt == _affixIndexByToken.end()) {
						continue;
					}
					const auto affixIdx = idxIt->second;

					if (affixIdx < _activeCounts.size()) {
						_activeCounts[affixIdx] += 1;
					}

					// Collect passive spells for equipped suffixes
					if (affixIdx < _affixes.size() && _affixes[affixIdx].slot == AffixSlot::kSuffix && _affixes[affixIdx].passiveSpell) {
						desiredPassives.insert(_affixes[affixIdx].passiveSpell);
					}

					// Accumulate crit damage bonus for equipped suffixes
					if (affixIdx < _affixes.size() && _affixes[affixIdx].critDamageBonusPct > 0.0f) {
						_activeCritDamageBonusPct += _affixes[affixIdx].critDamageBonusPct;
					}

					// "Best Slot Wins" penalty only for prefixes
					if (affixIdx < _affixes.size() && _affixes[affixIdx].slot != AffixSlot::kSuffix) {
						const float penalty = kMultiAffixProcPenalty[std::min<std::uint8_t>(slots.count, static_cast<std::uint8_t>(kMaxAffixesPerItem)) - 1];
						if (affixIdx < _activeSlotPenalty.size()) {
							_activeSlotPenalty[affixIdx] = std::max(_activeSlotPenalty[affixIdx], penalty);
						}
					}
				}
			}
		}

		if (_loot.debugLog) {
			std::uint32_t shown = 0;
			for (std::size_t i = 0; i < _affixes.size() && i < _activeCounts.size(); i++) {
				if (_activeCounts[i] == 0) {
					continue;
				}
				spdlog::debug("CalamityAffixes: active affix (id={}, count={})", _affixes[i].id, _activeCounts[i]);
				shown += 1;
				if (shown >= 50) {
					break;
				}
			}
			if (shown == 0) {
				spdlog::debug("CalamityAffixes: active affix list is empty (no equipped affix instances detected).");
			}
		}

		// Apply/remove passive suffix spells
		if (player) {
			// Remove spells no longer needed
			for (auto it = _appliedPassiveSpells.begin(); it != _appliedPassiveSpells.end(); ) {
				if (desiredPassives.find(*it) == desiredPassives.end()) {
					player->RemoveSpell(*it);
					SKSE::log::debug("CalamityAffixes: removed passive suffix spell {:08X}.", (*it)->GetFormID());
					it = _appliedPassiveSpells.erase(it);
				} else {
					++it;
				}
			}
			// Add new spells
			for (auto* spell : desiredPassives) {
				if (_appliedPassiveSpells.find(spell) == _appliedPassiveSpells.end()) {
					player->AddSpell(spell);
					_appliedPassiveSpells.insert(spell);
					SKSE::log::debug("CalamityAffixes: applied passive suffix spell {:08X}.", spell->GetFormID());
				}
			}
		}
	}

}
