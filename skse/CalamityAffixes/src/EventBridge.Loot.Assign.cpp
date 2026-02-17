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

	bool EventBridge::TryClearStaleLootDisplayName(RE::InventoryEntryData* a_entry, RE::ExtraDataList* a_xList)
	{
		if (!a_entry || !a_entry->object || !a_xList) {
			return false;
		}

		auto* text = a_xList->GetExtraTextDisplayData();
		if (!text) {
			return false;
		}

		const auto* name = text->GetDisplayName(a_entry->object, 0);
		if (!name || name[0] == '\0' || !HasLeadingLootStarPrefix(name)) {
			return false;
		}

		std::string cleaned = std::string(StripLeadingLootStarPrefix(name));
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
		_runewordInstanceStates.erase(a_key);
		if (_runewordSelectedBaseKey && *_runewordSelectedBaseKey == a_key) {
			_runewordSelectedBaseKey.reset();
		}

		const bool renamed = TryClearStaleLootDisplayName(a_entry, a_xList);
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
		std::discrete_distribution<unsigned int> dist({
			static_cast<double>(kAffixCountWeights[0]),
			static_cast<double>(kAffixCountWeights[1]),
			static_cast<double>(kAffixCountWeights[2])
		});
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
		bool a_allowRunewordFragmentRoll)
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
				(void)TryClearStaleLootDisplayName(a_entry, a_xList);
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
				(void)TryClearStaleLootDisplayName(a_entry, a_xList);
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
			(void)TryClearStaleLootDisplayName(a_entry, a_xList);
			return;
		}

		// 2) Legacy cleanup path:
		// If stale old data left only the star marker, treat as already evaluated and strip marker.
		if (!skipStaleMarkerShortCircuit && TryClearStaleLootDisplayName(a_entry, a_xList)) {
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

		// Roll how many affixes this item gets (1-3)
		const std::uint8_t targetCount = RollAffixCount();

		// Determine prefix/suffix composition
		const auto targets = detail::DetermineLootPrefixSuffixTargets(targetCount);
		const std::uint8_t prefixTarget = targets.prefixTarget;
		const std::uint8_t suffixTarget = targets.suffixTarget;

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

		std::string baseName = stripKnownAffixTags(StripLeadingLootStarPrefix(currentName));

		if (baseName.empty()) {
			const char* objectNameRaw = a_entry->object->GetName();
			baseName = objectNameRaw ? objectNameRaw : "";
		}

		// Strip trailing whitespace/newlines that some mods embed in form names
		while (!baseName.empty() && (baseName.back() == '\n' || baseName.back() == '\r' ||
			baseName.back() == ' ' || baseName.back() == '\t')) {
			baseName.pop_back();
		}

		// Build name with star prefix based on affix count
		std::string prefix;
		if (a_slots.count >= 3) {
			prefix = "*** ";
		} else if (a_slots.count == 2) {
			prefix = "** ";
		} else {
			prefix = "* ";
		}

		const std::string newName = prefix + baseName;
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

		void EventBridge::ProcessLootAcquired(RE::FormID a_baseObj, std::int32_t a_count, std::uint16_t a_uniqueID, bool a_allowRunewordFragmentRoll)
	{
		if (!_configLoaded || !_runtimeEnabled || _loot.chancePercent <= 0.0f) {
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		auto* changes = player->GetInventoryChanges();
		if (!changes || !changes->entryList) {
			return;
		}

		auto* form = RE::TESForm::LookupByID<RE::TESForm>(a_baseObj);
		if (!form) {
			return;
		}

		auto* weap = form->As<RE::TESObjectWEAP>();
		auto* armo = form->As<RE::TESObjectARMO>();
		if (!weap && !armo) {
			return;
		}

		RE::InventoryEntryData* entry = nullptr;
		for (auto* e : *changes->entryList) {
			if (e && e->object && e->object->GetFormID() == a_baseObj) {
				entry = e;
				break;
			}
		}

		if (!entry) {
			SKSE::log::debug("CalamityAffixes: ProcessLootAcquired skipped (entry not found).");
			return;
		}

		if (!entry->extraLists) {
			SKSE::log::debug("CalamityAffixes: ProcessLootAcquired skipped (entry->extraLists is null).");
			return;
		}

		if (!IsLootObjectEligibleForAffixes(entry->object)) {
			if (_loot.cleanupInvalidLegacyAffixes) {
				auto cleanupByUnique = [&](RE::ExtraDataList* a_xList) {
					if (!a_xList) {
						return false;
					}

					auto* uid = a_xList->GetByType<RE::ExtraUniqueID>();
					if (!uid || uid->baseID != a_baseObj) {
						return false;
					}
					if (a_uniqueID != 0 && uid->uniqueID != a_uniqueID) {
						return false;
					}

					CleanupInvalidLootInstance(
						entry,
						a_xList,
						MakeInstanceKey(uid->baseID, uid->uniqueID),
						"ProcessLootAcquired.ineligible");
					return true;
				};

				if (a_uniqueID != 0) {
					for (auto* xList : *entry->extraLists) {
						if (cleanupByUnique(xList)) {
							break;
						}
					}
				} else {
					for (auto* xList : *entry->extraLists) {
						(void)cleanupByUnique(xList);
					}
				}
			}

			if (_loot.debugLog) {
				const char* editorId = armo ? armo->GetFormEditorID() : nullptr;
				SKSE::log::debug(
					"CalamityAffixes: ProcessLootAcquired skipped non-eligible item (baseObj={:08X}, editorId={}).",
					a_baseObj,
					editorId ? editorId : "<none>");
			}
			return;
		}

		const auto itemType = entry->object->As<RE::TESObjectWEAP>() ? LootItemType::kWeapon : LootItemType::kArmor;

		if (a_uniqueID != 0) {
			for (auto* xList : *entry->extraLists) {
				if (!xList) {
					continue;
				}

				auto* uid = xList->GetByType<RE::ExtraUniqueID>();
				if (uid && uid->baseID == a_baseObj && uid->uniqueID == a_uniqueID) {
					ApplyMultipleAffixes(changes, entry, xList, itemType, a_allowRunewordFragmentRoll);
					return;
				}
			}
		}

		// Fallback:
		// If the container change event doesn't provide a usable uniqueID, attempt to apply loot rolls to
		// currently-unassigned instances (extraLists).
		// Collect unassigned lists first, then process in REVERSE order — newly added items are
		// typically appended at the end of extraLists, so reverse iteration ensures the most
		// recently acquired instance receives the affix instead of an older one.
		std::vector<RE::ExtraDataList*> unevaluatedCandidates;
		std::vector<RE::ExtraDataList*> evaluatedCandidates;
		for (auto* xList : *entry->extraLists) {
			if (!xList) {
				continue;
			}

			if (auto* uid = xList->GetByType<RE::ExtraUniqueID>()) {
				const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
				if (auto mappedIt = _instanceAffixes.find(key); mappedIt != _instanceAffixes.end()) {
					auto& slots = mappedIt->second;
					(void)SanitizeInstanceAffixSlotsForCurrentLootRules(
						key,
						slots,
						"ProcessLootAcquired.fallback");
					if (slots.count == 0) {
						_instanceAffixes.erase(mappedIt);
						ForgetLootEvaluatedInstance(key);
						(void)TryClearStaleLootDisplayName(entry, xList);
						unevaluatedCandidates.push_back(xList);
						continue;
					}

					EnsureMultiAffixDisplayName(entry, xList, slots);
					continue;
				}
				if (IsLootEvaluatedInstance(key)) {
					evaluatedCandidates.push_back(xList);
					continue;
				}
			}

			unevaluatedCandidates.push_back(xList);
		}

		std::int32_t processed = 0;
		for (auto it = unevaluatedCandidates.rbegin(); it != unevaluatedCandidates.rend() && processed < a_count; ++it) {
			ApplyMultipleAffixes(changes, entry, *it, itemType, a_allowRunewordFragmentRoll);
			processed += 1;
		}

		for (auto it = evaluatedCandidates.rbegin(); it != evaluatedCandidates.rend() && processed < a_count; ++it) {
			ApplyMultipleAffixes(changes, entry, *it, itemType, a_allowRunewordFragmentRoll);
			processed += 1;
		}
	}
}
