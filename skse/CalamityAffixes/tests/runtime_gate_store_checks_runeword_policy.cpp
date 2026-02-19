#include "runtime_gate_store_checks_common.h"

#include <cstdlib>
#include <string_view>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace RuntimeGateStoreChecks
{
		bool CheckRunewordCompletedSelectionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path baseSelectionFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.BaseSelection.cpp";
			const fs::path recipeEntriesFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeEntries.cpp";
			const fs::path recipeUiFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeUi.cpp";

			std::ifstream recipeEntriesIn(recipeEntriesFile);
			if (!recipeEntriesIn.is_open()) {
				std::cerr << "runeword_completed_selection: failed to open source file: " << recipeEntriesFile << "\n";
				return false;
			}

			std::string recipeEntriesSource(
				(std::istreambuf_iterator<char>(recipeEntriesIn)),
				std::istreambuf_iterator<char>());

			std::ifstream recipeUiIn(recipeUiFile);
			if (!recipeUiIn.is_open()) {
				std::cerr << "runeword_completed_selection: failed to open source file: " << recipeUiFile << "\n";
				return false;
			}

			std::string recipeUiSource(
				(std::istreambuf_iterator<char>(recipeUiIn)),
				std::istreambuf_iterator<char>());

			std::ifstream baseIn(baseSelectionFile);
			if (!baseIn.is_open()) {
				std::cerr << "runeword_completed_selection: failed to open source file: " << baseSelectionFile << "\n";
				return false;
			}

			std::string baseSource(
				(std::istreambuf_iterator<char>(baseIn)),
				std::istreambuf_iterator<char>());

			// Completed runeword base must pin recipe highlight to completed recipe token.
			if (recipeEntriesSource.find("bool resolvedFromCompleted = false;") == std::string::npos ||
				recipeEntriesSource.find("selectedToken = completed->token;") == std::string::npos ||
				recipeEntriesSource.find("if (!resolvedFromCompleted)") == std::string::npos) {
				std::cerr << "runeword_completed_selection: completed recipe highlight guard is missing\n";
				return false;
			}

			// Completed runeword base must not persist mutable in-progress recipe state.
			if (recipeUiSource.find("const bool completedBase = ResolveCompletedRunewordRecipe(selectedKey) != nullptr;") ==
				    std::string::npos ||
				recipeUiSource.find("ShouldClearRunewordInProgressState(completedBase)") == std::string::npos ||
				recipeUiSource.find("_runewordInstanceStates.erase(selectedKey);") == std::string::npos) {
				std::cerr << "runeword_completed_selection: completed-base state write guard is missing\n";
				return false;
			}

			// Base selection should also clear mutable in-progress state for completed runeword bases.
			if (baseSource.find("if (completedRecipe)") == std::string::npos ||
				baseSource.find("_runewordInstanceStates.erase(a_instanceKey);") == std::string::npos) {
				std::cerr << "runeword_completed_selection: base-selection completed-state guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordRecipeEntriesMappingPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path recipeEntriesFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeEntries.cpp";

			std::ifstream recipeEntriesIn(recipeEntriesFile);
			if (!recipeEntriesIn.is_open()) {
				std::cerr << "runeword_recipe_entries_mapping: failed to open source file: " << recipeEntriesFile << "\n";
				return false;
			}

			std::string source(
				(std::istreambuf_iterator<char>(recipeEntriesIn)),
				std::istreambuf_iterator<char>());

			if (source.find("kRecommendedBaseOverrides") == std::string::npos ||
				source.find("kEffectSummaryOverrides") == std::string::npos ||
				source.find("FindKeyOverride(id, kRecommendedBaseOverrides)") == std::string::npos ||
				source.find("FindKeyOverride(id, kEffectSummaryOverrides)") == std::string::npos ||
				source.find("HasDuplicateOverrideIds(") == std::string::npos ||
				source.find("static_assert(!HasDuplicateOverrideIds(kRecommendedBaseOverrides)") == std::string::npos ||
				source.find("static_assert(!HasDuplicateOverrideIds(kEffectSummaryOverrides)") == std::string::npos ||
				source.find("{ \"rw_spirit\", \"sword_shield\" }") == std::string::npos ||
				source.find("{ \"rw_spirit\", \"signature_spirit\" }") == std::string::npos) {
				std::cerr << "runeword_recipe_entries_mapping: recipe-entry table mapping guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordCoverageConsistencyPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path recipeEntriesFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeEntries.cpp";
			const fs::path synthesisFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.RunewordSynthesis.cpp";
			const fs::path defaultContractFile = testFile.parent_path().parent_path().parent_path().parent_path() /
				"Data" / "SKSE" / "Plugins" / "CalamityAffixes" / "runtime_contract.json";
			const char* overridePath = std::getenv("CAFF_RUNTIME_CONTRACT_PATH");
			const fs::path contractFile =
				(overridePath && *overridePath) ? fs::path(overridePath) : defaultContractFile;

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto recipeEntriesText = loadText(recipeEntriesFile);
			if (!recipeEntriesText.has_value()) {
				std::cerr << "runeword_coverage_consistency: failed to open source file: " << recipeEntriesFile << "\n";
				return false;
			}
			const auto synthesisText = loadText(synthesisFile);
			if (!synthesisText.has_value()) {
				std::cerr << "runeword_coverage_consistency: failed to open source file: " << synthesisFile << "\n";
				return false;
			}
			const auto contractText = loadText(contractFile);
			if (!contractText.has_value()) {
				std::cerr << "runeword_coverage_consistency: failed to open contract file: " << contractFile << "\n";
				return false;
			}

			nlohmann::json contract = nlohmann::json::object();
			try {
				contract = nlohmann::json::parse(*contractText);
			} catch (const std::exception& e) {
				std::cerr << "runeword_coverage_consistency: contract parse failed: " << e.what() << "\n";
				return false;
			}

			const auto catalogIt = contract.find("runewordCatalog");
			if (catalogIt == contract.end() || !catalogIt->is_array() || catalogIt->empty()) {
				std::cerr << "runeword_coverage_consistency: runewordCatalog missing or empty\n";
				return false;
			}

			std::vector<std::string> recipeIds;
			recipeIds.reserve(catalogIt->size());
			for (const auto& entry : *catalogIt) {
				if (!entry.is_object()) {
					continue;
				}
				const auto idIt = entry.find("id");
				if (idIt == entry.end() || !idIt->is_string()) {
					continue;
				}
				recipeIds.push_back(idIt->get<std::string>());
			}
			if (recipeIds.empty()) {
				std::cerr << "runeword_coverage_consistency: no runeword ids parsed from contract\n";
				return false;
			}

			auto extractSection = [](const std::string& source, std::string_view start, std::string_view end) -> std::optional<std::string> {
				const auto begin = source.find(start);
				if (begin == std::string::npos) {
					return std::nullopt;
				}
				const auto finish = source.find(end, begin);
				if (finish == std::string::npos || finish <= begin) {
					return std::nullopt;
				}
				return source.substr(begin, finish - begin);
			};

			const auto styleSection = extractSection(
				*synthesisText,
				"auto resolveRunewordStyle = [&](const RunewordRecipe& a_recipe)",
				"auto toDurationMs = [](float a_seconds)");
			if (!styleSection.has_value()) {
				std::cerr << "runeword_coverage_consistency: style section not found\n";
				return false;
			}
			const auto tuningSection = extractSection(
				*synthesisText,
				"static constexpr RecipeTuning kRecipeTunings[] = {",
				"};");
			if (!tuningSection.has_value()) {
				std::cerr << "runeword_coverage_consistency: tuning section not found\n";
				return false;
			}
			const auto summarySection = extractSection(
				*recipeEntriesText,
				"static constexpr RecipeKeyOverride kEffectSummaryOverrides[] = {",
				"};");
			if (!summarySection.has_value()) {
				std::cerr << "runeword_coverage_consistency: summary section not found\n";
				return false;
			}

			auto collectMissing = [&](const std::string& section) {
				std::vector<std::string> missing;
				for (const auto& id : recipeIds) {
					const std::string needle = "\"" + id + "\"";
					if (section.find(needle) == std::string::npos) {
						missing.push_back(id);
					}
				}
				return missing;
			};

			const auto missingStyle = collectMissing(*styleSection);
			const auto missingTuning = collectMissing(*tuningSection);
			const auto missingSummary = collectMissing(*summarySection);
			if (missingStyle.empty() && missingTuning.empty() && missingSummary.empty()) {
				return true;
			}

			auto printMissing = [](std::string_view label, const std::vector<std::string>& missing) {
				if (missing.empty()) {
					return;
				}
				std::cerr << "runeword_coverage_consistency: missing " << label << " ids:";
				for (const auto& id : missing) {
					std::cerr << ' ' << id;
				}
				std::cerr << "\n";
			};

			printMissing("style", missingStyle);
			printMissing("tuning", missingTuning);
			printMissing("summary", missingSummary);
			return false;
		}

		bool CheckLowHealthTriggerSnapshotPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path triggersFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Triggers.cpp";

			std::ifstream in(triggersFile);
			if (!in.is_open()) {
				std::cerr << "low_health_trigger_snapshot: failed to open source file: " << triggersFile << "\n";
				return false;
			}

			std::string source(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());

			const auto snapshotDeclPos = source.find("const RE::FormID lowHealthOwnerFormID =");
			const auto previousDeclPos = source.find("const float lowHealthPreviousPct = [&]() {");
			const auto gateCallPos = source.find("PassesLowHealthTriggerGate(affix, a_owner, lowHealthPreviousPct, lowHealthCurrentPct)");
			const auto mapUpdatePos = source.find("_lowHealthLastObservedPct[lowHealthOwnerFormID] = lowHealthCurrentPct;");

			if (snapshotDeclPos == std::string::npos ||
				previousDeclPos == std::string::npos ||
				gateCallPos == std::string::npos ||
				mapUpdatePos == std::string::npos ||
				mapUpdatePos < gateCallPos) {
				std::cerr << "low_health_trigger_snapshot: single-snapshot low-health gate guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordRecipeRuntimeEligibilityPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path recipeEntriesFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeEntries.cpp";
			const fs::path recipeUiFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeUi.cpp";
			const fs::path catalogFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.Catalog.cpp";
			const fs::path baseSelectionFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.BaseSelection.cpp";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto recipeEntriesText = loadText(recipeEntriesFile);
			if (!recipeEntriesText.has_value()) {
				std::cerr << "runeword_recipe_runtime_eligibility: failed to open source file: " << recipeEntriesFile << "\n";
				return false;
			}
			const auto recipeUiText = loadText(recipeUiFile);
			if (!recipeUiText.has_value()) {
				std::cerr << "runeword_recipe_runtime_eligibility: failed to open source file: " << recipeUiFile << "\n";
				return false;
			}
			const auto catalogText = loadText(catalogFile);
			if (!catalogText.has_value()) {
				std::cerr << "runeword_recipe_runtime_eligibility: failed to open source file: " << catalogFile << "\n";
				return false;
			}
			const auto baseSelectionText = loadText(baseSelectionFile);
			if (!baseSelectionText.has_value()) {
				std::cerr << "runeword_recipe_runtime_eligibility: failed to open source file: " << baseSelectionFile << "\n";
				return false;
			}

			const auto recipeUiGuardPos = recipeUiText->find("Runeword Recipe: runtime effect not available.");
			const auto recipeUiCursorUpdatePos = recipeUiText->find("_runewordRecipeCycleCursor = static_cast<std::uint32_t>(idx);");

			if (recipeEntriesText->find("if (const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);") == std::string::npos ||
				recipeEntriesText->find("affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size())") == std::string::npos ||
				recipeUiText->find("Runeword Recipe: runtime effect not available.") == std::string::npos ||
				recipeUiGuardPos == std::string::npos ||
				recipeUiCursorUpdatePos == std::string::npos ||
				recipeUiCursorUpdatePos < recipeUiGuardPos ||
				catalogText->find("const auto isEligible = [&](std::size_t a_idx)") == std::string::npos ||
				catalogText->find("if (!isEligible(cursor))") == std::string::npos ||
				catalogText->find("if (const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);") == std::string::npos ||
				baseSelectionText->find("const auto rwIt = _runewordRecipeIndexByResultAffixToken.find(token);") == std::string::npos ||
				baseSelectionText->find("if (const auto affixIt = _affixIndexByToken.find(recipe.resultAffixToken);") == std::string::npos ||
				baseSelectionText->find("return std::addressof(recipe);") == std::string::npos) {
				std::cerr << "runeword_recipe_runtime_eligibility: unsupported recipe guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordContractSnapshotPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path defaultContractFile = testFile.parent_path().parent_path().parent_path().parent_path() /
				"Data" / "SKSE" / "Plugins" / "CalamityAffixes" / "runtime_contract.json";
			const char* overridePath = std::getenv("CAFF_RUNTIME_CONTRACT_PATH");
			const fs::path contractFile =
				(overridePath && *overridePath) ? fs::path(overridePath) : defaultContractFile;

			std::ifstream in(contractFile);
			if (!in.is_open()) {
				std::cerr << "runeword_contract_snapshot: failed to open contract file: " << contractFile << "\n";
				return false;
			}

			nlohmann::json contract = nlohmann::json::object();
			try {
				in >> contract;
			} catch (const std::exception& e) {
				std::cerr << "runeword_contract_snapshot: parse failed: " << e.what() << "\n";
				return false;
			}

			const auto catalogIt = contract.find("runewordCatalog");
			const auto weightsIt = contract.find("runewordRuneWeights");
			if (catalogIt == contract.end() || !catalogIt->is_array() || catalogIt->empty() ||
				weightsIt == contract.end() || !weightsIt->is_array() || weightsIt->empty()) {
				std::cerr << "runeword_contract_snapshot: runeword catalog or rune weights are missing/empty\n";
				return false;
			}

			const auto findRecipeById = [&](std::string_view id) -> const nlohmann::json* {
				for (const auto& entry : *catalogIt) {
					if (!entry.is_object()) {
						continue;
					}
					const auto idIt = entry.find("id");
					if (idIt == entry.end() || !idIt->is_string()) {
						continue;
					}
					if (idIt->get<std::string>() == id) {
						return &entry;
					}
				}
				return nullptr;
			};

			const auto* nadir = findRecipeById("rw_nadir");
			if (!nadir) {
				std::cerr << "runeword_contract_snapshot: rw_nadir recipe is missing\n";
				return false;
			}

			const auto resultAffixIt = nadir->find("resultAffixId");
			if (resultAffixIt == nadir->end() || !resultAffixIt->is_string() ||
				resultAffixIt->get<std::string>() != "runeword_nadir_final") {
				std::cerr << "runeword_contract_snapshot: rw_nadir result affix mismatch\n";
				return false;
			}

			const auto recommendedBaseIt = nadir->find("recommendedBase");
			if (recommendedBaseIt == nadir->end() || !recommendedBaseIt->is_string() ||
				recommendedBaseIt->get<std::string>() != "Armor") {
				std::cerr << "runeword_contract_snapshot: rw_nadir recommended base mismatch\n";
				return false;
			}

			const auto runesIt = nadir->find("runes");
			if (runesIt == nadir->end() || !runesIt->is_array() || runesIt->empty()) {
				std::cerr << "runeword_contract_snapshot: rw_nadir rune sequence is missing\n";
				return false;
			}

			std::unordered_set<std::string> nadirRunes;
			for (const auto& rune : *runesIt) {
				if (!rune.is_string()) {
					continue;
				}
				nadirRunes.insert(rune.get<std::string>());
			}
			if (nadirRunes.find("Nef") == nadirRunes.end() || nadirRunes.find("Tir") == nadirRunes.end()) {
				std::cerr << "runeword_contract_snapshot: rw_nadir rune sequence mismatch\n";
				return false;
			}

			std::unordered_set<std::string> weightedRunes;
			for (const auto& weightEntry : *weightsIt) {
				if (!weightEntry.is_object()) {
					continue;
				}
				const auto runeIt = weightEntry.find("rune");
				const auto weightIt2 = weightEntry.find("weight");
				if (runeIt == weightEntry.end() || !runeIt->is_string() ||
					weightIt2 == weightEntry.end() || !weightIt2->is_number() || !(weightIt2->get<double>() > 0.0)) {
					std::cerr << "runeword_contract_snapshot: invalid rune weight entry\n";
					return false;
				}
				weightedRunes.insert(runeIt->get<std::string>());
			}

			if (weightedRunes.find("El") == weightedRunes.end() || weightedRunes.find("Zod") == weightedRunes.end()) {
				std::cerr << "runeword_contract_snapshot: expected rune weights are missing\n";
				return false;
			}

			return true;
		}

		bool CheckSynthesizedAffixDisplayNameFallbackPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path synthesisFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.AffixRegistry.cpp";

			std::ifstream in(synthesisFile);
			if (!in.is_open()) {
				std::cerr << "synthesized_affix_displayname_fallback: failed to open source file: " << synthesisFile << "\n";
				return false;
			}

			std::string source(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());

			if (source.find("if (a_affix.displayName.empty()) {") == std::string::npos ||
				source.find("if (a_affix.displayNameEn.empty()) {") == std::string::npos ||
				source.find("if (a_affix.displayNameKo.empty()) {") == std::string::npos ||
				source.find("a_affix.displayNameEn = a_affix.displayName;") == std::string::npos ||
				source.find("a_affix.displayNameKo = a_affix.displayName;") == std::string::npos) {
				std::cerr << "synthesized_affix_displayname_fallback: synthesized affix tooltip-name fallback guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckSynthesizedRunewordTooltipSummaryPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path tooltipFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.TooltipResolution.cpp";
			const fs::path synthesisFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.RunewordSynthesis.cpp";
			const fs::path catalogFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.Catalog.cpp";

			std::ifstream tooltipIn(tooltipFile);
			if (!tooltipIn.is_open()) {
				std::cerr << "synthesized_runeword_tooltip_summary: failed to open source file: " << tooltipFile << "\n";
				return false;
			}
			std::ifstream synthesisIn(synthesisFile);
			if (!synthesisIn.is_open()) {
				std::cerr << "synthesized_runeword_tooltip_summary: failed to open source file: " << synthesisFile << "\n";
				return false;
			}
			std::ifstream catalogIn(catalogFile);
			if (!catalogIn.is_open()) {
				std::cerr << "synthesized_runeword_tooltip_summary: failed to open source file: " << catalogFile << "\n";
				return false;
			}

			std::string tooltipSource(
				(std::istreambuf_iterator<char>(tooltipIn)),
				std::istreambuf_iterator<char>());
			std::string synthesisSource(
				(std::istreambuf_iterator<char>(synthesisIn)),
				std::istreambuf_iterator<char>());
			std::string catalogSource(
				(std::istreambuf_iterator<char>(catalogIn)),
				std::istreambuf_iterator<char>());

			if (tooltipSource.find("IsSynthesizedRunewordAffixId(") == std::string::npos ||
				tooltipSource.find("buildRunewordAutoSummary") == std::string::npos ||
				tooltipSource.find("a_affixId.rfind(\"rw_\", 0) == 0") == std::string::npos ||
				tooltipSource.find("a_affixId.rfind(\"runeword_\", 0) == 0") == std::string::npos ||
				tooltipSource.find("a_affixId.substr(a_affixId.size() - 5) != \"_auto\"") == std::string::npos ||
				tooltipSource.find("headerEn = \"On \"") == std::string::npos ||
				tooltipSource.find("발동 ") == std::string::npos ||
				tooltipSource.find("Adaptive elemental cast") == std::string::npos ||
				tooltipSource.find("적응형 원소 시전") == std::string::npos ||
				tooltipSource.find("Self-cast ") == std::string::npos ||
				tooltipSource.find("자신에게 ") == std::string::npos) {
				std::cerr << "synthesized_runeword_tooltip_summary: synthesized runeword tooltip-summary guard is missing\n";
				return false;
			}
			if (synthesisSource.find("const std::string recipeNameEn = recipe.displayNameEn.empty() ? recipe.displayName : recipe.displayNameEn;") == std::string::npos ||
				synthesisSource.find("const std::string recipeNameKo = recipe.displayNameKo.empty() ? recipe.displayName : recipe.displayNameKo;") == std::string::npos ||
				synthesisSource.find("out.displayNameEn = \"Runeword \" + recipeNameEn + \" \" + suffix;") == std::string::npos ||
				synthesisSource.find("out.displayNameKo = \"룬워드 \" + recipeNameKo + \" \" + suffix;") == std::string::npos ||
				synthesisSource.find("out.displayName = out.displayNameKo;") == std::string::npos) {
				std::cerr << "synthesized_runeword_tooltip_summary: synthesized runeword display-name localization guard is missing\n";
				return false;
			}
			if (catalogSource.find("ResolveRunewordDisplayNameKo(") == std::string::npos ||
				catalogSource.find("{ \"rw_metamorphosis\", \"메타모포시스\" }") == std::string::npos ||
				catalogSource.find("{ \"rw_enigma\", \"수수께끼\" }") == std::string::npos ||
				catalogSource.find("recipe.displayName = recipe.displayNameKo;") == std::string::npos) {
				std::cerr << "synthesized_runeword_tooltip_summary: runeword recipe-name localization table guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordTransmuteSafetyPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path recipeUiFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeUi.cpp";
			const fs::path panelStateFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.PanelState.cpp";
			const fs::path transmuteFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.Transmute.cpp";
			const fs::path policyFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.Policy.cpp";

			std::ifstream recipeUiIn(recipeUiFile);
			if (!recipeUiIn.is_open()) {
				std::cerr << "runeword_transmute_safety: failed to open recipe-ui source: " << recipeUiFile << "\n";
				return false;
			}
			std::string recipeUiSource(
				(std::istreambuf_iterator<char>(recipeUiIn)),
				std::istreambuf_iterator<char>());

			if (recipeUiSource.find("const bool completedBase = ResolveCompletedRunewordRecipe(selectedKey) != nullptr;") ==
				    std::string::npos ||
				recipeUiSource.find("_runewordInstanceStates.erase(selectedKey);") == std::string::npos) {
				std::cerr << "runeword_transmute_safety: completed-base recipe selection state guard is missing\n";
				return false;
			}

			std::ifstream panelIn(panelStateFile);
			if (!panelIn.is_open()) {
				std::cerr << "runeword_transmute_safety: failed to open panel-state source: " << panelStateFile << "\n";
				return false;
			}
			std::string panelSource(
				(std::istreambuf_iterator<char>(panelIn)),
				std::istreambuf_iterator<char>());

			if (panelSource.find("ResolveRunewordApplyBlockReason(*_runewordSelectedBaseKey, *recipe)") == std::string::npos ||
				panelSource.find("BuildRunewordApplyBlockMessage(applyBlockReason)") == std::string::npos ||
				panelSource.find("CanFinalizeRunewordFromPanel(") == std::string::npos ||
				panelSource.find("CanInsertRunewordFromPanel(") == std::string::npos) {
				std::cerr << "runeword_transmute_safety: panel canInsert/apply safety guard is missing\n";
				return false;
			}

			std::ifstream policyIn(policyFile);
			if (!policyIn.is_open()) {
				std::cerr << "runeword_transmute_safety: failed to open policy source: " << policyFile << "\n";
				return false;
			}
			std::string policySource(
				(std::istreambuf_iterator<char>(policyIn)),
				std::istreambuf_iterator<char>());

			if (policySource.find("Runeword result affix missing") == std::string::npos ||
				policySource.find("Affix slots full (max ") == std::string::npos) {
				std::cerr << "runeword_transmute_safety: policy messages for apply block reasons are missing\n";
				return false;
			}

			std::ifstream transmuteIn(transmuteFile);
			if (!transmuteIn.is_open()) {
				std::cerr << "runeword_transmute_safety: failed to open transmute source: " << transmuteFile << "\n";
				return false;
			}
			std::string transmuteSource(
				(std::istreambuf_iterator<char>(transmuteIn)),
				std::istreambuf_iterator<char>());

			if (transmuteSource.find("ResolveRunewordApplyBlockReason(a_instanceKey, a_recipe)") == std::string::npos ||
				transmuteSource.find("ResolveRunewordApplyBlockReason(*_runewordSelectedBaseKey, *recipe)") == std::string::npos ||
				transmuteSource.find("runeword result affix missing before transmute") == std::string::npos ||
				transmuteSource.find("note.append(BuildRunewordApplyBlockMessage(blockReason));") == std::string::npos) {
				std::cerr << "runeword_transmute_safety: transmute pre-consume safety guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordUiPolicyHelpers()
		{
			if (!CalamityAffixes::ShouldClearRunewordInProgressState(true) ||
				CalamityAffixes::ShouldClearRunewordInProgressState(false)) {
				std::cerr << "runeword_ui_policy: completed-base clear policy helper mismatch\n";
				return false;
			}

			if (!CalamityAffixes::CanFinalizeRunewordFromPanel(3u, 3u, true) ||
				CalamityAffixes::CanFinalizeRunewordFromPanel(2u, 3u, true) ||
				CalamityAffixes::CanFinalizeRunewordFromPanel(3u, 3u, false)) {
				std::cerr << "runeword_ui_policy: finalize helper mismatch\n";
				return false;
			}

			if (!CalamityAffixes::CanInsertRunewordFromPanel(true, true) ||
				CalamityAffixes::CanInsertRunewordFromPanel(true, false) ||
				CalamityAffixes::CanInsertRunewordFromPanel(false, true)) {
				std::cerr << "runeword_ui_policy: can-insert helper mismatch\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordReforgeSafetyPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path reforgeFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.Reforge.cpp";

			std::ifstream reforgeIn(reforgeFile);
			if (!reforgeIn.is_open()) {
				std::cerr << "runeword_reforge_safety: failed to open reforge source: " << reforgeFile << "\n";
				return false;
			}
			std::string reforgeSource(
				(std::istreambuf_iterator<char>(reforgeIn)),
				std::istreambuf_iterator<char>());

			if (reforgeSource.find("const bool rerollRuneword = completedRunewordRecipe != nullptr;") == std::string::npos ||
				reforgeSource.find("pickReforgeRunewordRecipe") == std::string::npos ||
				reforgeSource.find("rolled.AddToken(attemptRunewordRecipe->resultAffixToken);") == std::string::npos ||
				reforgeSource.find("kMaxRegularAffixesPerItem") == std::string::npos ||
				reforgeSource.find("_runewordInstanceStates.erase(instanceKey);") == std::string::npos ||
				reforgeSource.find("IsLootObjectEligibleForAffixes(entry->object)") == std::string::npos ||
				reforgeSource.find("Reforge blocked: completed runeword base.") != std::string::npos) {
				std::cerr << "runeword_reforge_safety: completed-base reroll integration guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaTooltipImmediateRefreshPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path prismaHandleFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.HandleUiCommand.inl";
			const fs::path prismaCoreFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.cpp";
			const fs::path runtimeTooltipFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.TooltipResolution.cpp";
			const fs::path runewordPanelStateFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.PanelState.cpp";
			const fs::path prismaUiFile = testFile.parent_path().parent_path().parent_path().parent_path() /
				"Data" / "PrismaUI" / "views" / "CalamityAffixes" / "index.html";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto handleText = loadText(prismaHandleFile);
			if (!handleText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open handle source: " << prismaHandleFile << "\n";
				return false;
			}
			const auto coreText = loadText(prismaCoreFile);
			if (!coreText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open prisma source: " << prismaCoreFile << "\n";
				return false;
			}
			const auto uiText = loadText(prismaUiFile);
			if (!uiText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open ui source: " << prismaUiFile << "\n";
				return false;
			}
			const auto runtimeText = loadText(runtimeTooltipFile);
			if (!runtimeText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open runtime source: " << runtimeTooltipFile << "\n";
				return false;
			}
			const auto panelStateText = loadText(runewordPanelStateFile);
			if (!panelStateText.has_value()) {
				std::cerr << "prisma_tooltip_refresh: failed to open runeword panel state source: " << runewordPanelStateFile << "\n";
				return false;
			}

			if (coreText->find("PushSelectedTooltipSnapshot(") == std::string::npos ||
				handleText->find("PushSelectedTooltipSnapshot(true);") == std::string::npos ||
				coreText->find("const bool selectionChanged =") == std::string::npos ||
				coreText->find("if (a_force || selectionChanged || !g_lastTooltip.empty())") == std::string::npos ||
				coreText->find("if (a_force || selectionChanged || next != g_lastTooltip)") == std::string::npos ||
				coreText->find("setRunewordAffixPreview") == std::string::npos ||
				handleText->find("SetRunewordAffixPreview(") == std::string::npos ||
				uiText->find("PrismaUI_Interop(\"setRunewordAffixPreview\"") == std::string::npos ||
				uiText->find("function setRunewordAffixPreview(raw)") == std::string::npos ||
				runtimeText->find("std::uint64_t a_preferredInstanceKey") == std::string::npos ||
				runtimeText->find("if (a_preferredInstanceKey != 0u)") == std::string::npos ||
				runtimeText->find("a_candidate.instanceKey == a_preferredInstanceKey") == std::string::npos ||
				panelStateText->find("GetInstanceAffixTooltip(") == std::string::npos ||
				panelStateText->find("*_runewordSelectedBaseKey") == std::string::npos) {
				std::cerr << "prisma_tooltip_refresh: immediate tooltip refresh guard is missing for runeword actions\n";
				return false;
			}

			return true;
		}

}
