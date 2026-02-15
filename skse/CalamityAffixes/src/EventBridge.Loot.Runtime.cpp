#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootUiGuards.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
		EventBridge::InstanceRuntimeState& EventBridge::EnsureInstanceRuntimeState(
			std::uint64_t a_instanceKey,
			std::uint64_t a_affixToken)
		{
			return _instanceStates[MakeInstanceStateKey(a_instanceKey, a_affixToken)];
		}

	const EventBridge::InstanceRuntimeState* EventBridge::FindInstanceRuntimeState(
		std::uint64_t a_instanceKey,
		std::uint64_t a_affixToken) const
	{
		if (a_affixToken == 0u) {
			return nullptr;
		}

		const auto it = _instanceStates.find(MakeInstanceStateKey(a_instanceKey, a_affixToken));
		return (it != _instanceStates.end()) ? std::addressof(it->second) : nullptr;
	}

	std::size_t EventBridge::ResolveEvolutionStageIndex(const Action& a_action, const InstanceRuntimeState* a_state) const
	{
		if (!a_action.evolutionEnabled || a_action.evolutionThresholds.empty()) {
			return 0;
		}

		const std::uint32_t xp = a_state ? a_state->evolutionXp : 0u;
		std::size_t stage = 0;
		for (std::size_t i = 0; i < a_action.evolutionThresholds.size(); ++i) {
			if (xp < a_action.evolutionThresholds[i]) {
				break;
			}
			stage = i;
		}
		return stage;
	}

	float EventBridge::ResolveEvolutionMultiplier(const Action& a_action, const InstanceRuntimeState* a_state) const
	{
		if (!a_action.evolutionEnabled || a_action.evolutionMultipliers.empty()) {
			return 1.0f;
		}

		std::size_t stage = ResolveEvolutionStageIndex(a_action, a_state);
		if (stage >= a_action.evolutionMultipliers.size()) {
			stage = a_action.evolutionMultipliers.size() - 1;
		}

		const float mult = a_action.evolutionMultipliers[stage];
		return (mult > 0.0f) ? mult : 1.0f;
	}

	void EventBridge::AdvanceRuntimeStateForAffixProc(const AffixRuntime& a_affix)
	{
		const auto& action = a_affix.action;
		const bool usesEvolution = action.evolutionEnabled && action.evolutionXpPerProc > 0u;
		const bool usesModeCycle = action.modeCycleEnabled && !action.modeCycleSpells.empty() && !action.modeCycleManualOnly;
		if (!usesEvolution && !usesModeCycle) {
			return;
		}

		const auto* cachedKeys = FindEquippedInstanceKeysForAffixTokenCached(a_affix.token);
		std::vector<std::uint64_t> fallbackKeys;
		if (!cachedKeys) {
			fallbackKeys = CollectEquippedInstanceKeysForAffixToken(a_affix.token);
			cachedKeys = std::addressof(fallbackKeys);
		}
		if (!cachedKeys || cachedKeys->empty()) {
			return;
		}

		const std::uint32_t xpPerProc = action.evolutionXpPerProc;
		const std::uint32_t switchEvery = std::max<std::uint32_t>(1u, action.modeCycleSwitchEveryProcs);
		const auto modeCount = static_cast<std::uint32_t>(action.modeCycleSpells.size());

		for (const auto key : *cachedKeys) {
			auto& state = EnsureInstanceRuntimeState(key, a_affix.token);

			if (usesEvolution) {
				const auto maxVal = std::numeric_limits<std::uint32_t>::max();
				if (state.evolutionXp > maxVal - xpPerProc) {
					state.evolutionXp = maxVal;
				} else {
					state.evolutionXp += xpPerProc;
				}
			}

			if (usesModeCycle && modeCount > 0u) {
				const auto maxVal = std::numeric_limits<std::uint32_t>::max();
				if (state.modeCycleCounter < maxVal) {
					state.modeCycleCounter += 1u;
				}
				if (state.modeCycleCounter >= switchEvery) {
					state.modeCycleCounter = 0u;
					state.modeIndex = (state.modeIndex + 1u) % modeCount;
				}
			}
		}
	}

	void EventBridge::CycleManualModeForEquippedAffixes(std::int32_t a_direction, std::string_view a_affixIdFilter)
	{
		if (!_configLoaded || a_direction == 0) {
			return;
		}

		std::uint32_t switched = 0u;
		for (const auto& affix : _affixes) {
			const auto& action = affix.action;
			if (action.type != ActionType::kCastSpell ||
				!action.modeCycleEnabled ||
				!action.modeCycleManualOnly ||
				action.modeCycleSpells.empty()) {
				continue;
			}

			if (!a_affixIdFilter.empty() && affix.id != a_affixIdFilter) {
				continue;
			}

			const auto modeCount = static_cast<std::int32_t>(action.modeCycleSpells.size());
			if (modeCount <= 0) {
				continue;
			}

			const auto keys = CollectEquippedInstanceKeysForAffixToken(affix.token);
			for (const auto key : keys) {
				auto& state = EnsureInstanceRuntimeState(key, affix.token);
				const std::int32_t current = static_cast<std::int32_t>(state.modeIndex % static_cast<std::uint32_t>(modeCount));
				std::int32_t next = (current + a_direction) % modeCount;
				if (next < 0) {
					next += modeCount;
				}

				state.modeIndex = static_cast<std::uint32_t>(next);
				state.modeCycleCounter = 0u;
				switched += 1u;
			}
		}

		if (_loot.debugLog && switched > 0u) {
			spdlog::debug(
				"CalamityAffixes: manual mode cycle switched (count={}, direction={}, filter='{}').",
				switched,
				a_direction,
				std::string(a_affixIdFilter));
		}
	}

	std::optional<std::string> EventBridge::GetInstanceAffixTooltip(
		const RE::InventoryEntryData* a_item,
		std::string_view a_selectedDisplayName,
		int a_uiLanguageMode) const
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

		auto resolveCandidate = [&](RE::ExtraDataList* a_xList) -> std::optional<std::pair<const AffixRuntime*, std::uint64_t>> {
			if (!a_xList) {
				return std::nullopt;
			}

			const auto* uid = a_xList->GetByType<RE::ExtraUniqueID>();
			if (!uid) {
				return std::nullopt;
			}

			const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
			const auto it = _instanceAffixes.find(key);
			if (it == _instanceAffixes.end() || it->second.count == 0) {
				return std::nullopt;
			}

			const auto& slots = it->second;
			const auto primaryToken = slots.GetPrimary();
			if (primaryToken == 0u) {
				return std::nullopt;
			}

			const auto idxIt = _affixIndexByToken.find(primaryToken);
			if (idxIt == _affixIndexByToken.end()) {
				return std::nullopt;
			}
			const auto idx = idxIt->second;
			if (idx >= _affixes.size()) {
				return std::nullopt;
			}

			const auto& affix = _affixes[idx];
			if (affix.displayName.empty()) {
				return std::nullopt;
			}

			return std::pair<const AffixRuntime*, std::uint64_t>{ std::addressof(affix), key };
		};

		struct TooltipCandidate
		{
			const AffixRuntime* affix{ nullptr };
			std::uint64_t instanceKey{ 0 };
			std::string_view rowName{};
		};

		std::vector<TooltipCandidate> candidates;
		for (auto* xList : *a_item->extraLists) {
			const auto resolved = resolveCandidate(xList);
			if (!resolved) {
				continue;
			}

			const char* rowNameRaw = xList->GetDisplayName(a_item->object);
			const std::string_view rowName = rowNameRaw ? std::string_view(rowNameRaw) : std::string_view{};
			candidates.push_back(TooltipCandidate{
				.affix = resolved->first,
				.instanceKey = resolved->second,
				.rowName = rowName
			});
		}

		if (candidates.empty()) {
			for (auto* xList : *a_item->extraLists) {
				if (!xList) {
					continue;
				}
				const auto* uid = xList->GetByType<RE::ExtraUniqueID>();
				if (!uid) {
					continue;
				}

				const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);
				const auto rw = BuildRunewordTooltip(key);
				if (!rw.empty()) {
					return rw;
				}
			}

			return std::nullopt;
		}

		// Helper to format a single affix's detail lines (evolution, mode cycle)
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

		auto formatTooltip = [&](const TooltipCandidate& a_candidate) -> std::string {
			const auto slotsIt = _instanceAffixes.find(a_candidate.instanceKey);
			if (slotsIt == _instanceAffixes.end() || slotsIt->second.count == 0) {
				return {};
			}

			const auto& slots = slotsIt->second;
			std::string tooltip;

			for (std::uint8_t s = 0; s < slots.count; ++s) {
				const auto idxIt = _affixIndexByToken.find(slots.tokens[s]);
				if (idxIt == _affixIndexByToken.end() || idxIt->second >= _affixes.size()) {
					continue;
				}

				const auto& affix = _affixes[idxIt->second];

				// Resolve display name based on language mode
				std::string resolvedName;
				switch (a_uiLanguageMode) {
				case 0:  // English
					resolvedName = affix.displayNameEn;
					break;
				case 1:  // Korean
					resolvedName = affix.displayNameKo;
					break;
				case 2:  // Both
				default:
					if (affix.displayNameKo == affix.displayNameEn || affix.displayNameEn.empty()) {
						resolvedName = affix.displayNameKo;
					} else if (affix.displayNameKo.empty()) {
						resolvedName = affix.displayNameEn;
					} else {
						resolvedName = affix.displayNameKo;
						resolvedName.append(" / ");
						resolvedName.append(affix.displayNameEn);
					}
					break;
				}

				if (resolvedName.empty()) {
					continue;
				}

				if (!tooltip.empty()) {
					tooltip.push_back('\n');
				}

				// Slot number prefix for multi-affix items (with P/S indicator)
				if (slots.count > 1) {
					tooltip.push_back('[');
					if (affix.slot == AffixSlot::kPrefix) {
						tooltip.push_back('P');
					} else if (affix.slot == AffixSlot::kSuffix) {
						tooltip.push_back('S');
					}
					tooltip.append(std::to_string(s + 1));
					tooltip.push_back('/');
					tooltip.append(std::to_string(slots.count));
					tooltip.append("] ");
				}

				tooltip.append(resolvedName);

				// Append evolution/mode details for this affix
				const auto detail = formatAffixDetail(affix, a_candidate.instanceKey);
				if (!detail.empty()) {
					tooltip.push_back('\n');
					tooltip.append(detail);
				}
			}

			// Append runeword info
			const auto runewordTooltip = BuildRunewordTooltip(a_candidate.instanceKey);
			if (!runewordTooltip.empty()) {
				if (!tooltip.empty()) {
					tooltip.push_back('\n');
				}
				tooltip.append(runewordTooltip);
			}

			return tooltip;
		};

		if (!a_selectedDisplayName.empty()) {
			std::vector<TooltipResolutionCandidate> resolutionCandidates;
			resolutionCandidates.reserve(candidates.size());
			for (const auto& candidate : candidates) {
				resolutionCandidates.push_back(TooltipResolutionCandidate{
					.rowName = candidate.rowName,
					.affixToken = candidate.affix ? candidate.affix->token : 0u,
					.instanceKey = candidate.instanceKey
				});
			}

			const auto resolution = ResolveTooltipCandidateSelection(resolutionCandidates, a_selectedDisplayName);
			if (resolution.matchedIndex && *resolution.matchedIndex < candidates.size() && !resolution.ambiguous) {
				return formatTooltip(candidates[*resolution.matchedIndex]);
			}

			// If a specific row name is known but no safe exact match exists,
			// avoid returning a tooltip for a different inventory row.
			return std::nullopt;
		}

		return formatTooltip(candidates.front());
	}
}
