#include "runtime_gate_store_checks_common.h"

#include "CalamityAffixes/PluginLogging.h"

#include <stdexcept>

namespace RuntimeGateStoreChecks
{
	bool CheckNonHostileFirstHitGate()
	{
		using namespace std::chrono;

		CalamityAffixes::NonHostileFirstHitGate gate{};
		const auto now = steady_clock::now();

		// First non-hostile hit on a fresh pair is allowed.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now)) {
			std::cerr << "gate: expected first non-hostile hit to be allowed\n";
			return false;
		}

		// Reentry within the short window stays allowed.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(10))) {
			std::cerr << "gate: expected reentry within window to be allowed\n";
			return false;
		}

		// Reentry beyond the window is denied.
		if (gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(30))) {
			std::cerr << "gate: expected reentry after window to be denied\n";
			return false;
		}

		// Hostile transition clears and denies this hit.
		if (gate.Resolve(0x14u, 0x1234u, true, true, false, now + milliseconds(200))) {
			std::cerr << "gate: expected hostile transition to be denied\n";
			return false;
		}

		// After hostile clear, first non-hostile hit can be allowed again.
		if (!gate.Resolve(0x14u, 0x1234u, true, false, false, now + milliseconds(250))) {
			std::cerr << "gate: expected first non-hostile hit after clear to be allowed\n";
			return false;
		}

		// Guard rails.
		if (gate.Resolve(0x14u, 0x2234u, false, false, false, now)) {
			std::cerr << "gate: expected disabled setting to deny\n";
			return false;
		}
		if (gate.Resolve(0x14u, 0x3234u, true, false, true, now)) {
			std::cerr << "gate: expected player target to deny\n";
			return false;
		}

		return true;
	}

	bool CheckPerTargetCooldownStore()
	{
		using namespace std::chrono;

		CalamityAffixes::PerTargetCooldownStore store{};
		const auto now = steady_clock::now();

		// Fresh key is not blocked.
		if (store.IsBlocked(0xA5u, 0x99u, now)) {
			std::cerr << "store: expected fresh key to be unblocked\n";
			return false;
		}

		// Commit establishes blocking interval.
		store.Commit(0xA5u, 0x99u, milliseconds(200), now);
		if (!store.IsBlocked(0xA5u, 0x99u, now + milliseconds(100))) {
			std::cerr << "store: expected key to be blocked before ICD expiry\n";
			return false;
		}
		if (store.IsBlocked(0xA5u, 0x99u, now + milliseconds(200))) {
			std::cerr << "store: expected key to unblock at ICD boundary\n";
			return false;
		}

		// Invalid commit inputs must not create blocking state.
		store.Commit(0u, 0x99u, milliseconds(200), now);
		if (store.IsBlocked(0u, 0x99u, now + milliseconds(1))) {
			std::cerr << "store: expected zero-token commit to be ignored\n";
			return false;
		}

		// Clear removes existing state.
		store.Clear();
		if (store.IsBlocked(0xA5u, 0x99u, now + milliseconds(1))) {
			std::cerr << "store: expected clear to remove cooldown state\n";
			return false;
		}

		return true;
	}

	bool CheckHandleHealthDamageVfuncIndexPolicy()
	{
		if (CalamityAffixes::Hooks::HandleHealthDamageVfuncIndexForRuntime(false) != 0x104u) {
			std::cerr << "hooks: expected non-VR HandleHealthDamage index to be 0x104\n";
			return false;
		}

		if (CalamityAffixes::Hooks::HandleHealthDamageVfuncIndexForRuntime(true) != 0x106u) {
			std::cerr << "hooks: expected VR HandleHealthDamage index to be 0x106\n";
			return false;
		}

		return true;
	}

	bool CheckHooksDispatchExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path cmakeFile = repoRoot / "CMakeLists.txt";
		const fs::path hooksFile = repoRoot / "src" / "Hooks.cpp";
		const fs::path hooksDispatchHeaderFile = repoRoot / "src" / "Hooks.Dispatch.h";
		const fs::path hooksDispatchFile = repoRoot / "src" / "Hooks.Dispatch.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto cmakeText = loadText(cmakeFile);
		const auto hooksText = loadText(hooksFile);
		const auto hooksDispatchHeaderText = loadText(hooksDispatchHeaderFile);
		const auto hooksDispatchText = loadText(hooksDispatchFile);
		if (!cmakeText.has_value() || !hooksText.has_value() || !hooksDispatchHeaderText.has_value() ||
			!hooksDispatchText.has_value()) {
			std::cerr << "hooks_dispatch_extraction: failed to load source files\n";
			return false;
		}

		if (cmakeText->find("src/Hooks.Dispatch.cpp") == std::string::npos ||
			hooksText->find("#include \"Hooks.Dispatch.h\"") == std::string::npos ||
			hooksText->find("detail::AdjustDamageAndEvaluateSpecials(") == std::string::npos ||
			hooksText->find("detail::SchedulePostHealthDamageActions(") == std::string::npos ||
			hooksText->find("detail::ClearDispatchRuntimeState();") == std::string::npos ||
			hooksText->find("PlayCastOnCritProcFeedbackSfx(") != std::string::npos ||
			hooksText->find("ExecutePostHealthDamageActions(") != std::string::npos ||
			hooksDispatchHeaderText->find("struct DamageAdjustmentResult") == std::string::npos ||
			hooksDispatchHeaderText->find("void SchedulePostHealthDamageActions(") == std::string::npos ||
			hooksDispatchText->find("PlayCastOnCritProcFeedbackSfx(") == std::string::npos ||
			hooksDispatchText->find("ExecutePostHealthDamageActions(") == std::string::npos ||
			hooksDispatchText->find("thread_local bool g_inProcDispatch = false;") == std::string::npos) {
			std::cerr << "hooks_dispatch_extraction: hook plumbing and dispatch helpers are not cleanly separated\n";
			return false;
		}

		return true;
	}

	bool CheckPluginLoggingExceptionSafetyPolicy()
	{
		std::size_t errorCalls = 0u;
		std::string lastError;
		const bool shouldFailSafely = CalamityAffixes::ConfigurePluginLogger(
			[]() {
				throw std::runtime_error("expected sink failure");
			},
			[&](std::string_view a_reason) {
				++errorCalls;
				lastError.assign(a_reason);
			});
		if (shouldFailSafely) {
			std::cerr << "plugin_logging_exception_safety: expected throwing sink factory to return false\n";
			return false;
		}
		if (errorCalls != 1u || lastError.find("expected sink failure") == std::string::npos) {
			std::cerr << "plugin_logging_exception_safety: expected error handler to receive sink failure exactly once\n";
			return false;
		}

		std::size_t installCalls = 0u;
		const bool shouldSucceed = CalamityAffixes::ConfigurePluginLogger(
			[&]() {
				++installCalls;
			},
			[](std::string_view) {});
		if (!shouldSucceed || installCalls != 1u) {
			std::cerr << "plugin_logging_exception_safety: expected valid sink factory to install logger once\n";
			return false;
		}

		return true;
	}

	bool CheckRebuildActiveCountsLoggingPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path sourceFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Triggers.ActiveCounts.cpp";

		std::ifstream in(sourceFile);
		if (!in.is_open()) {
			std::cerr << "rebuild_active_counts_logging: failed to open source file: " << sourceFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("if (_loot.debugLog)") == std::string::npos ||
			source.find("SKSE::log::debug(") == std::string::npos ||
			source.find("\"CalamityAffixes: RebuildActiveCounts —") == std::string::npos ||
			source.find("Always log rebuild summary at INFO level") != std::string::npos ||
			source.find("SKSE::log::info(\n\t\t\t\t\t\"CalamityAffixes: RebuildActiveCounts —") != std::string::npos ||
			source.find("SKSE::log::info(\"CalamityAffixes: RebuildActiveCounts —") != std::string::npos) {
			std::cerr << "rebuild_active_counts_logging: expected summary logging to stay debug-only\n";
			return false;
		}

		return true;
	}

	bool CheckRebuildActiveCountsExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path privateApiFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.PrivateApi.inl";
		const fs::path sourceFile = repoRoot / "src" / "EventBridge.Triggers.ActiveCounts.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto privateApiText = loadText(privateApiFile);
		const auto sourceText = loadText(sourceFile);
		if (!privateApiText.has_value() || !sourceText.has_value()) {
			std::cerr << "rebuild_active_counts_extraction: failed to load source files\n";
			return false;
		}

		if (privateApiText->find("void ResetActiveCountsStateForRebuild();") == std::string::npos ||
			privateApiText->find("void RefreshInventoryInstanceActiveState(") == std::string::npos ||
			privateApiText->find("void AccumulateEquippedAffixState(") == std::string::npos ||
			privateApiText->find("void ApplyDesiredPassiveSpells(") == std::string::npos ||
			privateApiText->find("void LogRebuildActiveCountsDebugSummary(") == std::string::npos ||
			sourceText->find("ResetActiveCountsStateForRebuild();") == std::string::npos ||
			sourceText->find("RefreshInventoryInstanceActiveState(entry, xList, desiredPassives);") == std::string::npos ||
			sourceText->find("AccumulateEquippedAffixState(key, slots, a_desiredPassives);") == std::string::npos ||
			sourceText->find("ApplyDesiredPassiveSpells(player, desiredPassives);") == std::string::npos ||
			sourceText->find("LogActiveAffixListDebug();") == std::string::npos ||
			sourceText->find("LogRebuildActiveCountsDebugSummary(desiredPassives);") == std::string::npos) {
			std::cerr << "rebuild_active_counts_extraction: expected rebuild flow to stay decomposed into focused helpers\n";
			return false;
		}

		return true;
	}

	bool CheckHealthDamageSignatureWindowPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path sourceFile = repoRoot / "src" / "EventBridge.Triggers.HealthDamage.Routing.cpp";
		const fs::path triggerGuardsFile = repoRoot / "include" / "CalamityAffixes" / "TriggerGuards.h";

		std::ifstream in(sourceFile);
		if (!in.is_open()) {
			std::cerr << "health_damage_signature_window: failed to open source file: " << sourceFile << "\n";
			return false;
		}
		std::ifstream guardsIn(triggerGuardsFile);
		if (!guardsIn.is_open()) {
			std::cerr << "health_damage_signature_window: failed to open helper file: " << triggerGuardsFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		const std::string guards(
			(std::istreambuf_iterator<char>(guardsIn)),
			std::istreambuf_iterator<char>());

		if (guards.find("constexpr bool ShouldSuppressDuplicateHealthDamageSignature(") == std::string::npos ||
			guards.find("constexpr bool ShouldSuppressHealthDamageStaleLeak(") == std::string::npos ||
			guards.find("constexpr bool ShouldSuppressPerTargetRepeatWindow(") == std::string::npos ||
			source.find("const auto sig = MakeHealthDamageSignature") == std::string::npos ||
			source.find("ShouldSuppressDuplicateHealthDamageSignature(") == std::string::npos ||
			source.find("ShouldSuppressHealthDamageStaleLeak(expectedDealt, absDamage)") == std::string::npos ||
			source.find("ShouldSuppressPerTargetRepeatWindow(") == std::string::npos) {
			std::cerr << "health_damage_signature_window: expected routing guards to be extracted and used from the dedicated routing file\n";
			return false;
		}

		return true;
	}

	bool CheckHealthDamageGuardHelperFlow()
	{
		struct FakeHealthDamageState
		{
			std::uint64_t lastSignature{ 0u };
			std::uint64_t lastSignatureAtMs{ 0u };
			std::uint64_t lastTargetHitAtMs{ 0u };
		};

		struct FakeHealthDamageEvent
		{
			std::uint64_t signature{ 0u };
			std::uint64_t nowMs{ 0u };
			float expectedDealt{ 0.0f };
			float absDamage{ 0.0f };
			std::uint64_t repeatWindowMs{ 0u };
		};

		auto shouldRoute = [](FakeHealthDamageState& a_state, const FakeHealthDamageEvent& a_event) {
			if (CalamityAffixes::ShouldSuppressDuplicateHealthDamageSignature(
				a_event.signature,
				a_state.lastSignature,
				a_event.nowMs,
				a_state.lastSignatureAtMs,
				100u)) {
				return false;
			}
			if (CalamityAffixes::ShouldSuppressHealthDamageStaleLeak(a_event.expectedDealt, a_event.absDamage)) {
				return false;
			}
			if (a_state.lastTargetHitAtMs != 0u && a_event.nowMs >= a_state.lastTargetHitAtMs &&
				CalamityAffixes::ShouldSuppressPerTargetRepeatWindow(
					a_event.nowMs - a_state.lastTargetHitAtMs,
					a_event.repeatWindowMs)) {
				return false;
			}

			a_state.lastSignature = a_event.signature;
			a_state.lastSignatureAtMs = a_event.nowMs;
			a_state.lastTargetHitAtMs = a_event.nowMs;
			return true;
		};

		{
			FakeHealthDamageState state{};
			if (!shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xAAu, .nowMs = 100u, .expectedDealt = 18.0f, .absDamage = 18.0f })) {
				std::cerr << "health_damage_guard_flow: expected first fake routed hit to pass\n";
				return false;
			}
			if (shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xAAu, .nowMs = 150u, .expectedDealt = 18.0f, .absDamage = 18.0f })) {
				std::cerr << "health_damage_guard_flow: expected duplicate signature inside stale window to be suppressed\n";
				return false;
			}
			if (!shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xAAu, .nowMs = 200u, .expectedDealt = 18.0f, .absDamage = 18.0f })) {
				std::cerr << "health_damage_guard_flow: expected duplicate signature at stale window boundary to pass\n";
				return false;
			}
		}

		{
			FakeHealthDamageState state{};
			if (shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xBBu, .nowMs = 300u, .expectedDealt = 20.0f, .absDamage = 4.0f })) {
				std::cerr << "health_damage_guard_flow: expected stale-leak fake event to be suppressed\n";
				return false;
			}
			if (state.lastSignature != 0u || state.lastTargetHitAtMs != 0u) {
				std::cerr << "health_damage_guard_flow: expected suppressed stale-leak event to leave fake state untouched\n";
				return false;
			}
		}

		{
			FakeHealthDamageState state{};
			if (!shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xC1u, .nowMs = 1000u, .expectedDealt = 14.0f, .absDamage = 14.0f, .repeatWindowMs = 400u })) {
				std::cerr << "health_damage_guard_flow: expected initial same-target fake event to pass\n";
				return false;
			}
			if (shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xC2u, .nowMs = 1200u, .expectedDealt = 14.0f, .absDamage = 14.0f, .repeatWindowMs = 400u })) {
				std::cerr << "health_damage_guard_flow: expected repeat-window fake event to be suppressed\n";
				return false;
			}
			if (!shouldRoute(state, FakeHealthDamageEvent{ .signature = 0xC3u, .nowMs = 1400u, .expectedDealt = 14.0f, .absDamage = 14.0f, .repeatWindowMs = 400u })) {
				std::cerr << "health_damage_guard_flow: expected repeat-window boundary fake event to pass\n";
				return false;
			}
		}

		return true;
	}

	bool CheckTesHitFallbackSourceValidationPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path sourceFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Triggers.HitEvent.cpp";

		std::ifstream in(sourceFile);
		if (!in.is_open()) {
			std::cerr << "tes_hit_fallback_source_validation: failed to open source file: " << sourceFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("if (_combatState.procDepth > 0)") == std::string::npos ||
			source.find("HitDataUtil::HitDataMatchesActors(hitData, target, aggressor)") == std::string::npos ||
			source.find("HitDataUtil::HasHitLikeSource(hitData, aggressor)") == std::string::npos ||
			source.find("if (!hasCommittedHitData)") == std::string::npos ||
			source.find("ProcessTrigger(Trigger::kIncomingHit, target, aggressor, hitData);") == std::string::npos ||
			source.find("ProcessTrigger(Trigger::kLowHealth, target, aggressor, hitData);") == std::string::npos) {
			std::cerr << "tes_hit_fallback_source_validation: TESHitEvent fallback must validate committed hit-like source data\n";
			return false;
		}

		return true;
	}

	bool CheckBloomTrapProcFeedbackPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path feedbackHeaderFile = repoRoot / "include" / "CalamityAffixes" / "ProcFeedback.h";
		const fs::path trapActionFile = repoRoot / "src" / "EventBridge.Actions.Trap.cpp";
		const fs::path trapsFile = repoRoot / "src" / "EventBridge.Traps.cpp";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto feedbackHeaderText = loadText(feedbackHeaderFile);
		const auto trapActionText = loadText(trapActionFile);
		const auto trapsText = loadText(trapsFile);
		if (!feedbackHeaderText.has_value() || !trapActionText.has_value() || !trapsText.has_value()) {
			std::cerr << "bloom_trap_proc_feedback: failed to load source files\n";
			return false;
		}

		if (feedbackHeaderText->find("kBloomSpellEditorIdPrefix = \"CAFF_SPEL_DOT_BLOOM_\"") == std::string::npos ||
			feedbackHeaderText->find("inline void PlayBloomProcFeedback(") == std::string::npos ||
			feedbackHeaderText->find("inline std::string_view ResolveBloomProcDebugLabel(") == std::string::npos ||
			trapActionText->find("#include \"CalamityAffixes/ProcFeedback.h\"") == std::string::npos ||
			trapActionText->find("std::clamp(armDelaySeconds, 0.18f, 0.75f)") == std::string::npos ||
			trapActionText->find("RE::DebugNotification(note.c_str());") == std::string::npos ||
			trapActionText->find("Calamity: {} planted") == std::string::npos ||
			trapActionText->find("Calamity: {} skipped ({})") == std::string::npos ||
			trapActionText->find("SelectSpawnTrapTarget(a_action, a_owner, a_target, a_hitData, spawnTarget, &failureReason)") == std::string::npos ||
			trapsText->find("#include \"CalamityAffixes/ProcFeedback.h\"") == std::string::npos ||
			trapsText->find("ProcFeedback::PlayBloomProcFeedback(triggeredTarget, trapSnapshot.spell, 0.12f, false);") == std::string::npos ||
			trapsText->find("Calamity: {} burst") == std::string::npos) {
			std::cerr << "bloom_trap_proc_feedback: expected bloom trap proc feedback helper to stay wired at spawn and trigger time\n";
			return false;
		}

		return true;
	}

	bool CheckConfigLoadPipelineExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path privateApiFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.PrivateApi.inl";
		const fs::path configFile = repoRoot / "src" / "EventBridge.Config.cpp";
		const fs::path pipelineFile = repoRoot / "src" / "EventBridge.Config.LoadPipeline.cpp";
		const fs::path cmakeFile = repoRoot / "CMakeLists.txt";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto privateApiText = loadText(privateApiFile);
		const auto configText = loadText(configFile);
		const auto pipelineText = loadText(pipelineFile);
		const auto cmakeText = loadText(cmakeFile);
		if (!privateApiText.has_value() || !configText.has_value() || !cmakeText.has_value()) {
			std::cerr << "config_load_pipeline_extraction: failed to load source files\n";
			return false;
		}
		if (!pipelineText.has_value()) {
			std::cerr << "config_load_pipeline_extraction: missing pipeline source file: " << pipelineFile << "\n";
			return false;
		}

		if (privateApiText->find("void BuildConfigDerivedAffixState(const nlohmann::json& a_affixes, RE::TESDataHandler* a_handler);") == std::string::npos ||
			cmakeText->find("src/EventBridge.Config.LoadPipeline.cpp") == std::string::npos ||
			configText->find("BuildConfigDerivedAffixState(*affixes, handler);") == std::string::npos ||
			configText->find("ParseConfiguredAffixesFromJson(*affixes, handler);") != std::string::npos ||
			configText->find("IndexConfiguredAffixes();") != std::string::npos ||
			configText->find("SynthesizeRunewordRuntimeAffixes();") != std::string::npos ||
			configText->find("RebuildSharedLootPools();") != std::string::npos ||
			configText->find("_activeCounts.assign(_affixes.size(), 0);") != std::string::npos ||
			pipelineText->find("ParseConfiguredAffixesFromJson(a_affixes, a_handler);") == std::string::npos ||
			pipelineText->find("IndexConfiguredAffixes();") == std::string::npos ||
			pipelineText->find("SynthesizeRunewordRuntimeAffixes();") == std::string::npos ||
			pipelineText->find("RebuildSharedLootPools();") == std::string::npos ||
			pipelineText->find("_activeCounts.assign(_affixes.size(), 0);") == std::string::npos) {
			std::cerr << "config_load_pipeline_extraction: config load pipeline extraction is incomplete\n";
			return false;
		}

		return true;
	}

	bool CheckHybridCurrencyDropPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path sourceFile = testFile.parent_path().parent_path() / "src" / "EventBridge.Config.LootRuntime.cpp";

		std::ifstream in(sourceFile);
		if (!in.is_open()) {
			std::cerr << "hybrid_currency_drop_policy: failed to open source file: " << sourceFile << "\n";
			return false;
		}

		const std::string source(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		if (source.find("_loot.currencyDropMode = CurrencyDropMode::kHybrid;") == std::string::npos ||
			source.find("_loot.runtimeCurrencyDropsEnabled = false;") == std::string::npos ||
			source.find("_loot.runtimeCurrencyDropsEnabled = true;") != std::string::npos ||
			source.find("Hybrid keeps corpse") == std::string::npos ||
			source.find("container/world/activation/pickup rolls disabled") == std::string::npos ||
			source.find("corpseAuthority={}") == std::string::npos ||
			source.find("\"SPID/ESP\"") == std::string::npos) {
			std::cerr << "hybrid_currency_drop_policy: expected hybrid mode to keep SPID corpse authority and runtime rolls disabled\n";
			return false;
		}

		return true;
	}

	bool CheckAffixSpecialActionStateExtractionPolicy()
	{
		namespace fs = std::filesystem;
		const fs::path testFile{ __FILE__ };
		const fs::path repoRoot = testFile.parent_path().parent_path();
		const fs::path eventBridgeHeaderFile = repoRoot / "include" / "CalamityAffixes" / "EventBridge.h";
		const fs::path stateGroupsFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.StateGroups.inl";
		const fs::path specialActionHeaderFile = repoRoot / "include" / "CalamityAffixes" / "AffixSpecialActionState.h";
		const fs::path indexingFile = repoRoot / "src" / "EventBridge.Config.IndexingShared.cpp";
		const fs::path resetFile = repoRoot / "src" / "EventBridge.Config.Reset.cpp";
		const fs::path configFile = repoRoot / "src" / "EventBridge.Config.cpp";

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
		const auto stateGroupsText = loadText(stateGroupsFile);
		const auto specialActionHeaderText = loadText(specialActionHeaderFile);
		const auto indexingText = loadText(indexingFile);
		const auto resetText = loadText(resetFile);
		const auto configText = loadText(configFile);
		if (!eventBridgeHeaderText.has_value() || !stateGroupsText.has_value() ||
			!indexingText.has_value() || !resetText.has_value() || !configText.has_value()) {
			std::cerr << "affix_special_action_state_extraction: failed to load source files\n";
			return false;
		}
		if (!specialActionHeaderText.has_value()) {
			std::cerr << "affix_special_action_state_extraction: missing special action header file: " << specialActionHeaderFile << "\n";
			return false;
		}

		if (eventBridgeHeaderText->find("#include \"CalamityAffixes/AffixSpecialActionState.h\"") == std::string::npos ||
			eventBridgeHeaderText->find("#include \"detail/EventBridge.StateGroups.inl\"") == std::string::npos ||
			eventBridgeHeaderText->find("AffixSpecialActionState _affixSpecialActions{};") == std::string::npos ||
			eventBridgeHeaderText->find("AffixRuntimeCacheState _affixRuntimeState{};") == std::string::npos ||
			eventBridgeHeaderText->find("InstanceTrackingState _instanceTrackingState{};") == std::string::npos ||
			eventBridgeHeaderText->find("std::vector<AffixRuntime> _affixes;") != std::string::npos ||
			eventBridgeHeaderText->find("std::unordered_map<std::uint64_t, InstanceAffixSlots> _instanceAffixes;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _castOnCritAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _convertAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _mindOverMatterAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _archmageAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _corpseExplosionAffixIndices;") != std::string::npos ||
			eventBridgeHeaderText->find("std::vector<std::size_t> _summonCorpseExplosionAffixIndices;") != std::string::npos ||
			stateGroupsText->find("struct AffixRuntimeCacheState") == std::string::npos ||
			stateGroupsText->find("std::vector<AffixRuntime> affixes{};") == std::string::npos ||
			stateGroupsText->find("std::vector<std::size_t> activeHitTriggerAffixIndices{};") == std::string::npos ||
			stateGroupsText->find("struct InstanceTrackingState") == std::string::npos ||
			stateGroupsText->find("std::unordered_map<std::uint64_t, InstanceAffixSlots> instanceAffixes{};") == std::string::npos ||
			specialActionHeaderText->find("castOnCritAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("convertAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("mindOverMatterAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("archmageAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("corpseExplosionAffixIndices") == std::string::npos ||
			specialActionHeaderText->find("summonCorpseExplosionAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.castOnCritAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.convertAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.mindOverMatterAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.archmageAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.corpseExplosionAffixIndices") == std::string::npos ||
			indexingText->find("_affixSpecialActions.summonCorpseExplosionAffixIndices") == std::string::npos ||
			resetText->find("_affixSpecialActions = {};") == std::string::npos ||
			configText->find("_affixSpecialActions.convertAffixIndices.size()") == std::string::npos ||
			configText->find("_affixSpecialActions.castOnCritAffixIndices.size()") == std::string::npos ||
			configText->find("_affixSpecialActions.mindOverMatterAffixIndices.size()") == std::string::npos ||
			configText->find("_affixSpecialActions.archmageAffixIndices.size()") == std::string::npos ||
			configText->find("_affixSpecialActions.corpseExplosionAffixIndices.size()") == std::string::npos) {
			std::cerr << "affix_special_action_state_extraction: special action extraction is incomplete\n";
			return false;
		}

		return true;
	}

	bool CheckTriggerProcPolicyExtraction()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path repoRoot = testFile.parent_path().parent_path();
			const fs::path privateApiFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.PrivateApi.inl";
			const fs::path triggersDispatchFile = repoRoot / "src" / "EventBridge.Triggers.cpp";
			const fs::path triggersRuntimeFile = repoRoot / "src" / "EventBridge.Triggers.Runtime.cpp";
			const fs::path policyFile = repoRoot / "src" / "EventBridge.Triggers.Policy.cpp";
			const fs::path cmakeFile = repoRoot / "CMakeLists.txt";

		auto loadText = [](const fs::path& path) -> std::optional<std::string> {
			std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
			};

			const auto privateApiText = loadText(privateApiFile);
			const auto triggersDispatchText = loadText(triggersDispatchFile);
			const auto triggersRuntimeText = loadText(triggersRuntimeFile);
			const auto policyText = loadText(policyFile);
			const auto cmakeText = loadText(cmakeFile);
			if (!privateApiText.has_value() || !triggersDispatchText.has_value() || !triggersRuntimeText.has_value() ||
				!policyText.has_value() || !cmakeText.has_value()) {
				std::cerr << "trigger_proc_policy_extraction: failed to load source files\n";
				return false;
			}

			const std::string triggersText = *triggersDispatchText + *triggersRuntimeText;

			if (cmakeText->find("src/EventBridge.Triggers.Policy.cpp") == std::string::npos ||
				cmakeText->find("src/EventBridge.Triggers.Runtime.cpp") == std::string::npos ||
				privateApiText->find("bool PassesTriggerProcPreconditions(") == std::string::npos ||
				privateApiText->find("float ResolveTriggerProcChancePct(") == std::string::npos ||
				privateApiText->find("bool RollTriggerProcChance(float a_chancePct);") == std::string::npos ||
			privateApiText->find("void CommitTriggerProcRuntime(") == std::string::npos ||
				triggersText.find("PassesTriggerProcPreconditions(") == std::string::npos ||
				triggersText.find("ResolveTriggerProcChancePct(affix, a_affixIndex)") == std::string::npos ||
				triggersText.find("RollTriggerProcChance(chance)") == std::string::npos ||
				triggersText.find("CommitTriggerProcRuntime(affix, perTargetKey, usesPerTargetIcd, chance, a_now);") == std::string::npos ||
				triggersText.find("affix.procChancePct * _runtimeSettings.procChanceMult") != std::string::npos ||
				triggersText.find("ResolveTriggerProcCooldownMs(") != std::string::npos ||
			policyText->find("bool EventBridge::PassesTriggerProcPreconditions(") == std::string::npos ||
			policyText->find("float EventBridge::ResolveTriggerProcChancePct(") == std::string::npos ||
			policyText->find("bool EventBridge::RollTriggerProcChance(") == std::string::npos ||
			policyText->find("void EventBridge::CommitTriggerProcRuntime(") == std::string::npos) {
			std::cerr << "trigger_proc_policy_extraction: ProcessTrigger policy extraction is incomplete\n";
			return false;
		}

		return true;
	}

		bool CheckProcessTriggerExtractionPolicy()
		{
			namespace fs = std::filesystem;
			const fs::path testFile{ __FILE__ };
			const fs::path repoRoot = testFile.parent_path().parent_path();
			const fs::path privateApiFile = repoRoot / "include" / "CalamityAffixes" / "detail" / "EventBridge.PrivateApi.inl";
			const fs::path snapshotFile = repoRoot / "include" / "CalamityAffixes" / "LowHealthTriggerSnapshot.h";
			const fs::path triggersFile = repoRoot / "src" / "EventBridge.Triggers.cpp";

			auto loadText = [](const fs::path& path) -> std::optional<std::string> {
				std::ifstream in(path);
			if (!in.is_open()) {
				return std::nullopt;
			}
			return std::string(
				(std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
		};

		const auto privateApiText = loadText(privateApiFile);
		const auto snapshotText = loadText(snapshotFile);
		const auto triggersText = loadText(triggersFile);
		if (!privateApiText.has_value() || !snapshotText.has_value() || !triggersText.has_value()) {
			std::cerr << "process_trigger_extraction: failed to load source files\n";
			return false;
		}

		if (privateApiText->find("bool CanProcessTriggerDispatch(") == std::string::npos ||
			privateApiText->find("bool TryProcessTriggerAffix(") == std::string::npos ||
			privateApiText->find("void FinalizeTriggerDispatch(") == std::string::npos ||
			snapshotText->find("struct LowHealthTriggerSnapshot") == std::string::npos ||
			snapshotText->find("constexpr LowHealthTriggerSnapshot BuildLowHealthTriggerSnapshot(") == std::string::npos ||
			triggersText->find("#include \"CalamityAffixes/LowHealthTriggerSnapshot.h\"") == std::string::npos ||
			triggersText->find("const auto lowHealthSnapshot = BuildLowHealthTriggerSnapshot(") == std::string::npos ||
			triggersText->find("CanProcessTriggerDispatch(a_trigger, a_owner, a_target, indices)") == std::string::npos ||
			triggersText->find("TryProcessTriggerAffix(") == std::string::npos ||
			triggersText->find("FinalizeTriggerDispatch(") == std::string::npos) {
			std::cerr << "process_trigger_extraction: expected ProcessTrigger flow to stay decomposed into trigger helpers\n";
			return false;
		}

		return true;
	}

	bool CheckUniformLootRollSelection()
	{
		std::mt19937 rng{ 0xCAFFu };

		{
			const std::vector<std::size_t> empty{};
			if (CalamityAffixes::detail::SelectUniformEligibleLootIndex(rng, empty).has_value()) {
				std::cerr << "loot_select: expected empty candidate set to return nullopt\n";
				return false;
			}
		}

		{
			const std::vector<std::size_t> single{ 42u };
			const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndex(rng, single);
			if (!picked.has_value() || *picked != 42u) {
				std::cerr << "loot_select: expected single candidate set to return that element\n";
				return false;
			}
		}

		const std::vector<std::size_t> candidates{ 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u };
		std::array<std::size_t, 8> counts{};
		constexpr std::size_t kDraws = 100000u;

		for (std::size_t i = 0; i < kDraws; ++i) {
			const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndex(rng, candidates);
			if (!picked.has_value() || *picked >= counts.size()) {
				std::cerr << "loot_select: invalid pick while sampling uniform distribution\n";
				return false;
			}
			counts[*picked] += 1u;
		}

		const double expected = static_cast<double>(kDraws) / static_cast<double>(counts.size());
		const double tolerance = expected * 0.05;  // wide enough to avoid flake; tight enough to catch weighting regressions.
		for (std::size_t i = 0; i < counts.size(); ++i) {
			const double diff = std::abs(static_cast<double>(counts[i]) - expected);
			if (diff > tolerance) {
				std::cerr << "loot_select: bucket " << i << " outside tolerance (count="
				          << counts[i] << ", expected=" << expected << ", tolerance=" << tolerance << ")\n";
				return false;
			}
		}

		const auto [minIt, maxIt] = std::minmax_element(counts.begin(), counts.end());
		if (static_cast<double>(*maxIt - *minIt) > expected * 0.09) {
			std::cerr << "loot_select: spread too wide (min=" << *minIt << ", max=" << *maxIt << ")\n";
			return false;
		}

		return true;
	}

	bool CheckShuffleBagLootRollSelection()
	{
		std::mt19937 rng{ 0xBA6Au };
		const std::vector<std::size_t> pool{ 0u, 1u, 2u, 3u, 4u, 5u };
		std::vector<std::size_t> bag{};
		std::size_t cursor = 0u;

		// Full-eligible draws should behave as "without replacement" per cycle.
		for (int cycle = 0; cycle < 4; ++cycle) {
			std::array<std::size_t, 6> seen{};
			for (std::size_t draw = 0; draw < pool.size(); ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					pool,
					bag,
					cursor,
					[](std::size_t) { return true; });
				if (!picked.has_value() || *picked >= seen.size()) {
					std::cerr << "shuffle_bag: invalid draw in full-eligible cycle\n";
					return false;
				}
				seen[*picked] += 1u;
			}
			for (std::size_t i = 0; i < seen.size(); ++i) {
				if (seen[i] != 1u) {
					std::cerr << "shuffle_bag: expected exactly one hit per bucket in a cycle (bucket="
					          << i << ", count=" << seen[i] << ")\n";
					return false;
				}
			}
		}

		// Temporary ineligibility must not consume that entry from the bag.
		bag.clear();
		cursor = 0u;

		const auto first = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
			rng,
			pool,
			bag,
			cursor,
			[](std::size_t) { return true; });
		if (!first.has_value()) {
			std::cerr << "shuffle_bag: expected first draw to succeed\n";
			return false;
		}

		if (cursor >= bag.size()) {
			std::cerr << "shuffle_bag: invalid cursor after first draw\n";
			return false;
		}

		const auto temporarilyExcluded = bag[cursor];
		const auto second = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
			rng,
			pool,
			bag,
			cursor,
			[temporarilyExcluded](std::size_t idx) { return idx != temporarilyExcluded; });
		if (!second.has_value()) {
			std::cerr << "shuffle_bag: expected second draw to succeed with one temporary exclusion\n";
			return false;
		}

		const auto third = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
			rng,
			pool,
			bag,
			cursor,
			[](std::size_t) { return true; });
		if (!third.has_value()) {
			std::cerr << "shuffle_bag: expected third draw to succeed\n";
			return false;
		}
		if (*third != temporarilyExcluded) {
			std::cerr << "shuffle_bag: temporarily excluded entry was consumed unexpectedly\n";
			return false;
		}

		return true;
	}

	bool CheckWeightedShuffleBagLootRollSelection()
	{
		std::mt19937 rng{ 0xC0FFEEu };
		const std::vector<std::size_t> pool{ 0u, 1u, 2u, 3u };
		const std::array<float, 4> weights{ 8.0f, 4.0f, 2.0f, 1.0f };
		std::array<std::size_t, 4> counts{};

		std::vector<std::size_t> bag{};
		std::size_t cursor = 0u;
		constexpr std::size_t kDraws = 200000u;

		// Long-run frequency should track configured weights.
		for (std::size_t draw = 0; draw < kDraws; ++draw) {
			const auto picked = CalamityAffixes::detail::SelectWeightedEligibleLootIndexWithShuffleBag(
				rng,
				pool,
				bag,
				cursor,
				[](std::size_t) { return true; },
				[&](std::size_t idx) {
					if (idx >= weights.size()) {
						return 0.0f;
					}
					return weights[idx];
				});
			if (!picked.has_value() || *picked >= counts.size()) {
				std::cerr << "weighted_shuffle_bag: invalid weighted draw\n";
				return false;
			}
			counts[*picked] += 1u;
		}

		const double weightSum = 15.0;
		const std::array<double, 4> expected{
			8.0 / weightSum,
			4.0 / weightSum,
			2.0 / weightSum,
			1.0 / weightSum
		};

		for (std::size_t i = 0; i < counts.size(); ++i) {
			const double observed = static_cast<double>(counts[i]) / static_cast<double>(kDraws);
			if (std::abs(observed - expected[i]) > 0.015) {
				std::cerr << "weighted_shuffle_bag: distribution drift on bucket " << i
				          << " (observed=" << observed << ", expected=" << expected[i] << ")\n";
				return false;
			}
		}

		// Zero-weight entries should never win while a positive-weight option exists.
		const std::array<float, 4> zeroHeavy{ 10.0f, 0.0f, 0.0f, 0.0f };
		for (std::size_t draw = 0; draw < 1000u; ++draw) {
			const auto picked = CalamityAffixes::detail::SelectWeightedEligibleLootIndexWithShuffleBag(
				rng,
				pool,
				bag,
				cursor,
				[](std::size_t) { return true; },
				[&](std::size_t idx) {
					if (idx >= zeroHeavy.size()) {
						return 0.0f;
					}
					return zeroHeavy[idx];
				});
			if (!picked.has_value() || *picked != 0u) {
				std::cerr << "weighted_shuffle_bag: zero-weight candidate selected unexpectedly\n";
				return false;
			}
		}

		// All-zero weights must still produce a valid eligible pick (uniform fallback).
		const std::array<float, 4> allZero{ 0.0f, 0.0f, 0.0f, 0.0f };
		for (std::size_t draw = 0; draw < 1000u; ++draw) {
			const auto picked = CalamityAffixes::detail::SelectWeightedEligibleLootIndexWithShuffleBag(
				rng,
				pool,
				bag,
				cursor,
				[](std::size_t) { return true; },
				[&](std::size_t idx) {
					if (idx >= allZero.size()) {
						return 0.0f;
					}
					return allZero[idx];
				});
			if (!picked.has_value() || *picked >= pool.size()) {
				std::cerr << "weighted_shuffle_bag: all-zero fallback failed\n";
				return false;
			}
		}

		return true;
	}

	bool CheckFixedWindowBudget()
	{
		std::uint64_t windowStartMs = 0u;
		std::uint32_t consumed = 0u;

		// Disabled budget settings should always pass.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 0u, 5u, windowStartMs, consumed)) {
			std::cerr << "budget: expected windowMs=0 to disable budget gate\n";
			return false;
		}
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 50u, 0u, windowStartMs, consumed)) {
			std::cerr << "budget: expected maxPerWindow=0 to disable budget gate\n";
			return false;
		}

		windowStartMs = 0u;
		consumed = 0u;

		// First two consumes pass, third in same window fails.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected first consume to pass\n";
			return false;
		}
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(120u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected second consume to pass within same window\n";
			return false;
		}
		if (CalamityAffixes::TryConsumeFixedWindowBudget(130u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected third consume in same window to fail\n";
			return false;
		}

		// New window should reset budget.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(151u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected consume to pass after window rollover\n";
			return false;
		}

		// Time rewind should also reset window safely.
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(40u, 50u, 2u, windowStartMs, consumed)) {
			std::cerr << "budget: expected consume to pass after timestamp rewind reset\n";
			return false;
		}

		// Budget=1 allows exactly one consume, then blocks.
		windowStartMs = 0u;
		consumed = 0u;
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 50u, 1u, windowStartMs, consumed)) {
			std::cerr << "budget: expected single-budget first consume to pass\n";
			return false;
		}
		if (CalamityAffixes::TryConsumeFixedWindowBudget(110u, 50u, 1u, windowStartMs, consumed)) {
			std::cerr << "budget: expected single-budget second consume to fail\n";
			return false;
		}

		// Both windowMs=0 and maxPerWindow=0 at the same time still passes.
		windowStartMs = 0u;
		consumed = 0u;
		if (!CalamityAffixes::TryConsumeFixedWindowBudget(100u, 0u, 0u, windowStartMs, consumed)) {
			std::cerr << "budget: expected both-zero settings to pass\n";
			return false;
		}

		return true;
	}

	bool CheckRecentlyAndLuckyHitGuards()
	{
		// Recently window semantics.
		if (!CalamityAffixes::IsWithinRecentlyWindowMs(1200u, 1000u, 250u)) {
			std::cerr << "recently: expected event inside window to pass\n";
			return false;
		}
		if (CalamityAffixes::IsWithinRecentlyWindowMs(1301u, 1000u, 250u)) {
			std::cerr << "recently: expected event outside window to fail\n";
			return false;
		}
		if (!CalamityAffixes::IsOutsideRecentlyWindowMs(1300u, 1000u, 250u)) {
			std::cerr << "recently: expected not-hit-recently gate to pass for old hit\n";
			return false;
		}
		if (CalamityAffixes::IsOutsideRecentlyWindowMs(1199u, 1000u, 250u)) {
			std::cerr << "recently: expected not-hit-recently gate to fail for fresh hit\n";
			return false;
		}

		// Lucky-hit effective chance envelope.
		if (CalamityAffixes::ResolveLuckyHitEffectiveChancePct(25.0f, 0.5f) != 12.5f) {
			std::cerr << "lucky_hit: expected 25% * 0.5 = 12.5%\n";
			return false;
		}
		if (CalamityAffixes::ResolveLuckyHitEffectiveChancePct(80.0f, 2.0f) != 100.0f) {
			std::cerr << "lucky_hit: expected clamp to 100%\n";
			return false;
		}

		// Runtime-like stochastic sanity check for lucky-hit chance.
		std::mt19937 rng{ 0x1A2Bu };
		std::uniform_real_distribution<float> dist(0.0f, 100.0f);

		const float effectiveChance = CalamityAffixes::ResolveLuckyHitEffectiveChancePct(25.0f, 0.5f);  // 12.5%
		constexpr std::size_t kTrials = 120000u;
		std::size_t hits = 0u;
		for (std::size_t i = 0; i < kTrials; ++i) {
			if (dist(rng) < effectiveChance) {
				++hits;
			}
		}

		const double observedPct = static_cast<double>(hits) * 100.0 / static_cast<double>(kTrials);
		const double expectedPct = 12.5;
		const double tolerancePct = 0.8;  // wide enough to avoid flakes, tight enough to catch regressions.
		if (std::abs(observedPct - expectedPct) > tolerancePct) {
			std::cerr << "lucky_hit: stochastic distribution drifted (observed="
			          << observedPct << "%, expected=" << expectedPct << "%)\n";
			return false;
		}

		return true;
	}

	bool CheckShuffleBagSanitizeAndRollConstraints()
	{
		struct MockAffix
		{
			float effectiveLootWeight{ 0.0f };
			bool isSuffix{ false };
			std::string family{};
		};

		std::mt19937 rng{ 0x5EEDu };
		std::vector<MockAffix> affixes{
			{ 1.0f, false, "" },      // 0 weapon prefix
			{ 1.0f, false, "" },      // 1 weapon prefix
			{ 0.0f, false, "" },      // 2 weapon prefix (disabled)
			{ 1.0f, false, "" },      // 3 armor prefix
			{ 1.0f, false, "" },      // 4 armor prefix
			{ 1.0f, true, "life" },   // 5 suffix
			{ 1.0f, true, "mana" },   // 6 suffix
			{ 1.0f, true, "life" },   // 7 suffix
		};

		const std::vector<std::size_t> weaponPrefixes{ 0u, 1u, 2u };
		const std::vector<std::size_t> armorPrefixes{ 3u, 4u };
		const std::vector<std::size_t> sharedPrefixes{ 0u, 1u, 2u, 3u, 4u };
		const std::vector<std::size_t> sharedSuffixes{ 5u, 6u, 7u };

		std::vector<std::size_t> prefixWeaponBag{};
		std::size_t prefixWeaponCursor = 0u;
		std::vector<std::size_t> prefixSharedBag{};
		std::size_t prefixSharedCursor = 0u;
		std::vector<std::size_t> suffixSharedBag{};
		std::size_t suffixSharedCursor = 0u;

		{
			std::vector<std::size_t> dirtyBag{ 99u, 2u, 2u, 1u };
			std::size_t dirtyCursor = 99u;
			CalamityAffixes::detail::SanitizeLootShuffleBagOrder(sharedPrefixes, dirtyBag, dirtyCursor);
			const std::vector<std::size_t> expected{ 2u, 1u, 0u, 3u, 4u };
			if (dirtyBag != expected || dirtyCursor != expected.size()) {
				std::cerr << "shuffle_bag: sanitize did not normalize bag as expected\n";
				return false;
			}
		}

		{
			if (CalamityAffixes::detail::ResolveReforgeTargetAffixCount(0u) != 1u) {
				std::cerr << "reforge: zero-affix bases should reroll with one target slot\n";
				return false;
			}
			if (CalamityAffixes::detail::ResolveReforgeTargetAffixCount(7u) !=
				static_cast<std::uint8_t>(CalamityAffixes::kMaxRegularAffixesPerItem)) {
				std::cerr << "reforge: corrupted high affix counts should clamp to max slots\n";
				return false;
			}

			const auto zero = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(0u);
			if (zero.prefixTarget != 0u || zero.suffixTarget != 0u) {
				std::cerr << "shuffle_bag: target=0 composition should be (0/0)\n";
				return false;
			}
			const auto one = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(1u);
			if (one.prefixTarget != 1u || one.suffixTarget != 0u) {
				std::cerr << "shuffle_bag: target=1 composition should be (1/0)\n";
				return false;
			}
			const auto two = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(2u);
			if (two.prefixTarget != 1u || two.suffixTarget != 1u) {
				std::cerr << "shuffle_bag: target=2 composition should be (1P/1S)\n";
				return false;
			}
			const auto three = CalamityAffixes::detail::DetermineLootPrefixSuffixTargets(3u);
			if (three.prefixTarget != 1u || three.suffixTarget != 2u) {
				std::cerr << "shuffle_bag: target=3 composition should be (1P/2S)\n";
				return false;
			}

			if (!CalamityAffixes::detail::ShouldConsumeSuffixRollForSingleAffixTarget(1u, 0u)) {
				std::cerr << "shuffle_bag: single-affix suffix fallback should be allowed before prefix assignment\n";
				return false;
			}
			if (CalamityAffixes::detail::ShouldConsumeSuffixRollForSingleAffixTarget(1u, 1u)) {
				std::cerr << "shuffle_bag: single-affix suffix fallback should be blocked after prefix assignment\n";
				return false;
			}
			if (!CalamityAffixes::detail::ShouldConsumeSuffixRollForSingleAffixTarget(2u, 1u)) {
				std::cerr << "shuffle_bag: multi-affix suffix roll should remain enabled\n";
				return false;
			}

			{
				CalamityAffixes::InstanceAffixSlots previous{};
				CalamityAffixes::InstanceAffixSlots rolled{};
				(void)previous.AddToken(0xA0u);
				(void)previous.AddToken(0xB0u);
				(void)previous.AddToken(0xC0u);
				(void)rolled.AddToken(0xB0u);
				(void)rolled.AddToken(0xC0u);
				const auto regular = CalamityAffixes::detail::BuildRegularOnlyAffixSlots(previous, 0xA0u);
				if (regular.count != 2u || regular.tokens[0] != 0xB0u || regular.tokens[1] != 0xC0u) {
					std::cerr << "reforge: regular-slot snapshot should exclude preserved runeword token\n";
					return false;
				}
				if (!CalamityAffixes::detail::ShouldRetryRegularAffixReforgeRoll(regular, rolled, 0u, 4u)) {
					std::cerr << "reforge: identical regular reroll should retry before final attempt\n";
					return false;
				}
				if (CalamityAffixes::detail::ShouldRetryRegularAffixReforgeRoll(regular, rolled, 3u, 4u)) {
					std::cerr << "reforge: identical regular reroll should stop retrying on final attempt\n";
					return false;
				}
			}
		}

		{
			// Non-shared weapon pool must never pick armor indices and must skip disabled index 2.
			for (std::size_t draw = 0; draw < 40; ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					weaponPrefixes,
					prefixWeaponBag,
					prefixWeaponCursor,
					[&](std::size_t idx) {
						return idx < affixes.size() && affixes[idx].effectiveLootWeight > 0.0f;
					});
				if (!picked.has_value()) {
					std::cerr << "shuffle_bag: weapon-prefix draw unexpectedly failed\n";
					return false;
				}
				if (*picked >= 3u) {
					std::cerr << "shuffle_bag: weapon-only draw leaked into armor pool (idx=" << *picked << ")\n";
					return false;
				}
				if (*picked == 2u) {
					std::cerr << "shuffle_bag: disabled prefix was selected\n";
					return false;
				}
			}
		}

		{
			// Shared prefix pool with exclusions should converge to the only allowed prefix (idx=4).
			const std::vector<std::size_t> exclude{ 0u, 1u, 2u, 3u };
			for (std::size_t draw = 0; draw < 10; ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					sharedPrefixes,
					prefixSharedBag,
					prefixSharedCursor,
					[&](std::size_t idx) {
						if (idx >= affixes.size() || affixes[idx].effectiveLootWeight <= 0.0f) {
							return false;
						}
						return std::find(exclude.begin(), exclude.end(), idx) == exclude.end();
					});
				if (!picked.has_value() || *picked != 4u) {
					std::cerr << "shuffle_bag: shared-prefix exclusion constraint broken\n";
					return false;
				}
			}
		}

		{
			// Suffix family exclusion should never pick family='life' (idx=5,7).
			const std::vector<std::string> excludeFamilies{ "life" };
			for (std::size_t draw = 0; draw < 10; ++draw) {
				const auto picked = CalamityAffixes::detail::SelectUniformEligibleLootIndexWithShuffleBag(
					rng,
					sharedSuffixes,
					suffixSharedBag,
					suffixSharedCursor,
					[&](std::size_t idx) {
						if (idx >= affixes.size()) {
							return false;
						}
						const auto& affix = affixes[idx];
						if (!affix.isSuffix || affix.effectiveLootWeight <= 0.0f) {
							return false;
						}
						if (!affix.family.empty()) {
							return std::find(excludeFamilies.begin(), excludeFamilies.end(), affix.family) == excludeFamilies.end();
						}
						return true;
					});
				if (!picked.has_value() || *picked != 6u) {
					std::cerr << "shuffle_bag: suffix family exclusion constraint broken\n";
					return false;
				}
			}
		}

		(void)armorPrefixes;  // documents that armor pool exists in this integration-like scenario.
		return true;
	}

	bool CheckLootSlotSanitizer()
	{
		CalamityAffixes::InstanceAffixSlots slots{};
		slots.count = 3u;
		slots.tokens[0] = 101u;
		slots.tokens[1] = 202u;
		slots.tokens[2] = 101u;  // duplicate should be removed.

		std::array<std::uint64_t, CalamityAffixes::kMaxAffixesPerItem> removed{};
		std::uint8_t removedCount = 0u;
		const auto sanitized = CalamityAffixes::detail::BuildSanitizedInstanceAffixSlots(
			slots,
			[](std::uint64_t token) { return token != 202u; },
			&removed,
			&removedCount);

		if (sanitized.count != 1u || sanitized.tokens[0] != 101u) {
			std::cerr << "slot_sanitize: expected only token 101 to remain\n";
			return false;
		}
		if (removedCount != 2u) {
			std::cerr << "slot_sanitize: expected two removed tokens (disallowed + duplicate)\n";
			return false;
		}

		CalamityAffixes::InstanceAffixSlots corrupted{};
		corrupted.count = 7u;  // out-of-range count should be clamped internally.
		corrupted.tokens[0] = 0u;
		corrupted.tokens[1] = 301u;
		corrupted.tokens[2] = 302u;
		const auto sanitizedCorrupted = CalamityAffixes::detail::BuildSanitizedInstanceAffixSlots(
			corrupted,
			[](std::uint64_t) { return true; });
		if (sanitizedCorrupted.count != 2u ||
			sanitizedCorrupted.tokens[0] != 301u ||
			sanitizedCorrupted.tokens[1] != 302u) {
			std::cerr << "slot_sanitize: expected clamp+zero-filter behavior for corrupted slot input\n";
			return false;
		}

		return true;
	}
}
