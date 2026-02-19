#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootCurrencyLedger.h"
#include "CalamityAffixes/LootEligibility.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/LootSlotSanitizer.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
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
		using LootCurrencySourceTier = detail::LootCurrencySourceTier;

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

		[[nodiscard]] RE::TESObjectREFR* ResolveLootCurrencySourceContainerRef(RE::FormID a_oldContainer)
		{
			if (a_oldContainer == 0u || a_oldContainer == LootRerollGuard::kWorldContainer) {
				return nullptr;
			}

			auto* sourceForm = RE::TESForm::LookupByID<RE::TESForm>(a_oldContainer);
			if (!sourceForm) {
				return nullptr;
			}

			if (auto* sourceActor = sourceForm->As<RE::Actor>(); sourceActor && sourceActor->IsDead()) {
				return sourceActor;
			}

			if (auto* sourceRef = sourceForm->As<RE::TESObjectREFR>()) {
				if (auto* sourceBase = sourceRef->GetBaseObject(); sourceBase && sourceBase->As<RE::TESObjectCONT>()) {
					return sourceRef;
				}
			}

			return nullptr;
		}

		[[nodiscard]] std::uint32_t GetInGameDayStamp() noexcept
		{
			auto* calendar = RE::Calendar::GetSingleton();
			if (!calendar) {
				return 0u;
			}

			const float daysPassed = calendar->GetDaysPassed();
			if (!std::isfinite(daysPassed) || daysPassed <= 0.0f) {
				return 0u;
			}

			const auto dayFloor = std::floor(daysPassed);
			if (dayFloor >= static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
				return std::numeric_limits<std::uint32_t>::max();
			}

			return static_cast<std::uint32_t>(dayFloor);
		}
	}

	bool EventBridge::IsRuntimeCurrencyDropRollEnabled(std::string_view a_contextTag) const
	{
		if (_loot.runtimeCurrencyDropsEnabled) {
			return true;
		}

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: {} currency roll skipped (runtime currency disabled by loot.currencyDropMode).",
				a_contextTag);
		}
		return false;
	}

	float EventBridge::ResolveLootCurrencySourceChanceMultiplier(detail::LootCurrencySourceTier a_sourceTier) const noexcept
	{
		float sourceChanceMultiplier = _loot.lootSourceChanceMultContainer;
		switch (a_sourceTier) {
		case detail::LootCurrencySourceTier::kCorpse:
			sourceChanceMultiplier = _loot.lootSourceChanceMultCorpse;
			break;
		case detail::LootCurrencySourceTier::kContainer:
			sourceChanceMultiplier = _loot.lootSourceChanceMultContainer;
			break;
		case detail::LootCurrencySourceTier::kBossContainer:
			sourceChanceMultiplier = _loot.lootSourceChanceMultBossContainer;
			break;
		case detail::LootCurrencySourceTier::kWorld:
			sourceChanceMultiplier = _loot.lootSourceChanceMultWorld;
			break;
		default:
			sourceChanceMultiplier = _loot.lootSourceChanceMultContainer;
			break;
		}
		return std::clamp(sourceChanceMultiplier, 0.0f, 5.0f);
	}

	bool EventBridge::TryBeginLootCurrencyLedgerRoll(std::uint64_t a_ledgerKey, std::uint32_t a_dayStamp)
	{
		if (a_ledgerKey == 0u) {
			return true;
		}

		if (auto ledgerIt = _lootCurrencyRollLedger.find(a_ledgerKey);
			ledgerIt != _lootCurrencyRollLedger.end()) {
			if (detail::IsLootCurrencyLedgerExpired(
					ledgerIt->second,
					a_dayStamp,
					kLootCurrencyLedgerTtlDays)) {
				_lootCurrencyRollLedger.erase(ledgerIt);
			} else {
				return false;
			}
		}

		return true;
	}

	void EventBridge::FinalizeLootCurrencyLedgerRoll(std::uint64_t a_ledgerKey, std::uint32_t a_dayStamp)
	{
		if (a_ledgerKey == 0u) {
			return;
		}

		const auto [it, inserted] = _lootCurrencyRollLedger.emplace(a_ledgerKey, a_dayStamp);
		if (!inserted) {
			it->second = a_dayStamp;
		}
		if (inserted) {
			_lootCurrencyRollLedgerRecent.push_back(a_ledgerKey);
			while (_lootCurrencyRollLedgerRecent.size() > kLootCurrencyLedgerMaxEntries) {
				const auto oldest = _lootCurrencyRollLedgerRecent.front();
				_lootCurrencyRollLedgerRecent.pop_front();
				_lootCurrencyRollLedger.erase(oldest);
			}
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
		if (!IsRuntimeCurrencyDropRollEnabled("pickup")) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: pickup currency roll context (baseObj={:08X}, uniqueID={}, oldContainer={:08X}).",
					a_baseObj,
					a_uniqueID,
					a_oldContainer);
			}
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
		const float sourceChanceMultiplier = ResolveLootCurrencySourceChanceMultiplier(sourceTier);
		const auto dayStamp = GetInGameDayStamp();
		const auto ledgerKey = detail::BuildLootCurrencyLedgerKey(
			sourceTier,
			a_oldContainer,
			a_baseObj,
			a_uniqueID,
			dayStamp);
		if (!TryBeginLootCurrencyLedgerRoll(ledgerKey, dayStamp)) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: pickup loot roll skipped (ledger consumed) (baseObj={:08X}, uniqueID={}, oldContainer={:08X}, sourceTier={}, ledgerKey={:016X}, dayStamp={}).",
					a_baseObj,
					a_uniqueID,
					a_oldContainer,
					LootCurrencySourceTierLabel(sourceTier),
					ledgerKey,
					dayStamp);
			}
			return;
		}

		if (sourceTier != LootCurrencySourceTier::kWorld) {
			// Corpse/container sources are rolled at activation time to avoid
			// "first looting then extra drop appears" UX.
			return;
		}

		auto* sourceContainerRef = ResolveLootCurrencySourceContainerRef(a_oldContainer);
		const bool forceWorldPlacement = (sourceTier == LootCurrencySourceTier::kWorld);

		bool runewordDropGranted = false;
		bool reforgeDropGranted = false;
		// OnItemAdded.itemCount is the stack size for this pickup event.
		// Rolling per stack count causes burst grants (e.g., arrows/gold/stacked misc).
		// Keep currency roll strictly once per pickup event for stable economy pacing.
		const std::uint32_t rollCount = 1u;
		for (std::uint32_t i = 0; i < rollCount; ++i) {
			bool runewordPityTriggered = false;
			std::uint64_t runeToken = 0u;
			if (TryRollRunewordFragmentToken(sourceChanceMultiplier, runeToken, runewordPityTriggered)) {
				auto* fragmentItem = RunewordDetail::LookupRunewordFragmentItem(
					_runewordRuneNameByToken,
					runeToken);
				if (!fragmentItem) {
					SKSE::log::error(
						"CalamityAffixes: runeword fragment item missing (runeToken={:016X}).",
						runeToken);
				} else if (TryPlaceLootCurrencyItem(fragmentItem, sourceContainerRef, forceWorldPlacement)) {
					CommitRunewordFragmentGrant(true);
					runewordDropGranted = true;
					std::string runeName = "Rune";
					if (const auto nameIt = _runewordRuneNameByToken.find(runeToken);
						nameIt != _runewordRuneNameByToken.end()) {
						runeName = nameIt->second;
					}
					std::string note = "Runeword Fragment Drop: ";
					note.append(runeName);
					if (runewordPityTriggered) {
						note.append(" [Pity]");
					}
					RE::DebugNotification(note.c_str());
				}
			}

			bool reforgePityTriggered = false;
			if (TryRollReforgeOrbGrant(sourceChanceMultiplier, reforgePityTriggered)) {
				auto* orb = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_Misc_ReforgeOrb");
				if (!orb) {
					SKSE::log::error(
						"CalamityAffixes: reforge orb item missing (editorId=CAFF_Misc_ReforgeOrb).");
				} else if (TryPlaceLootCurrencyItem(orb, sourceContainerRef, forceWorldPlacement)) {
					CommitReforgeOrbGrant(true);
					reforgeDropGranted = true;
					RE::DebugNotification("Reforge Orb Drop");
					if (reforgePityTriggered && _loot.debugLog) {
						SKSE::log::debug("CalamityAffixes: reforge orb pity triggered.");
					}
				}
			}
		}

		FinalizeLootCurrencyLedgerRoll(ledgerKey, dayStamp);

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: pickup loot rolls applied (baseObj={:08X}, count={}, rolls={}, uniqueID={}, oldContainer={:08X}, sourceTier={}, sourceChanceMult={:.2f}, runewordDropped={}, reforgeDropped={}, ledgerKey={:016X}).",
				a_baseObj,
				a_count,
				rollCount,
				a_uniqueID,
				a_oldContainer,
				LootCurrencySourceTierLabel(sourceTier),
				sourceChanceMultiplier,
				runewordDropGranted,
				reforgeDropGranted,
				ledgerKey);
		}
	}
}
