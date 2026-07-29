#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::BuildConfigDerivedAffixState(const nlohmann::json& a_affixes, RE::TESDataHandler* a_handler)
	{
		ParseConfiguredAffixesFromJson(a_affixes, a_handler);
		IndexConfiguredAffixes();
		SynthesizeRunewordRuntimeAffixes();
		RebuildSharedLootPools();
		_affixRuntimeState.activeCounts.assign(_affixRuntimeState.affixes.size(), 0);
	}
}
