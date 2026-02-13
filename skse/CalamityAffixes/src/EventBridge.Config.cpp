#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <system_error>
#include <vector>

#include <Windows.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		std::string_view Trim(std::string_view a_text) noexcept
		{
			auto isWs = [](unsigned char c) {
				return c == ' ' || c == '\t' || c == '\r' || c == '\n';
			};

			while (!a_text.empty() && isWs(static_cast<unsigned char>(a_text.front()))) {
				a_text.remove_prefix(1);
			}
			while (!a_text.empty() && isWs(static_cast<unsigned char>(a_text.back()))) {
				a_text.remove_suffix(1);
			}

			return a_text;
		}

		std::string DeriveAffixLabel(std::string_view a_displayName)
		{
			// Typical convention: "Label (details): description".
			// We want a short label for list display / renaming.
			auto text = Trim(a_displayName);
			if (text.empty()) {
				return {};
			}

			const auto posColon = text.find(':');
			const auto posParen = text.find('(');

			std::size_t cut = std::string_view::npos;
			if (posColon != std::string_view::npos) {
				cut = posColon;
			}
			if (posParen != std::string_view::npos) {
				cut = (cut == std::string_view::npos) ? posParen : std::min(cut, posParen);
			}

			if (cut != std::string_view::npos) {
				text = text.substr(0, cut);
			}

			text = Trim(text);
			std::string label(text);

			auto replaceAll = [&](std::string_view a_from, std::string_view a_to) {
				if (a_from.empty()) {
					return;
				}

				std::size_t pos = 0;
				while ((pos = label.find(a_from, pos)) != std::string::npos) {
					label.replace(pos, a_from.size(), a_to);
					pos += a_to.size();
				}
			};

			// Some UI/font stacks render arrow glyphs as tofu squares.
			// Normalize into ASCII so item labels remain readable in all menus.
			replaceAll("→", "->");
			replaceAll("⇒", "->");
			replaceAll("➜", "->");
			replaceAll("↔", "<->");
			replaceAll("⟶", "->");
			return label;
		}

		RE::SpellItem* LookupSpellFromSpec(std::string_view a_spec, RE::TESDataHandler* a_handler)
		{
			const auto text = Trim(a_spec);
			if (text.empty()) {
				return nullptr;
			}

			const auto pipe = text.find('|');
			if (pipe == std::string_view::npos) {
				return RE::TESForm::LookupByEditorID<RE::SpellItem>(std::string(text));
			}

			if (!a_handler) {
				return nullptr;
			}

			const auto modName = text.substr(0, pipe);
			const auto formIDText = text.substr(pipe + 1);

			std::uint32_t localID = 0;
			auto begin = formIDText.data();
			auto end = formIDText.data() + formIDText.size();
			if (formIDText.starts_with("0x") || formIDText.starts_with("0X")) {
				begin += 2;
			}

			const auto result = std::from_chars(begin, end, localID, 16);
			if (result.ec != std::errc{}) {
				return nullptr;
			}

			return a_handler->LookupForm<RE::SpellItem>(static_cast<RE::FormID>(localID), modName);
		}

		std::optional<std::filesystem::path> GetRuntimeDirectory()
		{
			std::vector<wchar_t> buffer(0x4000);
			const DWORD len = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (len == 0 || len >= buffer.size()) {
				return std::nullopt;
			}

			std::filesystem::path path(buffer.data());
			path = path.remove_filename();
			return path;
		}
	}

	void EventBridge::ResetRuntimeStateForConfigReload()
	{
		_affixes.clear();
		_activeCounts.clear();
		_activeCritDamageBonusPct = 0.0f;
		_affixIndexById.clear();
		_affixIndexByToken.clear();
		_hitTriggerAffixIndices.clear();
		_incomingHitTriggerAffixIndices.clear();
		_dotApplyTriggerAffixIndices.clear();
		_killTriggerAffixIndices.clear();
		_castOnCritAffixIndices.clear();
		_convertAffixIndices.clear();
		_archmageAffixIndices.clear();
		_corpseExplosionAffixIndices.clear();
		_summonCorpseExplosionAffixIndices.clear();
		_lootWeaponAffixes.clear();
		_lootArmorAffixes.clear();
		_lootWeaponSuffixes.clear();
		_lootArmorSuffixes.clear();
		_appliedPassiveSpells.clear();
		_traps.clear();
		_hasActiveTraps.store(false, std::memory_order_relaxed);
			_dotCooldowns.clear();
			_dotCooldownsLastPruneAt = {};
			_dotObservedMagicEffects.clear();
			_dotTagSafetyWarned = false;
			_dotTagSafetySuppressed = false;
			_perTargetCooldowns.clear();
			_perTargetCooldownsLastPruneAt = {};
		_corpseExplosionSeenCorpses.clear();
		_corpseExplosionState = {};
		_summonCorpseExplosionSeenCorpses.clear();
		_summonCorpseExplosionState = {};
		_castOnCritNextAllowed = {};
		_castOnCritCycleCursor = 0;
		_equipResync.nextAtMs = 0;
		_equipResync.intervalMs = static_cast<std::uint64_t>(kEquipResyncInterval.count());
		_loot = {};
		_runtimeEnabled = true;
		_runtimeProcChanceMult = 1.0f;
		_lootRerollGuard.Reset();
		_playerStashedKeys.clear();
		_configLoaded = false;
		_lastHealthDamageSignature = 0;
		_lastHealthDamageSignatureAt = {};
	}

		void EventBridge::LoadConfig()
		{
			ResetRuntimeStateForConfigReload();
			InitializeRunewordCatalog();

		const auto runtimeDir = GetRuntimeDirectory();
		const auto configPath =
			runtimeDir ?
				(*runtimeDir / std::filesystem::path(kRuntimeConfigRelativePath)) :
				(std::filesystem::current_path() / std::filesystem::path(kRuntimeConfigRelativePath));

		SKSE::log::info("CalamityAffixes: loading runtime config from: {}", configPath.string());

		std::ifstream in(configPath);
		if (!in.is_open()) {
			SKSE::log::warn("CalamityAffixes: runtime config not found: {}", configPath.string());
			return;
		}

		nlohmann::json j;
		try {
			in >> j;
		} catch (const std::exception&) {
			SKSE::log::error("CalamityAffixes: failed to parse runtime config (affixes.json).");
			return;
		}

			const auto& loot = j.value("loot", nlohmann::json::object());
			if (loot.is_object()) {
				_loot.chancePercent = loot.value("chancePercent", 0.0f);
				_loot.runewordFragmentChancePercent =
					static_cast<float>(loot.value("runewordFragmentChancePercent", static_cast<double>(_loot.runewordFragmentChancePercent)));
				_loot.renameItem = loot.value("renameItem", false);
				_loot.sharedPool = loot.value("sharedPool", false);
				_loot.debugLog = loot.value("debugLog", false);
				_loot.dotTagSafetyAutoDisable = loot.value("dotTagSafetyAutoDisable", _loot.dotTagSafetyAutoDisable);
				const double dotTagSafetyThreshold =
					loot.value("dotTagSafetyUniqueEffectThreshold", static_cast<double>(_loot.dotTagSafetyUniqueEffectThreshold));
				const double trapGlobalMaxActive = loot.value("trapGlobalMaxActive", static_cast<double>(_loot.trapGlobalMaxActive));
				_loot.nameFormat = loot.value("nameFormat", std::string{ "{base} [{affix}]" });

			if (_loot.chancePercent < 0.0f) {
				_loot.chancePercent = 0.0f;
			} else if (_loot.chancePercent > 100.0f) {
				_loot.chancePercent = 100.0f;
			}

				if (_loot.runewordFragmentChancePercent < 0.0f) {
					_loot.runewordFragmentChancePercent = 0.0f;
				} else if (_loot.runewordFragmentChancePercent > 100.0f) {
					_loot.runewordFragmentChancePercent = 100.0f;
				}

				if (dotTagSafetyThreshold <= 0.0) {
					_loot.dotTagSafetyUniqueEffectThreshold = 0u;
				} else {
					const double clamped = std::clamp(dotTagSafetyThreshold, 8.0, 4096.0);
					_loot.dotTagSafetyUniqueEffectThreshold = static_cast<std::uint32_t>(clamped);
				}

				if (trapGlobalMaxActive <= 0.0) {
					_loot.trapGlobalMaxActive = 0u;
			} else {
				const double clamped = std::clamp(trapGlobalMaxActive, 1.0, 4096.0);
				_loot.trapGlobalMaxActive = static_cast<std::uint32_t>(clamped);
			}
		}

		spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
		spdlog::flush_on(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);

		const auto& keywords = j.value("keywords", nlohmann::json::object());
		const auto& affixes = keywords.value("affixes", nlohmann::json::array());
		if (!affixes.is_array()) {
			return;
		}

		auto* handler = RE::TESDataHandler::GetSingleton();

		auto parseTrigger = [](std::string_view a_trigger) -> std::optional<Trigger> {
			if (a_trigger == "Hit") {
				return Trigger::kHit;
			}
			if (a_trigger == "IncomingHit") {
				return Trigger::kIncomingHit;
			}
			if (a_trigger == "DotApply") {
				return Trigger::kDotApply;
			}
			if (a_trigger == "Kill") {
				return Trigger::kKill;
			}
			return std::nullopt;
		};

		auto parseSpell = [&](const nlohmann::json& a_action) -> RE::SpellItem* {
			const auto editorId = a_action.value("spellEditorId", std::string{});
			if (!editorId.empty()) {
				return LookupSpellFromSpec(editorId, handler);
			}

			const auto formSpec = a_action.value("spellForm", std::string{});
			return LookupSpellFromSpec(formSpec, handler);
		};

		auto parseSpellFromString = [&](std::string_view a_spec) -> RE::SpellItem* {
			return LookupSpellFromSpec(a_spec, handler);
		};

		auto parseSpellValue = [&](const nlohmann::json& a_value) -> RE::SpellItem* {
			if (a_value.is_string()) {
				return parseSpellFromString(a_value.get<std::string>());
			}
			if (a_value.is_object()) {
				return parseSpell(a_value);
			}
			return nullptr;
		};

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

		auto parseAdaptiveMode = [](std::string_view a_mode) -> AdaptiveElementMode {
			if (a_mode == "StrongestResist" || a_mode == "Strongest") {
				return AdaptiveElementMode::kStrongestResist;
			}
			return AdaptiveElementMode::kWeakestResist;
		};

		auto parseTrapSpawnAt = [](std::string_view a_spawnAt) -> TrapSpawnAt {
			if (a_spawnAt == "Self" || a_spawnAt == "Owner" || a_spawnAt == "OwnerFeet") {
				return TrapSpawnAt::kOwnerFeet;
			}
			return TrapSpawnAt::kTargetFeet;
		};

		auto parseAffixSlot = [](std::string_view a_slot) -> AffixSlot {
			if (a_slot == "Prefix" || a_slot == "prefix") {
				return AffixSlot::kPrefix;
			}
			if (a_slot == "Suffix" || a_slot == "suffix") {
				return AffixSlot::kSuffix;
			}
			return AffixSlot::kNone;
		};

		auto parseMagnitudeSource = [](std::string_view a_source) -> MagnitudeScaling::Source {
			if (a_source == "HitPhysicalDealt") {
				return MagnitudeScaling::Source::kHitPhysicalDealt;
			}
			if (a_source == "HitTotalDealt") {
				return MagnitudeScaling::Source::kHitTotalDealt;
			}
			return MagnitudeScaling::Source::kNone;
		};

		auto parseMagnitudeScaling = [&](const nlohmann::json& a_action, MagnitudeScaling& a_outScaling) {
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
		};

		auto isSpecialActionType = [](std::string_view a_type) -> bool {
			return a_type == "CastOnCrit" ||
			       a_type == "ConvertDamage" ||
			       a_type == "Archmage" ||
			       a_type == "CorpseExplosion" ||
			       a_type == "SummonCorpseExplosion";
		};

		for (const auto& a : affixes) {
			if (!a.is_object()) {
				continue;
			}

			AffixRuntime out{};
			out.id = a.value("id", std::string{});
			out.token = out.id.empty() ? 0u : MakeAffixToken(out.id);
			out.action.sourceToken = out.token;
			out.keywordEditorId = a.value("editorId", std::string{});
			if (out.keywordEditorId.empty()) {
				continue;
			}

			out.displayName = a.value("name", std::string{});
			if (out.displayName.empty()) {
				out.displayName = out.keywordEditorId;
			}
			out.displayNameEn = a.value("nameEn", std::string{});
			out.displayNameKo = a.value("nameKo", std::string{});
			if (out.displayNameEn.empty()) {
				out.displayNameEn = out.displayName;
			}
			if (out.displayNameKo.empty()) {
				out.displayNameKo = out.displayName;
			}
			out.label = DeriveAffixLabel(out.displayName);
			if (out.label.empty()) {
				out.label = out.displayName;
			}

			out.keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(out.keywordEditorId);
			if (!out.keyword) {
				continue;
			}

			{
				const auto slotIt = a.find("slot");
				if (slotIt != a.end() && slotIt->is_string()) {
					out.slot = parseAffixSlot(slotIt->get<std::string>());
				}
				const auto famIt = a.find("family");
				if (famIt != a.end() && famIt->is_string()) {
					out.family = famIt->get<std::string>();
				}
			}

			const auto& kid = a.value("kid", nlohmann::json::object());
			if (kid.is_object()) {
				const auto type = kid.value("type", std::string{});
				out.lootType = ParseLootItemType(type);
			}

			const auto& runtime = a.value("runtime", nlohmann::json::object());
			if (!runtime.is_object()) {
				continue;
			}

			const auto& action = runtime.value("action", nlohmann::json::object());
			if (action.is_object()) {
				out.action.debugNotify = action.value("debugNotify", false);
			}
			const auto type = action.value("type", std::string{});

			if (out.slot == AffixSlot::kSuffix) {
				out.trigger = Trigger::kHit;
				out.procChancePct = static_cast<float>(kid.value("chance", 1.0));

				const auto passiveSpellId = action.value("passiveSpellEditorId", std::string{});
				if (!passiveSpellId.empty()) {
					out.passiveSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>(passiveSpellId);
					if (!out.passiveSpell) {
						SKSE::log::warn(
							"CalamityAffixes: suffix passive spell not found (affixId={}, spellEditorId={}).",
							out.id, passiveSpellId);
					}
				}
				out.critDamageBonusPct = static_cast<float>(action.value("critDamageBonusPct", 0.0));
			} else {
				const auto triggerStr = runtime.value("trigger", std::string{});
				const auto trigger = parseTrigger(triggerStr);
				if (!trigger) {
					if (isSpecialActionType(type)) {
						// Special actions use dedicated execution paths and do not consume Trigger routing.
						// Keep loading them even if trigger text is missing/invalid.
						out.trigger = Trigger::kHit;
						SKSE::log::warn(
							"CalamityAffixes: special action uses invalid trigger (affixId={}, actionType={}, trigger='{}'); defaulting to Hit.",
							out.id,
							type,
							triggerStr);
					} else {
						SKSE::log::error(
							"CalamityAffixes: invalid trigger '{}' (affixId={}, actionType={}); skipping affix.",
							triggerStr,
							out.id,
							type);
						continue;
					}
				} else {
					out.trigger = *trigger;
				}

				out.procChancePct = runtime.value("procChancePercent", 0.0f);
				if (const auto lwIt = runtime.find("lootWeight"); lwIt != runtime.end() && lwIt->is_number()) {
					out.lootWeight = lwIt->get<float>();
				}
			const float icdSeconds = runtime.value("icdSeconds", 0.0f);
			if (icdSeconds > 0.0f) {
				out.icd = std::chrono::milliseconds(static_cast<std::int64_t>(icdSeconds * 1000.0f));
			}
			const float perTargetIcdSeconds = runtime.value("perTargetICDSeconds", 0.0f);
			if (perTargetIcdSeconds > 0.0f) {
				out.perTargetIcd = std::chrono::milliseconds(static_cast<std::int64_t>(perTargetIcdSeconds * 1000.0f));
			}
			}

			if (type == "DebugNotify") {
				out.action.type = ActionType::kDebugNotify;
				out.action.text = action.value("text", std::string{});
			} else if (type == "CastSpell") {
				out.action.type = ActionType::kCastSpell;
				out.action.spell = parseSpell(action);
				out.action.effectiveness = action.value("effectiveness", 1.0f);
				out.action.magnitudeOverride = action.value("magnitudeOverride", 0.0f);
				out.action.noHitEffectArt = action.value("noHitEffectArt", false);

				if (!out.action.spell) {
					SKSE::log::error(
						"CalamityAffixes: CastSpell action missing spell (affixId={}, spellEditorId={}, spellForm={}).",
						out.id,
						action.value("spellEditorId", std::string{}),
						action.value("spellForm", std::string{}));
				}

				parseMagnitudeScaling(action, out.action.magnitudeScaling);

				const auto applyTo = action.value("applyTo", std::string{});
				out.action.applyToSelf = (applyTo == "Self");

				const auto& evolution = action.value("evolution", nlohmann::json::object());
				if (evolution.is_object()) {
					const int xpPerProc = evolution.value("xpPerProc", 1);
					out.action.evolutionXpPerProc = (xpPerProc > 0) ? static_cast<std::uint32_t>(xpPerProc) : 1u;

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

							out.action.evolutionThresholds.push_back(static_cast<std::uint32_t>(raw));
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

							out.action.evolutionMultipliers.push_back(value);
						}
					}

					std::sort(out.action.evolutionThresholds.begin(), out.action.evolutionThresholds.end());
					out.action.evolutionThresholds.erase(
						std::unique(out.action.evolutionThresholds.begin(), out.action.evolutionThresholds.end()),
						out.action.evolutionThresholds.end());
					if (out.action.evolutionThresholds.empty()) {
						out.action.evolutionThresholds.push_back(0u);
					}

					if (out.action.evolutionMultipliers.empty()) {
						out.action.evolutionMultipliers.push_back(1.0f);
					}
					while (out.action.evolutionMultipliers.size() < out.action.evolutionThresholds.size()) {
						out.action.evolutionMultipliers.push_back(out.action.evolutionMultipliers.back());
					}

					out.action.evolutionEnabled = true;
				}

				const auto& modeCycle = action.value("modeCycle", nlohmann::json::object());
				if (modeCycle.is_object()) {
					const auto& spells = modeCycle.value("spells", nlohmann::json::array());
					if (spells.is_array()) {
						for (const auto& entry : spells) {
							auto* parsed = parseSpellValue(entry);
							if (parsed) {
								out.action.modeCycleSpells.push_back(parsed);
							}
						}
					}

					const auto& labels = modeCycle.value("labels", nlohmann::json::array());
					if (labels.is_array()) {
						for (const auto& entry : labels) {
							if (!entry.is_string()) {
								continue;
							}
							out.action.modeCycleLabels.push_back(entry.get<std::string>());
						}
					}

					const int switchEveryProcs = modeCycle.value("switchEveryProcs", 1);
					out.action.modeCycleSwitchEveryProcs = (switchEveryProcs > 0) ? static_cast<std::uint32_t>(switchEveryProcs) : 1u;
					out.action.modeCycleManualOnly = modeCycle.value("manualOnly", false);
					if (!out.action.modeCycleSpells.empty()) {
						out.action.modeCycleEnabled = true;
					}
				}
			} else if (type == "CastSpellAdaptiveElement") {
				out.action.type = ActionType::kCastSpellAdaptiveElement;
				out.action.effectiveness = action.value("effectiveness", 1.0f);
				out.action.magnitudeOverride = action.value("magnitudeOverride", 0.0f);
				out.action.noHitEffectArt = action.value("noHitEffectArt", false);

				out.action.adaptiveMode = parseAdaptiveMode(action.value("mode", std::string{ "WeakestResist" }));

				const auto& spells = action.value("spells", nlohmann::json::object());
				if (spells.is_object()) {
					out.action.adaptiveFire = parseSpellValue(spells.value("Fire", nlohmann::json{}));
					out.action.adaptiveFrost = parseSpellValue(spells.value("Frost", nlohmann::json{}));
					out.action.adaptiveShock = parseSpellValue(spells.value("Shock", nlohmann::json{}));
				}

				if (!out.action.adaptiveFire && !out.action.adaptiveFrost && !out.action.adaptiveShock) {
					SKSE::log::error("CalamityAffixes: CastSpellAdaptiveElement missing spells (affixId={}).", out.id);
					continue;
				}

				parseMagnitudeScaling(action, out.action.magnitudeScaling);

				const auto applyTo = action.value("applyTo", std::string{});
				out.action.applyToSelf = (applyTo == "Self");
			} else if (type == "CastOnCrit") {
				out.action.type = ActionType::kCastOnCrit;
				out.action.spell = parseSpell(action);
				out.action.effectiveness = action.value("effectiveness", 1.0f);
				out.action.magnitudeOverride = action.value("magnitudeOverride", 0.0f);
				out.action.noHitEffectArt = action.value("noHitEffectArt", false);

				parseMagnitudeScaling(action, out.action.magnitudeScaling);

				if (!out.action.spell) {
					continue;
				}
			} else if (type == "ConvertDamage") {
				out.action.type = ActionType::kConvertDamage;
				const auto element = action.value("element", std::string{});
				out.action.element = parseElement(element);
				out.action.convertPct = action.value("percent", 0.0f);
				out.action.spell = parseSpell(action);
				out.action.effectiveness = action.value("effectiveness", 1.0f);
				out.action.noHitEffectArt = action.value("noHitEffectArt", false);

				if (!out.action.spell || out.action.element == Element::kNone || out.action.convertPct <= 0.0f) {
					continue;
				}
			} else if (type == "Archmage") {
				out.action.type = ActionType::kArchmage;
				out.action.spell = parseSpell(action);
				out.action.effectiveness = action.value("effectiveness", 1.0f);
				out.action.noHitEffectArt = action.value("noHitEffectArt", false);
				out.action.archmageDamagePctOfMaxMagicka = action.value("damagePctOfMaxMagicka", 0.0f);
				out.action.archmageCostPctOfMaxMagicka = action.value("costPctOfMaxMagicka", 0.0f);

				if (!out.action.spell || out.action.archmageDamagePctOfMaxMagicka <= 0.0f || out.action.archmageCostPctOfMaxMagicka <= 0.0f) {
					continue;
				}
			} else if (type == "CorpseExplosion" || type == "SummonCorpseExplosion") {
				out.action.type = (type == "SummonCorpseExplosion") ?
					ActionType::kSummonCorpseExplosion :
					ActionType::kCorpseExplosion;
				out.action.spell = parseSpell(action);
				out.action.effectiveness = action.value("effectiveness", 1.0f);
				out.action.noHitEffectArt = action.value("noHitEffectArt", true);

				out.action.corpseExplosionFlatDamage = action.value("flatDamage", 0.0f);
				out.action.corpseExplosionPctOfCorpseMaxHealth = action.value("pctOfCorpseMaxHealth", 0.0f);
				out.action.corpseExplosionRadius = action.value("radius", 0.0f);
				out.action.corpseExplosionMaxTargets = action.value("maxTargets", 8u);
				out.action.corpseExplosionMaxChainDepth = action.value("maxChainDepth", 1u);
				out.action.corpseExplosionChainFalloff = action.value("chainFalloff", 1.0f);

				const float chainWindowSeconds = action.value("chainWindowSeconds", 0.0f);
				if (chainWindowSeconds > 0.0f) {
					out.action.corpseExplosionChainWindow = std::chrono::milliseconds(static_cast<std::int64_t>(chainWindowSeconds * 1000.0f));
				}

				const float rateWindowSeconds = action.value("rateLimitWindowSeconds", 1.0f);
				if (rateWindowSeconds > 0.0f) {
					out.action.corpseExplosionRateLimitWindow = std::chrono::milliseconds(static_cast<std::int64_t>(rateWindowSeconds * 1000.0f));
				}
				out.action.corpseExplosionMaxExplosionsPerWindow = action.value("maxExplosionsPerWindow", 4u);

				if (!out.action.spell) {
					continue;
				}
				if (out.action.corpseExplosionRadius <= 0.0f || out.action.corpseExplosionMaxTargets == 0u) {
					continue;
				}
				if (out.action.corpseExplosionFlatDamage <= 0.0f && out.action.corpseExplosionPctOfCorpseMaxHealth <= 0.0f) {
					continue;
				}
				if (out.action.corpseExplosionMaxChainDepth == 0u) {
					out.action.corpseExplosionMaxChainDepth = 1u;
				}
				if (out.action.corpseExplosionChainFalloff <= 0.0f || out.action.corpseExplosionChainFalloff > 1.0f) {
					out.action.corpseExplosionChainFalloff = 1.0f;
				}
				if (out.action.corpseExplosionRateLimitWindow.count() <= 0 || out.action.corpseExplosionMaxExplosionsPerWindow == 0u) {
					out.action.corpseExplosionRateLimitWindow = std::chrono::milliseconds(1000);
					out.action.corpseExplosionMaxExplosionsPerWindow = 4u;
				}
			} else if (type == "SpawnTrap") {
				out.action.type = ActionType::kSpawnTrap;
				out.action.spell = parseSpell(action);
				out.action.effectiveness = action.value("effectiveness", 1.0f);
				out.action.magnitudeOverride = action.value("magnitudeOverride", 0.0f);
				out.action.noHitEffectArt = action.value("noHitEffectArt", true);

				out.action.trapSpawnAt = parseTrapSpawnAt(action.value("spawnAt", std::string{ "Target" }));
				out.action.trapRadius = action.value("radius", 0.0f);
				out.action.trapMaxActive = action.value("maxActive", 2u);
				out.action.trapMaxTriggers = action.value("maxTriggers", 1u);
				out.action.trapRequireCritOrPowerAttack = action.value("requireCritOrPowerAttack", false);
				out.action.trapRequireWeaponHit = action.value("requireWeaponHit", true);

				const float ttlSeconds = action.value("ttlSeconds", 0.0f);
				if (ttlSeconds > 0.0f) {
					out.action.trapTtl = std::chrono::milliseconds(static_cast<std::int64_t>(ttlSeconds * 1000.0f));
				}

				const float armDelaySeconds = action.value("armDelaySeconds", 0.0f);
				if (armDelaySeconds > 0.0f) {
					out.action.trapArmDelay = std::chrono::milliseconds(static_cast<std::int64_t>(armDelaySeconds * 1000.0f));
				}

				const float rearmDelaySeconds = action.value("rearmDelaySeconds", 0.0f);
				if (rearmDelaySeconds > 0.0f) {
					out.action.trapRearmDelay = std::chrono::milliseconds(static_cast<std::int64_t>(rearmDelaySeconds * 1000.0f));
				}

				const auto& extraSpells = action.value("extraSpells", nlohmann::json::array());
				if (extraSpells.is_array()) {
					out.action.trapExtraSpells.clear();
					for (const auto& entry : extraSpells) {
						RE::SpellItem* spell = nullptr;
						if (entry.is_string()) {
							const auto spec = entry.get<std::string>();
							spell = parseSpellFromString(spec);
						} else if (entry.is_object()) {
							spell = parseSpell(entry);
						}

						if (spell) {
							out.action.trapExtraSpells.push_back(spell);
						}
					}
				}

				parseMagnitudeScaling(action, out.action.magnitudeScaling);

				if (!out.action.spell || out.action.trapRadius <= 0.0f || out.action.trapTtl.count() <= 0) {
					continue;
				}
				if (out.action.trapMaxActive == 0u) {
					out.action.trapMaxActive = 1u;
				}
				if (out.action.trapMaxTriggers == 0u) {
					// 0 = unlimited triggers until TTL.
				} else if (out.action.trapMaxTriggers < 1u) {
					out.action.trapMaxTriggers = 1u;
				}
				if (out.action.trapRearmDelay.count() <= 0) {
					// No rearm: force one-shot semantics regardless of maxTriggers.
					out.action.trapMaxTriggers = 1u;
				}
			} else {
				continue;
			}

			const bool isSpecialAction =
				out.action.type == ActionType::kCastOnCrit ||
				out.action.type == ActionType::kConvertDamage ||
				out.action.type == ActionType::kArchmage ||
				out.action.type == ActionType::kCorpseExplosion ||
				out.action.type == ActionType::kSummonCorpseExplosion;

			if (isSpecialAction && out.perTargetIcd.count() > 0) {
				SKSE::log::warn(
					"CalamityAffixes: perTargetICDSeconds is ignored for special action (affixId={}, actionType={}).",
					out.id,
					type);
			}

			_affixes.push_back(std::move(out));
			const auto idx = _affixes.size() - 1;

			const bool isTriggerAction =
				_affixes[idx].action.type == ActionType::kDebugNotify ||
				_affixes[idx].action.type == ActionType::kCastSpell ||
				_affixes[idx].action.type == ActionType::kCastSpellAdaptiveElement ||
				_affixes[idx].action.type == ActionType::kSpawnTrap;

			if (isTriggerAction) {
				switch (_affixes[idx].trigger) {
				case Trigger::kHit:
					_hitTriggerAffixIndices.push_back(idx);
					break;
				case Trigger::kIncomingHit:
					_incomingHitTriggerAffixIndices.push_back(idx);
					break;
				case Trigger::kDotApply:
					_dotApplyTriggerAffixIndices.push_back(idx);
					break;
				case Trigger::kKill:
					_killTriggerAffixIndices.push_back(idx);
					break;
				}
			}

			if (_affixes[idx].action.type == ActionType::kCastOnCrit) {
				_castOnCritAffixIndices.push_back(idx);
			} else if (_affixes[idx].action.type == ActionType::kConvertDamage) {
				_convertAffixIndices.push_back(idx);
			} else if (_affixes[idx].action.type == ActionType::kArchmage) {
				_archmageAffixIndices.push_back(idx);
			} else if (_affixes[idx].action.type == ActionType::kCorpseExplosion) {
				_corpseExplosionAffixIndices.push_back(idx);
			} else if (_affixes[idx].action.type == ActionType::kSummonCorpseExplosion) {
				_summonCorpseExplosionAffixIndices.push_back(idx);
			}
		}

		for (std::size_t i = 0; i < _affixes.size(); i++) {
			const auto& affix = _affixes[i];
			if (!affix.id.empty()) {
				const auto [it, inserted] = _affixIndexById.emplace(affix.id, i);
				if (!inserted) {
					SKSE::log::error(
						"CalamityAffixes: duplicate affix id '{}' in runtime config (firstIdx={}, dupIdx={}).",
						affix.id,
						it->second,
						i);
				}
			}
			if (affix.token != 0u) {
				const auto [it, inserted] = _affixIndexByToken.emplace(affix.token, i);
				if (!inserted) {
					SKSE::log::error(
						"CalamityAffixes: duplicate affix token in runtime config (id='{}', firstIdx={}, dupIdx={}).",
						affix.id,
						it->second,
						i);
				}
			}

			if (affix.lootType) {
				if (affix.slot == AffixSlot::kSuffix) {
					if (*affix.lootType == LootItemType::kWeapon) {
						_lootWeaponSuffixes.push_back(i);
					} else if (*affix.lootType == LootItemType::kArmor) {
						_lootArmorSuffixes.push_back(i);
					}
				} else {
					if (*affix.lootType == LootItemType::kWeapon) {
						_lootWeaponAffixes.push_back(i);
					} else if (*affix.lootType == LootItemType::kArmor) {
						_lootArmorAffixes.push_back(i);
					}
				}
			}
		}

		auto registerRuntimeAffix = [&](AffixRuntime&& a_affix, bool a_warnOnDuplicate = true) {
			_affixes.push_back(std::move(a_affix));
			const auto idx = _affixes.size() - 1;
			const auto& affix = _affixes[idx];

			const bool isTriggerAction =
				affix.action.type == ActionType::kDebugNotify ||
				affix.action.type == ActionType::kCastSpell ||
				affix.action.type == ActionType::kCastSpellAdaptiveElement ||
				affix.action.type == ActionType::kSpawnTrap;
			if (isTriggerAction) {
				switch (affix.trigger) {
				case Trigger::kHit:
					_hitTriggerAffixIndices.push_back(idx);
					break;
				case Trigger::kIncomingHit:
					_incomingHitTriggerAffixIndices.push_back(idx);
					break;
				case Trigger::kDotApply:
					_dotApplyTriggerAffixIndices.push_back(idx);
					break;
				case Trigger::kKill:
					_killTriggerAffixIndices.push_back(idx);
					break;
				}
			}

			if (!affix.id.empty()) {
				const auto [it, inserted] = _affixIndexById.emplace(affix.id, idx);
				if (!inserted && a_warnOnDuplicate) {
					SKSE::log::warn(
						"CalamityAffixes: synthesized affix id duplicated (id='{}', firstIdx={}, dupIdx={}).",
						affix.id,
						it->second,
						idx);
				}
			}

			if (affix.token != 0u) {
				const auto [it, inserted] = _affixIndexByToken.emplace(affix.token, idx);
				if (!inserted && a_warnOnDuplicate) {
					SKSE::log::warn(
						"CalamityAffixes: synthesized affix token duplicated (id='{}', firstIdx={}, dupIdx={}).",
						affix.id,
						it->second,
						idx);
				}
			}

			if (affix.lootType) {
				if (affix.slot == AffixSlot::kSuffix) {
					if (*affix.lootType == LootItemType::kWeapon) {
						_lootWeaponSuffixes.push_back(idx);
					} else if (*affix.lootType == LootItemType::kArmor) {
						_lootArmorSuffixes.push_back(idx);
					}
				} else {
					if (*affix.lootType == LootItemType::kWeapon) {
						_lootWeaponAffixes.push_back(idx);
					} else if (*affix.lootType == LootItemType::kArmor) {
						_lootArmorAffixes.push_back(idx);
					}
				}
			}
		};

			auto* dynamicFire = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DMG_FIRE_DYNAMIC");
			auto* dynamicFrost = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DMG_FROST_DYNAMIC");
			auto* dynamicShock = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DMG_SHOCK_DYNAMIC");
			auto* spellArcLightning = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_ARC_LIGHTNING");
			auto* shredFire = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_FIRE_SHRED");
			auto* shredFrost = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_FROST_SHRED");
			auto* shredShock = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_SHOCK_SHRED");
			auto* spellWard = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_FORTITUDE_WARD");
			auto* spellPhase = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_ENIGMA_PHASE");
			auto* spellMeditation = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_INSIGHT_MEDITATION");
			auto* spellBarrier = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_EXILE_BARRIER");
			auto* spellPhoenix = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_RW_PHOENIX_SURGE");
			auto* spellHaste = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_SWAP_JACKPOT_HASTE");
			auto* spellDotPoison = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DOT_BLOOM_POISON");
			auto* spellDotTar = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DOT_BLOOM_TAR_SLOW");
			auto* spellDotSiphon = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_DOT_BLOOM_SIPHON_MAG");
			auto* spellChaosSunder = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_CHAOS_CURSE_SUNDER");
			auto* spellChaosFragile = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_CHAOS_CURSE_FRAGILE");
			auto* spellChaosSlowAttack = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_CHAOS_CURSE_SLOW_ATTACK");
			auto* spellVanillaSoulTrap = parseSpellFromString("Skyrim.esm|0004DBA4");
			auto* spellCustomInvisibility = RE::TESForm::LookupByEditorID<RE::SpellItem>("CAFF_SPEL_INVISIBILITY");
			auto* spellVanillaMuffle = parseSpellFromString("Skyrim.esm|0008F3EB");
			auto* spellVanillaFlameCloak = parseSpellFromString("Skyrim.esm|0003AE9F");
			auto* spellVanillaFrostCloak = parseSpellFromString("Skyrim.esm|0003AEA2");
			auto* spellVanillaShockCloak = parseSpellFromString("Skyrim.esm|0003AEA3");
			auto* spellVanillaOakflesh = parseSpellFromString("Skyrim.esm|0005AD5C");
			auto* spellVanillaStoneflesh = parseSpellFromString("Skyrim.esm|0005AD5D");
			auto* spellVanillaIronflesh = parseSpellFromString("Skyrim.esm|00051B16");
			auto* spellVanillaEbonyflesh = parseSpellFromString("Skyrim.esm|0005AD5E");
			auto* spellVanillaFear = parseSpellFromString("Skyrim.esm|0004DEEA");
			auto* spellVanillaFrenzy = parseSpellFromString("Skyrim.esm|0004DEEE");
			auto* spellSummonFamiliar = parseSpellFromString("Skyrim.esm|000640B6");
			auto* spellSummonFlameAtronach = parseSpellFromString("Skyrim.esm|000204C3");
			auto* spellSummonFrostAtronach = parseSpellFromString("Skyrim.esm|000204C4");
			auto* spellSummonStormAtronach = parseSpellFromString("Skyrim.esm|000204C5");
			auto* spellSummonDremoraLord = parseSpellFromString("Skyrim.esm|0010DDEC");

			enum class SyntheticRunewordStyle : std::uint8_t
			{
				kAdaptiveStrike,
				kAdaptiveExposure,
				kFireStrike,
				kFrostStrike,
				kShockStrike,
				kPoisonBloom,
				kTarBloom,
				kSiphonBloom,
				kCurseFragile,
				kCurseSlowAttack,
				kCurseFear,
				kCurseFrenzy,
				kSelfHaste,
				kSelfWard,
				kSelfBarrier,
				kSelfMeditation,
				kSelfPhase,
				kSelfPhoenix,
				kSelfFlameCloak,
				kSelfFrostCloak,
				kSelfShockCloak,
				kSelfOakflesh,
				kSelfStoneflesh,
				kSelfIronflesh,
				kSelfEbonyflesh,
				kSelfMuffle,
				kSelfInvisibility,
				kSoulTrap,
				kSummonFamiliar,
				kSummonFlameAtronach,
				kSummonFrostAtronach,
				kSummonStormAtronach,
				kSummonDremoraLord,
				kLegacyFallback
			};

			auto styleName = [](SyntheticRunewordStyle a_style) -> std::string_view {
				switch (a_style) {
				case SyntheticRunewordStyle::kAdaptiveStrike:
					return "AdaptiveStrike";
				case SyntheticRunewordStyle::kAdaptiveExposure:
					return "AdaptiveExposure";
				case SyntheticRunewordStyle::kFireStrike:
					return "FireStrike";
				case SyntheticRunewordStyle::kFrostStrike:
					return "FrostStrike";
				case SyntheticRunewordStyle::kShockStrike:
					return "ShockStrike";
				case SyntheticRunewordStyle::kPoisonBloom:
					return "PoisonBloom";
				case SyntheticRunewordStyle::kTarBloom:
					return "TarBloom";
				case SyntheticRunewordStyle::kSiphonBloom:
					return "SiphonBloom";
				case SyntheticRunewordStyle::kCurseFragile:
					return "CurseFragile";
				case SyntheticRunewordStyle::kCurseSlowAttack:
					return "CurseSlowAttack";
				case SyntheticRunewordStyle::kCurseFear:
					return "CurseFear";
				case SyntheticRunewordStyle::kCurseFrenzy:
					return "CurseFrenzy";
				case SyntheticRunewordStyle::kSelfHaste:
					return "SelfHaste";
				case SyntheticRunewordStyle::kSelfWard:
					return "SelfWard";
				case SyntheticRunewordStyle::kSelfBarrier:
					return "SelfBarrier";
				case SyntheticRunewordStyle::kSelfMeditation:
					return "SelfMeditation";
				case SyntheticRunewordStyle::kSelfPhase:
					return "SelfPhase";
				case SyntheticRunewordStyle::kSelfPhoenix:
					return "SelfPhoenix";
				case SyntheticRunewordStyle::kSelfFlameCloak:
					return "SelfFlameCloak";
				case SyntheticRunewordStyle::kSelfFrostCloak:
					return "SelfFrostCloak";
				case SyntheticRunewordStyle::kSelfShockCloak:
					return "SelfShockCloak";
				case SyntheticRunewordStyle::kSelfOakflesh:
					return "SelfOakflesh";
				case SyntheticRunewordStyle::kSelfStoneflesh:
					return "SelfStoneflesh";
				case SyntheticRunewordStyle::kSelfIronflesh:
					return "SelfIronflesh";
				case SyntheticRunewordStyle::kSelfEbonyflesh:
					return "SelfEbonyflesh";
				case SyntheticRunewordStyle::kSelfMuffle:
					return "SelfMuffle";
				case SyntheticRunewordStyle::kSelfInvisibility:
					return "SelfInvisibility";
				case SyntheticRunewordStyle::kSoulTrap:
					return "SoulTrap";
				case SyntheticRunewordStyle::kSummonFamiliar:
					return "SummonFamiliar";
				case SyntheticRunewordStyle::kSummonFlameAtronach:
					return "SummonFlameAtronach";
				case SyntheticRunewordStyle::kSummonFrostAtronach:
					return "SummonFrostAtronach";
				case SyntheticRunewordStyle::kSummonStormAtronach:
					return "SummonStormAtronach";
				case SyntheticRunewordStyle::kSummonDremoraLord:
					return "SummonDremoraLord";
				case SyntheticRunewordStyle::kLegacyFallback:
					return "LegacyFallback";
				default:
					return "Unknown";
				}
			};

			auto recipeBucket = [](std::string_view a_id, std::uint32_t a_modulo) -> std::uint32_t {
				if (a_modulo == 0u) {
					return 0u;
				}
				std::uint32_t hash = 2166136261u;
				for (const auto c : a_id) {
					hash ^= static_cast<std::uint8_t>(c);
					hash *= 16777619u;
				}
				return hash % a_modulo;
			};

			auto idIsOneOf = [](std::string_view a_id, std::initializer_list<std::string_view> a_candidates) -> bool {
				for (const auto candidate : a_candidates) {
					if (a_id == candidate) {
						return true;
					}
				}
				return false;
			};

			auto resolveRunewordStyle = [&](const RunewordRecipe& a_recipe) -> SyntheticRunewordStyle {
				const std::string_view id = a_recipe.id;
				if (idIsOneOf(id, { "rw_dragon", "rw_hand_of_justice", "rw_flickering_flame", "rw_temper" })) {
					return SyntheticRunewordStyle::kSelfFlameCloak;
				}
				if (idIsOneOf(id, { "rw_ice", "rw_voice_of_reason", "rw_hearth" })) {
					return SyntheticRunewordStyle::kSelfFrostCloak;
				}
				if (idIsOneOf(id, { "rw_holy_thunder", "rw_zephyr", "rw_wind" })) {
					return SyntheticRunewordStyle::kSelfShockCloak;
				}
				if (idIsOneOf(id, { "rw_bulwark", "rw_cure", "rw_ancients_pledge", "rw_lionheart", "rw_radiance" })) {
					return SyntheticRunewordStyle::kSelfOakflesh;
				}
				if (idIsOneOf(id, { "rw_sanctuary", "rw_duress", "rw_peace", "rw_myth" })) {
					return SyntheticRunewordStyle::kSelfStoneflesh;
				}
				if (idIsOneOf(id, { "rw_pride", "rw_stone", "rw_prudence", "rw_mist" })) {
					return SyntheticRunewordStyle::kSelfIronflesh;
				}
				if (idIsOneOf(id, { "rw_metamorphosis" })) {
					return SyntheticRunewordStyle::kSelfEbonyflesh;
				}
				if (idIsOneOf(id, { "rw_nadir" })) {
					return SyntheticRunewordStyle::kCurseFear;
				}
				if (idIsOneOf(id, { "rw_delirium", "rw_chaos" })) {
					return SyntheticRunewordStyle::kCurseFrenzy;
				}
				if (idIsOneOf(id, { "rw_malice", "rw_venom", "rw_plague", "rw_bramble" })) {
					return SyntheticRunewordStyle::kPoisonBloom;
				}
				if (idIsOneOf(id, { "rw_black", "rw_rift" })) {
					return SyntheticRunewordStyle::kTarBloom;
				}
				if (idIsOneOf(id, { "rw_mosaic", "rw_obsession", "rw_white" })) {
					return SyntheticRunewordStyle::kSiphonBloom;
				}
				if (idIsOneOf(id, { "rw_lawbringer", "rw_wrath", "rw_kingslayer", "rw_principle" })) {
					return SyntheticRunewordStyle::kCurseFragile;
				}
				if (idIsOneOf(id, { "rw_death", "rw_famine" })) {
					return SyntheticRunewordStyle::kCurseSlowAttack;
				}
				if (idIsOneOf(id, { "rw_leaf", "rw_destruction" })) {
					return SyntheticRunewordStyle::kFireStrike;
				}
				if (idIsOneOf(id, { "rw_crescent_moon" })) {
					return SyntheticRunewordStyle::kShockStrike;
				}
				if (idIsOneOf(id, { "rw_beast", "rw_hustle_w", "rw_harmony", "rw_fury", "rw_unbending_will", "rw_passion" })) {
					return SyntheticRunewordStyle::kSelfHaste;
				}
				if (idIsOneOf(id, { "rw_stealth", "rw_smoke", "rw_treachery" })) {
					return SyntheticRunewordStyle::kSelfMuffle;
				}
				if (idIsOneOf(id, { "rw_gloom" })) {
					return SyntheticRunewordStyle::kSelfInvisibility;
				}
				if (idIsOneOf(id, { "rw_wealth", "rw_obedience", "rw_honor", "rw_eternity" })) {
					return SyntheticRunewordStyle::kSoulTrap;
				}
				if (idIsOneOf(id, { "rw_memory", "rw_wisdom", "rw_lore", "rw_melody", "rw_enlightenment" })) {
					return SyntheticRunewordStyle::kSelfMeditation;
				}
				if (idIsOneOf(id, { "rw_pattern", "rw_strength", "rw_kings_grace", "rw_edge", "rw_oath" })) {
					return SyntheticRunewordStyle::kAdaptiveStrike;
				}
				if (idIsOneOf(id, { "rw_silence", "rw_brand" })) {
					return SyntheticRunewordStyle::kAdaptiveExposure;
				}
				if (idIsOneOf(id, { "rw_hustle_a", "rw_splendor", "rw_rhyme" })) {
					return SyntheticRunewordStyle::kSelfPhase;
				}
				if (idIsOneOf(id, { "rw_rain", "rw_ground" })) {
					return SyntheticRunewordStyle::kSelfPhoenix;
				}

				if (a_recipe.recommendedBaseType) {
					if (*a_recipe.recommendedBaseType == LootItemType::kWeapon) {
						switch (recipeBucket(id, 6u)) {
						case 0u:
							return SyntheticRunewordStyle::kAdaptiveStrike;
						case 1u:
							return SyntheticRunewordStyle::kAdaptiveExposure;
						case 2u:
							return SyntheticRunewordStyle::kFireStrike;
						case 3u:
							return SyntheticRunewordStyle::kFrostStrike;
						case 4u:
							return SyntheticRunewordStyle::kShockStrike;
						default:
							return SyntheticRunewordStyle::kSelfHaste;
						}
					}
					if (*a_recipe.recommendedBaseType == LootItemType::kArmor) {
						switch (recipeBucket(id, 6u)) {
						case 0u:
							return SyntheticRunewordStyle::kSelfWard;
						case 1u:
							return SyntheticRunewordStyle::kSelfBarrier;
						case 2u:
							return SyntheticRunewordStyle::kSelfMeditation;
						case 3u:
							return SyntheticRunewordStyle::kSelfPhase;
						case 4u:
							return SyntheticRunewordStyle::kSelfMuffle;
						default:
							return SyntheticRunewordStyle::kSelfPhoenix;
						}
					}
				}

				return recipeBucket(id, 2u) == 0u ? SyntheticRunewordStyle::kAdaptiveExposure : SyntheticRunewordStyle::kAdaptiveStrike;
			};

			auto toDurationMs = [](float a_seconds) -> std::chrono::milliseconds {
				const float clamped = std::max(0.0f, a_seconds);
				return std::chrono::milliseconds(static_cast<std::int64_t>(clamped * 1000.0f));
			};

			auto applyLegacyRunewordFallback = [&](AffixRuntime& a_out, const RunewordRecipe& a_recipe, std::uint32_t a_runeCount) -> bool {
				if (a_recipe.recommendedBaseType && *a_recipe.recommendedBaseType == LootItemType::kArmor) {
					RE::SpellItem* spell = nullptr;
					switch (a_runeCount % 4u) {
					case 0u:
						spell = spellWard;
						break;
					case 1u:
						spell = spellPhase;
						break;
					case 2u:
						spell = spellMeditation;
						break;
					default:
						spell = spellBarrier;
						break;
					}
					if (!spell) {
						spell = spellWard ? spellWard : (spellPhase ? spellPhase : (spellMeditation ? spellMeditation : spellBarrier));
					}
					if (!spell) {
						return false;
					}
					a_out.trigger = Trigger::kIncomingHit;
					a_out.procChancePct = std::clamp(14.0f + static_cast<float>(a_runeCount) * 1.9f, 14.0f, 28.0f);
					const float icdSeconds = std::clamp(20.0f - static_cast<float>(a_runeCount) * 1.2f, 8.0f, 20.0f);
					a_out.icd = toDurationMs(icdSeconds);
					a_out.action.type = ActionType::kCastSpell;
					a_out.action.spell = spell;
					a_out.action.applyToSelf = true;
					a_out.action.noHitEffectArt = true;
					return true;
				}
				if (a_recipe.recommendedBaseType && *a_recipe.recommendedBaseType == LootItemType::kWeapon) {
					if (!dynamicFire && !dynamicFrost && !dynamicShock) {
						return false;
					}
					a_out.trigger = Trigger::kHit;
					a_out.procChancePct = std::clamp(20.0f + static_cast<float>(a_runeCount) * 2.4f, 20.0f, 36.0f);
					const float icdSeconds = std::clamp(1.50f - static_cast<float>(a_runeCount) * 0.16f, 0.35f, 1.25f);
					a_out.icd = toDurationMs(icdSeconds);
					a_out.action.type = ActionType::kCastSpellAdaptiveElement;
					a_out.action.adaptiveMode = AdaptiveElementMode::kWeakestResist;
					a_out.action.adaptiveFire = dynamicFire;
					a_out.action.adaptiveFrost = dynamicFrost;
					a_out.action.adaptiveShock = dynamicShock;
					a_out.action.noHitEffectArt = false;
					a_out.action.magnitudeScaling.source = MagnitudeScaling::Source::kHitPhysicalDealt;
					a_out.action.magnitudeScaling.mult =
						std::clamp(0.08f + static_cast<float>(a_runeCount) * 0.012f, 0.08f, 0.18f);
					a_out.action.magnitudeScaling.spellBaseAsMin = true;
					return true;
				}
				if (!shredFire && !shredFrost && !shredShock) {
					return false;
				}
				a_out.trigger = Trigger::kHit;
				a_out.procChancePct = std::clamp(18.0f + static_cast<float>(a_runeCount) * 2.0f, 18.0f, 34.0f);
				const float icdSeconds = std::clamp(3.2f - static_cast<float>(a_runeCount) * 0.45f, 0.9f, 3.2f);
				a_out.icd = toDurationMs(icdSeconds);
				a_out.action.type = ActionType::kCastSpellAdaptiveElement;
				a_out.action.adaptiveMode = AdaptiveElementMode::kWeakestResist;
				a_out.action.adaptiveFire = shredFire;
				a_out.action.adaptiveFrost = shredFrost;
				a_out.action.adaptiveShock = shredShock;
				a_out.action.noHitEffectArt = false;
				return true;
			};

		const AffixRuntime* runewordTemplate = nullptr;
		for (const auto& affix : _affixes) {
			if (affix.id.rfind("runeword_", 0) == 0 && affix.keyword) {
				runewordTemplate = std::addressof(affix);
				break;
			}
		}

		std::uint32_t synthesizedRunewordAffixes = 0u;
		for (const auto& recipe : _runewordRecipes) {
			if (recipe.resultAffixToken == 0u || _affixIndexByToken.contains(recipe.resultAffixToken)) {
				continue;
			}

			const std::uint32_t runeCount =
				static_cast<std::uint32_t>(std::max<std::size_t>(recipe.runeIds.size(), 1u));

			std::string runeSequence;
			for (std::size_t i = 0; i < recipe.runeIds.size(); ++i) {
				if (i > 0) {
					runeSequence.push_back('-');
				}
				runeSequence.append(recipe.runeIds[i]);
			}

			AffixRuntime out{};
			out.id = recipe.id + "_auto";
			out.token = recipe.resultAffixToken;
			out.action.sourceToken = out.token;
			out.displayName = runeSequence.empty() ?
				("Runeword " + recipe.displayName + " (Auto)") :
				("Runeword " + recipe.displayName + " [" + runeSequence + "]");
			out.label = recipe.displayName;
			if (runewordTemplate) {
				out.keywordEditorId = runewordTemplate->keywordEditorId;
				out.keyword = runewordTemplate->keyword;
			}

				auto style = resolveRunewordStyle(recipe);
				const float runeScale = static_cast<float>(runeCount);
				const bool armorRecommended =
					(recipe.recommendedBaseType && *recipe.recommendedBaseType == LootItemType::kArmor);
				const Trigger summonTrigger = armorRecommended ? Trigger::kIncomingHit : Trigger::kHit;
				const Trigger offensiveTrigger = armorRecommended ? Trigger::kIncomingHit : Trigger::kHit;
				const Trigger auraTrigger = armorRecommended ? Trigger::kIncomingHit : Trigger::kHit;

				auto configureAdaptiveSpell = [&](AdaptiveElementMode a_mode,
											 RE::SpellItem* a_fire,
											 RE::SpellItem* a_frost,
											 RE::SpellItem* a_shock,
											 float a_procBase,
											 float a_procPerRune,
											 float a_procMin,
											 float a_procMax,
											 float a_icdBase,
											 float a_icdPerRune,
											 float a_icdMin,
											 float a_icdMax,
											 float a_scaleBase,
											 float a_scalePerRune,
											 float a_perTargetIcdSeconds) -> bool {
					if (!a_fire && !a_frost && !a_shock) {
						return false;
					}
					out.trigger = Trigger::kHit;
					out.procChancePct = std::clamp(a_procBase + runeScale * a_procPerRune, a_procMin, a_procMax);
					out.icd = toDurationMs(std::clamp(a_icdBase - runeScale * a_icdPerRune, a_icdMin, a_icdMax));
					if (a_perTargetIcdSeconds > 0.0f) {
						out.perTargetIcd = toDurationMs(a_perTargetIcdSeconds);
					}
					out.action.type = ActionType::kCastSpellAdaptiveElement;
					out.action.adaptiveMode = a_mode;
					out.action.adaptiveFire = a_fire;
					out.action.adaptiveFrost = a_frost;
					out.action.adaptiveShock = a_shock;
					out.action.noHitEffectArt = false;
					if (a_scaleBase > 0.0f || a_scalePerRune > 0.0f) {
						out.action.magnitudeScaling.source = MagnitudeScaling::Source::kHitPhysicalDealt;
						out.action.magnitudeScaling.mult = std::clamp(a_scaleBase + runeScale * a_scalePerRune, 0.0f, 0.25f);
						out.action.magnitudeScaling.spellBaseAsMin = true;
					}
					return true;
				};

				auto configureSingleTargetSpell = [&](RE::SpellItem* a_spell,
												 float a_procBase,
												 float a_procPerRune,
												 float a_procMin,
												 float a_procMax,
												 float a_icdBase,
												 float a_icdPerRune,
												 float a_icdMin,
												 float a_icdMax,
												 float a_scaleBase,
												 float a_scalePerRune,
												 float a_perTargetIcdSeconds,
												 Trigger a_trigger = Trigger::kHit) -> bool {
					if (!a_spell) {
						return false;
					}
					out.trigger = a_trigger;
					out.procChancePct = std::clamp(a_procBase + runeScale * a_procPerRune, a_procMin, a_procMax);
					out.icd = toDurationMs(std::clamp(a_icdBase - runeScale * a_icdPerRune, a_icdMin, a_icdMax));
					if (a_perTargetIcdSeconds > 0.0f) {
						out.perTargetIcd = toDurationMs(a_perTargetIcdSeconds);
					}
					out.action.type = ActionType::kCastSpell;
					out.action.spell = a_spell;
					out.action.applyToSelf = false;
					out.action.noHitEffectArt = true;
					if (a_scaleBase > 0.0f || a_scalePerRune > 0.0f) {
						out.action.magnitudeScaling.source = MagnitudeScaling::Source::kHitPhysicalDealt;
						out.action.magnitudeScaling.mult = std::clamp(a_scaleBase + runeScale * a_scalePerRune, 0.0f, 0.25f);
						out.action.magnitudeScaling.spellBaseAsMin = true;
					}
					return true;
				};

				auto configureSelfBuff = [&](RE::SpellItem* a_spell,
										 Trigger a_trigger,
										 float a_procBase,
										 float a_procPerRune,
										 float a_procMin,
										 float a_procMax,
										 float a_icdBase,
										 float a_icdPerRune,
										 float a_icdMin,
										 float a_icdMax) -> bool {
					if (!a_spell) {
						return false;
					}
					out.trigger = a_trigger;
					out.procChancePct = std::clamp(a_procBase + runeScale * a_procPerRune, a_procMin, a_procMax);
					out.icd = toDurationMs(std::clamp(a_icdBase - runeScale * a_icdPerRune, a_icdMin, a_icdMax));
					out.action.type = ActionType::kCastSpell;
					out.action.spell = a_spell;
					out.action.applyToSelf = true;
					out.action.noHitEffectArt = true;
					return true;
				};

				auto configureSummonSpell = [&](RE::SpellItem* a_spell,
										   float a_procBase,
										   float a_procPerRune,
										   float a_procMin,
										   float a_procMax,
										   float a_icdBase,
										   float a_icdPerRune,
										   float a_icdMin,
										   float a_icdMax) -> bool {
					if (!a_spell) {
						return false;
					}
					out.trigger = summonTrigger;
					out.procChancePct = std::clamp(a_procBase + runeScale * a_procPerRune, a_procMin, a_procMax);
					out.icd = toDurationMs(std::clamp(a_icdBase - runeScale * a_icdPerRune, a_icdMin, a_icdMax));
					out.action.type = ActionType::kCastSpell;
					out.action.spell = a_spell;
					out.action.applyToSelf = true;
					out.action.noHitEffectArt = true;
					return true;
				};

				bool ready = false;
				switch (style) {
				case SyntheticRunewordStyle::kAdaptiveStrike:
					ready = configureAdaptiveSpell(
						AdaptiveElementMode::kWeakestResist,
						dynamicFire,
						dynamicFrost,
						dynamicShock,
						20.0f,
						2.2f,
						20.0f,
						36.0f,
						1.45f,
						0.16f,
						0.35f,
						1.35f,
						0.08f,
						0.012f,
						0.0f);
					break;
				case SyntheticRunewordStyle::kAdaptiveExposure:
					ready = configureAdaptiveSpell(
						AdaptiveElementMode::kStrongestResist,
						shredFire,
						shredFrost,
						shredShock,
						16.0f,
						1.6f,
						16.0f,
						30.0f,
						3.2f,
						0.35f,
						0.95f,
						3.2f,
						0.0f,
						0.0f,
						8.0f);
					break;
				case SyntheticRunewordStyle::kFireStrike:
					ready = configureSingleTargetSpell(
						dynamicFire,
						18.0f,
						1.8f,
						18.0f,
						34.0f,
						1.2f,
						0.12f,
						0.35f,
						1.2f,
						0.06f,
						0.010f,
						0.0f);
					break;
				case SyntheticRunewordStyle::kFrostStrike:
					ready = configureSingleTargetSpell(
						dynamicFrost,
						18.0f,
						1.8f,
						18.0f,
						34.0f,
						1.25f,
						0.13f,
						0.40f,
						1.25f,
						0.06f,
						0.010f,
						0.0f);
					break;
				case SyntheticRunewordStyle::kShockStrike:
					ready = configureSingleTargetSpell(
						spellArcLightning ? spellArcLightning : dynamicShock,
						19.0f,
						1.9f,
						19.0f,
						35.0f,
						1.05f,
						0.11f,
						0.30f,
						1.05f,
						0.07f,
						0.011f,
						0.0f);
					break;
				case SyntheticRunewordStyle::kPoisonBloom:
					ready = configureSingleTargetSpell(
						spellDotPoison,
						12.0f,
						1.6f,
						12.0f,
						26.0f,
						2.6f,
						0.26f,
						0.9f,
						2.6f,
						0.0f,
						0.0f,
						4.0f);
					break;
				case SyntheticRunewordStyle::kTarBloom:
					ready = configureSingleTargetSpell(
						spellDotTar,
						12.0f,
						1.5f,
						12.0f,
						24.0f,
						2.8f,
						0.30f,
						0.9f,
						2.8f,
						0.0f,
						0.0f,
						4.0f);
					break;
				case SyntheticRunewordStyle::kSiphonBloom:
					ready = configureSingleTargetSpell(
						spellDotSiphon,
						11.0f,
						1.5f,
						11.0f,
						24.0f,
						2.9f,
						0.30f,
						1.0f,
						2.9f,
						0.0f,
						0.0f,
						5.0f);
					break;
				case SyntheticRunewordStyle::kCurseFragile:
					ready = configureSingleTargetSpell(
						spellChaosFragile ? spellChaosFragile : spellChaosSunder,
						15.0f,
						1.6f,
						15.0f,
						29.0f,
						4.4f,
						0.45f,
						1.6f,
						4.4f,
						0.0f,
						0.0f,
						8.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kCurseSlowAttack:
					ready = configureSingleTargetSpell(
						spellChaosSlowAttack ? spellChaosSlowAttack : spellChaosSunder,
						16.0f,
						1.6f,
						16.0f,
						30.0f,
						4.0f,
						0.40f,
						1.4f,
						4.0f,
						0.0f,
						0.0f,
						7.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kCurseFear:
					ready = configureSingleTargetSpell(
						spellVanillaFear ? spellVanillaFear : spellChaosSlowAttack,
						13.0f,
						1.2f,
						13.0f,
						23.0f,
						6.0f,
						0.6f,
						2.4f,
						6.0f,
						0.0f,
						0.0f,
						8.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kCurseFrenzy:
					ready = configureSingleTargetSpell(
						spellVanillaFrenzy ? spellVanillaFrenzy : spellChaosFragile,
						11.0f,
						1.1f,
						11.0f,
						21.0f,
						6.8f,
						0.7f,
						2.6f,
						6.8f,
						0.0f,
						0.0f,
						9.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kSelfHaste:
					ready = configureSelfBuff(spellHaste, Trigger::kHit, 18.0f, 1.5f, 18.0f, 32.0f, 7.5f, 0.75f, 2.4f, 7.5f);
					break;
				case SyntheticRunewordStyle::kSelfWard:
					ready = configureSelfBuff(spellWard, Trigger::kIncomingHit, 15.0f, 1.7f, 15.0f, 28.0f, 19.0f, 1.1f, 8.0f, 19.0f);
					break;
				case SyntheticRunewordStyle::kSelfBarrier:
					ready = configureSelfBuff(spellBarrier, Trigger::kIncomingHit, 15.0f, 1.7f, 15.0f, 28.0f, 18.0f, 1.0f, 7.5f, 18.0f);
					break;
				case SyntheticRunewordStyle::kSelfMeditation:
					ready = configureSelfBuff(spellMeditation, Trigger::kIncomingHit, 16.0f, 1.5f, 16.0f, 27.0f, 17.0f, 0.9f, 7.0f, 17.0f);
					break;
				case SyntheticRunewordStyle::kSelfPhase:
					ready = configureSelfBuff(spellPhase, Trigger::kIncomingHit, 14.0f, 1.4f, 14.0f, 24.0f, 16.0f, 0.8f, 7.0f, 16.0f);
					break;
				case SyntheticRunewordStyle::kSelfPhoenix:
					ready = configureSelfBuff(spellPhoenix ? spellPhoenix : spellPhase, Trigger::kIncomingHit, 14.0f, 1.4f, 14.0f, 25.0f, 16.0f, 0.8f, 6.5f, 16.0f);
					break;
				case SyntheticRunewordStyle::kSelfFlameCloak:
					ready = configureSelfBuff(
						spellVanillaFlameCloak ? spellVanillaFlameCloak : (spellPhoenix ? spellPhoenix : spellPhase),
						auraTrigger,
						12.0f,
						1.3f,
						12.0f,
						23.0f,
						14.0f,
						0.7f,
						4.5f,
						14.0f);
					break;
				case SyntheticRunewordStyle::kSelfFrostCloak:
					ready = configureSelfBuff(
						spellVanillaFrostCloak ? spellVanillaFrostCloak : (spellWard ? spellWard : spellPhase),
						auraTrigger,
						12.0f,
						1.3f,
						12.0f,
						23.0f,
						14.0f,
						0.7f,
						4.5f,
						14.0f);
					break;
				case SyntheticRunewordStyle::kSelfShockCloak:
					ready = configureSelfBuff(
						spellVanillaShockCloak ? spellVanillaShockCloak : (spellBarrier ? spellBarrier : spellPhase),
						auraTrigger,
						12.0f,
						1.3f,
						12.0f,
						23.0f,
						14.0f,
						0.7f,
						4.5f,
						14.0f);
					break;
				case SyntheticRunewordStyle::kSelfOakflesh:
					ready = configureSelfBuff(
						spellVanillaOakflesh ? spellVanillaOakflesh : (spellWard ? spellWard : spellPhase),
						auraTrigger,
						14.0f,
						1.4f,
						14.0f,
						25.0f,
						16.0f,
						0.9f,
						6.0f,
						16.0f);
					break;
				case SyntheticRunewordStyle::kSelfStoneflesh:
					ready = configureSelfBuff(
						spellVanillaStoneflesh ? spellVanillaStoneflesh : (spellBarrier ? spellBarrier : spellPhase),
						auraTrigger,
						14.0f,
						1.4f,
						14.0f,
						25.0f,
						16.0f,
						0.9f,
						6.0f,
						16.0f);
					break;
				case SyntheticRunewordStyle::kSelfIronflesh:
					ready = configureSelfBuff(
						spellVanillaIronflesh ? spellVanillaIronflesh : (spellBarrier ? spellBarrier : spellPhase),
						auraTrigger,
						13.0f,
						1.4f,
						13.0f,
						24.0f,
						17.0f,
						0.9f,
						6.0f,
						17.0f);
					break;
				case SyntheticRunewordStyle::kSelfEbonyflesh:
					ready = configureSelfBuff(
						spellVanillaEbonyflesh ? spellVanillaEbonyflesh : (spellWard ? spellWard : spellPhase),
						auraTrigger,
						12.0f,
						1.2f,
						12.0f,
						22.0f,
						18.0f,
						1.0f,
						7.0f,
						18.0f);
					break;
				case SyntheticRunewordStyle::kSelfMuffle:
					ready = configureSelfBuff(
						spellVanillaMuffle ? spellVanillaMuffle : spellPhase,
						Trigger::kIncomingHit,
						13.0f,
						1.4f,
						13.0f,
						24.0f,
						20.0f,
						1.1f,
						8.0f,
						20.0f);
					break;
				case SyntheticRunewordStyle::kSelfInvisibility:
					ready = configureSelfBuff(
						spellCustomInvisibility ? spellCustomInvisibility : spellPhase,
						Trigger::kIncomingHit,
						8.0f,
						0.9f,
						8.0f,
						15.0f,
						28.0f,
						1.6f,
						14.0f,
						28.0f);
					break;
				case SyntheticRunewordStyle::kSoulTrap:
					ready = configureSingleTargetSpell(
						spellVanillaSoulTrap ? spellVanillaSoulTrap : spellChaosFragile,
						16.0f,
						1.4f,
						16.0f,
						27.0f,
						7.0f,
						0.7f,
						2.5f,
						7.0f,
						0.0f,
						0.0f,
						6.0f,
						offensiveTrigger);
					break;
				case SyntheticRunewordStyle::kSummonFamiliar:
					ready = configureSummonSpell(
						spellSummonFamiliar,
						8.0f,
						0.8f,
						8.0f,
						14.0f,
						28.0f,
						1.6f,
						18.0f,
						30.0f);
					break;
				case SyntheticRunewordStyle::kSummonFlameAtronach:
					ready = configureSummonSpell(
						spellSummonFlameAtronach,
						7.0f,
						0.7f,
						7.0f,
						13.0f,
						30.0f,
						1.5f,
						20.0f,
						32.0f);
					break;
				case SyntheticRunewordStyle::kSummonFrostAtronach:
					ready = configureSummonSpell(
						spellSummonFrostAtronach,
						6.5f,
						0.7f,
						6.5f,
						12.0f,
						32.0f,
						1.5f,
						21.0f,
						34.0f);
					break;
				case SyntheticRunewordStyle::kSummonStormAtronach:
					ready = configureSummonSpell(
						spellSummonStormAtronach,
						6.0f,
						0.6f,
						6.0f,
						11.0f,
						34.0f,
						1.6f,
						22.0f,
						36.0f);
					break;
				case SyntheticRunewordStyle::kSummonDremoraLord:
					ready = configureSummonSpell(
						spellSummonDremoraLord,
						4.0f,
						0.5f,
						4.0f,
						8.0f,
						38.0f,
						2.0f,
						26.0f,
						40.0f);
					break;
				case SyntheticRunewordStyle::kLegacyFallback:
				default:
					ready = false;
					break;
				}

				if (!ready) {
					if (applyLegacyRunewordFallback(out, recipe, runeCount)) {
						ready = true;
						style = SyntheticRunewordStyle::kLegacyFallback;
					}
				}

				auto applyRunewordIndividualTuning = [&](std::string_view a_recipeId, SyntheticRunewordStyle a_style) {
					auto setProcIcd = [&](float a_procPct, float a_icdSec) {
						out.procChancePct = a_procPct;
						out.icd = toDurationMs(a_icdSec);
					};
					auto setPerTargetIcd = [&](float a_seconds) {
						out.perTargetIcd = toDurationMs(a_seconds);
					};
					auto setMagnitudeMult = [&](float a_mult) {
						if (out.action.magnitudeScaling.source != MagnitudeScaling::Source::kNone) {
							out.action.magnitudeScaling.mult = a_mult;
						}
					};

					if (a_recipeId == "rw_lawbringer") {
						setProcIcd(13.0f, 3.8f);
						setPerTargetIcd(10.0f);
					} else if (a_recipeId == "rw_wrath") {
						setProcIcd(16.0f, 3.2f);
						setPerTargetIcd(8.0f);
					} else if (a_recipeId == "rw_kingslayer") {
						setProcIcd(18.0f, 2.6f);
						setPerTargetIcd(7.0f);
					} else if (a_recipeId == "rw_principle") {
						setProcIcd(14.0f, 4.2f);
						setPerTargetIcd(9.0f);
					} else if (a_recipeId == "rw_black") {
						setProcIcd(14.0f, 2.4f);
						setPerTargetIcd(3.5f);
					} else if (a_recipeId == "rw_rift") {
						setProcIcd(15.0f, 2.0f);
						setPerTargetIcd(3.0f);
					} else if (a_recipeId == "rw_malice") {
						setProcIcd(14.0f, 2.3f);
						setPerTargetIcd(4.0f);
					} else if (a_recipeId == "rw_plague") {
						setProcIcd(14.0f, 2.2f);
						setPerTargetIcd(4.0f);
					} else if (a_recipeId == "rw_wealth") {
						setProcIcd(24.0f, 4.8f);
						setPerTargetIcd(5.0f);
					} else if (a_recipeId == "rw_obedience") {
						setProcIcd(22.0f, 5.2f);
						setPerTargetIcd(5.0f);
					} else if (a_recipeId == "rw_honor") {
						setProcIcd(20.0f, 5.6f);
						setPerTargetIcd(6.0f);
					} else if (a_recipeId == "rw_eternity") {
						setProcIcd(24.0f, 4.4f);
						setPerTargetIcd(4.5f);
					} else if (a_recipeId == "rw_stealth") {
						setProcIcd(15.0f, 16.0f);
					} else if (a_recipeId == "rw_smoke") {
						setProcIcd(15.0f, 15.0f);
					} else if (a_recipeId == "rw_treachery") {
						setProcIcd(18.0f, 12.0f);
					} else if (a_recipeId == "rw_gloom") {
						setProcIcd(7.0f, 24.0f);
					} else if (a_recipeId == "rw_delirium") {
						setProcIcd(11.0f, 8.5f);
						setPerTargetIcd(11.0f);
					} else if (a_recipeId == "rw_nadir") {
						setProcIcd(14.0f, 6.5f);
						setPerTargetIcd(10.0f);
					} else if (a_recipeId == "rw_beast") {
						setProcIcd(26.0f, 3.0f);
					} else if (a_recipeId == "rw_dragon") {
						setProcIcd(16.0f, 10.0f);
					} else if (a_recipeId == "rw_hand_of_justice") {
						setProcIcd(18.0f, 8.5f);
					} else if (a_recipeId == "rw_flickering_flame") {
						setProcIcd(14.0f, 11.0f);
					} else if (a_recipeId == "rw_temper") {
						setProcIcd(17.0f, 8.8f);
					} else if (a_recipeId == "rw_voice_of_reason") {
						setProcIcd(16.0f, 9.5f);
					} else if (a_recipeId == "rw_ice") {
						setProcIcd(14.0f, 11.0f);
					} else if (a_recipeId == "rw_pride") {
						setProcIcd(14.0f, 12.5f);
					} else if (a_recipeId == "rw_metamorphosis") {
						setProcIcd(13.0f, 16.0f);
					} else if (a_recipeId == "rw_destruction") {
						setProcIcd(22.0f, 1.1f);
						setMagnitudeMult(0.17f);
					} else if (a_recipeId == "rw_hustle_w") {
						setProcIcd(24.0f, 2.8f);
					} else if (a_recipeId == "rw_harmony") {
						setProcIcd(21.0f, 3.8f);
					} else if (a_recipeId == "rw_unbending_will") {
						setProcIcd(25.0f, 2.6f);
					} else if (a_recipeId == "rw_stone") {
						setProcIcd(18.0f, 13.0f);
					} else if (a_recipeId == "rw_sanctuary") {
						setProcIcd(17.0f, 13.0f);
					} else if (a_recipeId == "rw_memory") {
						setProcIcd(18.0f, 12.0f);
					} else if (a_recipeId == "rw_wisdom") {
						setProcIcd(17.0f, 13.0f);
					} else if (a_recipeId == "rw_holy_thunder") {
						setProcIcd(17.0f, 9.0f);
					} else if (a_recipeId == "rw_mist") {
						setProcIcd(15.0f, 11.5f);
					}

					if (a_style == SyntheticRunewordStyle::kSummonDremoraLord) {
						setProcIcd(5.0f, 34.0f);
					} else if (a_style == SyntheticRunewordStyle::kSummonStormAtronach) {
						setProcIcd(7.0f, 26.0f);
					} else if (a_style == SyntheticRunewordStyle::kSummonFlameAtronach) {
						setProcIcd(8.0f, 24.0f);
					} else if (a_style == SyntheticRunewordStyle::kSummonFrostAtronach) {
						setProcIcd(8.0f, 24.0f);
					} else if (a_style == SyntheticRunewordStyle::kSummonFamiliar) {
						setProcIcd(10.0f, 22.0f);
					}
				};

				if (ready) {
					applyRunewordIndividualTuning(recipe.id, style);
				}

			if (!ready) {
				SKSE::log::warn(
					"CalamityAffixes: runeword result affix unresolved (recipe={}, resultToken={:016X}).",
					recipe.id,
					recipe.resultAffixToken);
				continue;
			}

				registerRuntimeAffix(std::move(out), false);
				synthesizedRunewordAffixes += 1u;
				SKSE::log::warn(
					"CalamityAffixes: synthesized runeword runtime affix (recipe={}, style={}, resultToken={:016X}).",
					recipe.id,
					styleName(style),
					recipe.resultAffixToken);
		}

		if (synthesizedRunewordAffixes > 0u) {
			SKSE::log::info(
				"CalamityAffixes: synthesized {} runeword runtime affix definitions.",
				synthesizedRunewordAffixes);
		}

		_activeCounts.assign(_affixes.size(), 0);

		SanitizeRunewordState();
		_configLoaded = true;

		RebuildActiveCounts();

			SKSE::log::info(
				"CalamityAffixes: runtime config loaded (affixes={}, prefixWeapon={}, prefixArmor={}, suffixWeapon={}, suffixArmor={}, lootChance={}%).",
				_affixes.size(),
				_lootWeaponAffixes.size(), _lootArmorAffixes.size(),
				_lootWeaponSuffixes.size(), _lootArmorSuffixes.size(),
				_loot.chancePercent);

		if (_affixes.empty()) {
			SKSE::log::error("CalamityAffixes: no affixes loaded. Is the generated CalamityAffixes plugin enabled?");
		}
	}
}
