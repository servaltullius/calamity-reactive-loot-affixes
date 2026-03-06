#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::BuildConfigDerivedAffixState(const nlohmann::json& a_affixes, RE::TESDataHandler* a_handler)
	{
		ParseConfiguredAffixesFromJson(a_affixes, a_handler);
		IndexConfiguredAffixes();
		SynthesizeRunewordRuntimeAffixes();
		RebuildSharedLootPools();
		_activeCounts.assign(_affixes.size(), 0);
	}
}
