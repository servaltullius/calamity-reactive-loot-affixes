#include "runtime_gate_store_checks_common.h"

namespace RuntimeGateStoreChecks
{
	bool CheckRunewordTooltipOverlayPolicy()
	{
		if (CalamityAffixes::ShouldShowRunewordTooltipInItemOverlay()) {
			std::cerr << "tooltip_policy: runeword text must stay panel-only\n";
			return false;
		}

		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path runtimeTooltipFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.TooltipResolution.cpp";

		std::ifstream in(runtimeTooltipFile);
		if (!in.is_open()) {
			std::cerr << "tooltip_policy: failed to open runtime tooltip source: " << runtimeTooltipFile << "\n";
			return false;
		}

		std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("BuildRunewordTooltip(") != std::string::npos) {
			std::cerr << "tooltip_policy: runtime tooltip source still references runeword tooltip builder\n";
			return false;
		}

		return true;
	}

	bool CheckLootPreviewRuntimePolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path assignFile = repoRoot / "src" / "EventBridge.Loot.Assign.cpp";
		const fs::path previewStoreFile = repoRoot / "src" / "EventBridge.Loot.PreviewClaimStore.cpp";
		const fs::path affixSlotRollFile = repoRoot / "src" / "EventBridge.Loot.AffixSlotRoll.cpp";
		const fs::path lootFile = repoRoot / "src" / "EventBridge.Loot.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto assignText = loadText(assignFile);
		if (!assignText.has_value()) {
			std::cerr << "loot_preview_policy: failed to open assign source: " << assignFile << "\n";
			return false;
		}
			if (assignText->find("no random affix assignment on pickup") == std::string::npos ||
				assignText->find("!a_allowRunewordFragmentRoll || a_count <= 0") == std::string::npos ||
				assignText->find("ResolveLootCurrencySourceTier(") == std::string::npos ||
				assignText->find("_loot.bossContainerEditorIdAllowContains") == std::string::npos ||
				assignText->find("_loot.lootSourceChanceMultBossContainer") == std::string::npos ||
				assignText->find("MaybeGrantRandomRunewordFragment(sourceChanceMultiplier);") == std::string::npos ||
				assignText->find("MaybeGrantRandomReforgeOrb(sourceChanceMultiplier);") == std::string::npos ||
				assignText->find("const std::uint32_t rollCount = 1u;") == std::string::npos ||
				assignText->find("allowLegacyPickupAffixRoll") != std::string::npos) {
			std::cerr << "loot_preview_policy: pickup flow must remain orb/fragment-only, include source weighting, and avoid legacy affix roll branch\n";
			return false;
		}

		if (CalamityAffixes::ShouldEnableSyntheticLootPreviewTooltip()) {
			std::cerr << "loot_preview_policy: synthetic preview rollout must remain disabled by policy helper\n";
			return false;
		}
		const auto previewStoreText = loadText(previewStoreFile);
		if (!previewStoreText.has_value()) {
			std::cerr << "loot_preview_policy: failed to open preview store source: " << previewStoreFile << "\n";
			return false;
		}
		const auto affixSlotRollText = loadText(affixSlotRollFile);
		if (!affixSlotRollText.has_value()) {
			std::cerr << "loot_preview_policy: failed to open affix-slot roll source: " << affixSlotRollFile << "\n";
			return false;
		}
		if (previewStoreText->find("void EventBridge::RememberLootPreviewSlots(") == std::string::npos ||
			previewStoreText->find("const InstanceAffixSlots* EventBridge::FindLootPreviewSlots(") == std::string::npos ||
			affixSlotRollText->find("std::optional<InstanceAffixSlots> EventBridge::BuildLootPreviewAffixSlots(") == std::string::npos ||
			affixSlotRollText->find("std::uint64_t EventBridge::HashLootPreviewSeed(") == std::string::npos) {
			std::cerr << "loot_preview_policy: loot runtime modular split guards are missing\n";
			return false;
		}

		const auto lootText = loadText(lootFile);
		if (!lootText.has_value()) {
			std::cerr << "loot_preview_policy: failed to open loot event source: " << lootFile << "\n";
			return false;
		}
		if (lootText->find("ProcessLootAcquired(baseObj, count, uid, oldContainer, allowRunewordFragmentRoll)") == std::string::npos ||
			lootText->find("_loot.chancePercent <= 0.0f") != std::string::npos) {
			std::cerr << "loot_preview_policy: pickup event path must always dispatch loot processing independent of loot chance\n";
			return false;
		}

		return true;
	}

	bool CheckLootRerollExploitGuardPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path lootFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Loot.cpp";

		std::ifstream in(lootFile);
		if (!in.is_open()) {
			std::cerr << "loot_reroll_exploit_guard: failed to open loot source: " << lootFile << "\n";
			return false;
		}

		std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("IsLootSourceCorpseChestOrWorld(") == std::string::npos ||
			source.find("_lootRerollGuard.ConsumeIfPlayerDropPickup(") == std::string::npos ||
			source.find("_playerContainerStash[stashKey] += a_event->itemCount;") == std::string::npos ||
			source.find("skipping loot roll (player dropped + re-picked)") == std::string::npos ||
			source.find("skipping loot roll (player stashed + retrieved)") == std::string::npos) {
			std::cerr << "loot_reroll_exploit_guard: anti-reroll pickup guards are missing\n";
			return false;
		}

		return true;
	}

	bool CheckLootChanceMcmCleanupPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path triggerEventsFile = repoRoot / "src" / "EventBridge.Triggers.Events.cpp";
		const fs::path configFile = repoRoot / "src" / "EventBridge.Config.cpp";
		const fs::path headerFile = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto headerText = loadText(headerFile);
		if (!headerText.has_value()) {
			std::cerr << "loot_chance_mcm_cleanup: failed to open header file: " << headerFile << "\n";
			return false;
		}
		if (headerText->find("kMcmSetLootChanceEvent") != std::string::npos) {
			std::cerr << "loot_chance_mcm_cleanup: legacy loot-chance MCM event constant still exists in EventBridge.h\n";
			return false;
		}

		const auto triggerText = loadText(triggerEventsFile);
		if (!triggerText.has_value()) {
			std::cerr << "loot_chance_mcm_cleanup: failed to open trigger events source: " << triggerEventsFile << "\n";
			return false;
		}
		if (triggerText->find("eventName == kMcmSetLootChanceEvent") != std::string::npos ||
			triggerText->find("PersistLootChancePercentToMcmSettings(chancePct, true)") != std::string::npos) {
			std::cerr << "loot_chance_mcm_cleanup: trigger source still handles deprecated loot-chance MCM event\n";
			return false;
		}

		const auto configText = loadText(configFile);
		if (!configText.has_value()) {
			std::cerr << "loot_chance_mcm_cleanup: failed to open config source: " << configFile << "\n";
			return false;
		}
		if (configText->find("LoadLootChancePercentFromMcmSettings()") != std::string::npos ||
			configText->find("PersistLootChancePercentToMcmSettings(") != std::string::npos) {
			std::cerr << "loot_chance_mcm_cleanup: config source still contains deprecated loot-chance MCM sync helpers\n";
			return false;
		}

		return true;
	}

	bool CheckMcmDropChanceRuntimeBridgePolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path().parent_path().parent_path();
		const fs::path configJsonFile = repoRoot / "Data" / "MCM" / "Config" / "CalamityAffixes" / "config.json";
		const fs::path mcmConfigScriptFile = repoRoot / "Data" / "Scripts" / "Source" / "CalamityAffixes_MCMConfig.psc";
		const fs::path modeControlScriptFile = repoRoot / "Data" / "Scripts" / "Source" / "CalamityAffixes_ModeControl.psc";
		const fs::path eventBridgeHeaderFile = repoRoot / "skse" / "CalamityAffixes" / "include" / "CalamityAffixes" / "EventBridge.h";
		const fs::path triggerEventsFile = repoRoot / "skse" / "CalamityAffixes" / "src" / "EventBridge.Triggers.Events.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto configJsonText = loadText(configJsonFile);
		if (!configJsonText.has_value()) {
			std::cerr << "mcm_drop_chance_bridge: failed to open MCM config: " << configJsonFile << "\n";
			return false;
		}
		if (configJsonText->find("\"id\": \"fRunewordFragmentChancePercent:General\"") == std::string::npos ||
			configJsonText->find("\"function\": \"SetRunewordFragmentChancePercent\"") == std::string::npos ||
			configJsonText->find("\"id\": \"fReforgeOrbChancePercent:General\"") == std::string::npos ||
			configJsonText->find("\"function\": \"SetReforgeOrbChancePercent\"") == std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: MCM slider/action wiring is missing\n";
			return false;
		}
		const auto countOccurrences = [](std::string_view haystack, std::string_view needle) -> std::size_t {
			std::size_t count = 0;
			std::size_t pos = 0;
			while ((pos = haystack.find(needle, pos)) != std::string_view::npos) {
				++count;
				pos += needle.size();
			}
			return count;
		};
		if (countOccurrences(*configJsonText, "\"formatString\": \"{1}%\"") < 2 ||
			configJsonText->find("\"formatString\": \"%.1f%%\"") != std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: chance slider formatString must use MCM Helper token syntax ({1}%).\n";
			return false;
		}

		const auto mcmConfigText = loadText(mcmConfigScriptFile);
		if (!mcmConfigText.has_value()) {
			std::cerr << "mcm_drop_chance_bridge: failed to open MCM script: " << mcmConfigScriptFile << "\n";
			return false;
		}
		if (mcmConfigText->find("Event OnSettingChange(string a_ID)") == std::string::npos ||
			mcmConfigText->find("a_ID == RunewordFragmentChanceSettingName || a_ID == ReforgeOrbChanceSettingName") == std::string::npos ||
			mcmConfigText->find("float runewordChance = GetModSettingFloat(RunewordFragmentChanceSettingName)") == std::string::npos ||
			mcmConfigText->find("runewordChance = RunewordFragmentChanceDefault") == std::string::npos ||
			mcmConfigText->find("float reforgeChance = GetModSettingFloat(ReforgeOrbChanceSettingName)") == std::string::npos ||
			mcmConfigText->find("reforgeChance = ReforgeOrbChanceDefault") == std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: MCM setting-change/sentinel fallback guard is missing\n";
			return false;
		}

		const auto modeControlText = loadText(modeControlScriptFile);
		if (!modeControlText.has_value()) {
			std::cerr << "mcm_drop_chance_bridge: failed to open mode-control script: " << modeControlScriptFile << "\n";
			return false;
		}
		if (modeControlText->find("Emit(\"CalamityAffixes_MCM_SetRunewordFragmentChance\"") == std::string::npos ||
			modeControlText->find("Emit(\"CalamityAffixes_MCM_SetReforgeOrbChance\"") == std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: mode-control mod-event bridge is missing\n";
			return false;
		}

		const auto eventBridgeHeaderText = loadText(eventBridgeHeaderFile);
		if (!eventBridgeHeaderText.has_value()) {
			std::cerr << "mcm_drop_chance_bridge: failed to open EventBridge header: " << eventBridgeHeaderFile << "\n";
			return false;
		}
		if (eventBridgeHeaderText->find("kMcmSetRunewordFragmentChanceEvent") == std::string::npos ||
			eventBridgeHeaderText->find("kMcmSetReforgeOrbChanceEvent") == std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: EventBridge event constants are missing\n";
			return false;
		}

		const auto triggerText = loadText(triggerEventsFile);
		if (!triggerText.has_value()) {
			std::cerr << "mcm_drop_chance_bridge: failed to open trigger source: " << triggerEventsFile << "\n";
			return false;
		}
		if (triggerText->find("eventName == kMcmSetRunewordFragmentChanceEvent") == std::string::npos ||
			triggerText->find("_loot.runewordFragmentChancePercent = std::clamp(a_event->numArg, 0.0f, 100.0f);") == std::string::npos ||
			triggerText->find("eventName == kMcmSetReforgeOrbChanceEvent") == std::string::npos ||
			triggerText->find("_loot.reforgeOrbChancePercent = std::clamp(a_event->numArg, 0.0f, 100.0f);") == std::string::npos) {
			std::cerr << "mcm_drop_chance_bridge: trigger event handlers are missing\n";
			return false;
		}

		return true;
	}

}
