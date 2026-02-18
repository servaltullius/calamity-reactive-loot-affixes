#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootUiGuards.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
		[[nodiscard]] bool IsPreviewItemSource(std::string_view a_source) noexcept
		{
			return a_source == "barter" || a_source == "container" || a_source == "gift";
		}
	}

	std::optional<std::string> EventBridge::GetInstanceAffixTooltip(
		const RE::InventoryEntryData* a_item,
		std::string_view a_selectedDisplayName,
		int a_uiLanguageMode,
		std::string_view a_itemSource,
		RE::FormID a_sourceContainerFormID,
		std::uint64_t a_preferredInstanceKey)
	{
		if (!_configLoaded) {
			return std::nullopt;
		}
		if (!a_item) {
			return std::nullopt;
		}

		if (!a_item->extraLists) {
			return std::nullopt;
		}

		std::optional<LootItemType> itemType{};
		if (a_item->object && a_item->object->As<RE::TESObjectWEAP>()) {
			itemType = LootItemType::kWeapon;
		} else if (a_item->object && a_item->object->As<RE::TESObjectARMO>()) {
			itemType = LootItemType::kArmor;
		}

		// Preview-time synthetic affix rolls are disabled by policy.
		// Tooltip now surfaces only already-materialized instance data.
		const bool allowPreview = ShouldEnableSyntheticLootPreviewTooltip() &&
			itemType.has_value() &&
			IsPreviewItemSource(a_itemSource) &&
			IsLootObjectEligibleForAffixes(a_item->object);
		(void)a_sourceContainerFormID;

		struct TooltipCandidate
		{
			std::uint64_t instanceKey{ 0u };
			std::uint64_t primaryToken{ 0u };
			std::string rowName{};
			InstanceAffixSlots slots{};
			bool preview{ false };
		};

		std::vector<TooltipCandidate> candidates;
		for (auto* xList : *a_item->extraLists) {
			if (!xList) {
				continue;
			}

			auto* uid = xList->GetByType<RE::ExtraUniqueID>();
			if (!uid && allowPreview) {
				auto* player = RE::PlayerCharacter::GetSingleton();
				auto* changes = player ? player->GetInventoryChanges() : nullptr;
				if (changes) {
					auto* created = RE::BSExtraData::Create<RE::ExtraUniqueID>();
					if (created) {
						created->baseID = a_item->object ? a_item->object->GetFormID() : 0u;
						created->uniqueID = changes->GetNextUniqueID();
						xList->Add(created);
						uid = created;
					}
				}
			}

			if (!uid) {
				continue;
			}

			const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);

			TooltipCandidate candidate{};
			candidate.instanceKey = key;
			candidate.preview = false;
			candidate.slots.Clear();

			if (const auto mappedIt = _instanceAffixes.find(key); mappedIt != _instanceAffixes.end()) {
				candidate.slots = mappedIt->second;
			}

			if (candidate.slots.count == 0 && allowPreview) {
				if (const auto* previewSlots = FindLootPreviewSlots(key)) {
					candidate.slots = *previewSlots;
					candidate.preview = true;
				} else if (const auto generatedPreview = BuildLootPreviewAffixSlots(key, *itemType); generatedPreview.has_value()) {
					RememberLootPreviewSlots(key, *generatedPreview);
					if (const auto* storedPreview = FindLootPreviewSlots(key)) {
						candidate.slots = *storedPreview;
						candidate.preview = true;
					}
				}
			}

			// For normal runtime candidates, skip items without mapped slots.
			// For preview mode, keep zero-slot candidates so users can see "no affix" before pickup.
			if (candidate.slots.count == 0 && !candidate.preview) {
				continue;
			}

			candidate.primaryToken = candidate.slots.GetPrimary();
			const char* rowNameRaw = xList->GetDisplayName(a_item->object);
			candidate.rowName = rowNameRaw ? rowNameRaw : "";
			candidates.push_back(std::move(candidate));
		}

		if (candidates.empty()) {
			return std::nullopt;
		}

		auto formatAffixDetail = [&](const AffixRuntime& a_affix, std::uint64_t a_instanceKey) -> std::string {
			std::string detail;
			const auto* state = FindInstanceRuntimeState(a_instanceKey, a_affix.token);
			const auto& action = a_affix.action;

			if (action.modeCycleEnabled && !action.modeCycleSpells.empty()) {
				const auto modeCount = static_cast<std::uint32_t>(action.modeCycleSpells.size());
				const std::uint32_t idx = modeCount > 0u ? ((state ? state->modeIndex : 0u) % modeCount) : 0u;

				detail.append("Mode ");
				if (idx < action.modeCycleLabels.size() && !action.modeCycleLabels[idx].empty()) {
					detail.append(action.modeCycleLabels[idx]);
				} else {
					detail.append(std::to_string(idx + 1));
					detail.push_back('/');
					detail.append(std::to_string(modeCount));
				}
				if (action.modeCycleManualOnly) {
					detail.append(" (Manual)");
				}
			}

			return detail;
		};

		auto resolveDisplayName = [&](const AffixRuntime& a_affix) -> std::string {
			switch (a_uiLanguageMode) {
			case 0:
				return a_affix.displayNameEn;
			case 1:
				return a_affix.displayNameKo;
			case 2:
			default:
				if (a_affix.displayNameKo == a_affix.displayNameEn || a_affix.displayNameEn.empty()) {
					return a_affix.displayNameKo;
				}
				if (a_affix.displayNameKo.empty()) {
					return a_affix.displayNameEn;
				}
				return a_affix.displayNameKo + " / " + a_affix.displayNameEn;
			}
		};

		auto formatTooltip = [&](const TooltipCandidate& a_candidate) -> std::string {
			if (a_candidate.slots.count == 0) {
				if (a_candidate.preview) {
					return "Preview: No affix";
				}
				return {};
			}

			std::string tooltip;
			if (a_candidate.preview) {
				tooltip.append("Preview\n");
			}

			for (std::uint8_t s = 0; s < a_candidate.slots.count; ++s) {
				const auto token = a_candidate.slots.tokens[s];
				const auto idxIt = _affixIndexByToken.find(token);
				if (idxIt == _affixIndexByToken.end() || idxIt->second >= _affixes.size()) {
					continue;
				}

				const auto& affix = _affixes[idxIt->second];
				auto resolvedName = resolveDisplayName(affix);
				if (resolvedName.empty()) {
					continue;
				}

				if (!tooltip.empty() && tooltip.back() != '\n') {
					tooltip.push_back('\n');
				}

				if (a_candidate.slots.count > 1) {
					tooltip.push_back('[');
					if (affix.slot == AffixSlot::kPrefix) {
						tooltip.push_back('P');
					} else if (affix.slot == AffixSlot::kSuffix) {
						tooltip.push_back('S');
					}
					tooltip.append(std::to_string(s + 1));
					tooltip.push_back('/');
					tooltip.append(std::to_string(a_candidate.slots.count));
					tooltip.append("] ");
				}

				tooltip.append(resolvedName);

				const auto detail = formatAffixDetail(affix, a_candidate.instanceKey);
				if (!detail.empty()) {
					tooltip.push_back('\n');
					tooltip.append(detail);
				}
			}

			return tooltip;
		};

		std::vector<TooltipResolutionCandidate> resolutionCandidates;
		resolutionCandidates.reserve(candidates.size());
		for (const auto& candidate : candidates) {
			resolutionCandidates.push_back(TooltipResolutionCandidate{
				.rowName = candidate.rowName,
				.affixToken = candidate.primaryToken,
				.instanceKey = candidate.instanceKey
			});
		}

		if (a_preferredInstanceKey != 0u) {
			const auto preferredIt = std::find_if(
				candidates.begin(),
				candidates.end(),
				[a_preferredInstanceKey](const TooltipCandidate& a_candidate) {
					return a_candidate.instanceKey == a_preferredInstanceKey;
				});

			if (preferredIt == candidates.end()) {
				return std::nullopt;
			}

			const auto preferredTooltip = formatTooltip(*preferredIt);
			if (preferredTooltip.empty()) {
				return std::nullopt;
			}
			return preferredTooltip;
		}

		const auto resolution = ResolveTooltipCandidateSelection(resolutionCandidates, a_selectedDisplayName);
		if (resolution.matchedIndex &&
			*resolution.matchedIndex < candidates.size() &&
			!resolution.ambiguous) {
			const auto& matched = candidates[*resolution.matchedIndex];
			const auto formatted = formatTooltip(matched);
			if (formatted.empty()) {
				return std::nullopt;
			}
			return formatted;
		}

		if (!a_selectedDisplayName.empty()) {
			return std::nullopt;
		}

		const auto fallback = formatTooltip(candidates.front());
		if (fallback.empty()) {
			return std::nullopt;
		}
		return fallback;
	}
}
