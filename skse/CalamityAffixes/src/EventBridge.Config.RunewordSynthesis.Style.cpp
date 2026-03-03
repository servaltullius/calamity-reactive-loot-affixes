#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace CalamityAffixes
{
	void EventBridge::ApplyRunewordIndividualTuning(
		AffixRuntime& a_out,
		std::string_view a_recipeId,
		SyntheticRunewordStyle a_style)
	{
		constexpr float kUnsetValue = -1.0f;
		constexpr std::int32_t kUnsetTrigger = -1;

		struct RecipeTuning
		{
			std::string_view id;
			float procPct;
			float icdSec;
			float perTargetIcdSec;
			float magnitudeMult;
			std::int32_t triggerOverride{ kUnsetTrigger };
			float lowHealthThresholdPct{ kUnsetValue };
			float lowHealthRearmPct{ kUnsetValue };
		};

		static constexpr std::array kRecipeTunings = {
			RecipeTuning{ "rw_grief", 34.0f, 0.55f, kUnsetValue, 0.2f },
			RecipeTuning{ "rw_infinity", 26.0f, 1.8f, 4.5f, kUnsetValue },
			RecipeTuning{ "rw_enigma", 24.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_call_to_arms", 28.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_spirit", 22.0f, 9.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_insight", 23.0f, 8.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_fortitude", 21.0f, 9.2f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 34.0f, 48.0f },
			RecipeTuning{ "rw_heart_of_the_oak", 24.0f, 2.2f, 4.2f, kUnsetValue },
			RecipeTuning{ "rw_last_wish", 17.0f, 2.9f, 6.0f, kUnsetValue },
			RecipeTuning{ "rw_exile", 20.0f, 8.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 33.0f, 46.0f },
			RecipeTuning{ "rw_breath_of_the_dying", 31.0f, 0.8f, kUnsetValue, 0.19f },
			RecipeTuning{ "rw_chains_of_honor", 19.0f, 10.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 36.0f, 49.0f },
			RecipeTuning{ "rw_dream", 23.0f, 1.0f, kUnsetValue, 0.17f },
			RecipeTuning{ "rw_faith", 28.0f, 2.4f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_phoenix", 18.0f, 8.2f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_doom", 21.0f, 1.2f, kUnsetValue, 0.16f },
			RecipeTuning{ "rw_sanctuary", 17.0f, 13.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_obsession", 13.0f, 2.8f, 5.5f, kUnsetValue },
			RecipeTuning{ "rw_beast", 26.0f, 3.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_hustle_w", 24.0f, 2.8f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_hustle_a", 21.0f, 8.8f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_treachery", 18.0f, 12.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_honor", 20.0f, 5.6f, 6.0f, kUnsetValue },
			RecipeTuning{ "rw_eternity", 24.0f, 4.4f, 4.5f, kUnsetValue },
			RecipeTuning{ "rw_leaf", 22.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_wrath", 26.0f, 5.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_brand", 28.0f, 5.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_voice_of_reason", 22.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_pride", 28.0f, 4.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_mist", 18.0f, 6.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_holy_thunder", 22.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_crescent_moon", 22.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_destruction", 28.0f, 5.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_malice", 20.0f, 9.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_venom", 22.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_plague", 40.0f, 4.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_nadir", 12.0f, 16.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_peace", 10.0f, 16.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_rift", 20.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_delirium", 10.0f, 15.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_black", 8.0f, 20.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_ice", 100.0f, 0.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_flickering_flame", 100.0f, 0.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_silence", 26.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_obedience", 18.0f, 5.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_wind", 20.0f, 5.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_famine", 26.0f, 6.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_kingslayer", 18.0f, 2.6f, 7.0f, kUnsetValue },
			RecipeTuning{ "rw_death", 24.0f, 5.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_mosaic", 26.0f, 5.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_chaos", 16.0f, 5.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_steel", 18.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_fury", 24.0f, 3.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_hand_of_justice", 20.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_passion", 22.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_harmony", 20.0f, 6.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_memory", 20.0f, 9.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_temper", 20.0f, 9.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_pattern", 20.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_strength", 20.0f, 9.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_edge", 20.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_oath", 24.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_zephyr", 20.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_melody", 18.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_unbending_will", 22.0f, 9.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_bulwark", 22.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_duress", 20.0f, 12.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_stone", 22.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_prudence", 17.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_bone", 16.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_splendor", 18.0f, 10.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_hearth", 22.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_ground", 22.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_principle", 28.0f, 10.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_ancients_pledge", 20.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_myth", 20.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_dragon", 24.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_bramble", 22.0f, 10.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_metamorphosis", 5.0f, 30.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_lore", 20.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_enlightenment", 22.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_white", 18.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_kings_grace", 22.0f, 9.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_cure", 22.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_rain", 22.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_wisdom", 20.0f, 10.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_lionheart", 100.0f, 45.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 30.0f, 49.0f },
			RecipeTuning{ "rw_rhyme", 20.0f, 9.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 38.0f, 52.0f },
			RecipeTuning{ "rw_wealth", 20.0f, 18.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_radiance", 15.0f, 40.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_stealth", 10.0f, 20.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_smoke", 15.0f, 15.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_gloom", 12.0f, 18.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_lawbringer", 40.0f, 6.0f, kUnsetValue, kUnsetValue }
		};

	struct StyleTuning
	{
		SyntheticRunewordStyle style;
		float procPct;
		float icdSec;
	};

	static constexpr std::array kSummonStyleTunings = {
		StyleTuning{ SyntheticRunewordStyle::kSummonDremoraLord, 5.0f, 34.0f },
		StyleTuning{ SyntheticRunewordStyle::kSummonStormAtronach, 7.0f, 26.0f },
		StyleTuning{ SyntheticRunewordStyle::kSummonFlameAtronach, 8.0f, 24.0f },
		StyleTuning{ SyntheticRunewordStyle::kSummonFrostAtronach, 8.0f, 24.0f },
		StyleTuning{ SyntheticRunewordStyle::kSummonFamiliar, 10.0f, 22.0f }
	};

		const auto toDurationMs = [](float a_seconds) -> std::chrono::milliseconds {
			const float clamped = std::max(0.0f, a_seconds);
			return std::chrono::milliseconds(static_cast<std::int64_t>(clamped * 1000.0f));
		};
		const auto setProcIcd = [&](float a_procPct, float a_icdSec) {
			a_out.procChancePct = a_procPct;
			a_out.icd = toDurationMs(a_icdSec);
		};
		const auto setPerTargetIcd = [&](float a_seconds) {
			a_out.perTargetIcd = toDurationMs(a_seconds);
		};
		const auto setMagnitudeMult = [&](float a_mult) {
			if (a_out.action.magnitudeScaling.source != MagnitudeScaling::Source::kNone) {
				a_out.action.magnitudeScaling.mult = a_mult;
			}
		};
		const auto setTrigger = [&](Trigger a_trigger) {
			a_out.trigger = a_trigger;
		};
		const auto setLowHealthWindow = [&](float a_thresholdPct, float a_rearmPct) {
			a_out.lowHealthThresholdPct = std::clamp(a_thresholdPct, 1.0f, 95.0f);
			a_out.lowHealthRearmPct = std::clamp(a_rearmPct, a_out.lowHealthThresholdPct + 1.0f, 100.0f);
		};

		for (const auto& tuning : kRecipeTunings) {
			if (tuning.id != a_recipeId) {
				continue;
			}
			setProcIcd(tuning.procPct, tuning.icdSec);
			if (tuning.perTargetIcdSec > 0.0f) {
				setPerTargetIcd(tuning.perTargetIcdSec);
			}
			if (tuning.magnitudeMult > 0.0f) {
				setMagnitudeMult(tuning.magnitudeMult);
			}
			if (tuning.triggerOverride != kUnsetTrigger) {
				setTrigger(static_cast<Trigger>(tuning.triggerOverride));
			}
			if (tuning.lowHealthThresholdPct > 0.0f) {
				const float rearmPct =
					tuning.lowHealthRearmPct > 0.0f ?
					tuning.lowHealthRearmPct :
					(tuning.lowHealthThresholdPct + 10.0f);
				setLowHealthWindow(tuning.lowHealthThresholdPct, rearmPct);
			}
			break;
		}

	for (const auto& tuning : kSummonStyleTunings) {
		if (tuning.style != a_style) {
			continue;
		}
			setProcIcd(tuning.procPct, tuning.icdSec);
			break;
		}
	}
}
