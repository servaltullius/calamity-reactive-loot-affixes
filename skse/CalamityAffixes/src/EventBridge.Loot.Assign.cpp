#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootEligibility.h"
#include "CalamityAffixes/LootUiGuards.h"

#include <algorithm>
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
		bool hasTemplateArmor = false;

		const RE::TESObjectARMO* cursor = a_armor;
		for (std::size_t depth = 0; cursor && depth < kArmorTemplateScanDepth; ++depth) {
			hasSlotMask = hasSlotMask || (cursor->GetSlotMask() != RE::BIPED_MODEL::BipedObjectSlot::kNone);
			hasArmorAddons = hasArmorAddons || !cursor->armorAddons.empty();
			if (!cursor->templateArmor) {
				break;
			}
			hasTemplateArmor = true;
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
			.hasTemplateArmor = hasTemplateArmor,
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
		if (!a_skipChanceCheck) {
			if (_loot.chancePercent <= 0.0f) {
				return std::nullopt;
			}

			if (_loot.chancePercent < 100.0f) {
				std::uniform_real_distribution<float> dist(0.0f, 100.0f);
				if (dist(_rng) >= _loot.chancePercent) {
					return std::nullopt;
				}
			}
		}

		// Build eligible pool, excluding already-chosen indices and chance<=0 affixes.
		// When sharedPool is enabled, combine weapon + armor pools (original behavior).
		std::vector<std::size_t> eligible;
		std::vector<double> weights;

		auto collectFromPool = [&](const std::vector<std::size_t>& a_pool) {
			for (const auto idx : a_pool) {
				if (a_exclude) {
					if (std::find(a_exclude->begin(), a_exclude->end(), idx) != a_exclude->end()) {
						continue;
					}
				}
				if (idx >= _affixes.size()) {
					continue;
				}
				const float weight = _affixes[idx].EffectiveLootWeight();
				if (weight <= 0.0f) {
					continue;
				}
				eligible.push_back(idx);
				weights.push_back(static_cast<double>(weight));
			}
		};

		if (_loot.sharedPool) {
			collectFromPool(_lootWeaponAffixes);
			collectFromPool(_lootArmorAffixes);
		} else {
			const auto& typePool = (a_itemType == LootItemType::kWeapon) ? _lootWeaponAffixes : _lootArmorAffixes;
			collectFromPool(typePool);
		}

		if (eligible.empty()) {
			return std::nullopt;
		}

		if (eligible.size() == 1) {
			return eligible[0];
		}

		std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
		return eligible[dist(_rng)];
	}

	std::optional<std::size_t> EventBridge::RollSuffixIndex(
		LootItemType a_itemType,
		const std::vector<std::string>* a_excludeFamilies)
	{
		std::vector<std::size_t> eligible;
		std::vector<double> weights;

		auto collectFromPool = [&](const std::vector<std::size_t>& a_pool) {
			for (const auto idx : a_pool) {
				if (idx >= _affixes.size()) {
					continue;
				}
				const auto& affix = _affixes[idx];
				if (affix.slot != AffixSlot::kSuffix) {
					continue;
				}
				if (a_excludeFamilies && !affix.family.empty()) {
					if (std::find(a_excludeFamilies->begin(), a_excludeFamilies->end(), affix.family) != a_excludeFamilies->end()) {
						continue;
					}
				}
				const float weight = affix.EffectiveLootWeight();
				if (weight <= 0.0f) {
					continue;
				}
				eligible.push_back(idx);
				weights.push_back(static_cast<double>(weight));
			}
		};

		if (_loot.sharedPool) {
			collectFromPool(_lootWeaponSuffixes);
			collectFromPool(_lootArmorSuffixes);
		} else {
			const auto& typePool = (a_itemType == LootItemType::kWeapon) ? _lootWeaponSuffixes : _lootArmorSuffixes;
			collectFromPool(typePool);
		}

		if (eligible.empty()) {
			return std::nullopt;
		}

		if (eligible.size() == 1) {
			return eligible[0];
		}

		std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
		return eligible[dist(_rng)];
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

		if (!IsLootObjectEligibleForAffixes(a_entry->object)) {
			if (_loot.cleanupInvalidLegacyAffixes) {
				CleanupInvalidLootInstance(a_entry, a_xList, key, "ApplyMultipleAffixes.ineligible");
			} else {
				(void)TryClearStaleLootDisplayName(a_entry, a_xList);
			}
			return;
		}

		// If already assigned, just ensure display name and return
		if (const auto existingIt = _instanceAffixes.find(key); existingIt != _instanceAffixes.end()) {
			const auto& existingSlots = existingIt->second;
			MarkLootEvaluatedInstance(key);
			for (std::uint8_t s = 0; s < existingSlots.count; ++s) {
				EnsureInstanceRuntimeState(key, existingSlots.tokens[s]);
			}
			EnsureMultiAffixDisplayName(a_entry, a_xList, existingSlots);
			return;
		}

		// 1) Hard guard: any instance evaluated once is never rolled again.
		if (IsLootEvaluatedInstance(key)) {
			(void)TryClearStaleLootDisplayName(a_entry, a_xList);
			return;
		}

		// 2) Legacy cleanup path:
		// If stale old data left only the star marker, treat as already evaluated and strip marker.
		if (TryClearStaleLootDisplayName(a_entry, a_xList)) {
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
		if (_loot.chancePercent <= 0.0f) {
			return;
		}
		if (_loot.chancePercent < 100.0f) {
			std::uniform_real_distribution<float> dist(0.0f, 100.0f);
			if (dist(_rng) >= _loot.chancePercent) {
				return;
			}
		}

		// Roll how many affixes this item gets (1-3)
		const std::uint8_t targetCount = RollAffixCount();

		// Determine prefix/suffix composition
		std::uint8_t prefixTarget = 0;
		std::uint8_t suffixTarget = 0;

		if (targetCount == 1) {
			std::uniform_int_distribution<int> coinFlip(0, 1);
			if (coinFlip(_rng) == 0) {
				prefixTarget = 1;
			} else {
				suffixTarget = 1;
			}
		} else if (targetCount == 2) {
			prefixTarget = 1;
			suffixTarget = 1;
		} else {
			std::uniform_int_distribution<int> coinFlip(0, 1);
			if (coinFlip(_rng) == 0) {
				prefixTarget = 2;
				suffixTarget = 1;
			} else {
				prefixTarget = 1;
				suffixTarget = 2;
			}
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
			return;
		}

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
				if (const auto mappedIt = _instanceAffixes.find(key); mappedIt != _instanceAffixes.end()) {
					EnsureMultiAffixDisplayName(entry, xList, mappedIt->second);
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
