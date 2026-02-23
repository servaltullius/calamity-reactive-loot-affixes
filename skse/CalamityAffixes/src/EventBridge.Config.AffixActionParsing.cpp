#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RuntimeContract.h"
#include "EventBridge.Config.Shared.h"

#include <algorithm>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		RE::SpellItem* ParseSpellFromString(std::string_view a_spec, RE::TESDataHandler* a_handler)
		{
			return ConfigShared::LookupSpellFromSpec(a_spec, a_handler);
		}

		RE::SpellItem* ParseSpell(const nlohmann::json& a_action, RE::TESDataHandler* a_handler)
		{
			const auto editorId = a_action.value("spellEditorId", std::string{});
			if (!editorId.empty()) {
				return ParseSpellFromString(editorId, a_handler);
			}

			const auto formSpec = a_action.value("spellForm", std::string{});
			return ParseSpellFromString(formSpec, a_handler);
		}

		RE::SpellItem* ParseSpellValue(const nlohmann::json& a_value, RE::TESDataHandler* a_handler)
		{
			if (a_value.is_string()) {
				return ParseSpellFromString(a_value.get<std::string>(), a_handler);
			}
			if (a_value.is_object()) {
				return ParseSpell(a_value, a_handler);
			}
			return nullptr;
		}

		void ParseMagnitudeScaling(const nlohmann::json& a_action, MagnitudeScaling& a_outScaling)
		{
			auto parseMagnitudeSource = [](std::string_view a_source) -> MagnitudeScaling::Source {
				if (a_source == "HitPhysicalDealt") {
					return MagnitudeScaling::Source::kHitPhysicalDealt;
				}
				if (a_source == "HitTotalDealt") {
					return MagnitudeScaling::Source::kHitTotalDealt;
				}
				return MagnitudeScaling::Source::kNone;
			};

			const auto& magScaling = a_action.value("magnitudeScaling", nlohmann::json::object());
			if (!magScaling.is_object()) {
				return;
			}

			a_outScaling.source = parseMagnitudeSource(magScaling.value("source", std::string{}));
			a_outScaling.mult = magScaling.value("mult", 0.0f);
			a_outScaling.add = magScaling.value("add", 0.0f);
			a_outScaling.min = magScaling.value("min", 0.0f);
			a_outScaling.max = magScaling.value("max", 0.0f);
			a_outScaling.spellBaseAsMin = magScaling.value("spellBaseAsMin", false);
		}

		template <class ActionState>
		void ParseActionEvolutionAndModeCycle(const nlohmann::json& a_action, RE::TESDataHandler* a_handler, ActionState& a_outAction)
		{
			const auto& evolution = a_action.value("evolution", nlohmann::json::object());
			if (evolution.is_object()) {
				const int xpPerProc = evolution.value("xpPerProc", 1);
				a_outAction.evolutionXpPerProc = (xpPerProc > 0) ? static_cast<std::uint32_t>(xpPerProc) : 1u;

				const auto& thresholds = evolution.value("thresholds", nlohmann::json::array());
				if (thresholds.is_array()) {
					for (const auto& entry : thresholds) {
						if (!entry.is_number_integer() && !entry.is_number_unsigned()) {
							continue;
						}

						const auto raw = entry.get<std::int64_t>();
						if (raw < 0) {
							continue;
						}

						a_outAction.evolutionThresholds.push_back(static_cast<std::uint32_t>(raw));
					}
				}

				const auto& multipliers = evolution.value("multipliers", nlohmann::json::array());
				if (multipliers.is_array()) {
					for (const auto& entry : multipliers) {
						if (!entry.is_number()) {
							continue;
						}

						const float value = entry.get<float>();
						if (value <= 0.0f) {
							continue;
						}

						a_outAction.evolutionMultipliers.push_back(value);
					}
				}

				std::sort(a_outAction.evolutionThresholds.begin(), a_outAction.evolutionThresholds.end());
				a_outAction.evolutionThresholds.erase(
					std::unique(a_outAction.evolutionThresholds.begin(), a_outAction.evolutionThresholds.end()),
					a_outAction.evolutionThresholds.end());
				if (a_outAction.evolutionThresholds.empty()) {
					a_outAction.evolutionThresholds.push_back(0u);
				}

				if (a_outAction.evolutionMultipliers.empty()) {
					a_outAction.evolutionMultipliers.push_back(1.0f);
				}
				while (a_outAction.evolutionMultipliers.size() < a_outAction.evolutionThresholds.size()) {
					a_outAction.evolutionMultipliers.push_back(a_outAction.evolutionMultipliers.back());
				}

				a_outAction.evolutionEnabled = true;
			}

			const auto& modeCycle = a_action.value("modeCycle", nlohmann::json::object());
			if (!modeCycle.is_object()) {
				return;
			}

			const auto& spells = modeCycle.value("spells", nlohmann::json::array());
			if (spells.is_array()) {
				for (const auto& entry : spells) {
					auto* parsed = ParseSpellValue(entry, a_handler);
					if (parsed) {
						a_outAction.modeCycleSpells.push_back(parsed);
					}
				}
			}

			const auto& labels = modeCycle.value("labels", nlohmann::json::array());
			if (labels.is_array()) {
				for (const auto& entry : labels) {
					if (!entry.is_string()) {
						continue;
					}
					a_outAction.modeCycleLabels.push_back(entry.get<std::string>());
				}
			}

			const int switchEveryProcs = modeCycle.value("switchEveryProcs", 1);
			a_outAction.modeCycleSwitchEveryProcs = (switchEveryProcs > 0) ? static_cast<std::uint32_t>(switchEveryProcs) : 1u;
			a_outAction.modeCycleManualOnly = modeCycle.value("manualOnly", false);
			if (!a_outAction.modeCycleSpells.empty()) {
				a_outAction.modeCycleEnabled = true;
			}
		}
	}

	bool EventBridge::ParseRuntimeSpellActionFromJson(
		const nlohmann::json& a_action,
		std::string_view a_type,
		RE::TESDataHandler* a_handler,
		AffixRuntime& a_out) const
	{
		auto parseAdaptiveMode = [](std::string_view a_mode) -> AdaptiveElementMode {
			if (a_mode == "StrongestResist" || a_mode == "Strongest") {
				return AdaptiveElementMode::kStrongestResist;
			}
			return AdaptiveElementMode::kWeakestResist;
		};

		if (a_type == RuntimeContract::kActionCastSpell) {
			a_out.action.type = ActionType::kCastSpell;
			a_out.action.spell = ParseSpell(a_action, a_handler);
			a_out.action.effectiveness = a_action.value("effectiveness", 1.0f);
			a_out.action.magnitudeOverride = a_action.value("magnitudeOverride", 0.0f);
			a_out.action.noHitEffectArt = a_action.value("noHitEffectArt", false);

			if (!a_out.action.spell) {
				SKSE::log::error(
					"CalamityAffixes: CastSpell action missing spell (affixId={}, spellEditorId={}, spellForm={}).",
					a_out.id,
					a_action.value("spellEditorId", std::string{}),
					a_action.value("spellForm", std::string{}));
			}

			ParseMagnitudeScaling(a_action, a_out.action.magnitudeScaling);

			const auto applyTo = a_action.value("applyTo", std::string{});
			a_out.action.applyToSelf = (applyTo == "Self");
			ParseActionEvolutionAndModeCycle(a_action, a_handler, a_out.action);
			return true;
		}

		if (a_type == RuntimeContract::kActionCastSpellAdaptiveElement) {
			a_out.action.type = ActionType::kCastSpellAdaptiveElement;
			a_out.action.effectiveness = a_action.value("effectiveness", 1.0f);
			a_out.action.magnitudeOverride = a_action.value("magnitudeOverride", 0.0f);
			a_out.action.noHitEffectArt = a_action.value("noHitEffectArt", false);

			a_out.action.adaptiveMode = parseAdaptiveMode(a_action.value("mode", std::string{ "WeakestResist" }));

			const auto& spells = a_action.value("spells", nlohmann::json::object());
			if (spells.is_object()) {
				a_out.action.adaptiveFire = ParseSpellValue(spells.value("Fire", nlohmann::json{}), a_handler);
				a_out.action.adaptiveFrost = ParseSpellValue(spells.value("Frost", nlohmann::json{}), a_handler);
				a_out.action.adaptiveShock = ParseSpellValue(spells.value("Shock", nlohmann::json{}), a_handler);
			}

			if (!a_out.action.adaptiveFire && !a_out.action.adaptiveFrost && !a_out.action.adaptiveShock) {
				SKSE::log::error("CalamityAffixes: CastSpellAdaptiveElement missing spells (affixId={}).", a_out.id);
				return false;
			}

			ParseMagnitudeScaling(a_action, a_out.action.magnitudeScaling);

			const auto applyTo = a_action.value("applyTo", std::string{});
			a_out.action.applyToSelf = (applyTo == "Self");
			ParseActionEvolutionAndModeCycle(a_action, a_handler, a_out.action);
			if (a_out.action.modeCycleEnabled && !a_out.action.modeCycleManualOnly) {
				a_out.action.modeCycleManualOnly = true;
				SKSE::log::warn(
					"CalamityAffixes: CastSpellAdaptiveElement modeCycle coerced to manualOnly=true (affixId={}).",
					a_out.id);
			}
			return true;
		}

		return false;
	}

	bool EventBridge::ParseRuntimeSpecialActionFromJson(
		const nlohmann::json& a_action,
		std::string_view a_type,
		RE::TESDataHandler* a_handler,
		AffixRuntime& a_out) const
	{
		auto parseElement = [](std::string_view a_element) -> Element {
			if (a_element == "Fire") {
				return Element::kFire;
			}
			if (a_element == "Frost") {
				return Element::kFrost;
			}
			if (a_element == "Shock") {
				return Element::kShock;
			}
			return Element::kNone;
		};

		if (a_type == RuntimeContract::kActionCastOnCrit) {
			a_out.action.type = ActionType::kCastOnCrit;
			a_out.action.spell = ParseSpell(a_action, a_handler);
			a_out.action.effectiveness = a_action.value("effectiveness", 1.0f);
			a_out.action.magnitudeOverride = a_action.value("magnitudeOverride", 0.0f);
			a_out.action.noHitEffectArt = a_action.value("noHitEffectArt", false);
			ParseMagnitudeScaling(a_action, a_out.action.magnitudeScaling);
			return a_out.action.spell != nullptr;
		}

		if (a_type == RuntimeContract::kActionConvertDamage) {
			a_out.action.type = ActionType::kConvertDamage;
			a_out.action.element = parseElement(a_action.value("element", std::string{}));
			a_out.action.convertPct = a_action.value("percent", 0.0f);
			a_out.action.spell = ParseSpell(a_action, a_handler);
			a_out.action.effectiveness = a_action.value("effectiveness", 1.0f);
			a_out.action.noHitEffectArt = a_action.value("noHitEffectArt", false);
			return a_out.action.spell && a_out.action.element != Element::kNone && a_out.action.convertPct > 0.0f;
		}

		if (a_type == RuntimeContract::kActionMindOverMatter) {
			a_out.action.type = ActionType::kMindOverMatter;

			auto readMindOverMatterNumber = [&](std::string_view a_key, double a_fallback = 0.0) -> double {
				const auto key = std::string(a_key);
				const auto it = a_action.find(key);
				if (it == a_action.end()) {
					return a_fallback;
				}
				if (it->is_number()) {
					return it->get<double>();
				}

				SKSE::log::warn(
					"CalamityAffixes: MindOverMatter expects numeric '{}' (affixId={}, gotType={}); using fallback={}.",
					key,
					a_out.id,
					it->type_name(),
					a_fallback);
				return a_fallback;
			};

			a_out.action.mindOverMatterDamageToMagickaPct =
				static_cast<float>(readMindOverMatterNumber("damageToMagickaPct"));
			a_out.action.mindOverMatterMaxRedirectPerHit =
				static_cast<float>(readMindOverMatterNumber("maxRedirectPerHit"));

			if (a_out.action.mindOverMatterDamageToMagickaPct <= 0.0f) {
				return false;
			}
			if (a_out.trigger != Trigger::kIncomingHit) {
				SKSE::log::warn(
					"CalamityAffixes: MindOverMatter requires trigger=IncomingHit (affixId={}, trigger={}); skipping.",
					a_out.id,
					static_cast<std::uint32_t>(a_out.trigger));
				return false;
			}
			a_out.action.mindOverMatterDamageToMagickaPct = std::clamp(a_out.action.mindOverMatterDamageToMagickaPct, 0.0f, 100.0f);
			a_out.action.mindOverMatterMaxRedirectPerHit = std::max(0.0f, a_out.action.mindOverMatterMaxRedirectPerHit);
			return true;
		}

		if (a_type == RuntimeContract::kActionArchmage) {
			a_out.action.type = ActionType::kArchmage;
			a_out.action.spell = ParseSpell(a_action, a_handler);
			a_out.action.effectiveness = a_action.value("effectiveness", 1.0f);
			a_out.action.noHitEffectArt = a_action.value("noHitEffectArt", false);
			a_out.action.archmageDamagePctOfMaxMagicka = a_action.value("damagePctOfMaxMagicka", 0.0f);
			a_out.action.archmageCostPctOfMaxMagicka = a_action.value("costPctOfMaxMagicka", 0.0f);
			return a_out.action.spell &&
			       a_out.action.archmageDamagePctOfMaxMagicka > 0.0f &&
			       a_out.action.archmageCostPctOfMaxMagicka > 0.0f;
		}

		return false;
	}

	bool EventBridge::ParseRuntimeAreaActionFromJson(
		const nlohmann::json& a_action,
		std::string_view a_type,
		RE::TESDataHandler* a_handler,
		AffixRuntime& a_out) const
	{
		auto parseTrapSpawnAt = [](std::string_view a_spawnAt) -> TrapSpawnAt {
			if (a_spawnAt == "Self" || a_spawnAt == "Owner" || a_spawnAt == "OwnerFeet") {
				return TrapSpawnAt::kOwnerFeet;
			}
			return TrapSpawnAt::kTargetFeet;
		};

		if (a_type == RuntimeContract::kActionCorpseExplosion || a_type == RuntimeContract::kActionSummonCorpseExplosion) {
			a_out.action.type = (a_type == RuntimeContract::kActionSummonCorpseExplosion) ?
				ActionType::kSummonCorpseExplosion :
				ActionType::kCorpseExplosion;
			a_out.action.spell = ParseSpell(a_action, a_handler);
			a_out.action.effectiveness = a_action.value("effectiveness", 1.0f);
			a_out.action.noHitEffectArt = a_action.value("noHitEffectArt", true);

			a_out.action.corpseExplosionFlatDamage = a_action.value("flatDamage", 0.0f);
			a_out.action.corpseExplosionPctOfCorpseMaxHealth = a_action.value("pctOfCorpseMaxHealth", 0.0f);
			a_out.action.corpseExplosionRadius = a_action.value("radius", 0.0f);
			a_out.action.corpseExplosionMaxTargets = a_action.value("maxTargets", 8u);
			a_out.action.corpseExplosionMaxChainDepth = a_action.value("maxChainDepth", 1u);
			a_out.action.corpseExplosionChainFalloff = a_action.value("chainFalloff", 1.0f);

			const float chainWindowSeconds = a_action.value("chainWindowSeconds", 0.0f);
			if (chainWindowSeconds > 0.0f) {
				a_out.action.corpseExplosionChainWindow = std::chrono::milliseconds(static_cast<std::int64_t>(chainWindowSeconds * 1000.0f));
			}

			const float rateWindowSeconds = a_action.value("rateLimitWindowSeconds", 1.0f);
			if (rateWindowSeconds > 0.0f) {
				a_out.action.corpseExplosionRateLimitWindow = std::chrono::milliseconds(static_cast<std::int64_t>(rateWindowSeconds * 1000.0f));
			}
			a_out.action.corpseExplosionMaxExplosionsPerWindow = a_action.value("maxExplosionsPerWindow", 4u);

			if (!a_out.action.spell) {
				return false;
			}
			if (a_out.action.corpseExplosionRadius <= 0.0f || a_out.action.corpseExplosionMaxTargets == 0u) {
				return false;
			}
			if (a_out.action.corpseExplosionFlatDamage <= 0.0f && a_out.action.corpseExplosionPctOfCorpseMaxHealth <= 0.0f) {
				return false;
			}
			if (a_out.action.corpseExplosionMaxChainDepth == 0u) {
				a_out.action.corpseExplosionMaxChainDepth = 1u;
			}
			if (a_out.action.corpseExplosionChainFalloff <= 0.0f || a_out.action.corpseExplosionChainFalloff > 1.0f) {
				a_out.action.corpseExplosionChainFalloff = 1.0f;
			}
			if (a_out.action.corpseExplosionRateLimitWindow.count() <= 0 || a_out.action.corpseExplosionMaxExplosionsPerWindow == 0u) {
				a_out.action.corpseExplosionRateLimitWindow = std::chrono::milliseconds(1000);
				a_out.action.corpseExplosionMaxExplosionsPerWindow = 4u;
			}
			return true;
		}

		if (a_type == RuntimeContract::kActionSpawnTrap) {
			a_out.action.type = ActionType::kSpawnTrap;
			a_out.action.spell = ParseSpell(a_action, a_handler);
			a_out.action.effectiveness = a_action.value("effectiveness", 1.0f);
			a_out.action.magnitudeOverride = a_action.value("magnitudeOverride", 0.0f);
			a_out.action.noHitEffectArt = a_action.value("noHitEffectArt", true);

			a_out.action.trapSpawnAt = parseTrapSpawnAt(a_action.value("spawnAt", std::string{ "Target" }));
			a_out.action.trapRadius = a_action.value("radius", 0.0f);
			a_out.action.trapMaxActive = a_action.value("maxActive", 2u);
			a_out.action.trapMaxTriggers = a_action.value("maxTriggers", 1u);
			a_out.action.trapMaxTargetsPerTrigger = a_action.value("maxTargetsPerTrigger", 1u);
			a_out.action.trapRequireCritOrPowerAttack = a_action.value("requireCritOrPowerAttack", false);
			a_out.action.trapRequireWeaponHit = a_action.value("requireWeaponHit", true);

			const float ttlSeconds = a_action.value("ttlSeconds", 0.0f);
			if (ttlSeconds > 0.0f) {
				a_out.action.trapTtl = std::chrono::milliseconds(static_cast<std::int64_t>(ttlSeconds * 1000.0f));
			}

			const float armDelaySeconds = a_action.value("armDelaySeconds", 0.0f);
			if (armDelaySeconds > 0.0f) {
				a_out.action.trapArmDelay = std::chrono::milliseconds(static_cast<std::int64_t>(armDelaySeconds * 1000.0f));
			}

			const float rearmDelaySeconds = a_action.value("rearmDelaySeconds", 0.0f);
			if (rearmDelaySeconds > 0.0f) {
				a_out.action.trapRearmDelay = std::chrono::milliseconds(static_cast<std::int64_t>(rearmDelaySeconds * 1000.0f));
			}

			const auto& extraSpells = a_action.value("extraSpells", nlohmann::json::array());
			if (extraSpells.is_array()) {
				a_out.action.trapExtraSpells.clear();
				for (const auto& entry : extraSpells) {
					auto* spell = ParseSpellValue(entry, a_handler);
					if (spell) {
						a_out.action.trapExtraSpells.push_back(spell);
					}
				}
			}

			ParseMagnitudeScaling(a_action, a_out.action.magnitudeScaling);

			if (!a_out.action.spell || a_out.action.trapRadius <= 0.0f || a_out.action.trapTtl.count() <= 0) {
				return false;
			}
			if (a_out.action.trapMaxActive == 0u) {
				a_out.action.trapMaxActive = 1u;
			}
			if (a_out.action.trapMaxTriggers != 0u && a_out.action.trapMaxTriggers < 1u) {
				a_out.action.trapMaxTriggers = 1u;
			}
			if (a_out.action.trapMaxTargetsPerTrigger == 0u) {
				a_out.action.trapMaxTargetsPerTrigger = 1u;
			} else if (a_out.action.trapMaxTargetsPerTrigger > 64u) {
				a_out.action.trapMaxTargetsPerTrigger = 64u;
			}
			if (a_out.action.trapRearmDelay.count() <= 0) {
				// No rearm: force one-shot semantics regardless of maxTriggers.
				a_out.action.trapMaxTriggers = 1u;
			}
			return true;
		}

		return false;
	}

	bool EventBridge::ParseRuntimeActionFromJson(
		const nlohmann::json& a_action,
		std::string_view a_type,
		RE::TESDataHandler* a_handler,
		AffixRuntime& a_out) const
	{
		if (!a_action.is_object()) {
			SKSE::log::warn(
				"CalamityAffixes: runtime action payload must be object (affixId={}, actionType={}); skipping affix.",
				a_out.id,
				a_type);
			return false;
		}

		if (a_type == RuntimeContract::kActionDebugNotify) {
			a_out.action.type = ActionType::kDebugNotify;
			a_out.action.text = a_action.value("text", std::string{});
			return true;
		}

		if (ParseRuntimeSpellActionFromJson(a_action, a_type, a_handler, a_out)) {
			return true;
		}
		if (ParseRuntimeSpecialActionFromJson(a_action, a_type, a_handler, a_out)) {
			return true;
		}
		return ParseRuntimeAreaActionFromJson(a_action, a_type, a_handler, a_out);
	}
}
