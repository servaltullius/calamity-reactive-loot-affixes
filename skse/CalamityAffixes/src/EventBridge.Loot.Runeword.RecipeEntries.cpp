#include "CalamityAffixes/EventBridge.h"
#include "EventBridge.Loot.Runeword.Detail.h"
#include "EventBridge.Loot.Runeword.SummaryText.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	namespace
	{
		struct RecipeKeyOverride
		{
			std::string_view id;
			std::string_view key;
		};

		template <std::size_t N>
		[[nodiscard]] constexpr std::string_view FindKeyOverride(
			std::string_view a_id,
			const RecipeKeyOverride (&a_overrides)[N]) noexcept
		{
			for (const auto& entry : a_overrides) {
				if (entry.id == a_id) {
					return entry.key;
				}
			}
			return {};
		}

		template <std::size_t N>
		[[nodiscard]] constexpr bool HasDuplicateOverrideIds(const RecipeKeyOverride (&a_overrides)[N]) noexcept
		{
			for (std::size_t i = 0; i < N; ++i) {
				for (std::size_t j = i + 1; j < N; ++j) {
					if (a_overrides[i].id == a_overrides[j].id) {
						return true;
					}
				}
			}
			return false;
		}

		[[nodiscard]] constexpr std::uint32_t RecipeBucket(std::string_view a_id, std::uint32_t a_modulo) noexcept
		{
			if (a_modulo == 0u) {
				return 0u;
			}
			std::uint32_t hash = 2166136261u;
			for (const auto c : a_id) {
				hash ^= static_cast<std::uint8_t>(c);
				hash *= 16777619u;
			}
			return hash % a_modulo;
		}

		static constexpr RecipeKeyOverride kRecommendedBaseOverrides[] = {
			{ "rw_insight", "polearm" },
			{ "rw_infinity", "polearm" },
			{ "rw_pride", "polearm" },
			{ "rw_obedience", "polearm" },
			{ "rw_faith", "bow" },
			{ "rw_ice", "bow" },
			{ "rw_mist", "bow" },
			{ "rw_harmony", "bow" },
			{ "rw_edge", "bow" },
			{ "rw_melody", "bow" },
			{ "rw_wrath", "bow" },
			{ "rw_brand", "bow" },
			{ "rw_leaf", "staff_wand" },
			{ "rw_memory", "staff_wand" },
			{ "rw_white", "staff_wand" },
			{ "rw_obsession", "staff_wand" },
			{ "rw_chaos", "claw" },
			{ "rw_mosaic", "claw" },
			{ "rw_grief", "sword" },
			{ "rw_lawbringer", "sword" },
			{ "rw_oath", "sword" },
			{ "rw_unbending_will", "sword" },
			{ "rw_kingslayer", "sword" },
			{ "rw_exile", "shield" },
			{ "rw_sanctuary", "shield" },
			{ "rw_ancients_pledge", "shield" },
			{ "rw_rhyme", "shield" },
			{ "rw_splendor", "shield" },
			{ "rw_lore", "helm" },
			{ "rw_nadir", "helm" },
			{ "rw_delirium", "helm" },
			{ "rw_bulwark", "helm" },
			{ "rw_cure", "helm" },
			{ "rw_ground", "helm" },
			{ "rw_hearth", "helm" },
			{ "rw_temper", "helm" },
			{ "rw_flickering_flame", "helm" },
			{ "rw_wisdom", "helm" },
			{ "rw_metamorphosis", "helm" },
			{ "rw_enigma", "armor" },
			{ "rw_chains_of_honor", "armor" },
			{ "rw_stealth", "armor" },
			{ "rw_smoke", "armor" },
			{ "rw_treachery", "armor" },
			{ "rw_duress", "armor" },
			{ "rw_gloom", "armor" },
			{ "rw_stone", "armor" },
			{ "rw_bramble", "armor" },
			{ "rw_myth", "armor" },
			{ "rw_peace", "armor" },
			{ "rw_prudence", "armor" },
			{ "rw_rain", "armor" },
			{ "rw_spirit", "sword_shield" },
			{ "rw_phoenix", "weapon_shield" },
			{ "rw_dream", "helm_shield" },
			{ "rw_dragon", "armor_shield" },
		};

		static_assert(!HasDuplicateOverrideIds(kRecommendedBaseOverrides),
			"kRecommendedBaseOverrides must not contain duplicate recipe ids.");
	}

	std::vector<EventBridge::RunewordRecipeEntry> EventBridge::GetRunewordRecipeEntries()
	{
		std::vector<RunewordRecipeEntry> entries;
		if (!_configLoaded) {
			return entries;
		}

		SanitizeRunewordState();
		entries.reserve(_runewordRecipes.size());

		auto resolveRecommendedBaseKey = [&](const RunewordRecipe& a_recipe) -> std::string_view {
			const std::string_view id = a_recipe.id;
			if (const auto overrideKey = FindKeyOverride(id, kRecommendedBaseOverrides); !overrideKey.empty()) {
				return overrideKey;
			}

			if (!a_recipe.recommendedBaseType) {
				return "mixed";
			}
			return *a_recipe.recommendedBaseType == LootItemType::kArmor ? "armor" : "weapon";
		};

		auto resolveRecipeEffectSummaryKey = [&](const RunewordRecipe& a_recipe) -> std::string_view {
			RunewordSummary::BaseType baseType = RunewordSummary::BaseType::kUnknown;
			if (a_recipe.recommendedBaseType.has_value()) {
				baseType = (*a_recipe.recommendedBaseType == LootItemType::kWeapon) ?
				               RunewordSummary::BaseType::kWeapon :
				               RunewordSummary::BaseType::kArmor;
			}
			return RunewordSummary::ResolveEffectSummaryKey(a_recipe.id, baseType);
		};

		auto formatFloat1 = [](float a_value) {
			char buffer[32]{};
			std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<double>(a_value));
			return std::string(buffer);
		};

		auto formatNumberCompact = [&](float a_value) {
			std::string text = formatFloat1(a_value);
			if (text.size() >= 2 && text.compare(text.size() - 2, 2, ".0") == 0) {
				text.erase(text.size() - 2);
			}
			return text;
		};

		auto appendDelimited = [](std::string& a_out, std::string_view a_part) {
			if (a_part.empty()) {
				return;
			}
			if (!a_out.empty()) {
				a_out.append(" · ");
			}
			a_out.append(a_part);
		};

		auto triggerText = [](Trigger a_trigger) -> std::string_view {
			switch (a_trigger) {
			case Trigger::kHit:
				return "적중 시";
			case Trigger::kIncomingHit:
				return "피격 시";
			case Trigger::kDotApply:
				return "지속 피해 적용 시";
			case Trigger::kKill:
				return "처치 시";
			case Trigger::kLowHealth:
				return "저체력 시";
			}
			return "트리거 시";
		};

		auto spellNameOr = [](RE::SpellItem* a_spell, std::string_view a_fallback) {
			if (!a_spell) {
				return std::string(a_fallback);
			}
			const char* rawName = a_spell->GetName();
			if (!rawName || rawName[0] == '\0') {
				return std::string(a_fallback);
			}
			return std::string(rawName);
		};

		auto buildSpellProfileText = [&](RE::SpellItem* a_spell, std::string_view a_prefix) -> std::string {
			if (!a_spell) {
				return {};
			}

			const auto spellName = spellNameOr(a_spell, "Spell");
			std::vector<std::string> effectProfiles;
			effectProfiles.reserve(3);
			std::size_t totalEffects = 0u;

			for (const auto* effect : a_spell->effects) {
				if (!effect || !effect->baseEffect) {
					continue;
				}

				++totalEffects;
				if (effectProfiles.size() >= 3u) {
					continue;
				}

				std::string effectProfile;
				const char* effectNameRaw = effect->baseEffect->GetName();
				if (effectNameRaw && effectNameRaw[0] != '\0') {
					effectProfile.append(effectNameRaw);
				} else {
					effectProfile.append("Effect");
				}

				std::string metrics;
				if (std::abs(effect->effectItem.magnitude) >= 0.01f) {
					appendDelimited(metrics, "위력 " + formatFloat1(effect->effectItem.magnitude));
				}
				if (effect->effectItem.duration > 0u) {
					appendDelimited(metrics, "지속 " + std::to_string(effect->effectItem.duration) + "초");
				}
				if (effect->effectItem.area > 0u) {
					appendDelimited(metrics, "범위 " + std::to_string(effect->effectItem.area));
				}

				if (!metrics.empty()) {
					effectProfile.append(" (");
					effectProfile.append(metrics);
					effectProfile.push_back(')');
				}
				effectProfiles.push_back(std::move(effectProfile));
			}

			std::string profile;
			if (!a_prefix.empty()) {
				profile.append(a_prefix);
				profile.append(": ");
			}
			profile.append(spellName);
			if (!effectProfiles.empty()) {
				profile.append(" -> ");
				for (std::size_t i = 0; i < effectProfiles.size(); ++i) {
					if (i > 0u) {
						profile.append("; ");
					}
					profile.append(effectProfiles[i]);
				}
				if (totalEffects > effectProfiles.size()) {
					profile.append(" 외 ");
					profile.append(std::to_string(totalEffects - effectProfiles.size()));
					profile.append("개 효과");
				}
			}
			return profile;
		};

		auto actionSummaryText = [&](const Action& a_action) -> std::string {
			switch (a_action.type) {
			case ActionType::kCastSpell: {
				const auto spellName = spellNameOr(a_action.spell, "Spell");
				return a_action.applyToSelf ?
					       ("자신에게 " + spellName + " 시전") :
					       ("적에게 " + spellName + " 시전");
			}
			case ActionType::kCastSpellAdaptiveElement: {
				std::string elements;
				if (a_action.adaptiveFire) {
					elements.append("화염");
				}
				if (a_action.adaptiveFrost) {
					appendDelimited(elements, "냉기");
				}
				if (a_action.adaptiveShock) {
					appendDelimited(elements, "번개");
				}
				if (!elements.empty()) {
					return "적에게 적응형 원소 주문 시전 (" + elements + ")";
				}
				return "적에게 적응형 원소 주문 시전";
			}
			case ActionType::kConvertDamage: {
				std::string_view element = "원소";
				switch (a_action.element) {
				case Element::kFire:
					element = "화염";
					break;
				case Element::kFrost:
					element = "냉기";
					break;
				case Element::kShock:
					element = "번개";
					break;
				case Element::kNone:
				default:
					break;
				}
				return formatNumberCompact(std::max(0.0f, a_action.convertPct)) + "%를 " + std::string(element) + " 피해로 전환";
			}
			case ActionType::kMindOverMatter:
				return "피해 일부를 마나로 전환";
			case ActionType::kArchmage:
				return "아크메이지 추가 피해 발동";
			case ActionType::kSpawnTrap: {
				const auto spellName = spellNameOr(a_action.spell, "Trap");
				return spellName + " 함정 설치";
			}
			case ActionType::kCorpseExplosion:
				return "시체 폭발";
			case ActionType::kSummonCorpseExplosion:
				return "소환수 시체 폭발";
			case ActionType::kCastOnCrit: {
				const auto spellName = spellNameOr(a_action.spell, "Spell");
				return "치명타 시 " + spellName + " 시전";
			}
			case ActionType::kDebugNotify:
				return "디버그 알림";
			default:
				break;
			}

			if (!a_action.text.empty()) {
				return a_action.text;
			}
			return "효과 발동";
		};

		auto maxSpellDurationSeconds = [](RE::SpellItem* a_spell) {
			if (!a_spell) {
				return 0.0f;
			}
			float maxDuration = 0.0f;
			for (const auto* effect : a_spell->effects) {
				if (!effect || effect->effectItem.duration == 0u) {
					continue;
				}
				maxDuration = std::max(maxDuration, static_cast<float>(effect->effectItem.duration));
			}
			return maxDuration;
		};

		auto actionDurationSeconds = [&](const Action& a_action) {
			switch (a_action.type) {
			case ActionType::kCastSpell:
				return maxSpellDurationSeconds(a_action.spell);
			case ActionType::kCastOnCrit:
				return maxSpellDurationSeconds(a_action.spell);
			case ActionType::kCastSpellAdaptiveElement: {
				float maxDuration = 0.0f;
				std::unordered_set<RE::SpellItem*> seenSpells;
				if (a_action.adaptiveFire && seenSpells.insert(a_action.adaptiveFire).second) {
					maxDuration = std::max(maxDuration, maxSpellDurationSeconds(a_action.adaptiveFire));
				}
				if (a_action.adaptiveFrost && seenSpells.insert(a_action.adaptiveFrost).second) {
					maxDuration = std::max(maxDuration, maxSpellDurationSeconds(a_action.adaptiveFrost));
				}
				if (a_action.adaptiveShock && seenSpells.insert(a_action.adaptiveShock).second) {
					maxDuration = std::max(maxDuration, maxSpellDurationSeconds(a_action.adaptiveShock));
				}
				return maxDuration;
			}
			case ActionType::kSpawnTrap:
				if (a_action.trapTtl > std::chrono::milliseconds::zero()) {
					return static_cast<float>(a_action.trapTtl.count()) / 1000.0f;
				}
				return maxSpellDurationSeconds(a_action.spell);
			default:
				return 0.0f;
			}
		};

		auto magnitudeScalingSourceText = [](MagnitudeScaling::Source a_source) -> std::string_view {
			switch (a_source) {
			case MagnitudeScaling::Source::kHitPhysicalDealt:
				return "가한 물리 피해 (HitPhysicalDealt)";
			case MagnitudeScaling::Source::kHitTotalDealt:
				return "가한 총 피해 (HitTotalDealt)";
			case MagnitudeScaling::Source::kNone:
			default:
				return "";
			}
		};

		auto buildEffectSummaryText = [&](const AffixRuntime& a_affix, std::string_view a_summaryKey) {
			std::string summary;
			summary.append(triggerText(a_affix.trigger));
			summary.push_back(' ');
			const auto chancePct = std::clamp(a_affix.procChancePct, 0.0f, 100.0f);
			if (chancePct > 0.0f) {
				summary.append(formatNumberCompact(chancePct));
				summary.append("% 확률로 ");
			} else {
				summary.append("항상 ");
			}
			const auto mappedActionText = RunewordSummary::ActionSummaryTextByKey(a_summaryKey);
			if (!mappedActionText.empty()) {
				summary.append(mappedActionText);
			} else {
				summary.append(actionSummaryText(a_affix.action));
			}

			const float icdSeconds = static_cast<float>(a_affix.icd.count()) / 1000.0f;
			if (icdSeconds > 0.0f) {
				summary.append(" (쿨타임 ");
				summary.append(formatNumberCompact(icdSeconds));
				summary.append("초)");
			}

			const float perTargetIcdSeconds = static_cast<float>(a_affix.perTargetIcd.count()) / 1000.0f;
			if (perTargetIcdSeconds > 0.0f) {
				summary.append(" (대상별 쿨타임 ");
				summary.append(formatNumberCompact(perTargetIcdSeconds));
				summary.append("초)");
			}

			const float durationSeconds = actionDurationSeconds(a_affix.action);
			if (durationSeconds > 0.0f) {
				summary.append(" (지속 ");
				summary.append(formatNumberCompact(durationSeconds));
				summary.append("초)");
			}
			return summary;
		};

		auto buildEffectDetailText = [&](const AffixRuntime& a_affix) {
			std::string detail;

			auto appendSpellProfile = [&](std::string_view a_prefix, RE::SpellItem* a_spell) {
				const auto spellProfile = buildSpellProfileText(a_spell, a_prefix);
				if (!spellProfile.empty()) {
					appendDelimited(detail, spellProfile);
				}
			};

			switch (a_affix.action.type) {
			case ActionType::kCastSpell:
				appendSpellProfile(
					a_affix.action.applyToSelf ? "버프 효과 (Self Buff)" : "공격/디버프 효과 (Hit Effect)",
					a_affix.action.spell);
				break;
			case ActionType::kCastSpellAdaptiveElement: {
				std::unordered_set<RE::SpellItem*> seenSpells;
				if (a_affix.action.adaptiveFire && seenSpells.insert(a_affix.action.adaptiveFire).second) {
					appendSpellProfile("화염 효과 (Fire)", a_affix.action.adaptiveFire);
				}
				if (a_affix.action.adaptiveFrost && seenSpells.insert(a_affix.action.adaptiveFrost).second) {
					appendSpellProfile("냉기 효과 (Frost)", a_affix.action.adaptiveFrost);
				}
				if (a_affix.action.adaptiveShock && seenSpells.insert(a_affix.action.adaptiveShock).second) {
					appendSpellProfile("번개 효과 (Shock)", a_affix.action.adaptiveShock);
				}
				break;
			}
			case ActionType::kSpawnTrap:
				appendSpellProfile("함정 발동 효과 (Trap Trigger)", a_affix.action.spell);
				break;
			default:
				break;
			}

			const float icdSeconds = static_cast<float>(a_affix.icd.count()) / 1000.0f;
			if (icdSeconds > 0.0f) {
				appendDelimited(detail, "내부 쿨다운 (ICD): " + formatFloat1(icdSeconds) + "초");
			}

			const float perTargetIcdSeconds = static_cast<float>(a_affix.perTargetIcd.count()) / 1000.0f;
			if (perTargetIcdSeconds > 0.0f) {
				appendDelimited(detail, "대상별 쿨다운 (Per-target): " + formatFloat1(perTargetIcdSeconds) + "초");
			}

			if (a_affix.luckyHitChancePct > 0.0f) {
				appendDelimited(detail, "행운 적중 (Lucky Hit): " + formatFloat1(a_affix.luckyHitChancePct) + "%");
			}

			if (std::abs(a_affix.action.effectiveness - 1.0f) >= 0.01f) {
				appendDelimited(detail, "효과 배율 (Effectiveness): x" + formatFloat1(a_affix.action.effectiveness));
			}

			if (std::abs(a_affix.action.magnitudeOverride) >= 0.01f) {
				appendDelimited(detail, "고정 위력 (Magnitude): " + formatFloat1(a_affix.action.magnitudeOverride));
			}

			if (a_affix.action.type == ActionType::kMindOverMatter &&
				a_affix.action.mindOverMatterDamageToMagickaPct > 0.0f) {
				std::string moMText = "피해-마나 전환 (Damage->Magicka): " +
				                      formatFloat1(a_affix.action.mindOverMatterDamageToMagickaPct) + "%";
				if (a_affix.action.mindOverMatterMaxRedirectPerHit > 0.0f) {
					moMText.append(", 최대 (max) ");
					moMText.append(formatFloat1(a_affix.action.mindOverMatterMaxRedirectPerHit));
				}
				appendDelimited(detail, moMText);
			}

			if (a_affix.action.type == ActionType::kArchmage) {
				if (a_affix.action.archmageDamagePctOfMaxMagicka > 0.0f) {
					appendDelimited(
						detail,
						"추가 피해 (Damage): + " + formatFloat1(a_affix.action.archmageDamagePctOfMaxMagicka) + "% 최대 마력 (Max Magicka)");
				}
				if (a_affix.action.archmageCostPctOfMaxMagicka > 0.0f) {
					appendDelimited(
						detail,
						"추가 소모 (Cost): + " + formatFloat1(a_affix.action.archmageCostPctOfMaxMagicka) + "% 최대 마력 (Max Magicka)");
				}
			}

			if (a_affix.action.type == ActionType::kCorpseExplosion ||
				a_affix.action.type == ActionType::kSummonCorpseExplosion) {
				if (a_affix.action.corpseExplosionFlatDamage > 0.0f) {
					appendDelimited(detail, "고정 피해 (Flat): " + formatFloat1(a_affix.action.corpseExplosionFlatDamage));
				}
				if (a_affix.action.corpseExplosionPctOfCorpseMaxHealth > 0.0f) {
					appendDelimited(detail, "시체 최대 체력 비례 (CorpseMaxHP): " +
					                        formatFloat1(a_affix.action.corpseExplosionPctOfCorpseMaxHealth) + "%");
				}
				if (a_affix.action.corpseExplosionRadius > 0.0f) {
					appendDelimited(detail, "범위 (Radius): " + formatFloat1(a_affix.action.corpseExplosionRadius));
				}
			}

			if (a_affix.action.type == ActionType::kSpawnTrap) {
				if (a_affix.action.trapRadius > 0.0f) {
					appendDelimited(detail, "함정 범위 (Trap Radius): " + formatFloat1(a_affix.action.trapRadius));
				}
				if (a_affix.action.trapTtl > std::chrono::milliseconds::zero()) {
					const float ttlSeconds = static_cast<float>(a_affix.action.trapTtl.count()) / 1000.0f;
					appendDelimited(detail, "함정 지속시간 (Trap TTL): " + formatFloat1(ttlSeconds) + "초");
				}
			}

			if (a_affix.action.magnitudeScaling.source != MagnitudeScaling::Source::kNone) {
				std::string scalingText = "스케일링 (Scaling): ";
				const auto sourceText = magnitudeScalingSourceText(a_affix.action.magnitudeScaling.source);
				if (!sourceText.empty()) {
					scalingText.append(sourceText);
					scalingText.push_back(' ');
				}
				scalingText.append("x");
				scalingText.append(formatFloat1(a_affix.action.magnitudeScaling.mult));
				if (std::abs(a_affix.action.magnitudeScaling.add) >= 0.01f) {
					scalingText.append(" + ");
					scalingText.append(formatFloat1(a_affix.action.magnitudeScaling.add));
				}
				if (a_affix.action.magnitudeScaling.min > 0.0f) {
					scalingText.append(" (최소 min ");
					scalingText.append(formatFloat1(a_affix.action.magnitudeScaling.min));
					scalingText.push_back(')');
				}
				if (a_affix.action.magnitudeScaling.max > 0.0f) {
					scalingText.append(" (최대 max ");
					scalingText.append(formatFloat1(a_affix.action.magnitudeScaling.max));
					scalingText.push_back(')');
				}
				if (a_affix.action.magnitudeScaling.spellBaseAsMin) {
					scalingText.append(" (스펠 기본값 이상 보장 / spell-base min)");
				}
				appendDelimited(detail, scalingText);
			}

			return detail;
		};

		std::uint64_t selectedToken = 0u;
		if (const auto* currentRecipe = GetCurrentRunewordRecipe()) {
			selectedToken = currentRecipe->token;
		}
		if (_runewordSelectedBaseKey) {
			const auto selectedKey = *_runewordSelectedBaseKey;
			bool resolvedFromCompleted = false;
			if (const auto* completed = ResolveCompletedRunewordRecipe(selectedKey)) {
				selectedToken = completed->token;
				resolvedFromCompleted = true;
			}
			if (!resolvedFromCompleted) {
				if (const auto stateIt = _runewordInstanceStates.find(selectedKey);
					stateIt != _runewordInstanceStates.end() &&
					stateIt->second.recipeToken != 0u) {
					selectedToken = stateIt->second.recipeToken;
				}
			}
		}

		for (const auto& recipe : _runewordRecipes) {
			if (const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);
				affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
				continue;
			}
			const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);
			if (affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
				continue;
			}
			const auto& affix = _affixes[affixIt->second];
			const auto effectSummaryKey = resolveRecipeEffectSummaryKey(recipe);

			std::string runes;
			for (std::size_t i = 0; i < recipe.runeIds.size(); ++i) {
				if (i > 0) {
					runes.push_back('-');
				}
				runes.append(recipe.runeIds[i]);
			}

			entries.push_back(RunewordRecipeEntry{
				.recipeToken = recipe.token,
				.displayName = recipe.displayName,
				.runeSequence = std::move(runes),
				.effectSummaryKey = std::string(effectSummaryKey),
				.effectSummaryText = buildEffectSummaryText(affix, effectSummaryKey),
				.effectDetailText = buildEffectDetailText(affix),
				.recommendedBaseKey = std::string(resolveRecommendedBaseKey(recipe)),
				.selected = (selectedToken != 0u && selectedToken == recipe.token)
			});
		}

		return entries;
	}

}
