#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <random>
#include <string>
#include <string_view>

namespace CalamityAffixes
{
	std::uint64_t EventBridge::MakeInstanceKey(RE::FormID a_baseID, std::uint16_t a_uniqueID) noexcept
	{
		return (static_cast<std::uint64_t>(a_baseID) << 16) | static_cast<std::uint64_t>(a_uniqueID);
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

	std::optional<std::size_t> EventBridge::RollLootAffixIndex(LootItemType a_itemType, const std::vector<std::size_t>* a_exclude)
	{
		if (_loot.chancePercent <= 0.0f) {
			return std::nullopt;
		}

		if (_loot.chancePercent < 100.0f) {
			std::uniform_real_distribution<float> dist(0.0f, 100.0f);
			if (dist(_rng) >= _loot.chancePercent) {
				return std::nullopt;
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
				// Use procChancePct as loot weight; treat 0 or negative as excluded (runeword-only affixes)
				const float weight = _affixes[idx].procChancePct;
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

		// If already assigned, just ensure display name and return
		if (const auto existingIt = _instanceAffixes.find(key); existingIt != _instanceAffixes.end()) {
			const auto& existingSlots = existingIt->second;
			for (std::uint8_t s = 0; s < existingSlots.count; ++s) {
				EnsureInstanceRuntimeState(key, existingSlots.tokens[s]);
			}
			EnsureMultiAffixDisplayName(a_entry, a_xList, existingSlots);
			return;
		}

		// Roll how many affixes this item gets (1-3)
		const std::uint8_t targetCount = RollAffixCount();

		InstanceAffixSlots slots;
		std::vector<std::size_t> chosen;
		chosen.reserve(targetCount);

		for (std::uint8_t attempt = 0; attempt < targetCount; ++attempt) {
			static constexpr std::uint8_t kMaxRetries = 3;
			bool found = false;
			for (std::uint8_t retry = 0; retry < kMaxRetries; ++retry) {
				const auto idx = RollLootAffixIndex(a_itemType, &chosen);
				if (!idx) {
					break;  // pool exhausted
				}
				if (std::find(chosen.begin(), chosen.end(), *idx) != chosen.end()) {
					continue;  // duplicate, retry
				}
				if (*idx >= _affixes.size() || _affixes[*idx].id.empty()) {
					continue;
				}
				chosen.push_back(*idx);
				slots.AddToken(_affixes[*idx].token);
				found = true;
				break;
			}
			if (!found) {
				break;  // can't find more unique affixes
			}
		}

		if (slots.count == 0) {
			return;
		}

		_instanceAffixes.emplace(key, slots);

		for (std::uint8_t s = 0; s < slots.count; ++s) {
			EnsureInstanceRuntimeState(key, slots.tokens[s]);
			if (s < chosen.size() && chosen[s] < _affixes.size()) {
				SKSE::log::debug(
					"CalamityAffixes: loot affix '{}' applied to {:08X}:{} (slot {}/{}).",
					_affixes[chosen[s]].id,
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

		// Find primary affix label
		const auto primaryToken = a_slots.GetPrimary();
		const auto idxIt = _affixIndexByToken.find(primaryToken);
		if (idxIt == _affixIndexByToken.end() || idxIt->second >= _affixes.size()) {
			return;
		}
		const auto& primaryAffix = _affixes[idxIt->second];
		if (primaryAffix.label.empty()) {
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

			const auto first = cleaned.find_first_not_of(' ');
			if (first == std::string::npos) {
				return std::string{};
			}
			const auto last = cleaned.find_last_not_of(' ');
			return cleaned.substr(first, last - first + 1);
		};

		// Strip star prefixes as well
		auto stripStarPrefix = [](std::string_view a_name) -> std::string_view {
			std::size_t pos = 0;
			while (pos < a_name.size()) {
				// UTF-8 star: E2 98 85 (★)
				if (pos + 2 < a_name.size() &&
					static_cast<unsigned char>(a_name[pos]) == 0xE2 &&
					static_cast<unsigned char>(a_name[pos + 1]) == 0x98 &&
					static_cast<unsigned char>(a_name[pos + 2]) == 0x85) {
					pos += 3;
					continue;
				}
				if (a_name[pos] == ' ') {
					++pos;
					continue;
				}
				break;
			}
			return a_name.substr(pos);
		};

		std::string baseName = stripKnownAffixTags(stripStarPrefix(currentName));

		if (baseName.empty()) {
			const char* objectNameRaw = a_entry->object->GetName();
			baseName = objectNameRaw ? objectNameRaw : "";
		}

		// Build name with star prefix for multi-affix items
		std::string prefix;
		if (a_slots.count == 3) {
			prefix = "\xe2\x98\x85\xe2\x98\x85 ";  // ★★
		} else if (a_slots.count == 2) {
			prefix = "\xe2\x98\x85 ";  // ★
		}

		const std::string formatted = FormatLootName(baseName, primaryAffix.label);
		const std::string newName = prefix + formatted;
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

		SKSE::log::debug(
			"CalamityAffixes: ProcessLootAcquired(baseObj={:08X}, count={}, uniqueID={}).",
			a_baseObj,
			a_count,
			a_uniqueID);

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

		const auto itemType = weap ? LootItemType::kWeapon : LootItemType::kArmor;

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
		std::int32_t processed = 0;
		for (auto* xList : *entry->extraLists) {
			if (processed >= a_count) {
				break;
			}

			if (!xList) {
				continue;
			}

			if (auto* uid = xList->GetByType<RE::ExtraUniqueID>()) {
				const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
				if (const auto mappedIt = _instanceAffixes.find(key); mappedIt != _instanceAffixes.end()) {
					EnsureMultiAffixDisplayName(entry, xList, mappedIt->second);
					continue;
				}
			}

			ApplyMultipleAffixes(changes, entry, xList, itemType, a_allowRunewordFragmentRoll);
			processed += 1;
		}
	}
}
