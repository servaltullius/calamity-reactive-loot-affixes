#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootEligibility.h"
#include "CalamityAffixes/RunewordContractSnapshot.h"
#include "CalamityAffixes/RuntimeContract.h"
#include "CalamityAffixes/RuntimePaths.h"
#include "CalamityAffixes/UserSettingsPersistence.h"
#include "EventBridge.Config.Shared.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		const nlohmann::json kEmptyAffixArray = nlohmann::json::array();
		using ConfigShared::LookupSpellFromSpec;
		using ConfigShared::Trim;

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

		[[nodiscard]] std::string ToLowerAscii(std::string_view a_text)
		{
			std::string out;
			out.reserve(a_text.size());
			for (char c : a_text) {
				out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
			}
			return out;
		}

		[[nodiscard]] std::optional<bool> ParseLootNameMarkerPositionIsTrailing(std::string_view a_text)
		{
			const auto normalized = ToLowerAscii(Trim(a_text));
			if (normalized == "leading") {
				return false;
			}
			if (normalized == "trailing") {
				return true;
			}
			return std::nullopt;
		}

		[[nodiscard]] std::optional<bool> ParseRuntimeCurrencyDropEnabled(std::string_view a_text)
		{
			const auto normalized = ToLowerAscii(Trim(a_text));
			if (normalized.empty() || normalized == "runtime" || normalized == "hybrid") {
				return true;
			}
			if (normalized == "leveledlist" || normalized == "leveled_list" || normalized == "leveled-list") {
				return false;
			}
			return std::nullopt;
		}

		void ValidateRuntimeContractSnapshot()
		{
			const auto contractPath = RuntimePaths::ResolveRuntimeRelativePath(RuntimeContract::kRuntimeContractRelativePath);

			std::ifstream in(contractPath, std::ios::binary);
			if (!in.is_open()) {
				SKSE::log::warn(
					"CalamityAffixes: runtime contract snapshot missing at {}. Re-run generator to refresh contract artifacts.",
					contractPath.string());
				return;
			}

			nlohmann::json contract = nlohmann::json::object();
			try {
				in >> contract;
			} catch (const std::exception& e) {
				SKSE::log::warn(
					"CalamityAffixes: failed to parse runtime contract snapshot {} ({}).",
					contractPath.string(),
					e.what());
				return;
			}

			auto validateArray = [&](std::string_view a_key, const auto& a_expected, const auto& a_isSupported) {
				const auto it = contract.find(std::string(a_key));
				if (it == contract.end() || !it->is_array()) {
					SKSE::log::warn(
						"CalamityAffixes: runtime contract snapshot field '{}' is missing/invalid in {}.",
						a_key,
						contractPath.string());
					return false;
				}

				std::unordered_set<std::string> actual{};
				for (const auto& entry : *it) {
					if (!entry.is_string()) {
						SKSE::log::warn(
							"CalamityAffixes: runtime contract snapshot field '{}' contains non-string entries in {}.",
							a_key,
							contractPath.string());
						return false;
					}
					const auto value = entry.get<std::string>();
					if (!a_isSupported(value)) {
						SKSE::log::warn(
							"CalamityAffixes: runtime contract snapshot field '{}' contains unsupported value '{}' in {}.",
							a_key,
							value,
							contractPath.string());
						return false;
					}
					actual.insert(value);
				}

				for (const auto expected : a_expected) {
					if (actual.find(std::string(expected)) == actual.end()) {
						SKSE::log::warn(
							"CalamityAffixes: runtime contract snapshot field '{}' is missing expected value '{}' in {}.",
							a_key,
							expected,
							contractPath.string());
						return false;
					}
				}
				return true;
			};

			const bool triggersOk = validateArray(
				RuntimeContract::kFieldSupportedTriggers,
				RuntimeContract::kSupportedTriggers,
				[](std::string_view value) { return RuntimeContract::IsSupportedTrigger(value); });
			const bool actionsOk = validateArray(
				RuntimeContract::kFieldSupportedActionTypes,
				RuntimeContract::kSupportedActionTypes,
				[](std::string_view value) { return RuntimeContract::IsSupportedActionType(value); });

			RunewordContractSnapshot runewordContractSnapshot{};
			const bool runewordOk = LoadRunewordContractSnapshotFromRuntime(
				RuntimeContract::kRuntimeContractRelativePath,
				runewordContractSnapshot);

			if (triggersOk && actionsOk && runewordOk) {
				SKSE::log::info(
					"CalamityAffixes: runtime contract snapshot validated at {}.",
					contractPath.string());
			}
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
			_lowHealthTriggerAffixIndices.clear();
			_castOnCritAffixIndices.clear();
		_convertAffixIndices.clear();
		_mindOverMatterAffixIndices.clear();
		_archmageAffixIndices.clear();
		_corpseExplosionAffixIndices.clear();
		_summonCorpseExplosionAffixIndices.clear();
		_lootWeaponAffixes.clear();
		_lootArmorAffixes.clear();
		_lootWeaponSuffixes.clear();
		_lootArmorSuffixes.clear();
		_lootSharedAffixes.clear();
		_lootSharedSuffixes.clear();
		_lootPrefixSharedBag = {};
		_lootPrefixWeaponBag = {};
		_lootPrefixArmorBag = {};
		_lootSuffixSharedBag = {};
		_lootSuffixWeaponBag = {};
		_lootSuffixArmorBag = {};
		_lootPreviewAffixes.clear();
		_lootPreviewRecent.clear();
		_appliedPassiveSpells.clear();
		_equippedInstanceKeysByToken.clear();
		_equippedTokenCacheReady = false;
		_traps.clear();
		_hasActiveTraps.store(false, std::memory_order_relaxed);
			_dotCooldowns.clear();
			_dotCooldownsLastPruneAt = {};
				_dotObservedMagicEffects.clear();
					_dotTagSafetyWarned = false;
					_dotTagSafetySuppressed = false;
					_perTargetCooldownStore.Clear();
					_nonHostileFirstHitGate.Clear();
			_corpseExplosionSeenCorpses.clear();
			_corpseExplosionState = {};
			_summonCorpseExplosionSeenCorpses.clear();
		_summonCorpseExplosionState = {};
		_castOnCritNextAllowed = {};
		_castOnCritCycleCursor = 0;
		_equipResync.nextAtMs = 0;
		_equipResync.intervalMs = static_cast<std::uint64_t>(kEquipResyncInterval.count());
			_loot = {};
			_lootChanceEligibleFailStreak = 0;
			_runewordFragmentFailStreak = 0;
			_reforgeOrbFailStreak = 0;
		_runtimeEnabled = true;
		_runtimeProcChanceMult = 1.0f;
		_allowNonHostilePlayerOwnedOutgoingProcs.store(false, std::memory_order_relaxed);
		_runtimeUserSettingsPersist = {};
		_lootRerollGuard.Reset();
		_playerContainerStash.clear();
		_configLoaded = false;
		_lastHealthDamageSignature = 0;
		_lastHealthDamageSignatureAt = {};
		_triggerProcBudgetWindowStartMs = 0u;
		_triggerProcBudgetConsumed = 0u;
		_lastHitAt = {};
		_lastHit = {};
			_recentOwnerHitAt.clear();
			_recentOwnerKillAt.clear();
			_recentOwnerIncomingHitAt.clear();
			_lowHealthTriggerConsumed.clear();
			_lowHealthLastObservedPct.clear();
		}

		void EventBridge::LoadConfig()
		{
			ResetRuntimeStateForConfigReload();
			InitializeRunewordCatalog();

		nlohmann::json j;
		if (!LoadRuntimeConfigJson(j)) {
			return;
		}
		ValidateRuntimeContractSnapshot();

		ApplyLootConfigFromJson(j);
		ApplyRuntimeUserSettingsOverrides();

		const auto* affixes = ResolveAffixArray(j);
		if (!affixes) {
			SKSE::log::error("CalamityAffixes: invalid runtime config schema (keywords.affixes).");
			return;
		}

		auto* handler = RE::TESDataHandler::GetSingleton();

			auto parseTrigger = [](std::string_view a_trigger) -> std::optional<Trigger> {
				if (a_trigger == RuntimeContract::kTriggerHit) {
					return Trigger::kHit;
				}
			if (a_trigger == RuntimeContract::kTriggerIncomingHit) {
				return Trigger::kIncomingHit;
			}
			if (a_trigger == RuntimeContract::kTriggerDotApply) {
				return Trigger::kDotApply;
			}
				if (a_trigger == RuntimeContract::kTriggerKill) {
					return Trigger::kKill;
				}
				if (a_trigger == RuntimeContract::kTriggerLowHealth) {
					return Trigger::kLowHealth;
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
			return a_type == RuntimeContract::kActionCastOnCrit ||
			       a_type == RuntimeContract::kActionConvertDamage ||
			       a_type == RuntimeContract::kActionMindOverMatter ||
			       a_type == RuntimeContract::kActionArchmage ||
			       a_type == RuntimeContract::kActionCorpseExplosion ||
			       a_type == RuntimeContract::kActionSummonCorpseExplosion;
		};

		for (const auto& a : *affixes) {
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
			const float kidChancePct = kid.is_object() ?
				static_cast<float>(kid.value("chance", 0.0)) :
				0.0f;
			if (kidChancePct > 0.0f) {
				// Loot-time drop weighting is sourced from explicit loot weight or legacy kid.chance.
				// Keep this independent from runtime trigger proc chance.
				out.lootWeight = kidChancePct;
			}
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
				out.procChancePct = (kidChancePct > 0.0f) ? kidChancePct : 1.0f;

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

				const auto& conditions = runtime.value("conditions", nlohmann::json::object());
				auto readRuntimeNumber = [&](std::string_view a_key, double a_fallback = 0.0) -> double {
					const auto key = std::string(a_key);
					if (const auto it = runtime.find(key); it != runtime.end() && it->is_number()) {
						return it->get<double>();
					}
					if (conditions.is_object()) {
						if (const auto it = conditions.find(key); it != conditions.end() && it->is_number()) {
							return it->get<double>();
						}
					}
					return a_fallback;
				};

				const double requireRecentlyHitSeconds = std::clamp(readRuntimeNumber("requireRecentlyHitSeconds"), 0.0, 600.0);
				if (requireRecentlyHitSeconds > 0.0) {
					out.requireRecentlyHit = std::chrono::milliseconds(static_cast<std::int64_t>(requireRecentlyHitSeconds * 1000.0));
				}

				const double requireRecentlyKillSeconds = std::clamp(readRuntimeNumber("requireRecentlyKillSeconds"), 0.0, 600.0);
				if (requireRecentlyKillSeconds > 0.0) {
					out.requireRecentlyKill = std::chrono::milliseconds(static_cast<std::int64_t>(requireRecentlyKillSeconds * 1000.0));
				}

					const double requireNotHitRecentlySeconds = std::clamp(readRuntimeNumber("requireNotHitRecentlySeconds"), 0.0, 600.0);
					if (requireNotHitRecentlySeconds > 0.0) {
						out.requireNotHitRecently = std::chrono::milliseconds(static_cast<std::int64_t>(requireNotHitRecentlySeconds * 1000.0));
					}

					const float lowHealthThresholdPct = static_cast<float>(std::clamp(readRuntimeNumber("lowHealthThresholdPercent", 35.0), 1.0, 95.0));
					const float lowHealthRearmPct = static_cast<float>(std::clamp(
						readRuntimeNumber("lowHealthRearmPercent", static_cast<double>(lowHealthThresholdPct + 10.0f)),
						static_cast<double>(lowHealthThresholdPct + 1.0f),
						100.0));
					out.lowHealthThresholdPct = lowHealthThresholdPct;
					out.lowHealthRearmPct = lowHealthRearmPct;

					out.luckyHitChancePct = static_cast<float>(std::clamp(readRuntimeNumber("luckyHitChancePercent"), 0.0, 100.0));
					out.luckyHitProcCoefficient = static_cast<float>(std::clamp(readRuntimeNumber("luckyHitProcCoefficient", 1.0), 0.0, 5.0));
				}

			if (action.is_object()) {
				const float rawScrollNoConsumeChancePct = static_cast<float>(action.value("scrollNoConsumeChancePct", 0.0));
				out.scrollNoConsumeChancePct = std::clamp(rawScrollNoConsumeChancePct, 0.0f, 100.0f);
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
				} else if (type == "MindOverMatter") {
					out.action.type = ActionType::kMindOverMatter;

					auto readMindOverMatterNumber = [&](std::string_view a_key, double a_fallback = 0.0) -> double {
						const auto key = std::string(a_key);
						const auto it = action.find(key);
						if (it == action.end()) {
							return a_fallback;
						}
						if (it->is_number()) {
							return it->get<double>();
						}

						SKSE::log::warn(
							"CalamityAffixes: MindOverMatter expects numeric '{}' (affixId={}, gotType={}); using fallback={}.",
							key,
							out.id,
							it->type_name(),
							a_fallback);
						return a_fallback;
					};

					out.action.mindOverMatterDamageToMagickaPct =
						static_cast<float>(readMindOverMatterNumber("damageToMagickaPct"));
					out.action.mindOverMatterMaxRedirectPerHit =
						static_cast<float>(readMindOverMatterNumber("maxRedirectPerHit"));

					if (out.action.mindOverMatterDamageToMagickaPct <= 0.0f) {
						continue;
					}
					if (out.trigger != Trigger::kIncomingHit) {
						SKSE::log::warn(
							"CalamityAffixes: MindOverMatter requires trigger=IncomingHit (affixId={}, trigger={}); skipping.",
							out.id,
							static_cast<std::uint32_t>(out.trigger));
						continue;
					}
					out.action.mindOverMatterDamageToMagickaPct = std::clamp(out.action.mindOverMatterDamageToMagickaPct, 0.0f, 100.0f);
					out.action.mindOverMatterMaxRedirectPerHit = std::max(0.0f, out.action.mindOverMatterMaxRedirectPerHit);
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
				out.action.trapMaxTargetsPerTrigger = action.value("maxTargetsPerTrigger", 1u);
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
				if (out.action.trapMaxTargetsPerTrigger == 0u) {
					out.action.trapMaxTargetsPerTrigger = 1u;
				} else if (out.action.trapMaxTargetsPerTrigger > 64u) {
					out.action.trapMaxTargetsPerTrigger = 64u;
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
					out.action.type == ActionType::kMindOverMatter ||
					out.action.type == ActionType::kArchmage ||
					out.action.type == ActionType::kCorpseExplosion ||
					out.action.type == ActionType::kSummonCorpseExplosion;

				if (out.luckyHitChancePct > 0.0f) {
					bool luckyHitSupported = false;
					switch (out.action.type) {
					case ActionType::kCastOnCrit:
					case ActionType::kConvertDamage:
					case ActionType::kMindOverMatter:
					case ActionType::kArchmage:
						luckyHitSupported = true;
						break;
					case ActionType::kCorpseExplosion:
					case ActionType::kSummonCorpseExplosion:
						luckyHitSupported = false;
						break;
					default:
						luckyHitSupported = (out.trigger == Trigger::kHit || out.trigger == Trigger::kIncomingHit);
						break;
					}

					if (!luckyHitSupported) {
						SKSE::log::warn(
							"CalamityAffixes: luckyHitChancePercent is ignored for unsupported affix context (affixId={}, actionType={}, trigger={}).",
							out.id,
							type,
							static_cast<std::uint32_t>(out.trigger));
						out.luckyHitChancePct = 0.0f;
						out.luckyHitProcCoefficient = 1.0f;
					}
				}

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
					case Trigger::kLowHealth:
						_lowHealthTriggerAffixIndices.push_back(idx);
						break;
					}
				}

			if (_affixes[idx].action.type == ActionType::kCastOnCrit) {
				_castOnCritAffixIndices.push_back(idx);
			} else if (_affixes[idx].action.type == ActionType::kConvertDamage) {
				_convertAffixIndices.push_back(idx);
			} else if (_affixes[idx].action.type == ActionType::kMindOverMatter) {
				_mindOverMatterAffixIndices.push_back(idx);
			} else if (_affixes[idx].action.type == ActionType::kArchmage) {
				_archmageAffixIndices.push_back(idx);
			} else if (_affixes[idx].action.type == ActionType::kCorpseExplosion) {
				_corpseExplosionAffixIndices.push_back(idx);
			} else if (_affixes[idx].action.type == ActionType::kSummonCorpseExplosion) {
				_summonCorpseExplosionAffixIndices.push_back(idx);
			}
		}

		IndexConfiguredAffixes();
		SynthesizeRunewordRuntimeAffixes();
		RebuildSharedLootPools();

		_activeCounts.assign(_affixes.size(), 0);

		SanitizeRunewordState();
		SanitizeAllTrackedLootInstancesForCurrentLootRules("LoadConfig.postIndex");
		_configLoaded = true;

		RebuildActiveCounts();

			SKSE::log::info(
				"CalamityAffixes: runtime config loaded (affixes={}, prefixWeapon={}, prefixArmor={}, suffixWeapon={}, suffixArmor={}, lootChance={}%, runeFragChance={}%, reforgeOrbChance={}%, runtimeCurrencyDropsEnabled={}, sourceMult(corpse/container/boss/world)={:.2f}/{:.2f}/{:.2f}/{:.2f}, triggerBudget={}/{}, trapCastBudgetPerTick={}).",
				_affixes.size(),
				_lootWeaponAffixes.size(), _lootArmorAffixes.size(),
				_lootWeaponSuffixes.size(), _lootArmorSuffixes.size(),
				_loot.chancePercent,
				_loot.runewordFragmentChancePercent,
				_loot.reforgeOrbChancePercent,
				_loot.runtimeCurrencyDropsEnabled,
				_loot.lootSourceChanceMultCorpse,
				_loot.lootSourceChanceMultContainer,
				_loot.lootSourceChanceMultBossContainer,
				_loot.lootSourceChanceMultWorld,
				_loot.triggerProcBudgetPerWindow,
				_loot.triggerProcBudgetWindowMs,
				_loot.trapCastBudgetPerTick);

		if (_affixes.empty()) {
			SKSE::log::error("CalamityAffixes: no affixes loaded. Is the generated CalamityAffixes plugin enabled?");
		}
	}

	bool EventBridge::LoadRuntimeConfigJson(nlohmann::json& a_outJson) const
	{
		const auto configPath = RuntimePaths::ResolveRuntimeRelativePath(kRuntimeConfigRelativePath);

		SKSE::log::info("CalamityAffixes: loading runtime config from: {}", configPath.string());

		std::ifstream in(configPath);
		if (!in.is_open()) {
			SKSE::log::warn("CalamityAffixes: runtime config not found: {}", configPath.string());
			return false;
		}

		try {
			in >> a_outJson;
		} catch (const std::exception&) {
			SKSE::log::error("CalamityAffixes: failed to parse runtime config (affixes.json).");
			return false;
		}

		return true;
	}

	void EventBridge::ApplyLootConfigFromJson(const nlohmann::json& a_configRoot)
	{
		_loot.armorEditorIdDenyContains = detail::MakeDefaultLootArmorEditorIdDenyContains();
		_loot.bossContainerEditorIdAllowContains = detail::MakeDefaultBossContainerEditorIdAllowContains();
		_loot.bossContainerEditorIdDenyContains.clear();
		_loot.nameMarkerPosition = LootNameMarkerPosition::kTrailing;
		_loot.runtimeCurrencyDropsEnabled = true;
		const auto& loot = a_configRoot.value("loot", nlohmann::json::object());
		if (loot.is_object()) {
			_loot.chancePercent = loot.value("chancePercent", 0.0f);
			_loot.runewordFragmentChancePercent =
				static_cast<float>(loot.value("runewordFragmentChancePercent", static_cast<double>(_loot.runewordFragmentChancePercent)));
			_loot.reforgeOrbChancePercent =
				static_cast<float>(loot.value("reforgeOrbChancePercent", static_cast<double>(_loot.reforgeOrbChancePercent)));
			if (const auto modeIt = loot.find("currencyDropMode"); modeIt != loot.end()) {
				if (modeIt->is_string()) {
					if (const auto runtimeCurrencyEnabled = ParseRuntimeCurrencyDropEnabled(modeIt->get<std::string>());
						runtimeCurrencyEnabled.has_value()) {
						_loot.runtimeCurrencyDropsEnabled = *runtimeCurrencyEnabled;
					} else {
						SKSE::log::warn(
							"CalamityAffixes: invalid loot.currencyDropMode '{}'; expected runtime/leveledList/hybrid. Falling back to runtime.",
							modeIt->get<std::string>());
						_loot.runtimeCurrencyDropsEnabled = true;
					}
				} else {
					SKSE::log::warn(
						"CalamityAffixes: loot.currencyDropMode must be a string (runtime/leveledList/hybrid). Falling back to runtime.");
					_loot.runtimeCurrencyDropsEnabled = true;
				}
			}
			_loot.lootSourceChanceMultCorpse =
				static_cast<float>(loot.value("lootSourceChanceMultCorpse", static_cast<double>(_loot.lootSourceChanceMultCorpse)));
			_loot.lootSourceChanceMultContainer =
				static_cast<float>(loot.value("lootSourceChanceMultContainer", static_cast<double>(_loot.lootSourceChanceMultContainer)));
			_loot.lootSourceChanceMultBossContainer =
				static_cast<float>(loot.value("lootSourceChanceMultBossContainer", static_cast<double>(_loot.lootSourceChanceMultBossContainer)));
			_loot.lootSourceChanceMultWorld =
				static_cast<float>(loot.value("lootSourceChanceMultWorld", static_cast<double>(_loot.lootSourceChanceMultWorld)));
			_loot.renameItem = loot.value("renameItem", false);
			_loot.sharedPool = loot.value("sharedPool", false);
			_loot.debugLog = loot.value("debugLog", false);
			_loot.dotTagSafetyAutoDisable = loot.value("dotTagSafetyAutoDisable", _loot.dotTagSafetyAutoDisable);
			const double dotTagSafetyThreshold =
				loot.value("dotTagSafetyUniqueEffectThreshold", static_cast<double>(_loot.dotTagSafetyUniqueEffectThreshold));
			const double trapGlobalMaxActive = loot.value("trapGlobalMaxActive", static_cast<double>(_loot.trapGlobalMaxActive));
			const double trapCastBudgetPerTick = loot.value("trapCastBudgetPerTick", static_cast<double>(_loot.trapCastBudgetPerTick));
			const double triggerProcBudgetPerWindow = loot.value("triggerProcBudgetPerWindow", static_cast<double>(_loot.triggerProcBudgetPerWindow));
			const double triggerProcBudgetWindowMs = loot.value("triggerProcBudgetWindowMs", static_cast<double>(_loot.triggerProcBudgetWindowMs));
			_loot.cleanupInvalidLegacyAffixes = loot.value("cleanupInvalidLegacyAffixes", _loot.cleanupInvalidLegacyAffixes);
			_loot.stripTrackedSuffixSlots = loot.value("stripTrackedSuffixSlots", _loot.stripTrackedSuffixSlots);
			_loot.nameFormat = loot.value("nameFormat", std::string{ "{base} [{affix}]" });
			if (const auto markerPosIt = loot.find("nameMarkerPosition"); markerPosIt != loot.end()) {
				if (markerPosIt->is_string()) {
					if (const auto isTrailing = ParseLootNameMarkerPositionIsTrailing(markerPosIt->get<std::string>()); isTrailing.has_value()) {
						_loot.nameMarkerPosition = *isTrailing ? LootNameMarkerPosition::kTrailing : LootNameMarkerPosition::kLeading;
					} else {
						SKSE::log::warn(
							"CalamityAffixes: invalid loot.nameMarkerPosition '{}'; expected 'leading' or 'trailing'. Falling back to trailing.",
							markerPosIt->get<std::string>());
						_loot.nameMarkerPosition = LootNameMarkerPosition::kTrailing;
					}
				} else {
					SKSE::log::warn(
						"CalamityAffixes: loot.nameMarkerPosition must be a string ('leading' or 'trailing'). Falling back to trailing.");
					_loot.nameMarkerPosition = LootNameMarkerPosition::kTrailing;
				}
			}

			if (const auto denyIt = loot.find("armorEditorIdDenyContains"); denyIt != loot.end()) {
				if (denyIt->is_array()) {
					_loot.armorEditorIdDenyContains.clear();
					for (const auto& raw : *denyIt) {
						if (!raw.is_string()) {
							continue;
						}
						std::string markerRaw = raw.get<std::string>();
						const auto marker = Trim(markerRaw);
						if (!marker.empty()) {
							_loot.armorEditorIdDenyContains.emplace_back(marker);
						}
					}
				} else {
					SKSE::log::warn("CalamityAffixes: loot.armorEditorIdDenyContains must be an array of strings.");
				}
			}

			if (const auto allowIt = loot.find("bossContainerEditorIdAllowContains"); allowIt != loot.end()) {
				if (allowIt->is_array()) {
					_loot.bossContainerEditorIdAllowContains.clear();
					for (const auto& raw : *allowIt) {
						if (!raw.is_string()) {
							continue;
						}
						std::string markerRaw = raw.get<std::string>();
						const auto marker = Trim(markerRaw);
						if (!marker.empty()) {
							_loot.bossContainerEditorIdAllowContains.emplace_back(marker);
						}
					}
				} else {
					SKSE::log::warn("CalamityAffixes: loot.bossContainerEditorIdAllowContains must be an array of strings.");
				}
			}

			if (const auto denyBossIt = loot.find("bossContainerEditorIdDenyContains"); denyBossIt != loot.end()) {
				if (denyBossIt->is_array()) {
					_loot.bossContainerEditorIdDenyContains.clear();
					for (const auto& raw : *denyBossIt) {
						if (!raw.is_string()) {
							continue;
						}
						std::string markerRaw = raw.get<std::string>();
						const auto marker = Trim(markerRaw);
						if (!marker.empty()) {
							_loot.bossContainerEditorIdDenyContains.emplace_back(marker);
						}
					}
				} else {
					SKSE::log::warn("CalamityAffixes: loot.bossContainerEditorIdDenyContains must be an array of strings.");
				}
			}

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

			if (_loot.reforgeOrbChancePercent < 0.0f) {
				_loot.reforgeOrbChancePercent = 0.0f;
			} else if (_loot.reforgeOrbChancePercent > 100.0f) {
				_loot.reforgeOrbChancePercent = 100.0f;
			}

			_loot.lootSourceChanceMultCorpse = std::clamp(_loot.lootSourceChanceMultCorpse, 0.0f, 5.0f);
			_loot.lootSourceChanceMultContainer = std::clamp(_loot.lootSourceChanceMultContainer, 0.0f, 5.0f);
			_loot.lootSourceChanceMultBossContainer = std::clamp(_loot.lootSourceChanceMultBossContainer, 0.0f, 5.0f);
			_loot.lootSourceChanceMultWorld = std::clamp(_loot.lootSourceChanceMultWorld, 0.0f, 5.0f);

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

			if (trapCastBudgetPerTick <= 0.0) {
				_loot.trapCastBudgetPerTick = 0u;
			} else {
				const double clamped = std::clamp(trapCastBudgetPerTick, 1.0, 256.0);
				_loot.trapCastBudgetPerTick = static_cast<std::uint32_t>(clamped);
			}

			if (triggerProcBudgetPerWindow <= 0.0) {
				_loot.triggerProcBudgetPerWindow = 0u;
			} else {
				const double clamped = std::clamp(triggerProcBudgetPerWindow, 1.0, 512.0);
				_loot.triggerProcBudgetPerWindow = static_cast<std::uint32_t>(clamped);
			}

				if (triggerProcBudgetWindowMs <= 0.0) {
					_loot.triggerProcBudgetWindowMs = 0u;
				} else {
					const double clamped = std::clamp(triggerProcBudgetWindowMs, 1.0, 5000.0);
					_loot.triggerProcBudgetWindowMs = static_cast<std::uint32_t>(clamped);
				}
			}

		spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
		spdlog::flush_on(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
	}

	void EventBridge::ApplyRuntimeUserSettingsOverrides()
	{
		_runtimeUserSettingsPersist = {};
		_runtimeUserSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();

		nlohmann::json root;
		if (!UserSettingsPersistence::LoadJsonObject(kUserSettingsRelativePath, root)) {
			return;
		}

		if (!root.is_object()) {
			SKSE::log::warn(
				"CalamityAffixes: user settings root is not an object ({}).",
				std::string(kUserSettingsRelativePath));
			return;
		}

		const auto runtimeIt = root.find("runtime");
		if (runtimeIt == root.end()) {
			return;
		}
		if (!runtimeIt->is_object()) {
			SKSE::log::warn(
				"CalamityAffixes: user settings runtime section is invalid ({}).",
				std::string(kUserSettingsRelativePath));
			return;
		}

		const auto& runtime = *runtimeIt;
		_runtimeEnabled = runtime.value("enabled", _runtimeEnabled);
		_loot.debugLog = runtime.value("debugNotifications", _loot.debugLog);
		_loot.dotTagSafetyAutoDisable = runtime.value("dotSafetyAutoDisable", _loot.dotTagSafetyAutoDisable);

		const double validationIntervalSeconds =
			runtime.value("validationIntervalSeconds", static_cast<double>(_equipResync.intervalMs) / 1000.0);
		const double procChanceMultiplier =
			runtime.value("procChanceMultiplier", static_cast<double>(_runtimeProcChanceMult));
		const double runewordFragmentChancePercent =
			runtime.value("runewordFragmentChancePercent", static_cast<double>(_loot.runewordFragmentChancePercent));
		const double reforgeOrbChancePercent =
			runtime.value("reforgeOrbChancePercent", static_cast<double>(_loot.reforgeOrbChancePercent));
		const bool allowNonHostileFirstHitProc = runtime.value(
			"allowNonHostileFirstHitProc",
			_allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed));

		const float clampedValidationIntervalSeconds =
			std::clamp(static_cast<float>(validationIntervalSeconds), 0.0f, 30.0f);
		_equipResync.intervalMs =
			(clampedValidationIntervalSeconds <= 0.0f)
				? 0u
				: static_cast<std::uint64_t>(clampedValidationIntervalSeconds * 1000.0f);
		_equipResync.nextAtMs = 0;

		_runtimeProcChanceMult = std::clamp(static_cast<float>(procChanceMultiplier), 0.0f, 3.0f);
		_loot.runewordFragmentChancePercent = std::clamp(static_cast<float>(runewordFragmentChancePercent), 0.0f, 100.0f);
		_loot.reforgeOrbChancePercent = std::clamp(static_cast<float>(reforgeOrbChancePercent), 0.0f, 100.0f);
		_allowNonHostilePlayerOwnedOutgoingProcs.store(allowNonHostileFirstHitProc, std::memory_order_relaxed);

		if (!_loot.dotTagSafetyAutoDisable) {
			_dotTagSafetySuppressed = false;
		}

		spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
		spdlog::flush_on(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);

		SKSE::log::info(
			"CalamityAffixes: runtime overrides loaded from {} (enabled={}, procMult={}, runeFrag={}%, reforgeOrb={}%, runtimeCurrencyDropsEnabled={}).",
			std::string(kUserSettingsRelativePath),
			_runtimeEnabled,
			_runtimeProcChanceMult,
			_loot.runewordFragmentChancePercent,
			_loot.reforgeOrbChancePercent,
			_loot.runtimeCurrencyDropsEnabled);

		_runtimeUserSettingsPersist.lastPersistedPayload = BuildRuntimeUserSettingsPayload();
	}

	std::string EventBridge::BuildRuntimeUserSettingsPayload() const
	{
		nlohmann::json runtime = nlohmann::json::object();
		runtime["enabled"] = _runtimeEnabled;
		runtime["debugNotifications"] = _loot.debugLog;
		runtime["validationIntervalSeconds"] = static_cast<double>(_equipResync.intervalMs) / 1000.0;
		runtime["procChanceMultiplier"] = _runtimeProcChanceMult;
		runtime["runewordFragmentChancePercent"] = _loot.runewordFragmentChancePercent;
		runtime["reforgeOrbChancePercent"] = _loot.reforgeOrbChancePercent;
		runtime["dotSafetyAutoDisable"] = _loot.dotTagSafetyAutoDisable;
		runtime["allowNonHostileFirstHitProc"] =
			_allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed);
		return runtime.dump();
	}

	void EventBridge::MarkRuntimeUserSettingsDirty()
	{
		(void)RuntimeUserSettingsDebounce::MarkDirty(
			_runtimeUserSettingsPersist,
			BuildRuntimeUserSettingsPayload(),
			std::chrono::steady_clock::now(),
			kRuntimeUserSettingsPersistDebounce);
	}

	void EventBridge::MaybeFlushRuntimeUserSettings(std::chrono::steady_clock::time_point a_now, bool a_force)
	{
		if (!RuntimeUserSettingsDebounce::ShouldFlush(_runtimeUserSettingsPersist, a_now, a_force)) {
			return;
		}

		if (_runtimeUserSettingsPersist.pendingPayload.empty()) {
			_runtimeUserSettingsPersist.pendingPayload = BuildRuntimeUserSettingsPayload();
		}
		if (_runtimeUserSettingsPersist.pendingPayload == _runtimeUserSettingsPersist.lastPersistedPayload) {
			RuntimeUserSettingsDebounce::MarkPersistSuccess(_runtimeUserSettingsPersist);
			return;
		}

		if (PersistRuntimeUserSettings()) {
			_runtimeUserSettingsPersist.lastPersistedPayload = _runtimeUserSettingsPersist.pendingPayload;
			RuntimeUserSettingsDebounce::MarkPersistSuccess(_runtimeUserSettingsPersist);
		} else {
			RuntimeUserSettingsDebounce::MarkPersistFailure(
				_runtimeUserSettingsPersist,
				a_now,
				kRuntimeUserSettingsPersistDebounce);
		}
	}

	bool EventBridge::PersistRuntimeUserSettings()
	{
		const bool updated = UserSettingsPersistence::UpdateJsonObject(
			kUserSettingsRelativePath,
			[&](nlohmann::json& root) {
				root["version"] = 1;
				auto& runtime = root["runtime"];
				if (!runtime.is_object()) {
					runtime = nlohmann::json::object();
				}

				runtime["enabled"] = _runtimeEnabled;
				runtime["debugNotifications"] = _loot.debugLog;
				runtime["validationIntervalSeconds"] = static_cast<double>(_equipResync.intervalMs) / 1000.0;
				runtime["procChanceMultiplier"] = _runtimeProcChanceMult;
				runtime["runewordFragmentChancePercent"] = _loot.runewordFragmentChancePercent;
				runtime["reforgeOrbChancePercent"] = _loot.reforgeOrbChancePercent;
				runtime["dotSafetyAutoDisable"] = _loot.dotTagSafetyAutoDisable;
				runtime["allowNonHostileFirstHitProc"] =
					_allowNonHostilePlayerOwnedOutgoingProcs.load(std::memory_order_relaxed);
			});

		if (!updated) {
			SKSE::log::warn(
				"CalamityAffixes: failed to persist runtime user settings to {}.",
				std::string(kUserSettingsRelativePath));
		}
		return updated;
	}

	const nlohmann::json* EventBridge::ResolveAffixArray(const nlohmann::json& a_configRoot) const
	{
		const auto keywordsIt = a_configRoot.find("keywords");
		if (keywordsIt == a_configRoot.end()) {
			return &kEmptyAffixArray;
		}
		if (!keywordsIt->is_object()) {
			SKSE::log::error("CalamityAffixes: runtime config field 'keywords' must be an object.");
			return nullptr;
		}

		const auto affixesIt = keywordsIt->find("affixes");
		if (affixesIt == keywordsIt->end()) {
			return &kEmptyAffixArray;
		}
		if (!affixesIt->is_array()) {
			SKSE::log::error("CalamityAffixes: runtime config field 'keywords.affixes' must be an array.");
			return nullptr;
		}

		return &(*affixesIt);
	}

}
