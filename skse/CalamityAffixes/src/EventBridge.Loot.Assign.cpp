#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootEligibility.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/LootSlotSanitizer.h"
#include "CalamityAffixes/LootUiGuards.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
		static constexpr std::size_t kArmorTemplateScanDepth = 8;

		enum class LootCurrencySourceTier : std::uint8_t
		{
			kUnknown = 0,
			kCorpse,
			kContainer,
			kBossContainer,
			kWorld
		};

		[[nodiscard]] bool ContainsAsciiNoCase(std::string_view a_text, std::string_view a_needle)
		{
			if (a_text.empty() || a_needle.empty() || a_needle.size() > a_text.size()) {
				return false;
			}

			for (std::size_t i = 0; i + a_needle.size() <= a_text.size(); ++i) {
				bool match = true;
				for (std::size_t j = 0; j < a_needle.size(); ++j) {
					const auto lhs = static_cast<char>(std::tolower(static_cast<unsigned char>(a_text[i + j])));
					const auto rhs = static_cast<char>(std::tolower(static_cast<unsigned char>(a_needle[j])));
					if (lhs != rhs) {
						match = false;
						break;
					}
				}
				if (match) {
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] bool IsBossContainerEditorId(
			std::string_view a_editorId,
			const std::vector<std::string>& a_allowContains,
			const std::vector<std::string>& a_denyContains)
		{
			if (a_editorId.empty()) {
				return false;
			}

			if (detail::MatchesAnyCaseInsensitiveMarker(a_editorId, a_denyContains)) {
				return false;
			}

			if (!a_allowContains.empty()) {
				return detail::MatchesAnyCaseInsensitiveMarker(a_editorId, a_allowContains);
			}

			// Fallback heuristic when allow-list is intentionally empty.
			return ContainsAsciiNoCase(a_editorId, "boss");
		}

		[[nodiscard]] LootCurrencySourceTier ResolveLootCurrencySourceTier(
			RE::FormID a_oldContainer,
			const std::vector<std::string>& a_bossAllowContains,
			const std::vector<std::string>& a_bossDenyContains)
		{
			if (a_oldContainer == LootRerollGuard::kWorldContainer) {
				return LootCurrencySourceTier::kWorld;
			}

			auto* sourceForm = RE::TESForm::LookupByID<RE::TESForm>(a_oldContainer);
			if (!sourceForm) {
				return LootCurrencySourceTier::kUnknown;
			}

			if (auto* sourceActor = sourceForm->As<RE::Actor>(); sourceActor && sourceActor->IsDead()) {
				return LootCurrencySourceTier::kCorpse;
			}

			auto isBossContainer = [&](const RE::TESForm* a_form) {
				if (!a_form) {
					return false;
				}
				const char* editorIdRaw = a_form->GetFormEditorID();
				return editorIdRaw && IsBossContainerEditorId(editorIdRaw, a_bossAllowContains, a_bossDenyContains);
			};

			if (auto* sourceRef = sourceForm->As<RE::TESObjectREFR>()) {
				if (auto* sourceBase = sourceRef->GetBaseObject(); sourceBase && sourceBase->As<RE::TESObjectCONT>()) {
					if (isBossContainer(sourceRef) || isBossContainer(sourceBase)) {
						return LootCurrencySourceTier::kBossContainer;
					}
					return LootCurrencySourceTier::kContainer;
				}
			}

			if (sourceForm->As<RE::TESObjectCONT>()) {
				return isBossContainer(sourceForm) ? LootCurrencySourceTier::kBossContainer : LootCurrencySourceTier::kContainer;
			}

			return LootCurrencySourceTier::kUnknown;
		}

		[[nodiscard]] std::string_view LootCurrencySourceTierLabel(LootCurrencySourceTier a_tier)
		{
			switch (a_tier) {
			case LootCurrencySourceTier::kCorpse:
				return "corpse";
			case LootCurrencySourceTier::kContainer:
				return "container";
			case LootCurrencySourceTier::kBossContainer:
				return "boss_container";
			case LootCurrencySourceTier::kWorld:
				return "world";
			default:
				return "unknown";
			}
		}

		[[nodiscard]] bool IsCalamityResourceCurrencyItem(const RE::TESBoundObject* a_object) noexcept
		{
			if (!a_object || !a_object->As<RE::TESObjectMISC>()) {
				return false;
			}

			const char* editorIdRaw = a_object->GetFormEditorID();
			if (!editorIdRaw || editorIdRaw[0] == '\0') {
				return false;
			}

			const std::string_view editorId(editorIdRaw);
			return editorId == "CAFF_Misc_ReforgeOrb" || editorId.starts_with("CAFF_RuneFrag_");
		}
	}

	std::uint64_t EventBridge::MakeInstanceKey(RE::FormID a_baseID, std::uint16_t a_uniqueID) noexcept
	{
		return (static_cast<std::uint64_t>(a_baseID) << 16) | static_cast<std::uint64_t>(a_uniqueID);
	}

	bool EventBridge::IsLootArmorEligibleForAffixes(const RE::TESObjectARMO* a_armor) const
	{
		if (!a_armor) {
			return false;
		}

		bool hasSlotMask = false;
		bool hasArmorAddons = false;

		const RE::TESObjectARMO* cursor = a_armor;
		for (std::size_t depth = 0; cursor && depth < kArmorTemplateScanDepth; ++depth) {
			hasSlotMask = hasSlotMask || (cursor->GetSlotMask() != RE::BIPED_MODEL::BipedObjectSlot::kNone);
			hasArmorAddons = hasArmorAddons || !cursor->armorAddons.empty();
			if (!cursor->templateArmor) {
				break;
			}
			if (cursor->templateArmor == cursor) {
				break;
			}
			cursor = cursor->templateArmor;
		}

		const char* editorIdRaw = a_armor->GetFormEditorID();
		const std::string_view editorId = editorIdRaw ? std::string_view(editorIdRaw) : std::string_view{};
		const bool editorIdDenied =
			detail::MatchesAnyCaseInsensitiveMarker(editorId, _loot.armorEditorIdDenyContains);

		const detail::LootArmorEligibilityInput input{
			.playable = a_armor->GetPlayable(),
			.hasSlotMask = hasSlotMask,
			.hasArmorAddons = hasArmorAddons,
			.editorIdDenied = editorIdDenied
		};
		return detail::IsLootArmorEligible(input);
	}

	bool EventBridge::IsLootObjectEligibleForAffixes(const RE::TESBoundObject* a_object) const
	{
		if (!a_object) {
			return false;
		}
		if (a_object->As<RE::TESObjectWEAP>()) {
			return true;
		}
		if (const auto* armo = a_object->As<RE::TESObjectARMO>()) {
			return IsLootArmorEligibleForAffixes(armo);
		}
		return false;
	}

	bool EventBridge::TryClearStaleLootDisplayName(
		RE::InventoryEntryData* a_entry,
		RE::ExtraDataList* a_xList,
		bool a_allowTrailingMarkerClear)
	{
		if (!a_entry || !a_entry->object || !a_xList) {
			return false;
		}

		auto* text = a_xList->GetExtraTextDisplayData();
		if (!text) {
			return false;
		}

		const auto* name = text->GetDisplayName(a_entry->object, 0);
		if (!name || name[0] == '\0') {
			return false;
		}

		const bool hasLeadingMarker = HasLeadingLootStarPrefix(name);
		const bool hasTrailingMarker = HasTrailingLootStarMarker(name);
		if (!hasLeadingMarker && !(a_allowTrailingMarkerClear && hasTrailingMarker)) {
			return false;
		}

		std::string cleaned;
		if (a_allowTrailingMarkerClear && hasTrailingMarker) {
			cleaned = std::string(StripLootStarMarkers(name));
		} else {
			cleaned = std::string(StripLeadingLootStarPrefix(name));
		}
		if (cleaned.empty()) {
			const char* objectNameRaw = a_entry->object->GetName();
			cleaned = objectNameRaw ? objectNameRaw : "";
		}
		if (cleaned.empty() || cleaned == name) {
			return false;
		}

		text->SetName(cleaned.c_str());
		return true;
	}

	void EventBridge::CleanupInvalidLootInstance(
		RE::InventoryEntryData* a_entry,
		RE::ExtraDataList* a_xList,
		std::uint64_t a_key,
		std::string_view a_reason)
	{
		const auto erasedAffixCount = _instanceAffixes.erase(a_key);
		EraseInstanceRuntimeStates(a_key);
		ForgetLootEvaluatedInstance(a_key);
		ForgetLootPreviewSlots(a_key);
		_runewordInstanceStates.erase(a_key);
		if (_runewordSelectedBaseKey && *_runewordSelectedBaseKey == a_key) {
			_runewordSelectedBaseKey.reset();
		}

		const bool renamed = TryClearStaleLootDisplayName(a_entry, a_xList, true);
		if (_loot.debugLog && (erasedAffixCount > 0 || renamed)) {
			SKSE::log::debug(
				"CalamityAffixes: cleaned invalid loot instance (key={:016X}, reason={}, removedAffixes={}, renamed={}).",
				a_key,
				a_reason,
				erasedAffixCount,
				renamed);
		}
	}

	std::optional<EventBridge::LootItemType> EventBridge::ParseLootItemType(std::string_view a_kidType) const
	{
		std::string type{ a_kidType };
		std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (type.find("weapon") != std::string::npos) {
			return LootItemType::kWeapon;
		}
		if (type.find("armor") != std::string::npos) {
			return LootItemType::kArmor;
		}

		return std::nullopt;
	}

	std::string EventBridge::FormatLootName(std::string_view a_baseName, std::string_view a_affixName) const
	{
		std::string out = _loot.nameFormat;

		auto replaceAll = [](std::string& a_text, std::string_view a_from, std::string_view a_to) {
			if (a_from.empty()) {
				return;
			}

			std::size_t pos = 0;
			while ((pos = a_text.find(a_from, pos)) != std::string::npos) {
				a_text.replace(pos, a_from.size(), a_to);
				pos += a_to.size();
			}
		};

		replaceAll(out, "{base}", a_baseName);
		replaceAll(out, "{affix}", a_affixName);
		return out;
	}

	std::optional<std::size_t> EventBridge::RollLootAffixIndex(LootItemType a_itemType, const std::vector<std::size_t>* a_exclude, bool a_skipChanceCheck)
	{
		if (!a_skipChanceCheck && !RollLootChanceGateForEligibleInstance()) {
			return std::nullopt;
		}

		// Build eligible pool, excluding already-chosen indices and disabled entries.
		// EffectiveLootWeight is used as the weighted roll source (>0 eligible, larger = more likely).
		const std::vector<std::size_t>* sourcePool = nullptr;
		if (_loot.sharedPool) {
			sourcePool = &_lootSharedAffixes;
		} else {
			sourcePool = (a_itemType == LootItemType::kWeapon) ? &_lootWeaponAffixes : &_lootArmorAffixes;
		}

		LootShuffleBagState& bagState = _loot.sharedPool ?
			_lootPrefixSharedBag :
			((a_itemType == LootItemType::kWeapon) ? _lootPrefixWeaponBag : _lootPrefixArmorBag);
		const auto picked = detail::SelectWeightedEligibleLootIndexWithShuffleBag(
			_rng,
			*sourcePool,
			bagState.order,
			bagState.cursor,
				[&](std::size_t a_idx) {
					if (a_idx >= _affixes.size()) {
						return false;
					}
					if (_affixes[a_idx].slot == AffixSlot::kSuffix) {
						// Hard gate: prefix roll path must never emit suffix entries.
						return false;
					}
					if (a_exclude && std::find(a_exclude->begin(), a_exclude->end(), a_idx) != a_exclude->end()) {
						return false;
					}
					return _affixes[a_idx].EffectiveLootWeight() > 0.0f;
				},
			[&](std::size_t a_idx) {
				if (a_idx >= _affixes.size()) {
					return 0.0f;
				}
				return _affixes[a_idx].EffectiveLootWeight();
			});
		return picked;
	}

	std::optional<std::size_t> EventBridge::RollSuffixIndex(
		LootItemType a_itemType,
		const std::vector<std::string>* a_excludeFamilies)
	{
		const std::vector<std::size_t>* sourcePool = nullptr;
		if (_loot.sharedPool) {
			sourcePool = &_lootSharedSuffixes;
		} else {
			sourcePool = (a_itemType == LootItemType::kWeapon) ? &_lootWeaponSuffixes : &_lootArmorSuffixes;
		}

		LootShuffleBagState& bagState = _loot.sharedPool ?
			_lootSuffixSharedBag :
			((a_itemType == LootItemType::kWeapon) ? _lootSuffixWeaponBag : _lootSuffixArmorBag);
		const auto picked = detail::SelectWeightedEligibleLootIndexWithShuffleBag(
			_rng,
			*sourcePool,
			bagState.order,
			bagState.cursor,
			[&](std::size_t a_idx) {
				if (a_idx >= _affixes.size()) {
					return false;
				}
				const auto& affix = _affixes[a_idx];
				if (affix.slot != AffixSlot::kSuffix || affix.EffectiveLootWeight() <= 0.0f) {
					return false;
				}
				if (a_excludeFamilies && !affix.family.empty()) {
					if (std::find(a_excludeFamilies->begin(), a_excludeFamilies->end(), affix.family) != a_excludeFamilies->end()) {
						return false;
					}
				}
				return true;
			},
			[&](std::size_t a_idx) {
				if (a_idx >= _affixes.size()) {
					return 0.0f;
				}
				return _affixes[a_idx].EffectiveLootWeight();
			});
		return picked;
	}

	std::uint8_t EventBridge::RollAffixCount()
	{
		std::array<double, kMaxRegularAffixesPerItem> weights{};
		bool hasPositiveWeight = false;
		for (std::size_t i = 0; i < kAffixCountWeights.size(); ++i) {
			const double weight = std::max(0.0, static_cast<double>(kAffixCountWeights[i]));
			weights[i] = weight;
			hasPositiveWeight = hasPositiveWeight || (weight > 0.0);
		}
		if (!hasPositiveWeight) {
			return 1u;
		}

		std::discrete_distribution<unsigned int> dist(weights.begin(), weights.end());
		return static_cast<std::uint8_t>(dist(_rng) + 1);
	}

	bool EventBridge::RollLootChanceGateForEligibleInstance()
	{
		const float chancePct = std::clamp(_loot.chancePercent, 0.0f, 100.0f);
		if (chancePct <= 0.0f) {
			_lootChanceEligibleFailStreak = 0;
			return false;
		}

		if (_lootChanceEligibleFailStreak >= kLootChancePityFailThreshold) {
			_lootChanceEligibleFailStreak = 0;
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: loot chance pity triggered (threshold={} failures).",
					kLootChancePityFailThreshold);
			}
			return true;
		}

		if (chancePct >= 100.0f) {
			_lootChanceEligibleFailStreak = 0;
			return true;
		}

		std::uniform_real_distribution<float> dist(0.0f, 100.0f);
		if (dist(_rng) < chancePct) {
			_lootChanceEligibleFailStreak = 0;
			return true;
		}

		if (_lootChanceEligibleFailStreak < std::numeric_limits<std::uint32_t>::max()) {
			++_lootChanceEligibleFailStreak;
		}
		return false;
	}

	bool EventBridge::SanitizeInstanceAffixSlotsForCurrentLootRules(
		std::uint64_t a_instanceKey,
		InstanceAffixSlots& a_slots,
		std::string_view a_context)
	{
		if (a_slots.count == 0) {
			a_slots.Clear();
			return false;
		}

		std::array<std::uint64_t, kMaxAffixesPerItem> removedTokens{};
		std::uint8_t removedCount = 0;
		const auto sanitized = detail::BuildSanitizedInstanceAffixSlots(
			a_slots,
			[&](std::uint64_t a_token) {
				const auto idxIt = _affixIndexByToken.find(a_token);
				if (idxIt == _affixIndexByToken.end() || idxIt->second >= _affixes.size()) {
					return false;
				}
				if (_loot.stripTrackedSuffixSlots && _affixes[idxIt->second].slot == AffixSlot::kSuffix) {
					return false;
				}
				return true;
			},
			&removedTokens,
			&removedCount);

		auto matchesCurrent = [&]() {
			if (a_slots.count != sanitized.count) {
				return false;
			}
			for (std::uint8_t i = 0; i < a_slots.count; ++i) {
				if (a_slots.tokens[i] != sanitized.tokens[i]) {
					return false;
				}
			}
			return true;
		};
		const bool changed = !matchesCurrent();

		if (!changed) {
			return false;
		}

		a_slots = sanitized;
		for (std::uint8_t i = 0; i < removedCount; ++i) {
			const auto removedToken = removedTokens[i];
			if (removedToken == 0u || a_slots.HasToken(removedToken)) {
				continue;
			}
			_instanceStates.erase(MakeInstanceStateKey(a_instanceKey, removedToken));
		}
		if (a_slots.count == 0) {
			EraseInstanceRuntimeStates(a_instanceKey);
		}

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: sanitized mapped instance slots (context={}, key={:016X}, removed={}, remaining={}).",
				a_context,
				a_instanceKey,
				removedCount,
				a_slots.count);
		}
		return true;
	}

	void EventBridge::SanitizeAllTrackedLootInstancesForCurrentLootRules(std::string_view a_context)
	{
		if (_affixes.empty() || _affixIndexByToken.empty()) {
			return;
		}

		std::uint32_t sanitizedInstances = 0u;
		std::uint32_t erasedInstances = 0u;
		for (auto it = _instanceAffixes.begin(); it != _instanceAffixes.end();) {
			auto& slots = it->second;
			const bool changed = SanitizeInstanceAffixSlotsForCurrentLootRules(it->first, slots, a_context);
			if (changed) {
				++sanitizedInstances;
			}

			if (slots.count == 0) {
				ForgetLootEvaluatedInstance(it->first);
				it = _instanceAffixes.erase(it);
				++erasedInstances;
				continue;
			}
			++it;
		}

		if (_loot.debugLog && (sanitizedInstances > 0u || erasedInstances > 0u)) {
			SKSE::log::debug(
				"CalamityAffixes: sanitized tracked loot instances (context={}, changed={}, erased={}).",
				a_context,
				sanitizedInstances,
				erasedInstances);
		}
	}

	void EventBridge::ApplyMultipleAffixes(
		RE::InventoryChanges* a_changes,
		RE::InventoryEntryData* a_entry,
		RE::ExtraDataList* a_xList,
		LootItemType a_itemType,
		bool a_allowRunewordFragmentRoll,
		const std::optional<InstanceAffixSlots>& a_forcedSlots)
	{
		if (!a_changes || !a_entry || !a_entry->object || !a_xList) {
			return;
		}

		auto* uid = a_xList->GetByType<RE::ExtraUniqueID>();
		if (!uid) {
			auto* created = RE::BSExtraData::Create<RE::ExtraUniqueID>();
			created->baseID = a_entry->object->GetFormID();
			created->uniqueID = a_changes->GetNextUniqueID();
			a_xList->Add(created);
			uid = created;
		}

		const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
		bool skipStaleMarkerShortCircuit = false;

		if (!IsLootObjectEligibleForAffixes(a_entry->object)) {
			if (_loot.cleanupInvalidLegacyAffixes) {
				CleanupInvalidLootInstance(a_entry, a_xList, key, "ApplyMultipleAffixes.ineligible");
			} else {
					(void)TryClearStaleLootDisplayName(a_entry, a_xList, false);
			}
			return;
		}

		// If already assigned, just ensure display name and return.
		if (auto existingIt = _instanceAffixes.find(key); existingIt != _instanceAffixes.end()) {
			auto& existingSlots = existingIt->second;
			(void)SanitizeInstanceAffixSlotsForCurrentLootRules(
				key,
				existingSlots,
				"ApplyMultipleAffixes.existing");

			if (existingSlots.count == 0) {
				_instanceAffixes.erase(existingIt);
				ForgetLootEvaluatedInstance(key);
					(void)TryClearStaleLootDisplayName(a_entry, a_xList, true);
				skipStaleMarkerShortCircuit = true;
			} else {
				MarkLootEvaluatedInstance(key);
				for (std::uint8_t s = 0; s < existingSlots.count; ++s) {
					EnsureInstanceRuntimeState(key, existingSlots.tokens[s]);
				}
				EnsureMultiAffixDisplayName(a_entry, a_xList, existingSlots);
				return;
			}
		}

		// 1) Hard guard: any instance evaluated once is never rolled again.
		if (IsLootEvaluatedInstance(key)) {
			(void)TryClearStaleLootDisplayName(a_entry, a_xList, false);
			return;
		}

		auto applyPreviewPityOutcome = [&](bool a_success) {
			const float chancePct = std::clamp(_loot.chancePercent, 0.0f, 100.0f);
			if (chancePct <= 0.0f) {
				_lootChanceEligibleFailStreak = 0;
				return;
			}

			if (a_success) {
				_lootChanceEligibleFailStreak = 0;
				return;
			}

			if (_lootChanceEligibleFailStreak < std::numeric_limits<std::uint32_t>::max()) {
				++_lootChanceEligibleFailStreak;
			}
		};

		if (a_forcedSlots.has_value()) {
			auto slots = *a_forcedSlots;
			ForgetLootPreviewSlots(key);
			MarkLootEvaluatedInstance(key);

			if (slots.count == 0) {
				applyPreviewPityOutcome(false);
				return;
			}

			(void)SanitizeInstanceAffixSlotsForCurrentLootRules(
				key,
				slots,
				"ApplyMultipleAffixes.claim");
			if (slots.count == 0) {
				applyPreviewPityOutcome(false);
				return;
			}

			_instanceAffixes[key] = slots;
			for (std::uint8_t s = 0; s < slots.count; ++s) {
				EnsureInstanceRuntimeState(key, slots.tokens[s]);
			}
			EnsureMultiAffixDisplayName(a_entry, a_xList, slots);
			if (a_allowRunewordFragmentRoll) {
				MaybeGrantRandomRunewordFragment();
				MaybeGrantRandomReforgeOrb();
			}
			applyPreviewPityOutcome(true);
			return;
		}

		// Preview path: container/barter tooltip can pre-roll and cache this instance.
		// On pickup, consume cached preview so the observed result matches what the player inspected.
		if (const auto* previewSlots = FindLootPreviewSlots(key)) {
			const auto preview = *previewSlots;
			ForgetLootPreviewSlots(key);
			MarkLootEvaluatedInstance(key);

			if (preview.count == 0) {
				applyPreviewPityOutcome(false);
				return;
			}

			auto slots = preview;
			(void)SanitizeInstanceAffixSlotsForCurrentLootRules(
				key,
				slots,
				"ApplyMultipleAffixes.preview");
			if (slots.count == 0) {
				applyPreviewPityOutcome(false);
				return;
			}

			_instanceAffixes[key] = slots;
			for (std::uint8_t s = 0; s < slots.count; ++s) {
				EnsureInstanceRuntimeState(key, slots.tokens[s]);
			}
			EnsureMultiAffixDisplayName(a_entry, a_xList, slots);
			if (a_allowRunewordFragmentRoll) {
				MaybeGrantRandomRunewordFragment();
				MaybeGrantRandomReforgeOrb();
			}
			applyPreviewPityOutcome(true);
			return;
		}

		// 2) Legacy cleanup path:
		// If stale old data left only the star marker, treat as already evaluated and strip marker.
		if (!skipStaleMarkerShortCircuit && TryClearStaleLootDisplayName(a_entry, a_xList, false)) {
			MarkLootEvaluatedInstance(key);
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: stripped stale star marker without affix mapping (baseObj={:08X}, uniqueID={}).",
					uid->baseID,
					uid->uniqueID);
			}
			return;
		}

		// Mark before rolling so "failed/no-affix" outcomes are also one-shot.
		MarkLootEvaluatedInstance(key);

		// Global per-item loot chance gate (applies once regardless of P/S composition).
		// Includes pity: after N consecutive eligible failures, next eligible roll is guaranteed.
		if (!RollLootChanceGateForEligibleInstance()) {
			return;
		}

		// Roll how many affixes this item gets (1-4)
		const std::uint8_t targetCount = RollAffixCount();

		// Determine prefix/suffix composition
		const auto targets = detail::DetermineLootPrefixSuffixTargets(targetCount);
		std::uint8_t prefixTarget = targets.prefixTarget;
		std::uint8_t suffixTarget = targets.suffixTarget;
		if (_loot.stripTrackedSuffixSlots) {
			prefixTarget = targetCount;
			suffixTarget = 0u;
		}

		InstanceAffixSlots slots;
		std::vector<std::size_t> chosenIndices;
		chosenIndices.reserve(targetCount);

		// Roll prefixes
		std::vector<std::size_t> chosenPrefixIndices;
		for (std::uint8_t p = 0; p < prefixTarget; ++p) {
			static constexpr std::uint8_t kMaxRetries = 3;
			bool found = false;
			for (std::uint8_t retry = 0; retry < kMaxRetries; ++retry) {
				const auto idx = RollLootAffixIndex(a_itemType, &chosenPrefixIndices, /*a_skipChanceCheck=*/true);
				if (!idx) {
					break;
				}
				if (std::find(chosenPrefixIndices.begin(), chosenPrefixIndices.end(), *idx) != chosenPrefixIndices.end()) {
					continue;
				}
				if (*idx >= _affixes.size() || _affixes[*idx].id.empty()) {
					continue;
				}
				chosenPrefixIndices.push_back(*idx);
				chosenIndices.push_back(*idx);
				slots.AddToken(_affixes[*idx].token);
				found = true;
				break;
			}
			if (!found) {
				break;
			}
		}

		// Roll suffixes (family-based dedup)
		std::vector<std::string> chosenFamilies;
		for (std::uint8_t s = 0; s < suffixTarget; ++s) {
			if (!detail::ShouldConsumeSuffixRollForSingleAffixTarget(targetCount, slots.count)) {
				break;
			}

			const auto idx = RollSuffixIndex(a_itemType, &chosenFamilies);
			if (!idx) {
				break;
			}
			if (*idx >= _affixes.size() || _affixes[*idx].id.empty()) {
				continue;
			}
			if (!_affixes[*idx].family.empty()) {
				chosenFamilies.push_back(_affixes[*idx].family);
			}
			chosenIndices.push_back(*idx);
			slots.AddToken(_affixes[*idx].token);
		}

		if (slots.count == 0) {
			if (_lootChanceEligibleFailStreak < std::numeric_limits<std::uint32_t>::max()) {
				++_lootChanceEligibleFailStreak;
			}
			return;
		}

		_lootChanceEligibleFailStreak = 0;

		_instanceAffixes.emplace(key, slots);

		for (std::uint8_t s = 0; s < slots.count; ++s) {
			EnsureInstanceRuntimeState(key, slots.tokens[s]);
			if (s < chosenIndices.size() && chosenIndices[s] < _affixes.size()) {
				SKSE::log::debug(
					"CalamityAffixes: loot affix '{}' applied to {:08X}:{} (slot {}/{}).",
					_affixes[chosenIndices[s]].id,
					uid->baseID,
					uid->uniqueID,
					s + 1,
					slots.count);
			}
		}

		EnsureMultiAffixDisplayName(a_entry, a_xList, slots);
		if (a_allowRunewordFragmentRoll) {
			MaybeGrantRandomRunewordFragment();
			MaybeGrantRandomReforgeOrb();
		}
	}

	void EventBridge::EnsureMultiAffixDisplayName(
		RE::InventoryEntryData* a_entry,
		RE::ExtraDataList* a_xList,
		const InstanceAffixSlots& a_slots)
	{
		if (!_loot.renameItem || !a_entry || !a_entry->object || !a_xList) {
			return;
		}

		if (a_slots.count == 0) {
			return;
		}

		const char* currentRaw = a_xList->GetDisplayName(a_entry->object);
		const std::string currentName = currentRaw ? currentRaw : "";

		auto isKnownAffixLabel = [&](std::string_view a_label) {
			if (a_label.empty()) {
				return false;
			}

			for (const auto& affix : _affixes) {
				if (!affix.label.empty() && affix.label == a_label) {
					return true;
				}
			}
			return false;
		};

		auto stripKnownAffixTags = [&](std::string_view a_name) {
			std::string cleaned;
			cleaned.reserve(a_name.size());

			std::size_t pos = 0;
			while (pos < a_name.size()) {
				if (a_name[pos] == '[') {
					const auto right = a_name.find(']', pos + 1);
					if (right != std::string::npos && right > pos + 1) {
						const std::string_view label = a_name.substr(pos + 1, right - pos - 1);
						if (isKnownAffixLabel(label)) {
							while (!cleaned.empty() && cleaned.back() == ' ') {
								cleaned.pop_back();
							}
							pos = right + 1;
							while (pos < a_name.size() && a_name[pos] == ' ') {
								++pos;
							}
							continue;
						}
					}
				}

				cleaned.push_back(a_name[pos]);
				++pos;
			}

			const auto first = cleaned.find_first_not_of(" \n\r\t");
			if (first == std::string::npos) {
				return std::string{};
			}
			const auto last = cleaned.find_last_not_of(" \n\r\t");
			return cleaned.substr(first, last - first + 1);
		};

		std::string baseName = stripKnownAffixTags(StripLootStarMarkers(currentName));

		if (baseName.empty()) {
			const char* objectNameRaw = a_entry->object->GetName();
			baseName = objectNameRaw ? objectNameRaw : "";
		}

		// Strip trailing whitespace/newlines that some mods embed in form names
		while (!baseName.empty() && (baseName.back() == '\n' || baseName.back() == '\r' ||
			baseName.back() == ' ' || baseName.back() == '\t')) {
			baseName.pop_back();
		}

		// Build name marker based on affix count.
		std::string marker;
		if (a_slots.count >= 3) {
			marker = "***";
		} else if (a_slots.count == 2) {
			marker = "**";
		} else {
			marker = "*";
		}

		std::string newName;
		if (_loot.nameMarkerPosition == LootNameMarkerPosition::kTrailing) {
			// Requested UX default: "철검*" (no delimiter space) for stable name sorting.
			newName = baseName + marker;
		} else {
			newName = marker + " " + baseName;
		}
		if (newName.empty()) {
			return;
		}
		if (newName == currentName) {
			return;
		}

		if (auto* text = a_xList->GetExtraTextDisplayData()) {
			text->SetName(newName.c_str());
		} else {
			auto* created = RE::BSExtraData::Create<RE::ExtraTextDisplayData>();
			created->SetName(newName.c_str());
			a_xList->Add(created);
		}
	}

	void EventBridge::ProcessLootAcquired(
		RE::FormID a_baseObj,
		std::int32_t a_count,
		std::uint16_t a_uniqueID,
		RE::FormID a_oldContainer,
		bool a_allowRunewordFragmentRoll)
	{
		if (!_configLoaded || !_runtimeEnabled) {
			return;
		}

			// Loot policy:
			// - no random affix assignment on pickup
			// - pickup only rolls auxiliary currencies (runeword fragment / reforge orb)
			//   for eligible source pickups (corpse/chest/world)
			if (!a_allowRunewordFragmentRoll || a_count <= 0) {
				return;
			}

			auto* baseForm = RE::TESForm::LookupByID<RE::TESForm>(a_baseObj);
			auto* baseObj = baseForm ? baseForm->As<RE::TESBoundObject>() : nullptr;
			if (!baseObj) {
				return;
			}

			// Guard against recursive self-feedback on Calamity resource currencies.
		if (IsCalamityResourceCurrencyItem(baseObj)) {
				if (_loot.debugLog) {
					SKSE::log::debug(
						"CalamityAffixes: pickup loot roll skipped for resource currency item (baseObj={:08X}, uniqueID={}, oldContainer={:08X}).",
						a_baseObj,
						a_uniqueID,
						a_oldContainer);
				}
			return;
		}

		const auto sourceTier = ResolveLootCurrencySourceTier(
			a_oldContainer,
			_loot.bossContainerEditorIdAllowContains,
			_loot.bossContainerEditorIdDenyContains);
		float sourceChanceMultiplier = _loot.lootSourceChanceMultContainer;
		switch (sourceTier) {
		case LootCurrencySourceTier::kCorpse:
			sourceChanceMultiplier = _loot.lootSourceChanceMultCorpse;
			break;
		case LootCurrencySourceTier::kContainer:
			sourceChanceMultiplier = _loot.lootSourceChanceMultContainer;
			break;
		case LootCurrencySourceTier::kBossContainer:
			sourceChanceMultiplier = _loot.lootSourceChanceMultBossContainer;
			break;
		case LootCurrencySourceTier::kWorld:
			sourceChanceMultiplier = _loot.lootSourceChanceMultWorld;
			break;
		default:
			sourceChanceMultiplier = _loot.lootSourceChanceMultContainer;
			break;
		}
		sourceChanceMultiplier = std::clamp(sourceChanceMultiplier, 0.0f, 5.0f);
		const std::uint32_t rollCount = static_cast<std::uint32_t>(std::clamp(a_count, 1, 8));
		for (std::uint32_t i = 0; i < rollCount; ++i) {
			MaybeGrantRandomRunewordFragment(sourceChanceMultiplier);
			MaybeGrantRandomReforgeOrb(sourceChanceMultiplier);
		}

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: pickup loot rolls applied (baseObj={:08X}, count={}, rolls={}, uniqueID={}, oldContainer={:08X}, sourceTier={}, sourceChanceMult={:.2f}).",
				a_baseObj,
				a_count,
				rollCount,
				a_uniqueID,
				a_oldContainer,
				LootCurrencySourceTierLabel(sourceTier),
				sourceChanceMultiplier);
		}
	}
}
