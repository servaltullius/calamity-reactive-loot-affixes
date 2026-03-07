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

			// Completed runeword base sets default highlight; explicit instance state overrides it.
			if (recipeEntriesSource.find("selectedToken = completed->token;") == std::string::npos ||
				recipeEntriesSource.find("stateIt->second.recipeToken") == std::string::npos) {
				std::cerr << "runeword_completed_selection: completed recipe highlight guard is missing\n";
				return false;
			}

			// Completed runeword base must not persist mutable in-progress recipe state.
			if (recipeUiSource.find("const bool completedBase = ResolveCompletedRunewordRecipe(selectedKey) != nullptr;") ==
				    std::string::npos ||
				recipeUiSource.find("ShouldClearRunewordInProgressState(completedBase)") == std::string::npos ||
				recipeUiSource.find("_runewordState.instanceStates.erase(selectedKey);") == std::string::npos) {
				std::cerr << "runeword_completed_selection: completed-base state write guard is missing\n";
				return false;
			}

			// Base selection should also clear mutable in-progress state for completed runeword bases.
			if (baseSource.find("if (completedRecipe)") == std::string::npos ||
				baseSource.find("_runewordState.instanceStates.erase(a_instanceKey);") == std::string::npos) {
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
			const fs::path summaryHeaderFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.SummaryText.h";

			std::ifstream recipeEntriesIn(recipeEntriesFile);
			if (!recipeEntriesIn.is_open()) {
				std::cerr << "runeword_recipe_entries_mapping: failed to open source file: " << recipeEntriesFile << "\n";
				return false;
			}

			std::ifstream summaryHeaderIn(summaryHeaderFile);
			if (!summaryHeaderIn.is_open()) {
				std::cerr << "runeword_recipe_entries_mapping: failed to open summary header file: " << summaryHeaderFile << "\n";
				return false;
			}

			std::string source(
				(std::istreambuf_iterator<char>(recipeEntriesIn)),
				std::istreambuf_iterator<char>());
			std::string summaryHeader(
				(std::istreambuf_iterator<char>(summaryHeaderIn)),
				std::istreambuf_iterator<char>());

			if (source.find("kRecommendedBaseOverrides") == std::string::npos ||
				source.find("FindKeyOverride(id, kRecommendedBaseOverrides)") == std::string::npos ||
				source.find("HasDuplicateOverrideIds(") == std::string::npos ||
				source.find("static_assert(!HasDuplicateOverrideIds(kRecommendedBaseOverrides)") == std::string::npos ||
				source.find("{ \"rw_spirit\", \"sword_shield\" }") == std::string::npos ||
				source.find("RunewordSummary::ResolveEffectSummaryKey") == std::string::npos ||
				summaryHeader.find("kEffectSummaryOverrides") == std::string::npos ||
				summaryHeader.find("static_assert(!HasDuplicateOverrideIds(kEffectSummaryOverrides)") == std::string::npos ||
				summaryHeader.find("{ \"rw_spirit\", \"signature_spirit\" }") == std::string::npos) {
				std::cerr << "runeword_recipe_entries_mapping: recipe-entry table mapping guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordRecipeTooltipTextPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path eventBridgeHeaderFile = testFile.parent_path().parent_path() / "include" / "CalamityAffixes" / "EventBridge.h";
			const fs::path contractsHeaderFile = testFile.parent_path().parent_path() / "include" / "CalamityAffixes" / "RunewordUiContracts.h";
			const fs::path recipeEntriesFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.RecipeEntries.cpp";
			const fs::path prismaCoreFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.cpp";
			const fs::path prismaPanelDataFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.PanelData.inl";
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

			const auto headerText = loadText(eventBridgeHeaderFile);
			if (!headerText.has_value()) {
				std::cerr << "runeword_recipe_tooltip_text: failed to open header file: " << eventBridgeHeaderFile << "\n";
				return false;
			}
			const auto contractsHeaderText = loadText(contractsHeaderFile);
			if (!contractsHeaderText.has_value()) {
				std::cerr << "runeword_recipe_tooltip_text: failed to open contracts header file: " << contractsHeaderFile << "\n";
				return false;
			}
			const auto recipeEntriesText = loadText(recipeEntriesFile);
			if (!recipeEntriesText.has_value()) {
				std::cerr << "runeword_recipe_tooltip_text: failed to open recipe entries file: " << recipeEntriesFile << "\n";
				return false;
			}
			// PrismaTooltip.cpp includes PanelData.inl — concatenate for pattern matching.
			std::string prismaCoreTextCombined;
			for (const auto& sf : { prismaCoreFile, prismaPanelDataFile }) {
				const auto part = loadText(sf);
				if (!part.has_value()) {
					std::cerr << "runeword_recipe_tooltip_text: failed to open prisma source: " << sf << "\n";
					return false;
				}
				prismaCoreTextCombined += *part;
			}
			const auto prismaCoreText = std::optional<std::string>(std::move(prismaCoreTextCombined));
			const auto prismaUiText = loadText(prismaUiFile);
			if (!prismaUiText.has_value()) {
				std::cerr << "runeword_recipe_tooltip_text: failed to open prisma ui file: " << prismaUiFile << "\n";
				return false;
			}

			const std::string headerContractsText = *headerText + *contractsHeaderText;
			if (headerContractsText.find("std::string effectSummaryText{};") == std::string::npos ||
				headerContractsText.find("std::string effectDetailText{};") == std::string::npos ||
				recipeEntriesText->find(".effectSummaryText =") == std::string::npos ||
				recipeEntriesText->find(".effectDetailText =") == std::string::npos ||
				prismaCoreText->find("{ \"summary\", entry.effectSummaryText }") == std::string::npos ||
				prismaCoreText->find("{ \"detail\", entry.effectDetailText }") == std::string::npos ||
				prismaUiText->find("const fallbackSummary = typeof item?.summary === \"string\"") == std::string::npos ||
				prismaUiText->find("const fallbackDetail = typeof item?.detail === \"string\"") == std::string::npos ||
				prismaUiText->find("function resolveRecipeFlavorDetailText(item)") == std::string::npos ||
				prismaUiText->find("function buildRunewordTooltipLikeText(item") == std::string::npos ||
				prismaUiText->find("function buildRecipePreviewTooltipText(item)") == std::string::npos ||
				prismaUiText->find("buildRunewordTooltipLikeText(getSelectedRecipeItem()") == std::string::npos ||
				prismaUiText->find("const recipePreviewText = buildRunewordTooltipLikeText(getSelectedRecipeItem()") == std::string::npos) {
				std::cerr << "runeword_recipe_tooltip_text: recipe tooltip text enrichment guard is missing\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordUiContractExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path repoRoot = testFile.parent_path().parent_path();
			const fs::path eventBridgeHeaderFile = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";
			const fs::path contractsHeaderFile = repoRoot / "include" / "CalamityAffixes" / "RunewordUiContracts.h";
			const fs::path prismaPanelDataFile = repoRoot / "src" / "PrismaTooltip.PanelData.inl";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto eventBridgeHeaderText = loadText(eventBridgeHeaderFile);
			if (!eventBridgeHeaderText.has_value()) {
				std::cerr << "runeword_ui_contract_extraction: failed to open header file: " << eventBridgeHeaderFile << "\n";
				return false;
			}

			const auto contractsHeaderText = loadText(contractsHeaderFile);
			if (!contractsHeaderText.has_value()) {
				std::cerr << "runeword_ui_contract_extraction: failed to open contracts header file: " << contractsHeaderFile << "\n";
				return false;
			}

			const auto prismaPanelDataText = loadText(prismaPanelDataFile);
			if (!prismaPanelDataText.has_value()) {
				std::cerr << "runeword_ui_contract_extraction: failed to open Prisma panel data source: " << prismaPanelDataFile << "\n";
				return false;
			}

			if (eventBridgeHeaderText->find("#include \"CalamityAffixes/RunewordUiContracts.h\"") == std::string::npos ||
				eventBridgeHeaderText->find("struct RunewordBaseInventoryEntry") != std::string::npos ||
				eventBridgeHeaderText->find("struct RunewordRecipeEntry") != std::string::npos ||
				eventBridgeHeaderText->find("struct RunewordRuneRequirement") != std::string::npos ||
				eventBridgeHeaderText->find("struct RunewordPanelState") != std::string::npos ||
				eventBridgeHeaderText->find("struct OperationResult") != std::string::npos ||
				contractsHeaderText->find("struct RunewordBaseInventoryEntry") == std::string::npos ||
				contractsHeaderText->find("struct RunewordRecipeEntry") == std::string::npos ||
				contractsHeaderText->find("struct RunewordRuneRequirement") == std::string::npos ||
				contractsHeaderText->find("struct RunewordPanelState") == std::string::npos ||
				contractsHeaderText->find("struct OperationResult") == std::string::npos ||
				prismaPanelDataText->find("const std::vector<RunewordBaseInventoryEntry>&") == std::string::npos ||
				prismaPanelDataText->find("const std::vector<RunewordRecipeEntry>&") == std::string::npos ||
				prismaPanelDataText->find("const RunewordPanelState&") == std::string::npos) {
				std::cerr << "runeword_ui_contract_extraction: runeword UI DTO contract extraction is incomplete\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordRuntimeStateExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path repoRoot = testFile.parent_path().parent_path();
			const fs::path eventBridgeHeaderFile = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";
			const fs::path eventBridgeTypesFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.Types.inl";
			const fs::path catalogFile = repoRoot / "src" / "EventBridge.Loot.Runeword.Catalog.cpp";
			const fs::path loadFile = repoRoot / "src" / "EventBridge.Serialization.Load.cpp";
			const fs::path lifecycleFile = repoRoot / "src" / "EventBridge.Serialization.Lifecycle.cpp";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto eventBridgeHeaderText = loadText(eventBridgeHeaderFile);
			const auto eventBridgeTypesText = loadText(eventBridgeTypesFile);
			const auto catalogText = loadText(catalogFile);
			const auto loadTextValue = loadText(loadFile);
			const auto lifecycleText = loadText(lifecycleFile);
			if (!eventBridgeHeaderText.has_value() || !eventBridgeTypesText.has_value() ||
				!catalogText.has_value() || !loadTextValue.has_value() || !lifecycleText.has_value()) {
				std::cerr << "runeword_runtime_state_extraction: failed to load source files\n";
				return false;
			}

			if (eventBridgeHeaderText->find("RunewordRuntimeState _runewordState{};") == std::string::npos ||
				eventBridgeHeaderText->find("_runewordRecipes") != std::string::npos ||
				eventBridgeHeaderText->find("_runewordRecipeIndexByToken") != std::string::npos ||
				eventBridgeHeaderText->find("_runewordRuneFragments") != std::string::npos ||
				eventBridgeHeaderText->find("_runewordSelectedBaseKey") != std::string::npos ||
				eventBridgeHeaderText->find("_runewordTransmuteInProgress") != std::string::npos) {
				std::cerr << "runeword_runtime_state_extraction: EventBridge.h still owns runeword mutable state directly\n";
				return false;
			}

			if (eventBridgeTypesText->find("struct RunewordRuntimeState") == std::string::npos ||
				eventBridgeTypesText->find("std::vector<RunewordRecipe> recipes{};") == std::string::npos ||
				eventBridgeTypesText->find("std::unordered_map<std::uint64_t, RunewordInstanceState> instanceStates{};") == std::string::npos ||
				eventBridgeTypesText->find("void ResetCatalog() noexcept") == std::string::npos ||
				eventBridgeTypesText->find("void ResetSelectionAndProgress() noexcept") == std::string::npos) {
				std::cerr << "runeword_runtime_state_extraction: RunewordRuntimeState ownership or reset helpers are missing\n";
				return false;
			}

			if (catalogText->find("_runewordState.ResetCatalog();") == std::string::npos ||
				loadTextValue->find("_runewordState.ResetSelectionAndProgress();") == std::string::npos ||
				lifecycleText->find("_runewordState.ResetSelectionAndProgress();") == std::string::npos) {
				std::cerr << "runeword_runtime_state_extraction: catalog/load/revert paths must use RunewordRuntimeState helpers\n";
				return false;
			}

			return true;
		}

		bool CheckRunewordCoverageConsistencyPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path summaryHeaderFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.Runeword.SummaryText.h";
			const fs::path styleSelectionFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.RunewordSynthesis.StyleSelection.cpp";
			const fs::path tuningFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.RunewordSynthesis.Style.cpp";
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

			const auto summaryHeaderText = loadText(summaryHeaderFile);
			if (!summaryHeaderText.has_value()) {
				std::cerr << "runeword_coverage_consistency: failed to open summary header file: " << summaryHeaderFile << "\n";
				return false;
			}
			const auto styleSelectionText = loadText(styleSelectionFile);
			if (!styleSelectionText.has_value()) {
				std::cerr << "runeword_coverage_consistency: failed to open source file: " << styleSelectionFile << "\n";
				return false;
			}
			const auto tuningText = loadText(tuningFile);
			if (!tuningText.has_value()) {
				std::cerr << "runeword_coverage_consistency: failed to open source file: " << tuningFile << "\n";
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
				*styleSelectionText,
				"EventBridge::SyntheticRunewordStyle EventBridge::ResolveSyntheticRunewordStyle(const RunewordRecipe& a_recipe)",
				"std::string_view EventBridge::DescribeSyntheticRunewordStyle(");
			if (!styleSection.has_value()) {
				std::cerr << "runeword_coverage_consistency: style section not found\n";
				return false;
			}
			const auto tuningSection = extractSection(
				*tuningText,
				"static constexpr std::array kRecipeTunings = {",
				"};");
			if (!tuningSection.has_value()) {
				std::cerr << "runeword_coverage_consistency: tuning section not found\n";
				return false;
			}
			const auto summarySection = extractSection(
				*summaryHeaderText,
				"inline constexpr RecipeKeyOverride kEffectSummaryOverrides[] = {",
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

		bool CheckAffixRegistryStateExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path repoRoot = testFile.parent_path().parent_path();
			const fs::path eventBridgeHeaderFile = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";
			const fs::path registryHeaderFile = repoRoot / "include" / "CalamityAffixes" / "AffixRegistryState.h";
			const fs::path indexingFile = repoRoot / "src" / "EventBridge.Config.IndexingShared.cpp";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto eventBridgeHeaderText = loadText(eventBridgeHeaderFile);
			if (!eventBridgeHeaderText.has_value()) {
				std::cerr << "affix_registry_state_extraction: failed to open header file: " << eventBridgeHeaderFile << "\n";
				return false;
			}

			const auto registryHeaderText = loadText(registryHeaderFile);
			if (!registryHeaderText.has_value()) {
				std::cerr << "affix_registry_state_extraction: failed to open registry header file: " << registryHeaderFile << "\n";
				return false;
			}

			const auto indexingText = loadText(indexingFile);
			if (!indexingText.has_value()) {
				std::cerr << "affix_registry_state_extraction: failed to open indexing source: " << indexingFile << "\n";
				return false;
			}

			if (eventBridgeHeaderText->find("#include \"CalamityAffixes/AffixRegistryState.h\"") == std::string::npos ||
				eventBridgeHeaderText->find("AffixRegistryState _affixRegistry{};") == std::string::npos ||
				eventBridgeHeaderText->find("std::unordered_map<std::string, std::size_t> _affixIndexById;") != std::string::npos ||
				eventBridgeHeaderText->find("std::unordered_map<std::uint64_t, std::size_t> _affixIndexByToken;") != std::string::npos ||
				eventBridgeHeaderText->find("std::unordered_set<std::string> _affixLabelSet;") != std::string::npos ||
				eventBridgeHeaderText->find("std::vector<std::size_t> _hitTriggerAffixIndices;") != std::string::npos ||
				eventBridgeHeaderText->find("std::vector<std::size_t> _lootWeaponAffixes;") != std::string::npos ||
				registryHeaderText->find("affixIndexById") == std::string::npos ||
				registryHeaderText->find("affixIndexByToken") == std::string::npos ||
				registryHeaderText->find("hitTriggerAffixIndices") == std::string::npos ||
				registryHeaderText->find("lootWeaponAffixes") == std::string::npos ||
				indexingText->find("_affixRegistry.affixIndexById") == std::string::npos ||
				indexingText->find("_affixRegistry.affixIndexByToken") == std::string::npos ||
				indexingText->find("_affixRegistry.hitTriggerAffixIndices") == std::string::npos ||
				indexingText->find("_affixRegistry.lootWeaponAffixes") == std::string::npos) {
				std::cerr << "affix_registry_state_extraction: affix registry extraction is incomplete\n";
				return false;
			}

			return true;
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

			const auto snapshotStructPos = source.find("struct LowHealthSnapshot");
			const auto snapshotBuildPos = source.find("const auto buildLowHealthSnapshot = [&]() noexcept");
			const auto snapshotUsePos = source.find("const auto lowHealthSnapshot = buildLowHealthSnapshot();");
			const auto gateCallPos = (snapshotUsePos == std::string::npos) ? std::string::npos : source.find("TryProcessTriggerAffix(", snapshotUsePos);
			const auto mapUpdatePos = source.find("StoreLowHealthTriggerSnapshot(a_lowHealthOwnerFormID, a_lowHealthCurrentPct);");

			if (snapshotStructPos == std::string::npos ||
				snapshotBuildPos == std::string::npos ||
				snapshotUsePos == std::string::npos ||
				gateCallPos == std::string::npos ||
				mapUpdatePos == std::string::npos ||
				!(snapshotStructPos < snapshotBuildPos && snapshotBuildPos < snapshotUsePos && snapshotUsePos < gateCallPos)) {
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
			const auto recipeUiCursorUpdatePos = recipeUiText->find("_runewordState.recipeCycleCursor = static_cast<std::uint32_t>(idx);");

			if (recipeEntriesText->find("const auto affixIt = _affixRegistry.affixIndexByToken.find(recipe.resultAffixToken);") == std::string::npos ||
				recipeEntriesText->find("affixIt == _affixRegistry.affixIndexByToken.end() || affixIt->second >= _affixes.size())") == std::string::npos ||
				recipeUiText->find("Runeword Recipe: runtime effect not available.") == std::string::npos ||
				recipeUiGuardPos == std::string::npos ||
				recipeUiCursorUpdatePos == std::string::npos ||
				recipeUiCursorUpdatePos < recipeUiGuardPos ||
				catalogText->find("const auto isEligible = [&](std::size_t a_idx)") == std::string::npos ||
				catalogText->find("if (!isEligible(cursor))") == std::string::npos ||
				catalogText->find("if (const auto affixIt = _affixRegistry.affixIndexByToken.find(recipe.resultAffixToken);") == std::string::npos ||
				baseSelectionText->find("const auto rwIt = _runewordState.recipeIndexByResultAffixToken.find(token);") == std::string::npos ||
				baseSelectionText->find("if (const auto affixIt = _affixRegistry.affixIndexByToken.find(recipe.resultAffixToken);") == std::string::npos ||
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
				tooltipSource.find("RunewordSummary::ResolveEffectSummaryKey") == std::string::npos ||
				tooltipSource.find("RunewordSummary::ActionSummaryTextByKey") == std::string::npos ||
				tooltipSource.find("a_affixId.rfind(\"rw_\", 0) == 0") == std::string::npos ||
				tooltipSource.find("a_affixId.rfind(\"runeword_\", 0) == 0") == std::string::npos ||
				tooltipSource.find("a_affixId.substr(a_affixId.size() - 5) != \"_auto\"") == std::string::npos ||
				tooltipSource.find("% chance to ") == std::string::npos ||
				tooltipSource.find("% 확률로 ") == std::string::npos ||
				tooltipSource.find("(Cooldown ") == std::string::npos ||
				tooltipSource.find("(쿨타임 ") == std::string::npos ||
				tooltipSource.find("(지속 ") == std::string::npos ||
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
				recipeUiSource.find("_runewordState.instanceStates.erase(selectedKey);") == std::string::npos) {
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

			if (panelSource.find("ResolveRunewordApplyBlockReason(*_runewordState.selectedBaseKey, *recipe)") == std::string::npos ||
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
				transmuteSource.find("const auto instanceKey = *_runewordState.selectedBaseKey;") == std::string::npos ||
				transmuteSource.find("ResolveRunewordApplyBlockReason(instanceKey, *recipe)") == std::string::npos ||
				transmuteSource.find("runeword result affix missing before transmute") == std::string::npos ||
				transmuteSource.find("note.append(BuildRunewordApplyBlockMessage(blockReason));") == std::string::npos ||
				transmuteSource.find("rollbackConsumedRunes") == std::string::npos ||
				transmuteSource.find("_runewordState.transmuteInProgress") == std::string::npos ||
				transmuteSource.find("Runeword: transmute already in progress.") == std::string::npos ||
				transmuteSource.find("if (!PlayerHasInstanceKey(instanceKey))") == std::string::npos ||
				transmuteSource.find("consume-postcheck-partial") == std::string::npos ||
				transmuteSource.find("ApplyRunewordResult(instanceKey, *recipe, &applyFailureReason)") == std::string::npos ||
				transmuteSource.find(": fragments restored.") == std::string::npos ||
				transmuteSource.find(": fragment rollback partial.") == std::string::npos) {
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

			if (reforgeSource.find("preservedRunewordToken") == std::string::npos ||
				reforgeSource.find("previousRegularSlots.RemoveToken(preservedRunewordToken)") == std::string::npos ||
				reforgeSource.find("rolled.PromoteTokenToPrimary(preservedRunewordToken)") == std::string::npos ||
				reforgeSource.find("_runewordState.instanceStates.erase(instanceKey);") == std::string::npos ||
				reforgeSource.find("IsLootObjectEligibleForAffixes(entry->object)") == std::string::npos ||
				reforgeSource.find("Reforge blocked: completed runeword base.") != std::string::npos) {
				std::cerr << "runeword_reforge_safety: completed-base reroll integration guard is missing\n";
				return false;
			}

			// Verify runeword token exclusion from regular roll pool.
			if (reforgeSource.find("_affixes[*idx].token == preservedRunewordToken") == std::string::npos) {
				std::cerr << "runeword_reforge_safety: runeword token exclusion from regular roll pool is missing\n";
				return false;
			}

			// Verify final safety net for runeword token survival.
			if (reforgeSource.find("newSlots.HasToken(preservedRunewordToken)") == std::string::npos) {
				std::cerr << "runeword_reforge_safety: post-roll runeword token verification is missing\n";
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
			const fs::path prismaPanelDataFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.PanelData.inl";
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
			// PrismaTooltip.cpp includes PanelData.inl — concatenate for pattern matching.
			std::string coreTextCombined;
			for (const auto& sf : { prismaCoreFile, prismaPanelDataFile }) {
				const auto part = loadText(sf);
				if (!part.has_value()) {
					std::cerr << "prisma_tooltip_refresh: failed to open prisma source: " << sf << "\n";
					return false;
				}
				coreTextCombined += *part;
			}
			const auto coreText = std::optional<std::string>(std::move(coreTextCombined));
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
				coreText->find("void ClearSelectedTooltipFromView(bool a_force, bool a_selectionChanged)") == std::string::npos ||
				coreText->find("ClearSelectedTooltipFromView(a_force, selectionChanged);") == std::string::npos ||
				coreText->find("if (a_force || selectionChanged || next != g_lastTooltip)") == std::string::npos ||
				coreText->find("kInteropSetRunewordAffixPreview") == std::string::npos ||
				(handleText->find("SetRunewordAffixPreview(") == std::string::npos &&
				 handleText->find("RefreshRunewordPanelBindings(*bridge);") == std::string::npos) ||
				uiText->find("PrismaUI_Interop(prismaInteropMethod.runewordAffixPreview") == std::string::npos ||
				uiText->find("function setRunewordAffixPreview(raw)") == std::string::npos ||
				runtimeText->find("std::uint64_t a_preferredInstanceKey") == std::string::npos ||
				runtimeText->find("if (a_preferredInstanceKey != 0u)") == std::string::npos ||
				runtimeText->find("a_candidate.instanceKey == a_preferredInstanceKey") == std::string::npos ||
				panelStateText->find("GetInstanceAffixTooltip(") == std::string::npos ||
				panelStateText->find("*_runewordState.selectedBaseKey") == std::string::npos) {
				std::cerr << "prisma_tooltip_refresh: immediate tooltip refresh guard is missing for runeword actions\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaPanelUiBootstrapExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
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

			const auto uiText = loadText(prismaUiFile);
			if (!uiText.has_value()) {
				std::cerr << "prisma_panel_ui_bootstrap: failed to open ui source: " << prismaUiFile << "\n";
				return false;
			}

			const auto extractJsFunctionBody = [&](std::string_view a_signature) -> std::optional<std::string_view> {
				const auto start = uiText->find(a_signature);
				if (start == std::string::npos) {
					return std::nullopt;
				}

				const auto bodyStart = uiText->find('{', start);
				if (bodyStart == std::string::npos) {
					return std::nullopt;
				}

				const auto nextFunction = uiText->find("\n      function ", bodyStart + 1);
				if (nextFunction == std::string::npos) {
					return std::string_view(*uiText).substr(bodyStart + 1);
				}

				return std::string_view(*uiText).substr(bodyStart + 1, nextFunction - bodyStart - 1);
			};

			const auto setControlPanelBody = extractJsFunctionBody("function setControlPanel(raw)");
			const auto setPanelLayoutBody = extractJsFunctionBody("function setPanelLayout(raw)");
			const auto setTooltipLayoutBody = extractJsFunctionBody("function setTooltipLayout(raw)");
			const auto setInventoryItemsBody = extractJsFunctionBody("function setInventoryItems(raw)");
			const auto setRecipeItemsBody = extractJsFunctionBody("function setRecipeItems(raw)");
			const auto setRunewordPanelStateBody = extractJsFunctionBody("function setRunewordPanelState(raw)");
			const auto setTooltipBody = extractJsFunctionBody("function setTooltip(raw)");
			const auto renderInventoryItemsBody = extractJsFunctionBody("function renderInventoryItems()");
			const auto renderRecipeItemsBody = extractJsFunctionBody("function renderRecipeItems()");
			const auto wireButtonsBody = extractJsFunctionBody("function wireButtons()");
			const auto registerInteropBody = extractJsFunctionBody("function registerPrismaInteropHandlers()");
			const auto registerFallbackBody = extractJsFunctionBody("function registerWindowInteropFallbacks()");

			const auto interopArrayPos = uiText->find("function parseInteropArrayPayload(raw)");
			const auto interopObjectPos = uiText->find("function parseInteropObjectPayload(raw)");
			const auto controlPanelParsePos = uiText->find("function parseControlPanelOpenState(raw)");
			const auto controlPanelApplyPos = uiText->find("function applyControlPanelOpenState(nextOpen)");
			const auto interopRegisterPos = uiText->find("function registerPrismaInteropHandlers()");
			const auto fallbackRegisterPos = uiText->find("function registerWindowInteropFallbacks()");
			const auto eventRegisterPos = uiText->find("function registerPanelEventHandlers()");
			const auto bootstrapPos = uiText->find("function initializeCalamityPanel()");
			const auto bootstrapCallPos = uiText->find("initializeCalamityPanel();");

			if (interopArrayPos == std::string::npos ||
				interopObjectPos == std::string::npos ||
				controlPanelParsePos == std::string::npos ||
				controlPanelApplyPos == std::string::npos ||
				uiText->find("const panelRenderSection = Object.freeze({") == std::string::npos ||
				uiText->find("const prismaInteropMethod = Object.freeze({") == std::string::npos ||
				uiText->find("function flushPanelRender()") == std::string::npos ||
				uiText->find("function schedulePanelRender(...a_sections)") == std::string::npos ||
				uiText->find("function shouldInvalidatePreviewForCommand(command)") == std::string::npos ||
				uiText->find("function resolvePreviewPendingStateForCommand(command)") == std::string::npos ||
				uiText->find("function dispatchPanelCommand(button)") == std::string::npos ||
				uiText->find("function handleDelegatedPanelCommandClick(event)") == std::string::npos ||
				interopRegisterPos == std::string::npos ||
				fallbackRegisterPos == std::string::npos ||
				eventRegisterPos == std::string::npos ||
				bootstrapPos == std::string::npos ||
				bootstrapCallPos == std::string::npos ||
				uiText->find("const items = parseInteropArrayPayload(raw);") == std::string::npos ||
				uiText->find("recipeItemsState = parseInteropArrayPayload(raw);") == std::string::npos ||
				uiText->find("const data = parseInteropObjectPayload(raw) || {};") == std::string::npos ||
				!setInventoryItemsBody.has_value() ||
				setInventoryItemsBody->find("schedulePanelRender(") == std::string::npos ||
				setInventoryItemsBody->find("renderInventoryItems();") != std::string::npos ||
				!setRecipeItemsBody.has_value() ||
				setRecipeItemsBody->find("schedulePanelRender(") == std::string::npos ||
				setRecipeItemsBody->find("renderRecipeItems();") != std::string::npos ||
				!setRunewordPanelStateBody.has_value() ||
				setRunewordPanelStateBody->find("schedulePanelRender(") == std::string::npos ||
				setRunewordPanelStateBody->find("renderRunewordPanelState();") != std::string::npos ||
				!setTooltipBody.has_value() ||
				setTooltipBody->find("schedulePanelRender(panelRenderSection.tooltipPlacement);") == std::string::npos ||
				!setControlPanelBody.has_value() ||
				setControlPanelBody->find("applyControlPanelOpenState(parseControlPanelOpenState(raw));") == std::string::npos ||
				!setPanelLayoutBody.has_value() ||
				setPanelLayoutBody->find("const data = parseInteropObjectPayload(raw);") == std::string::npos ||
				!setTooltipLayoutBody.has_value() ||
				setTooltipLayoutBody->find("const data = parseInteropObjectPayload(raw);") == std::string::npos ||
				!renderInventoryItemsBody.has_value() ||
				renderInventoryItemsBody->find("button.setAttribute(panelCommandAttribute, `runeword.base.select:${key}`);") == std::string::npos ||
				renderInventoryItemsBody->find("button.addEventListener(\"click\"") != std::string::npos ||
				!renderRecipeItemsBody.has_value() ||
				renderRecipeItemsBody->find("button.setAttribute(panelCommandAttribute, `runeword.recipe.select:${token}`);") == std::string::npos ||
				renderRecipeItemsBody->find("button.addEventListener(\"click\"") != std::string::npos ||
				!wireButtonsBody.has_value() ||
				wireButtonsBody->find("document.addEventListener(\"click\", handleDelegatedPanelCommandClick);") == std::string::npos ||
				!registerInteropBody.has_value() ||
				registerInteropBody->find("PrismaUI_Interop(prismaInteropMethod.tooltip, (data) => setTooltip(data));") == std::string::npos ||
				!registerFallbackBody.has_value() ||
				registerFallbackBody->find("window[prismaInteropMethod.tooltip] = setTooltip;") == std::string::npos ||
				uiText->find("window.addEventListener(\"keydown\", handlePanelKeydown);") == std::string::npos) {
				std::cerr << "prisma_panel_ui_bootstrap: expected ui bootstrap extraction markers are missing\n";
				return false;
			}

			if (!(interopRegisterPos < fallbackRegisterPos &&
					fallbackRegisterPos < eventRegisterPos &&
					eventRegisterPos < bootstrapPos &&
					bootstrapPos < bootstrapCallPos)) {
				std::cerr << "prisma_panel_ui_bootstrap: bootstrap helper ordering is inconsistent\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaPanelDataExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path prismaPanelDataFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.PanelData.inl";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto panelDataText = loadText(prismaPanelDataFile);
			if (!panelDataText.has_value()) {
				std::cerr << "prisma_panel_data_extraction: failed to open panel data source: " << prismaPanelDataFile << "\n";
				return false;
			}

			if (panelDataText->find("bool TryReadSelectedItemContextFromMenus(") == std::string::npos ||
				panelDataText->find("void ClearSelectedTooltipFromView(bool a_force, bool a_selectionChanged)") == std::string::npos ||
				panelDataText->find("if (TryReadSelectedItemContextFromMenus(result, *ui, *bridge)) {") == std::string::npos ||
				panelDataText->find("ClearSelectedTooltipFromView(a_force, selectionChanged);") == std::string::npos) {
				std::cerr << "prisma_panel_data_extraction: panel data helper extraction markers are missing\n";
				return false;
			}

			if (panelDataText->find("const auto clearTooltip = [&]() {") != std::string::npos) {
				std::cerr << "prisma_panel_data_extraction: inline tooltip clear lambda still exists in panel data source\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaPanelRenderViewModelExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
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

			const auto uiText = loadText(prismaUiFile);
			if (!uiText.has_value()) {
				std::cerr << "prisma_panel_render_view_model: failed to open ui source: " << prismaUiFile << "\n";
				return false;
			}

			const auto extractJsFunctionBody = [&](std::string_view a_signature) -> std::optional<std::string_view> {
				const auto start = uiText->find(a_signature);
				if (start == std::string::npos) {
					return std::nullopt;
				}

				const auto bodyStart = uiText->find('{', start);
				if (bodyStart == std::string::npos) {
					return std::nullopt;
				}

				const auto nextFunction = uiText->find("\n      function ", bodyStart + 1);
				if (nextFunction == std::string::npos) {
					return std::string_view(*uiText).substr(bodyStart + 1);
				}

				return std::string_view(*uiText).substr(bodyStart + 1, nextFunction - bodyStart - 1);
			};

			const auto selectedBody = extractJsFunctionBody("function renderSelectedItemContext()");
			const auto inventoryBody = extractJsFunctionBody("function renderInventoryItems()");
			const auto recipeBody = extractJsFunctionBody("function renderRecipeItems()");
			const auto runewordBody = extractJsFunctionBody("function renderRunewordPanelState()");

			if (uiText->find("function resolveSelectedItemContextViewModel()") == std::string::npos ||
				uiText->find("function resolveInventoryListViewModel()") == std::string::npos ||
				uiText->find("function resolveRecipeSearchDocument(item)") == std::string::npos ||
				uiText->find("function resolveRecipeListViewModel()") == std::string::npos ||
				uiText->find("function resolveRunewordPanelActionState(state)") == std::string::npos ||
				!selectedBody.has_value() ||
				selectedBody->find("const viewModel = resolveSelectedItemContextViewModel();") == std::string::npos ||
				!inventoryBody.has_value() ||
				inventoryBody->find("const viewModel = resolveInventoryListViewModel();") == std::string::npos ||
				!recipeBody.has_value() ||
				recipeBody->find("const viewModel = resolveRecipeListViewModel();") == std::string::npos ||
				!runewordBody.has_value() ||
				runewordBody->find("const actionState = resolveRunewordPanelActionState(state);") == std::string::npos) {
				std::cerr << "prisma_panel_render_view_model: render view-model extraction markers are missing\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaPanelUxFlowPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
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

			const auto uiText = loadText(prismaUiFile);
			if (!uiText.has_value()) {
				std::cerr << "prisma_panel_ux_flow: failed to open ui source: " << prismaUiFile << "\n";
				return false;
			}

			const auto extractJsFunctionBody = [&](std::string_view a_signature) -> std::optional<std::string_view> {
				const auto start = uiText->find(a_signature);
				if (start == std::string::npos) {
					return std::nullopt;
				}

				const auto bodyStart = uiText->find('{', start);
				if (bodyStart == std::string::npos) {
					return std::nullopt;
				}

				const auto nextFunction = uiText->find("\n      function ", bodyStart + 1);
				if (nextFunction == std::string::npos) {
					return std::string_view(*uiText).substr(bodyStart + 1);
				}

				return std::string_view(*uiText).substr(bodyStart + 1, nextFunction - bodyStart - 1);
			};

			const auto extractElementOpenTagById = [&](std::string_view a_id) -> std::optional<std::string_view> {
				const std::string marker = std::string("id=\"") + std::string(a_id) + "\"";
				const auto start = uiText->find(marker);
				if (start == std::string::npos) {
					return std::nullopt;
				}

				const auto tagStart = uiText->rfind('<', start);
				if (tagStart == std::string::npos) {
					return std::nullopt;
				}

				const auto tagEnd = uiText->find('>', start);
				if (tagEnd == std::string::npos) {
					return std::nullopt;
				}

				return std::string_view(*uiText).substr(tagStart, tagEnd - tagStart + 1);
			};

			const auto tagHasClassToken = [](std::string_view a_tag, std::string_view a_token) {
				const auto classStart = a_tag.find("class=\"");
				if (classStart == std::string_view::npos) {
					return false;
				}

				const auto valueStart = classStart + 7;
				const auto valueEnd = a_tag.find('"', valueStart);
				if (valueEnd == std::string_view::npos) {
					return false;
				}

				const auto classValue = a_tag.substr(valueStart, valueEnd - valueStart);
				std::string needle = std::string(" ") + std::string(a_token) + " ";
				std::string haystack = std::string(" ") + std::string(classValue) + " ";
				return haystack.find(needle) != std::string::npos;
			};

			const auto runewordBody = extractJsFunctionBody("function renderRunewordPanelState()");
			const auto selectedBody = extractJsFunctionBody("function renderSelectedItemContext()");
			const auto tooltipPlacementBody = extractJsFunctionBody("function applyTooltipPlacement()");
			const auto openStateBody = extractJsFunctionBody("function applyControlPanelOpenState(nextOpen)");
			const auto controlPanelTag = extractElementOpenTagById("controlPanel");
			const auto progressTag = extractElementOpenTagById("runewordFlowProgress");
			const auto baseStepTag = extractElementOpenTagById("runewordBaseStep");
			const auto recipeStepTag = extractElementOpenTagById("runewordRecipeStep");
			const auto actionStepTag = extractElementOpenTagById("runewordActionStep");
			const auto recipeContextTag = extractElementOpenTagById("runewordContextRecipeName");
			const auto baseChooserDetailsTag = extractElementOpenTagById("runewordBaseChooserDetails");
			const auto cubeDetailsTag = extractElementOpenTagById("runewordCubeDetails");
			const auto baseListTag = extractElementOpenTagById("inventoryBaseList");
			const auto recipeListTag = extractElementOpenTagById("recipeList");
			const auto cubeGridTag = extractElementOpenTagById("runewordCubeGrid");
			const auto actionDetailsTag = extractElementOpenTagById("runewordActionDetails");
			const auto insertButtonTag = extractElementOpenTagById("runewordInsertButton");
			const auto debugPanelTag = extractElementOpenTagById("debugToolsPanel");
			const auto debugSummaryTag = extractElementOpenTagById("debugSectionSummary");

			if (uiText->find(":where(.tpPill, .qpPill, .cpIconButton, .cpTabButton, .cpButton, .cpListItem, .rwSearchInput, .cpDangerSummary):focus-visible") == std::string::npos ||
				!controlPanelTag.has_value() ||
				controlPanelTag->find("role=\"dialog\"") == std::string::npos ||
				uiText->find("class=\"rwWorkspace\"") == std::string::npos ||
				uiText->find("class=\"rwContextBar\"") == std::string::npos ||
				uiText->find("class=\"rwWorkbench\"") == std::string::npos ||
				uiText->find("overflow-y: auto;") == std::string::npos ||
				!progressTag.has_value() ||
				!tagHasClassToken(*progressTag, "rwProgressStrip") ||
				!baseStepTag.has_value() ||
				!tagHasClassToken(*baseStepTag, "cpStepCard") ||
				!tagHasClassToken(*baseStepTag, "rwBaseRail") ||
				!tagHasClassToken(*baseStepTag, "active") ||
				!recipeStepTag.has_value() ||
				!tagHasClassToken(*recipeStepTag, "cpStepCard") ||
				!tagHasClassToken(*recipeStepTag, "rwRecipeExplorer") ||
				!tagHasClassToken(*recipeStepTag, "muted") ||
				!actionStepTag.has_value() ||
				!tagHasClassToken(*actionStepTag, "cpStepCard") ||
				!tagHasClassToken(*actionStepTag, "cpActionCard") ||
				!tagHasClassToken(*actionStepTag, "rwActionInspector") ||
				!tagHasClassToken(*actionStepTag, "muted") ||
				!recipeContextTag.has_value() ||
				!baseChooserDetailsTag.has_value() ||
				!tagHasClassToken(*baseChooserDetailsTag, "rwRailDetails") ||
				!cubeDetailsTag.has_value() ||
				!tagHasClassToken(*cubeDetailsTag, "rwRailDetails") ||
				!baseListTag.has_value() ||
				!tagHasClassToken(*baseListTag, "rwBaseQuickList") ||
				!recipeListTag.has_value() ||
				!tagHasClassToken(*recipeListTag, "rwRecipeExplorerList") ||
				!cubeGridTag.has_value() ||
				!tagHasClassToken(*cubeGridTag, "compact") ||
				!actionDetailsTag.has_value() ||
				!tagHasClassToken(*actionDetailsTag, "rwActionDetails") ||
				!insertButtonTag.has_value() ||
				!tagHasClassToken(*insertButtonTag, "cpButton") ||
				!tagHasClassToken(*insertButtonTag, "cpPrimaryButton") ||
				!debugPanelTag.has_value() ||
				!tagHasClassToken(*debugPanelTag, "cpDangerDetails") ||
				!debugSummaryTag.has_value() ||
				!tagHasClassToken(*debugSummaryTag, "cpDangerSummary") ||
				uiText->find("function appendEmptyState(node, title, body, hint = \"\")") == std::string::npos ||
				uiText->find("class=\"cpAffixPane\"") == std::string::npos ||
				uiText->find("id=\"affixSelectedItemLabel\"") == std::string::npos ||
				uiText->find("id=\"panelTooltipLead\"") == std::string::npos ||
				uiText->find("function triggerRunewordStateShift(element, state)") == std::string::npos ||
				uiText->find("@keyframes rwStateShift") == std::string::npos ||
				uiText->find("@keyframes rwPrimaryPulse") == std::string::npos ||
				uiText->find("function renderRunewordFlowProgress(actionState, state)") == std::string::npos ||
				uiText->find("-webkit-line-clamp: 2;") == std::string::npos ||
				uiText->find("runewordActionDetailsSummary") == std::string::npos ||
				uiText->find("runewordBaseChooserSummary") == std::string::npos ||
				uiText->find("runewordCubeDetailsSummary") == std::string::npos ||
				uiText->find("rwRecipeListItem") == std::string::npos ||
				!selectedBody.has_value() ||
				selectedBody->find("affixSelectedItemName.textContent = viewModel.hasSelection") == std::string::npos ||
				uiText->find("button.title = name;") == std::string::npos ||
				uiText->find("button.setAttribute(\"aria-label\", name);") == std::string::npos ||
				!runewordBody.has_value() ||
				runewordBody->find("runewordContextRecipeName.textContent") == std::string::npos ||
				runewordBody->find("runewordContextRecipeMeta.textContent") == std::string::npos ||
				runewordBody->find("renderRunewordFlowProgress(actionState, state);") == std::string::npos ||
				uiText->find("runewordInsertButton.classList.toggle(\"attention\", canTransmute);") == std::string::npos ||
				runewordBody->find("runewordActionHint.textContent = actionState.buttonHint;") == std::string::npos ||
				!tooltipPlacementBody.has_value() ||
				tooltipPlacementBody->find("appendEmptyState(") == std::string::npos ||
				uiText->find("function focusCurrentMainTab()") == std::string::npos ||
				!openStateBody.has_value() ||
				openStateBody->find("focusCurrentMainTab();") == std::string::npos) {
				std::cerr << "prisma_panel_ux_flow: ux hierarchy/focus markers are missing\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaPanelCommandRoutingExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path prismaHandleFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.HandleUiCommand.inl";

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
				std::cerr << "prisma_panel_command_routing: failed to open handle source: " << prismaHandleFile << "\n";
				return false;
			}

			const auto handleUiCommandPos = handleText->find("void HandleUiCommand(std::string_view a_command)");
			if (handleUiCommandPos == std::string::npos) {
				std::cerr << "prisma_panel_command_routing: HandleUiCommand entrypoint is missing\n";
				return false;
			}

			const std::string_view handleUiCommandBody = std::string_view(*handleText).substr(handleUiCommandPos);

			if (handleText->find("bool HandleStructuredUiCommand(std::string_view a_command)") == std::string::npos ||
				handleText->find("void HandleEventBackedUiCommand(std::string_view a_command)") == std::string::npos ||
				handleText->find("HandlePanelVisibilityCommand(a_command) ||") == std::string::npos ||
				handleText->find("HandleImmediateRunewordCommand(a_command);") == std::string::npos ||
				handleText->find("if (HandleStructuredUiCommand(a_command)) {") == std::string::npos ||
				handleText->find("HandleEventBackedUiCommand(a_command);") == std::string::npos ||
				handleUiCommandBody.find("HandlePanelVisibilityCommand(a_command)") != std::string::npos ||
				handleUiCommandBody.find("HandleRunewordSelectionCommand(a_command)") != std::string::npos ||
				handleUiCommandBody.find("HandleLayoutSaveCommand(a_command)") != std::string::npos ||
				handleUiCommandBody.find("HandleImmediateRunewordCommand(a_command)") != std::string::npos) {
				std::cerr << "prisma_panel_command_routing: command routing extraction markers are missing\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaSettingsLayoutExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path settingsFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.SettingsLayout.inl";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto settingsText = loadText(settingsFile);
			if (!settingsText.has_value()) {
				std::cerr << "prisma_settings_layout_extraction: failed to open settings source: " << settingsFile << "\n";
				return false;
			}

			if (settingsText->find("bool TryReadPanelHotkeyFromMcmJson(std::istream& a_in, std::uint32_t& a_outResolvedKey)") == std::string::npos ||
				settingsText->find("bool TryResolveUiLanguageSettingsPath(std::filesystem::path& a_outPath)") == std::string::npos ||
				settingsText->find("bool TryReadUiLanguageModeFromIni(std::istream& a_in, int& a_outResolvedMode)") == std::string::npos ||
				settingsText->find("void MarkLayoutStateLoadedForPush(") == std::string::npos ||
				settingsText->find("std::optional<PanelLayout> TryLoadPanelLayoutStateFromDisk()") == std::string::npos ||
				settingsText->find("std::optional<TooltipLayout> TryLoadTooltipLayoutStateFromDisk()") == std::string::npos ||
				settingsText->find("void PushLayoutStateJsonToJs(") == std::string::npos ||
				settingsText->find("(void)TryReadPanelHotkeyFromMcmJson(in, resolvedKey);") == std::string::npos ||
				settingsText->find("(void)TryResolveUiLanguageSettingsPath(activePath);") == std::string::npos ||
				settingsText->find("(void)TryReadUiLanguageModeFromIni(in, resolvedMode);") == std::string::npos ||
				settingsText->find("MarkLayoutStateLoadedForPush(g_panelLayoutState, *loaded);") == std::string::npos ||
				settingsText->find("MarkLayoutStateLoadedForPush(g_tooltipLayoutState, *loaded);") == std::string::npos ||
				settingsText->find("PushLayoutStateJsonToJs(") == std::string::npos) {
				std::cerr << "prisma_settings_layout_extraction: settings/layout helper extraction markers are missing\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaTooltipWorkerSchedulingPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path prismaCoreFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.cpp";
			const fs::path prismaMenuInputFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.MenuInput.inl";
			const fs::path prismaPanelDataFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.PanelData.inl";
			const fs::path prismaViewLifecycleFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.ViewLifecycle.inl";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			// PrismaTooltip.cpp includes the PrismaTooltip*.inl files — concatenate the relevant ones for pattern matching.
			std::string coreTextCombined;
			for (const auto& sf : { prismaCoreFile, prismaMenuInputFile, prismaPanelDataFile, prismaViewLifecycleFile }) {
				const auto part = loadText(sf);
				if (!part.has_value()) {
					std::cerr << "prisma_worker_scheduling: failed to open prisma source: " << sf << "\n";
					return false;
				}
				coreTextCombined += *part;
			}
			const auto coreText = std::optional<std::string>(std::move(coreTextCombined));

			if (coreText->find("g_worker = std::jthread([](std::stop_token stopToken) {") == std::string::npos ||
				coreText->find("std::this_thread::sleep_for(kPollInterval);") == std::string::npos ||
				coreText->find("if (g_gatePollingOnMenus.load() &&") == std::string::npos ||
				coreText->find("g_relevantMenusOpen.load() <= 0") == std::string::npos ||
				coreText->find("!g_controlPanelOpen.load())") == std::string::npos ||
				coreText->find("if (auto* tasks = SKSE::GetTaskInterface()) {") == std::string::npos ||
				coreText->find("tasks->AddTask([]() {") == std::string::npos ||
				coreText->find("Tick();") == std::string::npos ||
				coreText->find("void ClearSelectedTooltipFromView(bool a_force, bool a_selectionChanged)") == std::string::npos ||
				coreText->find("StripRunewordOverlayTooltipLines(*selected.tooltip)") == std::string::npos) {
				std::cerr << "prisma_worker_scheduling: worker tick scheduling or tooltip-strip guards are missing\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaTooltipLifecycleExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path prismaCoreFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.cpp";
			const fs::path prismaMenuInputFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.MenuInput.inl";
			const fs::path prismaSettingsFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.SettingsLayout.inl";
			const fs::path prismaViewLifecycleFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.ViewLifecycle.inl";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			const auto coreText = loadText(prismaCoreFile);
			const auto menuText = loadText(prismaMenuInputFile);
			const auto settingsText = loadText(prismaSettingsFile);
			const auto lifecycleText = loadText(prismaViewLifecycleFile);
			if (!coreText.has_value() || !menuText.has_value() || !settingsText.has_value() || !lifecycleText.has_value()) {
				std::cerr << "prisma_lifecycle_extraction: failed to open prisma lifecycle sources\n";
				return false;
			}

			if (coreText->find("#include \"PrismaTooltip.MenuInput.inl\"") == std::string::npos ||
				coreText->find("#include \"PrismaTooltip.ViewLifecycle.inl\"") == std::string::npos ||
				coreText->find("constexpr auto kPrismaCommandListener = \"calamityCommand\";") == std::string::npos ||
				coreText->find("constexpr auto kInteropSetTooltip = \"setTooltip\";") == std::string::npos ||
				menuText->find("class PanelHotkeyEventSink final") == std::string::npos ||
				menuText->find("class MenuEventSink final") == std::string::npos ||
				settingsText->find("kInteropSetPanelLayout") == std::string::npos ||
				settingsText->find("kInteropSetTooltipLayout") == std::string::npos ||
				lifecycleText->find("void StartPrismaWorker()") == std::string::npos ||
				lifecycleText->find("void RegisterPrismaEventSinks()") == std::string::npos ||
				lifecycleText->find("void EnsurePrisma()") == std::string::npos ||
				lifecycleText->find("RegisterJSListener(g_view, kPrismaCommandListener") == std::string::npos) {
				std::cerr << "prisma_lifecycle_extraction: expected lifecycle/menu extraction markers are missing\n";
				return false;
			}

			if (coreText->find("class PanelHotkeyEventSink final") != std::string::npos ||
				coreText->find("g_worker = std::jthread") != std::string::npos) {
				std::cerr << "prisma_lifecycle_extraction: core prisma file still owns extracted lifecycle/menu bodies\n";
				return false;
			}

			return true;
		}

		bool CheckPrismaTooltipTelemetryPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path prismaCoreFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.cpp";
			const fs::path prismaTickFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.Tick.inl";
			const fs::path prismaHandleFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.HandleUiCommand.inl";
			const fs::path prismaPanelDataFile = testFile.parent_path().parent_path() / "src" / "PrismaTooltip.PanelData.inl";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
				if (!in.is_open()) {
					return std::nullopt;
				}
				return std::string(
					(std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
			};

			std::string combinedCore;
			for (const auto& sf : { prismaCoreFile, prismaTickFile, prismaPanelDataFile, prismaHandleFile }) {
				const auto part = loadText(sf);
				if (!part.has_value()) {
					std::cerr << "prisma_telemetry: failed to open prisma source: " << sf << "\n";
					return false;
				}
				combinedCore += *part;
			}

			if (combinedCore.find("struct PrismaTelemetryCounters") == std::string::npos ||
				combinedCore.find("std::atomic<std::uint64_t> workerEnqueues") == std::string::npos ||
				combinedCore.find("std::atomic<std::uint64_t> tooltipClears") == std::string::npos ||
				combinedCore.find("void ClearTooltipViewState(bool a_clearRunewordPanelData);") == std::string::npos ||
				combinedCore.find("void RefreshRunewordPanelBindings(CalamityAffixes::EventBridge& a_bridge);") == std::string::npos ||
				combinedCore.find("void MaybeLogPrismaTelemetry(std::chrono::steady_clock::time_point a_now, bool a_uiActive);") == std::string::npos ||
				combinedCore.find("g_prismaTelemetry.tickRuns.fetch_add(1u") == std::string::npos ||
				combinedCore.find("g_prismaTelemetry.uiCommands.fetch_add(1u") == std::string::npos ||
				combinedCore.find("g_prismaTelemetry.tooltipPushes.fetch_add(1u") == std::string::npos ||
				combinedCore.find("RefreshRunewordPanelBindings(*bridge);") == std::string::npos ||
				combinedCore.find("ClearTooltipViewState(true);") == std::string::npos ||
				combinedCore.find("ClearTooltipViewState(false);") == std::string::npos ||
				combinedCore.find("MaybeLogPrismaTelemetry(now, true);") == std::string::npos ||
				combinedCore.find("SKSE::log::debug(\n\t\t\t\t\"CalamityAffixes: Prisma telemetry") == std::string::npos &&
				combinedCore.find("SKSE::log::debug(\"CalamityAffixes: Prisma telemetry") == std::string::npos) {
				std::cerr << "prisma_telemetry: tooltip telemetry/helper extraction is incomplete\n";
				return false;
			}

			return true;
		}

}
