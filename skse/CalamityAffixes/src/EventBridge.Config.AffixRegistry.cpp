#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::RegisterSynthesizedAffix(AffixRuntime&& a_affix, bool a_warnOnDuplicate)
	{
		if (a_affix.displayName.empty()) {
			if (!a_affix.label.empty()) {
				a_affix.displayName = a_affix.label;
			} else {
				a_affix.displayName = a_affix.id;
			}
		}
		if (a_affix.displayNameEn.empty()) {
			a_affix.displayNameEn = a_affix.displayName;
		}
		if (a_affix.displayNameKo.empty()) {
			a_affix.displayNameKo = a_affix.displayName;
		}

		_affixes.push_back(std::move(a_affix));
		const auto idx = _affixes.size() - 1;
		const auto& affix = _affixes[idx];

		const bool isTriggerAction =
			affix.action.type == ActionType::kDebugNotify ||
			affix.action.type == ActionType::kCastSpell ||
			affix.action.type == ActionType::kCastSpellAdaptiveElement ||
			affix.action.type == ActionType::kSpawnTrap;
		if (isTriggerAction) {
			switch (affix.trigger) {
			case Trigger::kHit:
				_hitTriggerAffixIndices.push_back(idx);
				break;
			case Trigger::kIncomingHit:
				_incomingHitTriggerAffixIndices.push_back(idx);
				break;
			case Trigger::kDotApply:
				_dotApplyTriggerAffixIndices.push_back(idx);
				break;
			case Trigger::kKill:
				_killTriggerAffixIndices.push_back(idx);
				break;
			}
		}

		IndexAffixLookupKeys(affix, idx, true, a_warnOnDuplicate);
		IndexAffixLootPool(affix, idx);
	}
}
