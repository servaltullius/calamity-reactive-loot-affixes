#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::IndexConfiguredAffixes()
	{
		for (std::size_t i = 0; i < _affixes.size(); i++) {
			const auto& affix = _affixes[i];
			IndexAffixLookupKeys(affix, i, false, false);
			IndexAffixLootPool(affix, i);
		}
	}
}
