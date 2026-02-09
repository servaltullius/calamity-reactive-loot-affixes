#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootUiGuards.h"

#include <algorithm>
#include <limits>
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

		auto keys = CollectEquippedInstanceKeysForAffixToken(a_affix.token);
		if (keys.empty()) {
			return;
		}

		const std::uint32_t xpPerProc = action.evolutionXpPerProc;
		const std::uint32_t switchEvery = std::max<std::uint32_t>(1u, action.modeCycleSwitchEveryProcs);
		const auto modeCount = static_cast<std::uint32_t>(action.modeCycleSpells.size());

		for (const auto key : keys) {
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
		std::string_view a_selectedDisplayName) const
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
				std::uint64_t token = 0u;
				if (const auto it = _instanceAffixes.find(key); it != _instanceAffixes.end()) {
					token = it->second;
				} else if (const auto supIt = _instanceSupplementalAffixes.find(key); supIt != _instanceSupplementalAffixes.end()) {
					token = supIt->second;
				}
				if (token == 0u) {
					return std::nullopt;
				}

				const auto idxIt = _affixIndexByToken.find(token);
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

		auto formatTooltip = [&](const TooltipCandidate& a_candidate) -> std::string {
			const auto& affix = *a_candidate.affix;
			std::string tooltip = affix.displayName;
			if (tooltip.empty()) {
				return tooltip;
			}

			const auto* state = FindInstanceRuntimeState(a_candidate.instanceKey, affix.token);
			const auto& action = affix.action;

			bool appended = false;
			if (action.evolutionEnabled && !action.evolutionThresholds.empty()) {
				if (!appended) {
					tooltip.append("\n");
					appended = true;
				}

				const std::size_t stage = ResolveEvolutionStageIndex(action, state);
				const std::size_t totalStages = action.evolutionThresholds.size();
				const std::uint32_t xp = state ? state->evolutionXp : 0u;
				const float mult = ResolveEvolutionMultiplier(action, state);

				tooltip.append("Evolution Stage ");
				tooltip.append(std::to_string(stage + 1));
				tooltip.push_back('/');
				tooltip.append(std::to_string(totalStages));
				tooltip.append(" (XP ");
				tooltip.append(std::to_string(xp));
				tooltip.append(", x");
				tooltip.append(std::to_string(mult));
				tooltip.push_back(')');

				if (stage + 1 < action.evolutionThresholds.size()) {
					tooltip.append(" -> Next ");
					tooltip.append(std::to_string(action.evolutionThresholds[stage + 1]));
				}
			}

				if (action.modeCycleEnabled && !action.modeCycleSpells.empty()) {
					if (appended) {
						tooltip.push_back('\n');
					} else {
					tooltip.append("\n");
					appended = true;
				}

				const auto modeCount = static_cast<std::uint32_t>(action.modeCycleSpells.size());
				const std::uint32_t idx = modeCount > 0u ? ((state ? state->modeIndex : 0u) % modeCount) : 0u;

				tooltip.append("Mode ");
				if (idx < action.modeCycleLabels.size() && !action.modeCycleLabels[idx].empty()) {
					tooltip.append(action.modeCycleLabels[idx]);
				} else {
					tooltip.append(std::to_string(idx + 1));
					tooltip.push_back('/');
					tooltip.append(std::to_string(modeCount));
				}
					if (action.modeCycleManualOnly) {
						tooltip.append(" (Manual)");
					}
				}

					const auto runewordTooltip = BuildRunewordTooltip(a_candidate.instanceKey);
					if (!runewordTooltip.empty()) {
						if (appended) {
							tooltip.push_back('\n');
						} else {
							tooltip.append("\n");
						}
						tooltip.append(runewordTooltip);
						appended = true;
					}

					if (const auto supplementalIt = _instanceSupplementalAffixes.find(a_candidate.instanceKey);
						supplementalIt != _instanceSupplementalAffixes.end() && supplementalIt->second != affix.token) {
						if (const auto idxIt = _affixIndexByToken.find(supplementalIt->second);
							idxIt != _affixIndexByToken.end() && idxIt->second < _affixes.size()) {
							const auto& supplementalAffix = _affixes[idxIt->second];
							if (!supplementalAffix.displayName.empty()) {
								if (appended) {
									tooltip.push_back('\n');
								} else {
									tooltip.append("\n");
								}
								tooltip.append("Inherited Affix: ");
								tooltip.append(supplementalAffix.displayName);
							}
						}
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
