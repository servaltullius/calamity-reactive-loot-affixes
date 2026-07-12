#include "CalamityAffixes/EventBridge.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace CalamityAffixes::RunewordDetail
{
			std::string ToLowerAscii(std::string_view a_text)
		{
			std::string out(a_text);
			std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			return out;
		}

				[[nodiscard]] bool IsWeaponOrArmor(const RE::TESBoundObject* a_object)
				{
					if (!a_object) {
						return false;
					}

					return a_object->As<RE::TESObjectWEAP>() != nullptr || a_object->As<RE::TESObjectARMO>() != nullptr;
				}

				[[nodiscard]] std::string ResolveInventoryDisplayName(
					const RE::InventoryEntryData* a_entry,
					RE::ExtraDataList* a_xList)
				{
					if (!a_entry || !a_entry->object) {
						return "<Unknown>";
					}

					const char* displayRaw = a_xList ? a_xList->GetDisplayName(a_entry->object) : nullptr;
					std::string displayName = displayRaw ? displayRaw : "";
					if (displayName.empty()) {
						const char* objectNameRaw = a_entry->object->GetName();
						displayName = objectNameRaw ? objectNameRaw : "";
					}
					if (displayName.empty()) {
						displayName = "<Unknown>";
					}

					return displayName;
				}

				// Four broad rarity tiers preserve rune progression without an extreme tail.
				// Low/mid/high/top weights are capped at a 4:1 ratio.
				[[nodiscard]] double ResolveRunewordFragmentWeight(std::string_view a_runeName)
				{
					if (a_runeName.empty()) {
						return 1.0;
					}

					constexpr std::array<std::pair<std::string_view, double>, 33> kRuneWeights{ {
						{ "El", 4.0 },
						{ "Eld", 4.0 },
						{ "Tir", 4.0 },
						{ "Nef", 4.0 },
						{ "Eth", 4.0 },
						{ "Ith", 4.0 },
						{ "Tal", 4.0 },
						{ "Ral", 4.0 },
						{ "Ort", 4.0 },
						{ "Thul", 4.0 },
						{ "Amn", 4.0 },
						{ "Sol", 3.0 },
						{ "Shael", 3.0 },
						{ "Dol", 3.0 },
						{ "Hel", 3.0 },
						{ "Io", 3.0 },
						{ "Lum", 3.0 },
						{ "Ko", 3.0 },
						{ "Fal", 3.0 },
						{ "Lem", 3.0 },
						{ "Pul", 3.0 },
						{ "Um", 3.0 },
						{ "Mal", 2.0 },
						{ "Ist", 2.0 },
						{ "Gul", 2.0 },
						{ "Vex", 2.0 },
						{ "Ohm", 2.0 },
						{ "Lo", 2.0 },
						{ "Sur", 1.0 },
						{ "Ber", 1.0 },
						{ "Jah", 1.0 },
						{ "Cham", 1.0 },
						{ "Zod", 1.0 },
					} };

					for (const auto& [name, weight] : kRuneWeights) {
						if (a_runeName == name) {
							return weight;
						}
					}

					return 1.0;
				}
					constexpr std::string_view kRunewordFragmentEditorIdPrefix = "CAFF_RuneFrag_";

				RE::TESObjectMISC* LookupRunewordFragmentItem(
					const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
					std::uint64_t a_runeToken)
				{
					if (a_runeToken == 0u) {
						return nullptr;
					}

					static std::unordered_map<std::uint64_t, RE::TESObjectMISC*> cache;
					if (const auto cacheIt = cache.find(a_runeToken); cacheIt != cache.end()) {
						return cacheIt->second;
					}

					const auto nameIt = a_runeNameByToken.find(a_runeToken);
					if (nameIt == a_runeNameByToken.end() || nameIt->second.empty()) {
						return nullptr;
					}

					std::string editorId;
					editorId.reserve(kRunewordFragmentEditorIdPrefix.size() + nameIt->second.size());
					editorId.append(kRunewordFragmentEditorIdPrefix);
					editorId.append(nameIt->second);

					auto* item = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>(editorId);
					if (item) {
						cache.emplace(a_runeToken, item);
					}
					return item;
				}

				std::uint32_t GetOwnedRunewordFragmentCount(
					RE::PlayerCharacter* a_player,
					const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
					std::uint64_t a_runeToken)
				{
					if (!a_player) {
						return 0u;
					}

					auto* item = LookupRunewordFragmentItem(a_runeNameByToken, a_runeToken);
					if (!item) {
						return 0u;
					}

					const auto owned = a_player->GetItemCount(item);
					return owned > 0 ? static_cast<std::uint32_t>(owned) : 0u;
				}

				std::uint32_t GrantRunewordFragments(
					RE::PlayerCharacter* a_player,
					const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
					std::uint64_t a_runeToken,
					std::uint32_t a_amount)
				{
					if (!a_player || a_amount == 0u) {
						return 0u;
					}

					auto* item = LookupRunewordFragmentItem(a_runeNameByToken, a_runeToken);
					if (!item) {
						return 0u;
					}

					const auto ownedBeforeSigned = std::max(0, a_player->GetItemCount(item));
					const auto ownedBefore = static_cast<std::uint32_t>(ownedBeforeSigned);

					const auto maxGive = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
					const auto give = (a_amount > maxGive) ? maxGive : a_amount;
					a_player->AddObjectToContainer(item, nullptr, static_cast<std::int32_t>(give), nullptr);

					const auto ownedAfterSigned = std::max(0, a_player->GetItemCount(item));
					const auto ownedAfter = static_cast<std::uint32_t>(ownedAfterSigned);
					return (ownedAfter > ownedBefore) ? (ownedAfter - ownedBefore) : 0u;
				}

				bool TryConsumeRunewordFragment(
					RE::PlayerCharacter* a_player,
					const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
					std::uint64_t a_runeToken)
				{
					if (!a_player) {
						return false;
					}

					auto* item = LookupRunewordFragmentItem(a_runeNameByToken, a_runeToken);
					if (!item) {
						return false;
					}

					if (a_player->GetItemCount(item) <= 0) {
						return false;
					}

					a_player->RemoveItem(item, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
					return true;
				}
}
