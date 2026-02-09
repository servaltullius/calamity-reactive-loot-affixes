#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <cctype>
#include <limits>
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

	std::optional<std::size_t> EventBridge::RollLootAffixIndex(LootItemType a_itemType)
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

		const auto weaponCount = _lootWeaponAffixes.size();
		const auto armorCount = _lootArmorAffixes.size();

		if (_loot.sharedPool) {
			const auto total = weaponCount + armorCount;
			if (total == 0) {
				return std::nullopt;
			}

			std::uniform_int_distribution<std::size_t> pick(0, total - 1);
			const auto slot = pick(_rng);
			if (slot < weaponCount) {
				return _lootWeaponAffixes[slot];
			}
			return _lootArmorAffixes[slot - weaponCount];
		}

		const auto& pool = (a_itemType == LootItemType::kWeapon) ? _lootWeaponAffixes : _lootArmorAffixes;
		if (pool.empty()) {
			return std::nullopt;
		}

		std::uniform_int_distribution<std::size_t> pick(0, pool.size() - 1);
		return pool[pick(_rng)];
	}

		void EventBridge::ApplyChosenAffix(
			RE::InventoryChanges* a_changes,
			RE::InventoryEntryData* a_entry,
			RE::ExtraDataList* a_xList,
			LootItemType,
			std::size_t a_affixIndex,
			bool a_allowRunewordFragmentRoll)
		{
		if (!a_changes || !a_entry || !a_entry->object || !a_xList) {
			return;
		}

		if (a_affixIndex >= _affixes.size()) {
			return;
		}

		auto& affix = _affixes[a_affixIndex];
		if (affix.id.empty()) {
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
		if (const auto existingIt = _instanceAffixes.find(key); existingIt != _instanceAffixes.end()) {
			EnsureInstanceRuntimeState(key, existingIt->second);
			const auto idxIt = _affixIndexByToken.find(existingIt->second);
			if (idxIt != _affixIndexByToken.end() && idxIt->second < _affixes.size()) {
				EnsureAffixDisplayName(a_entry, a_xList, _affixes[idxIt->second]);
			}
			return;
		}

		_instanceAffixes.emplace(key, affix.token);
		EnsureInstanceRuntimeState(key, affix.token);
		SKSE::log::debug(
			"CalamityAffixes: loot affix '{}' applied to {:08X}:{}.",
			affix.id,
			uid->baseID,
			uid->uniqueID);

				EnsureAffixDisplayName(a_entry, a_xList, affix);
				if (a_allowRunewordFragmentRoll) {
					MaybeGrantRandomRunewordFragment();
				}
			}

		void EventBridge::ApplyAffixToInstance(
			RE::InventoryChanges* a_changes,
			RE::InventoryEntryData* a_entry,
			RE::ExtraDataList* a_xList,
			LootItemType a_itemType,
			bool a_allowRunewordFragmentRoll)
		{
			const auto idx = RollLootAffixIndex(a_itemType);
			if (!idx) {
				return;
			}

			ApplyChosenAffix(a_changes, a_entry, a_xList, a_itemType, *idx, a_allowRunewordFragmentRoll);
		}

		void EventBridge::EnsureAffixDisplayName(
			RE::InventoryEntryData* a_entry,
			RE::ExtraDataList* a_xList,
			const AffixRuntime& a_affix)
		{
			if (!_loot.renameItem || !a_entry || !a_entry->object || !a_xList) {
				return;
			}

			if (a_affix.label.empty()) {
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

			std::string baseName = stripKnownAffixTags(currentName);

			if (baseName.empty()) {
				const char* objectNameRaw = a_entry->object->GetName();
				baseName = objectNameRaw ? objectNameRaw : "";
			}

			const std::string newName = FormatLootName(baseName, a_affix.label);
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
						ApplyAffixToInstance(changes, entry, xList, itemType, a_allowRunewordFragmentRoll);
						return;
					}
				}
		}

		// Fallback:
		// If the container change event doesn't provide a usable uniqueID, attempt to apply loot rolls to
		// currently-unassigned instances (extraLists). This avoids constructing ExtraDataList (not supported
		// by CommonLibSSE-NG flatrim) and still covers most weapon/armor acquisitions.
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
					const auto idxIt = _affixIndexByToken.find(mappedIt->second);
					if (idxIt != _affixIndexByToken.end() && idxIt->second < _affixes.size()) {
						EnsureAffixDisplayName(entry, xList, _affixes[idxIt->second]);
					}
					continue;
				}
			}

				ApplyAffixToInstance(changes, entry, xList, itemType, a_allowRunewordFragmentRoll);
				processed += 1;
			}
		}
}
