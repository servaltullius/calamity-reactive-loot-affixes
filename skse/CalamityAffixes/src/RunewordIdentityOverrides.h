#pragma once

#include "CalamityAffixes/AdaptiveElement.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace CalamityAffixes::detail
{
	enum class RunewordIdentityActionKind : std::uint8_t
	{
		kCastSpell,
		kCastSpellAdaptiveElement,
	};

	enum class RunewordIdentitySpellRole : std::uint8_t
	{
		kDynamicFire,
		kDynamicFrost,
		kDynamicShock,
		kArcLightning,
		kHaste,
		kMeditation,
		kWard,
		kPhase,
		kDotPoison,
		kDotTar,
		kDotSiphon,
		kChaosSunder,
		kChaosFragile,
		kChaosSlowAttack,
		kFear,
		kSoulTrap,
	};

	struct RunewordIdentityOverride
	{
		RunewordIdentityActionKind actionKind{ RunewordIdentityActionKind::kCastSpell };
		RunewordIdentitySpellRole spell{ RunewordIdentitySpellRole::kDynamicFire };
		RunewordIdentitySpellRole adaptiveFire{ RunewordIdentitySpellRole::kDynamicFire };
		RunewordIdentitySpellRole adaptiveFrost{ RunewordIdentitySpellRole::kDynamicFrost };
		RunewordIdentitySpellRole adaptiveShock{ RunewordIdentitySpellRole::kDynamicShock };
		AdaptiveElementMode adaptiveMode{ AdaptiveElementMode::kWeakestResist };
		bool applyToSelf{ false };
		bool noHitEffectArt{ true };
	};

	[[nodiscard]] std::optional<RunewordIdentityOverride> ResolveRunewordIdentityOverride(std::string_view a_recipeId);
}

