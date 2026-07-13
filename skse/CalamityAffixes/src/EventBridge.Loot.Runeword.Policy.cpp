#include "CalamityAffixes/EventBridge.h"

#include <cstdint>
#include <string>

namespace CalamityAffixes
{
	EventBridge::RunewordApplyBlockReason EventBridge::ResolveRunewordApplyBlockReason(
		std::uint64_t a_instanceKey,
		const RunewordRecipe& a_recipe) const
	{
		const auto affixIt = _affixRegistry.affixIndexByToken.find(a_recipe.resultAffixToken);
		if (affixIt == _affixRegistry.affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
			return RunewordApplyBlockReason::kMissingResultAffix;
		}

		if (const auto it = _instanceAffixes.find(a_instanceKey); it != _instanceAffixes.end()) {
			const auto& slots = it->second;
			auto effectiveCount = slots.count;
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

	std::string EventBridge::BuildRunewordApplyBlockMessage(RunewordApplyBlockReason a_reason)
	{
		switch (a_reason) {
		case RunewordApplyBlockReason::kMissingResultAffix:
			return "Runeword result affix missing";
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
