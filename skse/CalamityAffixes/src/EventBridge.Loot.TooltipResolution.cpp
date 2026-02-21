#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootUiGuards.h"
#include "EventBridge.Loot.Runeword.SummaryText.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
		[[nodiscard]] bool IsPreviewItemSource(std::string_view a_source) noexcept
		{
			return a_source == "barter" || a_source == "container" || a_source == "gift";
		}

		[[nodiscard]] bool IsSynthesizedRunewordAffixId(std::string_view a_affixId) noexcept
		{
			if (a_affixId.size() < 5 || a_affixId.substr(a_affixId.size() - 5) != "_auto") {
				return false;
			}

			// Synthesized runtime runewords are currently emitted as rw_*_auto.
			// Keep runeword_*_auto for backward compatibility with legacy/generated ids.
			return a_affixId.rfind("rw_", 0) == 0 || a_affixId.rfind("runeword_", 0) == 0;
		}
	}

	std::optional<std::string> EventBridge::GetInstanceAffixTooltip(
		const RE::InventoryEntryData* a_item,
		std::string_view a_selectedDisplayName,
		int a_uiLanguageMode,
		std::string_view a_itemSource,
		RE::FormID a_sourceContainerFormID,
		std::uint64_t a_preferredInstanceKey)
	{
		if (!_configLoaded) {
			return std::nullopt;
		}
		if (!a_item) {
			return std::nullopt;
		}

		if (!a_item->extraLists) {
			return std::nullopt;
		}

		std::optional<LootItemType> itemType{};
		if (a_item->object && a_item->object->As<RE::TESObjectWEAP>()) {
			itemType = LootItemType::kWeapon;
		} else if (a_item->object && a_item->object->As<RE::TESObjectARMO>()) {
			itemType = LootItemType::kArmor;
		}

		// Preview-time synthetic affix rolls are disabled by policy.
		// Tooltip now surfaces only already-materialized instance data.
		const bool allowPreview = ShouldEnableSyntheticLootPreviewTooltip() &&
			itemType.has_value() &&
			IsPreviewItemSource(a_itemSource) &&
			IsLootObjectEligibleForAffixes(a_item->object);
		(void)a_sourceContainerFormID;

		struct TooltipCandidate
		{
			std::uint64_t instanceKey{ 0u };
			std::uint64_t primaryToken{ 0u };
			std::string rowName{};
			InstanceAffixSlots slots{};
			bool preview{ false };
		};

		std::vector<TooltipCandidate> candidates;
		for (auto* xList : *a_item->extraLists) {
			if (!xList) {
				continue;
			}

			auto* uid = xList->GetByType<RE::ExtraUniqueID>();
			if (!uid && allowPreview) {
				auto* player = RE::PlayerCharacter::GetSingleton();
				auto* changes = player ? player->GetInventoryChanges() : nullptr;
				if (changes) {
					auto* created = RE::BSExtraData::Create<RE::ExtraUniqueID>();
					if (created) {
						created->baseID = a_item->object ? a_item->object->GetFormID() : 0u;
						created->uniqueID = changes->GetNextUniqueID();
						xList->Add(created);
						uid = created;
					}
				}
			}

			if (!uid) {
				continue;
			}

			const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);

			TooltipCandidate candidate{};
			candidate.instanceKey = key;
			candidate.preview = false;
			candidate.slots.Clear();

			if (const auto mappedIt = _instanceAffixes.find(key); mappedIt != _instanceAffixes.end()) {
				candidate.slots = mappedIt->second;
			}

			if (candidate.slots.count == 0 && allowPreview) {
				if (const auto* previewSlots = FindLootPreviewSlots(key)) {
					candidate.slots = *previewSlots;
					candidate.preview = true;
				} else if (const auto generatedPreview = BuildLootPreviewAffixSlots(key, *itemType); generatedPreview.has_value()) {
					RememberLootPreviewSlots(key, *generatedPreview);
					if (const auto* storedPreview = FindLootPreviewSlots(key)) {
						candidate.slots = *storedPreview;
						candidate.preview = true;
					}
				}
			}

			// For normal runtime candidates, skip items without mapped slots.
			// For preview mode, keep zero-slot candidates so users can see "no affix" before pickup.
			if (candidate.slots.count == 0 && !candidate.preview) {
				continue;
			}

			candidate.primaryToken = candidate.slots.GetPrimary();
			const char* rowNameRaw = xList->GetDisplayName(a_item->object);
			candidate.rowName = rowNameRaw ? rowNameRaw : "";
			candidates.push_back(std::move(candidate));
		}

		if (candidates.empty()) {
			return std::nullopt;
		}

		auto formatAffixDetail = [&](const AffixRuntime& a_affix, std::uint64_t a_instanceKey) -> std::string {
			std::string detail;
			const auto* state = FindInstanceRuntimeState(a_instanceKey, a_affix.token);
			const auto& action = a_affix.action;

			if (action.modeCycleEnabled && !action.modeCycleSpells.empty()) {
				const bool adaptiveManualMode =
					(action.type == ActionType::kCastSpellAdaptiveElement) && action.modeCycleManualOnly;
				const auto modeCount = adaptiveManualMode ?
					static_cast<std::uint32_t>(action.modeCycleSpells.size() + 1u) :
					static_cast<std::uint32_t>(action.modeCycleSpells.size());
				const std::uint32_t idx = modeCount > 0u ? ((state ? state->modeIndex : 0u) % modeCount) : 0u;

				detail.append("Mode ");
				if (adaptiveManualMode && idx == 0u) {
					detail.append("Auto");
				} else {
					const std::size_t labelIdx = adaptiveManualMode ?
						static_cast<std::size_t>(idx - 1u) :
						static_cast<std::size_t>(idx);
					if (labelIdx < action.modeCycleLabels.size() && !action.modeCycleLabels[labelIdx].empty()) {
						detail.append(action.modeCycleLabels[labelIdx]);
					} else {
						const auto displayIndex = adaptiveManualMode ? idx : (idx + 1u);
						const auto displayCount = adaptiveManualMode ?
							static_cast<std::uint32_t>(action.modeCycleSpells.size()) :
							modeCount;
						detail.append(std::to_string(displayIndex));
						detail.push_back('/');
						detail.append(std::to_string(displayCount));
					}
				}
				if (action.modeCycleManualOnly) {
					detail.append(" (Manual)");
				}
			}

			auto formatFloat1 = [](float a_value) {
				char buf[32]{};
				std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(a_value));
				return std::string(buf);
			};

			auto triggerTextEn = [](Trigger a_trigger) -> std::string_view {
				switch (a_trigger) {
				case Trigger::kHit:
					return "On hit";
				case Trigger::kIncomingHit:
					return "On taking a hit";
				case Trigger::kDotApply:
					return "On DoT apply";
				case Trigger::kKill:
					return "On kill";
				case Trigger::kLowHealth:
					return "On low health";
				}
				return "On trigger";
			};

			auto triggerTextKo = [](Trigger a_trigger) -> std::string_view {
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

			auto formatNumberCompact = [&](float a_value) {
				std::string text = formatFloat1(a_value);
				if (text.size() >= 2 && text.compare(text.size() - 2, 2, ".0") == 0) {
					text.erase(text.size() - 2);
				}
				return text;
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
					if (a_action.adaptiveFire) {
						maxDuration = std::max(maxDuration, maxSpellDurationSeconds(a_action.adaptiveFire));
					}
					if (a_action.adaptiveFrost) {
						maxDuration = std::max(maxDuration, maxSpellDurationSeconds(a_action.adaptiveFrost));
					}
					if (a_action.adaptiveShock) {
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

			auto buildRunewordAutoSummary = [&](const AffixRuntime& a_autoAffix) -> std::string {
				if (!IsSynthesizedRunewordAffixId(a_autoAffix.id)) {
					return {};
				}

				RunewordSummary::BaseType baseType = RunewordSummary::BaseType::kUnknown;
				if (itemType.has_value()) {
					baseType = (*itemType == LootItemType::kWeapon) ?
					               RunewordSummary::BaseType::kWeapon :
					               RunewordSummary::BaseType::kArmor;
				}

				std::string_view recipeId = a_autoAffix.id;
				if (recipeId.size() > 5 && recipeId.substr(recipeId.size() - 5) == "_auto") {
					recipeId = recipeId.substr(0, recipeId.size() - 5);
				}
				const auto summaryKey = RunewordSummary::ResolveEffectSummaryKey(recipeId, baseType);

				const auto chancePct = std::clamp(a_autoAffix.procChancePct, 0.0f, 100.0f);
				const auto icdSec = static_cast<float>(a_autoAffix.icd.count()) / 1000.0f;
				const auto perTargetIcdSec = static_cast<float>(a_autoAffix.perTargetIcd.count()) / 1000.0f;
				const auto durationSec = actionDurationSeconds(a_autoAffix.action);

				std::string effectEn;
				std::string effectKo;
				switch (a_autoAffix.action.type) {
				case ActionType::kCastSpell: {
					const auto spellName = spellNameOr(a_autoAffix.action.spell, "Spell");
					if (a_autoAffix.action.applyToSelf) {
						effectEn = "Self-cast " + spellName;
						effectKo = "자신에게 " + spellName + " 시전";
					} else {
						effectEn = "Cast " + spellName;
						effectKo = "대상에게 " + spellName + " 시전";
					}
					break;
				}
				case ActionType::kCastSpellAdaptiveElement: {
					std::vector<std::string_view> elemsEn;
					std::vector<std::string_view> elemsKo;
					if (a_autoAffix.action.adaptiveFire) {
						elemsEn.push_back("Fire");
						elemsKo.push_back("화염");
					}
					if (a_autoAffix.action.adaptiveFrost) {
						elemsEn.push_back("Frost");
						elemsKo.push_back("냉기");
					}
					if (a_autoAffix.action.adaptiveShock) {
						elemsEn.push_back("Shock");
						elemsKo.push_back("번개");
					}

					auto joinElems = [](const std::vector<std::string_view>& a_values) {
						std::string out;
						for (std::size_t i = 0; i < a_values.size(); ++i) {
							if (i > 0) {
								out.append("/");
							}
							out.append(a_values[i]);
						}
						return out;
					};

					const auto elemEn = joinElems(elemsEn);
					const auto elemKo = joinElems(elemsKo);
					effectEn = elemEn.empty() ? "Adaptive elemental cast" : ("Adaptive elemental cast (" + elemEn + ")");
					effectKo = elemKo.empty() ? "적응형 원소 시전" : ("적응형 원소 시전 (" + elemKo + ")");
					break;
				}
				case ActionType::kSpawnTrap: {
					const auto spellName = spellNameOr(a_autoAffix.action.spell, "Trap");
					effectEn = "Deploy trap: " + spellName;
					effectKo = "함정 설치: " + spellName;
					break;
				}
				default:
					effectEn = "Auto-generated runeword effect";
					effectKo = "자동 생성된 룬워드 효과";
					break;
				}

				std::string headerEn;
				headerEn.append(triggerTextEn(a_autoAffix.trigger));
				headerEn.push_back(' ');
				if (chancePct > 0.0f) {
					headerEn.append(formatNumberCompact(chancePct));
					headerEn.append("% chance to ");
				} else {
					headerEn.append("Always ");
				}
				if (!effectEn.empty()) {
					headerEn.append(effectEn);
				} else {
					headerEn.append("trigger an effect");
				}
				if (icdSec > 0.0f) {
					headerEn.append(" (Cooldown ");
					headerEn.append(formatNumberCompact(icdSec));
					headerEn.append("s)");
				}
				if (perTargetIcdSec > 0.0f) {
					headerEn.append(" (Per-target cooldown ");
					headerEn.append(formatNumberCompact(perTargetIcdSec));
					headerEn.append("s)");
				}
				if (durationSec > 0.0f) {
					headerEn.append(" (Duration ");
					headerEn.append(formatNumberCompact(durationSec));
					headerEn.append("s)");
				}

				std::string headerKo;
				headerKo.append(triggerTextKo(a_autoAffix.trigger));
				headerKo.push_back(' ');
				if (chancePct > 0.0f) {
					headerKo.append(formatNumberCompact(chancePct));
					headerKo.append("% 확률로 ");
				} else {
					headerKo.append("항상 ");
				}
				const auto mappedKo = RunewordSummary::ActionSummaryTextByKey(summaryKey);
				if (!mappedKo.empty()) {
					headerKo.append(mappedKo);
				} else if (!effectKo.empty()) {
					headerKo.append(effectKo);
				} else {
					headerKo.append("효과 발동");
				}
				if (icdSec > 0.0f) {
					headerKo.append(" (쿨타임 ");
					headerKo.append(formatNumberCompact(icdSec));
					headerKo.append("초)");
				}
				if (perTargetIcdSec > 0.0f) {
					headerKo.append(" (대상별 쿨타임 ");
					headerKo.append(formatNumberCompact(perTargetIcdSec));
					headerKo.append("초)");
				}
				if (durationSec > 0.0f) {
					headerKo.append(" (지속 ");
					headerKo.append(formatNumberCompact(durationSec));
					headerKo.append("초)");
				}

				switch (a_uiLanguageMode) {
				case 0:
					return headerEn;
				case 1:
					return headerKo;
				case 2:
				default:
					return headerKo + " / " + headerEn;
				}
			};

			const auto autoSummary = buildRunewordAutoSummary(a_affix);
			if (!autoSummary.empty()) {
				if (!detail.empty()) {
					detail.push_back('\n');
				}
				detail.append(autoSummary);
			}

			return detail;
		};

		auto resolveDisplayName = [&](const AffixRuntime& a_affix) -> std::string {
			switch (a_uiLanguageMode) {
			case 0:
				return a_affix.displayNameEn;
			case 1:
				return a_affix.displayNameKo;
			case 2:
			default:
				if (a_affix.displayNameKo == a_affix.displayNameEn || a_affix.displayNameEn.empty()) {
					return a_affix.displayNameKo;
				}
				if (a_affix.displayNameKo.empty()) {
					return a_affix.displayNameEn;
				}
				return a_affix.displayNameKo + " / " + a_affix.displayNameEn;
			}
		};

		auto formatTooltip = [&](const TooltipCandidate& a_candidate) -> std::string {
			if (a_candidate.slots.count == 0) {
				if (a_candidate.preview) {
					return "Preview: No affix";
				}
				return {};
			}

			std::string tooltip;
			if (a_candidate.preview) {
				tooltip.append("Preview\n");
			}

			for (std::uint8_t s = 0; s < a_candidate.slots.count; ++s) {
				const auto token = a_candidate.slots.tokens[s];
				const auto idxIt = _affixIndexByToken.find(token);
				if (idxIt == _affixIndexByToken.end() || idxIt->second >= _affixes.size()) {
					continue;
				}

				const auto& affix = _affixes[idxIt->second];
				auto resolvedName = resolveDisplayName(affix);
				if (resolvedName.empty()) {
					continue;
				}

				if (!tooltip.empty() && tooltip.back() != '\n') {
					tooltip.push_back('\n');
				}

				if (a_candidate.slots.count > 1) {
					tooltip.push_back('[');
					if (affix.slot == AffixSlot::kPrefix) {
						tooltip.push_back('P');
					} else if (affix.slot == AffixSlot::kSuffix) {
						tooltip.push_back('S');
					}
					tooltip.append(std::to_string(s + 1));
					tooltip.push_back('/');
					tooltip.append(std::to_string(a_candidate.slots.count));
					tooltip.append("] ");
				}

				tooltip.append(resolvedName);

				const auto detail = formatAffixDetail(affix, a_candidate.instanceKey);
				if (!detail.empty()) {
					tooltip.push_back('\n');
					tooltip.append(detail);
				}
			}

			return tooltip;
		};

		std::vector<TooltipResolutionCandidate> resolutionCandidates;
		resolutionCandidates.reserve(candidates.size());
		for (const auto& candidate : candidates) {
			resolutionCandidates.push_back(TooltipResolutionCandidate{
				.rowName = candidate.rowName,
				.affixToken = candidate.primaryToken,
				.instanceKey = candidate.instanceKey
			});
		}

		if (a_preferredInstanceKey != 0u) {
			const auto preferredIt = std::find_if(
				candidates.begin(),
				candidates.end(),
				[a_preferredInstanceKey](const TooltipCandidate& a_candidate) {
					return a_candidate.instanceKey == a_preferredInstanceKey;
				});

			if (preferredIt == candidates.end()) {
				return std::nullopt;
			}

			const auto preferredTooltip = formatTooltip(*preferredIt);
			if (preferredTooltip.empty()) {
				return std::nullopt;
			}
			return preferredTooltip;
		}

		const auto resolution = ResolveTooltipCandidateSelection(resolutionCandidates, a_selectedDisplayName);
		if (resolution.matchedIndex &&
			*resolution.matchedIndex < candidates.size() &&
			!resolution.ambiguous) {
			const auto& matched = candidates[*resolution.matchedIndex];
			const auto formatted = formatTooltip(matched);
			if (formatted.empty()) {
				return std::nullopt;
			}
			return formatted;
		}

		if (!a_selectedDisplayName.empty()) {
			return std::nullopt;
		}

		const auto fallback = formatTooltip(candidates.front());
		if (fallback.empty()) {
			return std::nullopt;
		}
		return fallback;
	}
}
