#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RunewordUiPolicy.h"

#include <cstdint>
#include <string>
#include <vector>

namespace CalamityAffixes
{
	EventBridge::RunewordApplyBlockReason EventBridge::ResolveRunewordApplyBlockReason(
		std::uint64_t a_instanceKey,
		const RunewordRecipe& a_recipe) const
	{
		const auto affixIt = _affixRuntimeState.affixRegistry.affixIndexByToken.find(a_recipe.resultAffixToken);
		if (affixIt == _affixRuntimeState.affixRegistry.affixIndexByToken.end() || affixIt->second >= _affixRuntimeState.affixes.size()) {
			return RunewordApplyBlockReason::kMissingResultAffix;
		}

		const auto* completed = ResolveCompletedRunewordRecipe(a_instanceKey);
		if (IsSameCompletedRuneword(
				completed ? completed->resultAffixToken : 0u,
				a_recipe.resultAffixToken)) {
			return RunewordApplyBlockReason::kAlreadyComplete;
		}

		if (const auto it = _instanceTrackingState.instanceAffixes.find(a_instanceKey); it != _instanceTrackingState.instanceAffixes.end()) {
			const auto& slots = it->second;
			// The runeword replaces the prefix in the head slot, so it frees a slot too.
			auto effectiveCount = static_cast<std::uint8_t>(
				slots.count - ResolveRunewordDisplacedPrefixTokens(slots).size());
			if (!slots.HasToken(a_recipe.resultAffixToken)) {
				// Keep the completed result active for rollback, but count its slot as
				// replaceable while validating a re-transmutation.
				if (const auto* completed = ResolveCompletedRunewordRecipe(slots);
					completed && completed->resultAffixToken != a_recipe.resultAffixToken &&
					slots.HasToken(completed->resultAffixToken) && effectiveCount > 0u) {
					--effectiveCount;
				}
			}

			if (!slots.HasToken(a_recipe.resultAffixToken) &&
				effectiveCount >= static_cast<std::uint8_t>(kMaxAffixesPerItem)) {
				return RunewordApplyBlockReason::kAffixSlotsFull;
			}
		}

		return RunewordApplyBlockReason::kNone;
	}

	std::vector<std::uint64_t> EventBridge::ResolveRunewordDisplacedPrefixTokens(
		const InstanceAffixSlots& a_slots) const
	{
		std::vector<std::uint64_t> prefixes;
		for (const auto token : a_slots) {
			if (_runewordState.recipeIndexByResultAffixToken.contains(token)) {
				continue;
			}
			const auto it = _affixRuntimeState.affixRegistry.affixIndexByToken.find(token);
			if (it != _affixRuntimeState.affixRegistry.affixIndexByToken.end() &&
				it->second < _affixRuntimeState.affixes.size() &&
				_affixRuntimeState.affixes[it->second].slot == AffixSlot::kPrefix) {
				prefixes.push_back(token);
			}
		}
		return prefixes;
	}

	std::string EventBridge::BuildRunewordApplyBlockMessage(RunewordApplyBlockReason a_reason)
	{
		switch (a_reason) {
		case RunewordApplyBlockReason::kMissingResultAffix:
			return "Runeword result affix missing";
		case RunewordApplyBlockReason::kAlreadyComplete:
			return "Selected base already has this runeword";
		case RunewordApplyBlockReason::kAffixSlotsFull: {
			std::string reason = "Affix slots full (max ";
			reason.append(std::to_string(kMaxAffixesPerItem));
			reason.push_back(')');
			return reason;
		}
		default:
			break;
		}

		return {};
	}
}
