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
		// Keep this map small and intentional: only recipes where D2 identity drift is obvious.
		if (a_recipeId == "rw_edge") {
			return MakeCastSpell(RunewordIdentitySpellRole::kChaosFragile);
		}
		if (a_recipeId == "rw_strength") {
			return MakeCastSpell(RunewordIdentitySpellRole::kChaosSlowAttack);
		}
		if (a_recipeId == "rw_pattern") {
			return MakeCastSpell(RunewordIdentitySpellRole::kHaste, true);
		}
		if (a_recipeId == "rw_lawbringer") {
			return MakeCastSpell(RunewordIdentitySpellRole::kChaosSunder);
		}
		if (a_recipeId == "rw_wrath") {
			return MakeCastSpell(RunewordIdentitySpellRole::kChaosFragile);
		}
		if (a_recipeId == "rw_kingslayer") {
			return MakeCastSpell(RunewordIdentitySpellRole::kChaosSlowAttack);
		}
		if (a_recipeId == "rw_principle") {
			return MakeCastSpell(RunewordIdentitySpellRole::kFear);
		}
		if (a_recipeId == "rw_venom") {
			return MakeCastSpell(RunewordIdentitySpellRole::kDotSiphon);
		}
		if (a_recipeId == "rw_bramble") {
			return MakeCastSpell(RunewordIdentitySpellRole::kDotTar);
		}
		if (a_recipeId == "rw_mist") {
			return MakeCastSpell(RunewordIdentitySpellRole::kDynamicFrost);
		}
		if (a_recipeId == "rw_pride") {
			return MakeCastSpell(RunewordIdentitySpellRole::kWard, true);
		}
		if (a_recipeId == "rw_fury") {
			return MakeCastSpell(RunewordIdentitySpellRole::kArcLightning);
		}
		if (a_recipeId == "rw_obedience") {
			return MakeCastSpell(RunewordIdentitySpellRole::kChaosSunder);
		}
		if (a_recipeId == "rw_eternity") {
			return MakeCastSpell(RunewordIdentitySpellRole::kMeditation, true);
		}
		if (a_recipeId == "rw_harmony") {
			return MakeCastSpell(RunewordIdentitySpellRole::kHaste, true);
		}
		if (a_recipeId == "rw_honor") {
			return MakeCastSpell(RunewordIdentitySpellRole::kChaosSunder);
		}
		if (a_recipeId == "rw_wealth") {
			return MakeCastSpell(RunewordIdentitySpellRole::kPhase, true);
		}
		if (a_recipeId == "rw_memory") {
			return MakeCastSpell(RunewordIdentitySpellRole::kWard, true);
		}
		if (a_recipeId == "rw_melody") {
			return MakeCastSpell(RunewordIdentitySpellRole::kHaste, true);
		}
		if (a_recipeId == "rw_wisdom") {
			return MakeCastSpell(RunewordIdentitySpellRole::kPhase, true);
		}
		if (a_recipeId == "rw_lore") {
			return MakeCastSpell(RunewordIdentitySpellRole::kWard, true);
		}
		if (a_recipeId == "rw_enlightenment") {
			return MakeCastSpell(RunewordIdentitySpellRole::kMeditation, true);
		}

		return std::nullopt;
	}
}
