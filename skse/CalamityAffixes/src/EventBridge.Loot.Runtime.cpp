#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <cstdint>
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
			if ((action.type != ActionType::kCastSpell &&
				 action.type != ActionType::kCastSpellAdaptiveElement) ||
				!action.modeCycleEnabled ||
				!action.modeCycleManualOnly ||
				action.modeCycleSpells.empty()) {
				continue;
			}

			if (!a_affixIdFilter.empty() && affix.id != a_affixIdFilter) {
				continue;
			}

			// Adaptive manual mode keeps index 0 as "auto resist-based" and 1..N as overrides.
			const bool adaptiveManualMode = (action.type == ActionType::kCastSpellAdaptiveElement);
			const auto modeCount = adaptiveManualMode ?
				static_cast<std::int32_t>(action.modeCycleSpells.size() + 1u) :
				static_cast<std::int32_t>(action.modeCycleSpells.size());
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
}
