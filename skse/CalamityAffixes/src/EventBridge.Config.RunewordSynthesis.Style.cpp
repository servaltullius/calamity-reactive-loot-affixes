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
			RecipeTuning{ "rw_grief", 34.0f, 0.55f, kUnsetValue, 0.20f },
			RecipeTuning{ "rw_infinity", 26.0f, 1.8f, 4.5f, kUnsetValue },
			RecipeTuning{ "rw_enigma", 24.0f, 8.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_call_to_arms", 28.0f, 7.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_nadir", 18.0f, 10.0f, 12.0f, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 38.0f, 50.0f },
			RecipeTuning{ "rw_steel", 30.0f, 0.85f, kUnsetValue, 0.16f },
			RecipeTuning{ "rw_malice", 18.0f, 1.9f, 3.5f, kUnsetValue },
			RecipeTuning{ "rw_stealth", 22.0f, 14.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 40.0f, 55.0f },
			RecipeTuning{ "rw_leaf", 24.0f, 1.0f, kUnsetValue, 0.16f },
			RecipeTuning{ "rw_ancients_pledge", 20.0f, 11.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 35.0f, 48.0f },
			RecipeTuning{ "rw_holy_thunder", 22.0f, 7.8f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_zephyr", 24.0f, 6.9f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_pattern", 29.0f, 1.0f, kUnsetValue, 0.17f },
			RecipeTuning{ "rw_kings_grace", 28.0f, 1.05f, kUnsetValue, 0.16f },
			RecipeTuning{ "rw_strength", 27.0f, 0.95f, kUnsetValue, 0.15f },
			RecipeTuning{ "rw_edge", 24.0f, 1.2f, kUnsetValue, 0.14f },
			RecipeTuning{ "rw_lawbringer", 13.0f, 3.8f, 10.0f, kUnsetValue },
			RecipeTuning{ "rw_wrath", 16.0f, 3.2f, 8.0f, kUnsetValue },
			RecipeTuning{ "rw_kingslayer", 18.0f, 2.6f, 7.0f, kUnsetValue },
			RecipeTuning{ "rw_principle", 14.0f, 4.2f, 9.0f, kUnsetValue },
			RecipeTuning{ "rw_black", 14.0f, 2.4f, 3.5f, kUnsetValue },
			RecipeTuning{ "rw_rift", 15.0f, 2.0f, 3.0f, kUnsetValue },
			RecipeTuning{ "rw_plague", 14.0f, 2.2f, 4.0f, kUnsetValue },
			RecipeTuning{ "rw_wealth", 24.0f, 4.8f, 5.0f, kUnsetValue },
			RecipeTuning{ "rw_obedience", 22.0f, 5.2f, 5.0f, kUnsetValue },
			RecipeTuning{ "rw_honor", 20.0f, 5.6f, 6.0f, kUnsetValue },
			RecipeTuning{ "rw_eternity", 24.0f, 4.4f, 4.5f, kUnsetValue },
			RecipeTuning{ "rw_smoke", 15.0f, 15.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_treachery", 18.0f, 12.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_gloom", 7.0f, 24.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_delirium", 11.0f, 8.5f, 11.0f, kUnsetValue },
			RecipeTuning{ "rw_beast", 26.0f, 3.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_dragon", 16.0f, 10.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_hand_of_justice", 18.0f, 8.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_flickering_flame", 14.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_temper", 17.0f, 8.8f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_voice_of_reason", 16.0f, 9.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_ice", 14.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_pride", 14.0f, 12.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_metamorphosis", 13.0f, 16.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_destruction", 22.0f, 1.1f, kUnsetValue, 0.17f },
			RecipeTuning{ "rw_hustle_w", 24.0f, 2.8f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_harmony", 21.0f, 3.8f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_unbending_will", 25.0f, 2.6f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_stone", 18.0f, 13.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_sanctuary", 17.0f, 13.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_memory", 18.0f, 12.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_wisdom", 17.0f, 13.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_mist", 15.0f, 11.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_myth", 19.0f, 11.5f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 37.0f, 50.0f },
			RecipeTuning{ "rw_spirit", 22.0f, 9.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_lore", 19.0f, 10.7f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_radiance", 20.0f, 10.2f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 35.0f, 49.0f },
			RecipeTuning{ "rw_insight", 23.0f, 8.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_rhyme", 20.0f, 9.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 38.0f, 52.0f },
			RecipeTuning{ "rw_peace", 16.0f, 10.5f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 39.0f, 53.0f },
			RecipeTuning{ "rw_bulwark", 18.0f, 10.5f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 39.0f, 53.0f },
			RecipeTuning{ "rw_cure", 18.0f, 10.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 40.0f, 54.0f },
			RecipeTuning{ "rw_ground", 17.0f, 9.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 36.0f, 50.0f },
			RecipeTuning{ "rw_hearth", 17.0f, 9.2f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_white", 15.0f, 2.6f, 5.0f, kUnsetValue },
			RecipeTuning{ "rw_melody", 20.0f, 9.8f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_hustle_a", 21.0f, 8.8f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_lionheart", 19.0f, 10.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 36.0f, 49.0f },
			RecipeTuning{ "rw_passion", 26.0f, 2.6f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_enlightenment", 18.0f, 11.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_crescent_moon", 25.0f, 1.0f, kUnsetValue, 0.17f },
			RecipeTuning{ "rw_duress", 18.0f, 11.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 37.0f, 50.0f },
			RecipeTuning{ "rw_bone", 10.0f, 22.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 35.0f, 48.0f },
			RecipeTuning{ "rw_venom", 16.0f, 1.9f, 3.8f, kUnsetValue },
			RecipeTuning{ "rw_prudence", 17.0f, 12.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_oath", 31.0f, 0.88f, kUnsetValue, 0.18f },
			RecipeTuning{ "rw_rain", 16.0f, 9.5f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_death", 19.0f, 2.2f, 6.0f, kUnsetValue },
			RecipeTuning{ "rw_heart_of_the_oak", 24.0f, 2.2f, 4.2f, kUnsetValue },
			RecipeTuning{ "rw_fortitude", 21.0f, 9.2f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 34.0f, 48.0f },
			RecipeTuning{ "rw_bramble", 16.0f, 2.0f, 4.0f, kUnsetValue },
			RecipeTuning{ "rw_chains_of_honor", 19.0f, 10.0f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 36.0f, 49.0f },
			RecipeTuning{ "rw_fury", 27.0f, 2.2f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_famine", 19.0f, 2.4f, 6.0f, kUnsetValue },
			RecipeTuning{ "rw_wind", 20.0f, 6.6f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_brand", 22.0f, 2.1f, 5.0f, kUnsetValue },
			RecipeTuning{ "rw_dream", 23.0f, 1.0f, kUnsetValue, 0.17f },
			RecipeTuning{ "rw_faith", 28.0f, 2.4f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_last_wish", 17.0f, 2.9f, 6.0f, kUnsetValue },
			RecipeTuning{ "rw_phoenix", 18.0f, 8.2f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_doom", 21.0f, 1.2f, kUnsetValue, 0.16f },
			RecipeTuning{ "rw_exile", 20.0f, 8.8f, kUnsetValue, kUnsetValue, static_cast<std::int32_t>(Trigger::kLowHealth), 33.0f, 46.0f },
			RecipeTuning{ "rw_chaos", 14.0f, 6.4f, 8.0f, kUnsetValue },
			RecipeTuning{ "rw_mosaic", 14.0f, 2.4f, 4.8f, kUnsetValue },
			RecipeTuning{ "rw_obsession", 13.0f, 2.8f, 5.5f, kUnsetValue },
			RecipeTuning{ "rw_silence", 20.0f, 2.9f, 7.0f, kUnsetValue },
			RecipeTuning{ "rw_splendor", 20.0f, 9.0f, kUnsetValue, kUnsetValue },
			RecipeTuning{ "rw_breath_of_the_dying", 31.0f, 0.8f, kUnsetValue, 0.19f }
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
