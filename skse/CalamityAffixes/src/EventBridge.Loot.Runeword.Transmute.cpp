#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RunewordUtil.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	bool EventBridge::ApplyRunewordResult(
		std::uint64_t a_instanceKey,
		const RunewordRecipe& a_recipe,
		std::string* a_outFailureReason)
	{
		const auto blockReason = ResolveRunewordApplyBlockReason(a_instanceKey, a_recipe);
		if (blockReason == RunewordApplyBlockReason::kMissingResultAffix) {
			if (a_outFailureReason) {
				*a_outFailureReason = "missing-result-affix";
			}
			std::string note = "Runeword failed: missing affix ";
			note.append(a_recipe.displayName);
			RE::DebugNotification(note.c_str());
			SKSE::log::error(
				"CalamityAffixes: runeword result affix missing (recipe={}, resultToken={:016X}).",
				a_recipe.id,
				a_recipe.resultAffixToken);
			return false;
		}
		if (blockReason == RunewordApplyBlockReason::kAffixSlotsFull) {
			if (a_outFailureReason) {
				*a_outFailureReason = "affix-slots-full";
			}
			std::string note = "Runeword failed: base has max affixes";
			RE::DebugNotification(note.c_str());
			SKSE::log::warn(
				"CalamityAffixes: runeword apply blocked (instance={:016X}, recipe={}, reason=max-affixes-full, max={}).",
				a_instanceKey,
				a_recipe.id,
				kMaxAffixesPerItem);
			return false;
		}

		const auto affixIt = _affixRegistry.affixIndexByToken.find(a_recipe.resultAffixToken);
		if (affixIt == _affixRegistry.affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
			if (a_outFailureReason) {
				*a_outFailureReason = "result-affix-invalidated";
			}
			RE::DebugNotification("Runeword failed: internal error (affix invalidated).");
			SKSE::log::error(
				"CalamityAffixes: runeword result affix became invalid after policy check (recipe={}, resultToken={:016X}).",
				a_recipe.id,
				a_recipe.resultAffixToken);
			return false;
		}

		auto& slots = _instanceAffixes[a_instanceKey];

		if (!slots.PromoteTokenToPrimary(a_recipe.resultAffixToken)) {
			if (a_outFailureReason) {
				*a_outFailureReason = "promote-to-primary-failed";
			}
			RE::DebugNotification("Runeword failed: internal error (apply failed).");
			SKSE::log::error(
				"CalamityAffixes: runeword promote-to-primary failed after policy check (instance={:016X}, recipe={}, resultToken={:016X}).",
				a_instanceKey,
				a_recipe.id,
				a_recipe.resultAffixToken);
			return false;
		}

		EnsureInstanceRuntimeState(a_instanceKey, a_recipe.resultAffixToken);
		_runewordState.instanceStates.erase(a_instanceKey);

		RE::InventoryEntryData* entry = nullptr;
		RE::ExtraDataList* xList = nullptr;
		if (ResolvePlayerInventoryInstance(a_instanceKey, entry, xList) && entry && xList) {
			EnsureMultiAffixDisplayName(entry, xList, slots);
		}

		RebuildActiveCounts();

		std::string note = "Runeword Complete: ";
		note.append(a_recipe.displayName);
		RE::DebugNotification(note.c_str());
		SKSE::log::info(
			"CalamityAffixes: runeword completed (recipe={}, resultAffix={}).",
			a_recipe.id,
			_affixes[affixIt->second].id);
		return true;
	}

	void EventBridge::InsertRunewordRuneIntoSelectedBase()
	{
		if (!_configLoaded) {
			return;
		}
		if (_runewordState.transmuteInProgress) {
			RE::DebugNotification("Runeword: transmute already in progress.");
			return;
		}

		struct ScopedTransmuteInFlight
		{
			bool& flag;

			explicit ScopedTransmuteInFlight(bool& a_flag) :
				flag(a_flag)
			{
				flag = true;
			}

			~ScopedTransmuteInFlight()
			{
				flag = false;
			}
		};
		ScopedTransmuteInFlight transmuteGuard(_runewordState.transmuteInProgress);

		SanitizeRunewordState();
		std::uint64_t instanceKey = 0u;
		RE::InventoryEntryData* entry = nullptr;
		RE::ExtraDataList* xList = nullptr;
		std::string baseResolveFailure;
		if (!ResolveSelectedRunewordBaseInstance(instanceKey, entry, xList, &baseResolveFailure, false)) {
			std::string note = "Runeword: ";
			note.append(baseResolveFailure);
			RE::DebugNotification(note.c_str());
			return;
		}

		// If base already has a completed runeword, remove it to allow re-transmutation.
		if (const auto* completed = ResolveCompletedRunewordRecipe(instanceKey)) {
			const auto oldToken = completed->resultAffixToken;
			auto& slots = _instanceAffixes[instanceKey];
			slots.RemoveToken(oldToken);
			_instanceStates.erase(MakeInstanceStateKey(instanceKey, oldToken));
			_runewordState.instanceStates.erase(instanceKey);

			if (ResolvePlayerInventoryInstance(instanceKey, entry, xList) && entry && xList) {
				EnsureMultiAffixDisplayName(entry, xList, slots);
			}
			RebuildActiveCounts();

			SKSE::log::info(
				"CalamityAffixes: removed existing runeword for re-transmutation (instance={:016X}, oldToken={:016X}, displayName={}).",
				instanceKey,
				oldToken,
				completed->displayName);
		}

		auto& state = _runewordState.instanceStates[instanceKey];
		const RunewordRecipe* recipe = ResolvePendingRunewordRecipe(instanceKey);
		if (!recipe || recipe->runeTokens.empty()) {
			RE::DebugNotification("Runeword: recipe is not set.");
			return;
		}

		const auto blockReason = ResolveRunewordApplyBlockReason(instanceKey, *recipe);
		if (blockReason == RunewordApplyBlockReason::kMissingResultAffix) {
			std::string note = "Runeword failed: missing affix ";
			note.append(recipe->displayName);
			RE::DebugNotification(note.c_str());
			SKSE::log::error(
				"CalamityAffixes: runeword result affix missing before transmute (recipe={}, resultToken={:016X}).",
				recipe->id,
				recipe->resultAffixToken);
			return;
		}
		if (blockReason == RunewordApplyBlockReason::kAffixSlotsFull) {
			std::string note = "Runeword blocked: ";
			note.append(BuildRunewordApplyBlockMessage(blockReason));
			RE::DebugNotification(note.c_str());
			return;
		}

		const auto totalRunes = recipe->runeTokens.size();
		if (state.insertedRunes >= totalRunes) {
			// Legacy finalize path.
			(void)ApplyRunewordResult(instanceKey, *recipe);
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		// Final preflight right before consumption to minimize "consume then fail" cases.
		if (!ResolveSelectedRunewordBaseInstance(instanceKey, entry, xList, &baseResolveFailure, true)) {
			std::string note = "Runeword: ";
			note.append(baseResolveFailure == "selected base is no longer available." ? "selected base became unavailable." : baseResolveFailure);
			RE::DebugNotification(note.c_str());
			return;
		}
		const auto preflightBlockReason = ResolveRunewordApplyBlockReason(instanceKey, *recipe);
		if (preflightBlockReason != RunewordApplyBlockReason::kNone) {
			std::string note = "Runeword blocked: ";
			note.append(BuildRunewordApplyBlockMessage(preflightBlockReason));
			RE::DebugNotification(note.c_str());
			return;
		}

		const auto requiredCounts = BuildRuneTokenCounts<16>(
			std::span<const std::uint64_t>(recipe->runeTokens.data(), recipe->runeTokens.size()),
			state.insertedRunes);

		bool ready = true;
		std::string missingSummary;
		for (std::size_t i = 0; i < requiredCounts.size; ++i) {
			const auto token = requiredCounts.entries[i].token;
			const auto required = requiredCounts.entries[i].count;
			if (token == 0u || required == 0u) {
				continue;
			}

			std::string runeName = "Rune";
			if (const auto nameIt = _runewordState.runeNameByToken.find(token); nameIt != _runewordState.runeNameByToken.end()) {
				runeName = nameIt->second;
			}

			const auto owned = GetOwnedRunewordFragmentCount(player, _runewordState.runeNameByToken, token);
			if (owned >= required) {
				continue;
			}

			ready = false;
			const auto missing = required - owned;
			if (!missingSummary.empty()) {
				missingSummary.append(", ");
			}
			missingSummary.append(runeName);
			missingSummary.append(" x");
			missingSummary.append(std::to_string(missing));
		}

		if (!ready) {
			std::string note = "Runeword missing fragments: ";
			note.append(missingSummary.empty() ? "?" : missingSummary);
			RE::DebugNotification(note.c_str());
			return;
		}

		std::vector<ConsumedRunewordFragment> consumedRunes;
		consumedRunes.reserve(requiredCounts.size);

		// Consume all required fragments.
		for (std::size_t i = 0; i < requiredCounts.size; ++i) {
			const auto token = requiredCounts.entries[i].token;
			const auto required = requiredCounts.entries[i].count;
			if (token == 0u || required == 0u) {
				continue;
			}

			std::string runeName = "Rune";
			if (const auto nameIt = _runewordState.runeNameByToken.find(token); nameIt != _runewordState.runeNameByToken.end()) {
				runeName = nameIt->second;
			}

			std::string consumeFailureReason;
			if (!ConsumeRunewordFragmentRequirement(
				player,
				token,
				required,
				consumedRunes,
				&runeName,
				&consumeFailureReason)) {
				const bool rollbackOk = RollbackConsumedRunewordFragments(
					player,
					consumedRunes,
					consumeFailureReason.empty() ? std::string_view{"consume-failed"} : std::string_view{ consumeFailureReason });
				std::string note = "Runeword failed to consume fragment: ";
				note.append(runeName);
				if (rollbackOk) {
					note.append(" (rollback restored)");
				}
				RE::DebugNotification(note.c_str());
				if (consumeFailureReason == "fragment-item-missing") {
					SKSE::log::error(
						"CalamityAffixes: runeword fragment item missing (requiredRuneToken={:016X}, runeName={}, rollbackOk={}).",
						token,
						runeName,
						rollbackOk);
				} else {
					SKSE::log::warn(
						"CalamityAffixes: runeword fragment consume failed (runeToken={:016X}, runeName={}, required={}, reason={}, rollbackOk={}).",
						token,
						runeName,
						required,
						consumeFailureReason.empty() ? "unknown" : consumeFailureReason,
						rollbackOk);
				}
				return;
			}
		}

		if (!PlayerHasInstanceKey(instanceKey)) {
			const bool rollbackOk = RollbackConsumedRunewordFragments(player, consumedRunes, "base-unavailable-after-consume");
			std::string note = "Runeword failed (base-unavailable)";
			note.append(rollbackOk ? ": fragments restored." : ": fragment rollback partial.");
			RE::DebugNotification(note.c_str());
			if (_runewordState.selectedBaseKey && *_runewordState.selectedBaseKey == instanceKey) {
				_runewordState.selectedBaseKey.reset();
			}
			SKSE::log::error(
				"CalamityAffixes: runeword apply aborted after consume (reason=base-unavailable, instance={:016X}, recipe={}, rollbackOk={}).",
				instanceKey,
				recipe->id,
				rollbackOk);
			return;
		}

		std::string applyFailureReason;
		if (!ApplyRunewordResult(instanceKey, *recipe, &applyFailureReason)) {
			const bool rollbackOk = RollbackConsumedRunewordFragments(player, consumedRunes, "apply-failed");
			std::string note = "Runeword failed";
			if (!applyFailureReason.empty()) {
				note.append(" (");
				note.append(applyFailureReason);
				note.push_back(')');
			}
			note.append(rollbackOk ? ": fragments restored." : ": fragment rollback partial.");
			RE::DebugNotification(note.c_str());
			SKSE::log::error(
				"CalamityAffixes: runeword apply failed after fragment consume (recipe={}, reason={}, rollbackOk={}).",
				recipe->id,
				applyFailureReason.empty() ? "unknown" : applyFailureReason,
				rollbackOk);
			return;
		}
	}

}
