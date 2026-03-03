#include "RunewordIdentityOverrides.h"

namespace CalamityAffixes::detail
{
	namespace
	{
		[[nodiscard]] constexpr RunewordIdentityOverride MakeCastSpell(
			RunewordIdentitySpellRole a_spell,
			bool a_applyToSelf = false) noexcept
		{
			RunewordIdentityOverride out{};
			out.actionKind = RunewordIdentityActionKind::kCastSpell;
			out.spell = a_spell;
			out.applyToSelf = a_applyToSelf;
			out.noHitEffectArt = true;
			return out;
		}
	}

	std::optional<RunewordIdentityOverride> ResolveRunewordIdentityOverride(std::string_view a_recipeId)
	{
		// Signatures only — most runewords now use their JSON-configured spell directly.
		if (a_recipeId == "rw_honor") {
			return MakeCastSpell(RunewordIdentitySpellRole::kChaosSunder);
		}
		if (a_recipeId == "rw_eternity") {
			return MakeCastSpell(RunewordIdentitySpellRole::kMeditation, true);
		}
		if (a_recipeId == "rw_chaos") {
			RunewordIdentityOverride out{};
			out.actionKind = RunewordIdentityActionKind::kCastSpellAdaptiveElement;
			out.adaptiveMode = AdaptiveElementMode::kWeakestResist;
			out.noHitEffectArt = true;
			return out;
		}

		return std::nullopt;
	}
}
