#include "../src/EventBridge.Loot.Runeword.SummaryText.h"

namespace
{
	using namespace CalamityAffixes::RunewordSummary;

	// The keys ResolveEffectSummaryKey can return without an override.
	constexpr std::string_view kBucketKeys[] = {
		"adaptive_strike", "adaptive_exposure", "fire_strike", "frost_strike", "shock_strike", "self_haste",
		"self_ward", "self_barrier", "self_meditation", "self_phase", "self_muffle", "self_phoenix",
	};

	constexpr bool HasBothLanguages(std::string_view a_key)
	{
		return !ActionSummaryTextByKey(a_key).empty() && !ActionSummaryTextEnByKey(a_key).empty();
	}

	// items-10: every recipe the panel lists has an English summary, so English
	// never falls back to "cast Calamity: Runeword X".
	constexpr bool CheckEveryRecipeKeyIsBilingual()
	{
		for (const auto& entry : kEffectSummaryOverrides) {
			if (!HasBothLanguages(entry.key)) {
				return false;
			}
		}
		for (const auto key : kBucketKeys) {
			if (!HasBothLanguages(key)) {
				return false;
			}
		}
		return true;
	}
	static_assert(CheckEveryRecipeKeyIsBilingual());

	// items-11: the composer already writes the trigger ("피격 시 25% 확률로 ...",
	// "On hit 30% chance to ..."), so no entry repeats one.
	constexpr bool CheckNoEntryRepeatsTheTrigger()
	{
		constexpr std::string_view kKoreanTriggers[] = {
			"피격 시", "적중 시", "처치 시", "체력 위급 시", "타격 시", "체력 35% 이하",
		};
		constexpr std::string_view kEnglishTriggers[] = {
			"On hit", "On kill", "When hit", "On taking", "At low health",
		};
		for (const auto& entry : kActionSummaryTexts) {
			for (const auto trigger : kKoreanTriggers) {
				if (entry.ko.starts_with(trigger)) {
					return false;
				}
			}
			for (const auto trigger : kEnglishTriggers) {
				if (entry.en.starts_with(trigger)) {
					return false;
				}
			}
			if (entry.en.find("Calamity:") != std::string_view::npos || entry.ko.find("Calamity:") != std::string_view::npos) {
				return false;
			}
		}
		return true;
	}
	static_assert(CheckNoEntryRepeatsTheTrigger());

	constexpr bool CheckEntriesAreUnique()
	{
		constexpr auto count = sizeof(kActionSummaryTexts) / sizeof(kActionSummaryTexts[0]);
		for (std::size_t i = 0; i < count; ++i) {
			for (std::size_t j = i + 1; j < count; ++j) {
				if (kActionSummaryTexts[i].key == kActionSummaryTexts[j].key) {
					return false;
				}
			}
		}
		return true;
	}
	static_assert(CheckEntriesAreUnique());

	// Eternity: "(6초)" is already in the text, so " (지속 6초)" is not added.
	static_assert(StatesDurationSeconds(ActionSummaryTextByKey("signature_eternity"), "6", "초"));
	static_assert(StatesDurationSeconds("Invisibility and Move Speed +45%, 4s)", "4", "s"));
	static_assert(!StatesDurationSeconds("(14s)", "4", "s"));
	static_assert(!StatesDurationSeconds("(1.5s)", "5", "s"));
	static_assert(!StatesDurationSeconds("화염 피해 15", "6", "초"));
	// Famine states two durations; the longer one (6s) is the spell's.
	static_assert(StatesDurationSeconds(ActionSummaryTextByKey("signature_famine"), "6", "초"));
}

namespace
{
	using CalamityAffixes::RunewordSummary::AreaFeetToMetres;
	static_assert(AreaFeetToMetres(16u) == 5.0f);
	static_assert(AreaFeetToMetres(11u) == 3.5f);
	static_assert(AreaFeetToMetres(200u) == 60.0f);
	static_assert(AreaFeetToMetres(1u) == 0.5f);
}
