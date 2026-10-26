#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/PluginEditorIds.h"

namespace CalamityAffixes
{
	void EventBridge::BuildConfigDerivedAffixState(const nlohmann::json& a_affixes, RE::TESDataHandler* a_handler)
	{
		ParseConfiguredAffixesFromJson(a_affixes, a_handler);
		_parsedConfiguredAffixCount = _affixRuntimeState.affixes.size();
		// Pruning tokens the config does not know only removes retired affixes
		// when the config itself is trustworthy. With the plugin missing, renamed,
		// merged or overridden by an old copy, nearly every affix fails to parse
		// and pruning would wipe every saved item for good.
		_unknownAffixTokenPruningAllowed = PluginEditorIds::IsIndexed() && _parsedConfiguredAffixCount > 0u;
		if (!_unknownAffixTokenPruningAllowed) {
			SKSE::log::warn(
				"CalamityAffixes: affix config not trusted (editor ID index {}, {} affixes parsed); saved affixes the config does not know are kept as they are.",
				PluginEditorIds::IsIndexed() ? "built" : "missing",
				_parsedConfiguredAffixCount);
		}
		IndexConfiguredAffixes();
		SynthesizeRunewordRuntimeAffixes();
		RebuildSharedLootPools();
		_affixRuntimeState.activeCounts.assign(_affixRuntimeState.affixes.size(), 0);
	}
}
