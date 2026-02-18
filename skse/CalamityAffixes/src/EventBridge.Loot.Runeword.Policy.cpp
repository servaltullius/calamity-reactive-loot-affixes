#include "CalamityAffixes/EventBridge.h"

#include <cstdint>
#include <string>

namespace CalamityAffixes
{
	EventBridge::RunewordApplyBlockReason EventBridge::ResolveRunewordApplyBlockReason(
		std::uint64_t a_instanceKey,
		const RunewordRecipe& a_recipe) const
	{
		const auto affixIt = _affixIndexByToken.find(a_recipe.resultAffixToken);
		if (affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
			return RunewordApplyBlockReason::kMissingResultAffix;
		}

		if (const auto it = _instanceAffixes.find(a_instanceKey); it != _instanceAffixes.end()) {
			const auto& slots = it->second;
			if (!slots.HasToken(a_recipe.resultAffixToken) &&
				slots.count >= static_cast<std::uint8_t>(kMaxAffixesPerItem)) {
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
