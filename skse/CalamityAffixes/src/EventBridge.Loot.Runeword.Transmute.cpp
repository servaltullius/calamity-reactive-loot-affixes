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

		const auto affixIt = _affixIndexByToken.find(a_recipe.resultAffixToken);
		if (affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
			if (a_outFailureReason) {
				*a_outFailureReason = "result-affix-invalidated";
			}
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
			SKSE::log::error(
				"CalamityAffixes: runeword promote-to-primary failed after policy check (instance={:016X}, recipe={}, resultToken={:016X}).",
				a_instanceKey,
				a_recipe.id,
				a_recipe.resultAffixToken);
			return false;
		}

		EnsureInstanceRuntimeState(a_instanceKey, a_recipe.resultAffixToken);
		_runewordInstanceStates.erase(a_instanceKey);

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
		if (_runewordTransmuteInProgress) {
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
		ScopedTransmuteInFlight transmuteGuard(_runewordTransmuteInProgress);

		SanitizeRunewordState();
		if (!_runewordSelectedBaseKey) {
			RE::DebugNotification("Runeword: select a base first.");
			return;
		}

		RE::InventoryEntryData* entry = nullptr;
		RE::ExtraDataList* xList = nullptr;
		if (!ResolvePlayerInventoryInstance(*_runewordSelectedBaseKey, entry, xList)) {
			RE::DebugNotification("Runeword: selected base is no longer available.");
			_runewordSelectedBaseKey.reset();
			return;
		}

		// If base already has a completed runeword, block further crafting.
		if (const auto* completed = ResolveCompletedRunewordRecipe(*_runewordSelectedBaseKey)) {
			std::string note = "Runeword: already complete (";
			note.append(completed->displayName);
			note.push_back(')');
			RE::DebugNotification(note.c_str());
			return;
		}

		auto& state = _runewordInstanceStates[*_runewordSelectedBaseKey];
		if (state.recipeToken == 0u) {
			if (const auto* currentRecipe = GetCurrentRunewordRecipe()) {
				state.recipeToken = currentRecipe->token;
			}
		}

		const RunewordRecipe* recipe = FindRunewordRecipeByToken(state.recipeToken);
		if (!recipe) {
			recipe = GetCurrentRunewordRecipe();
			if (recipe) {
				state.recipeToken = recipe->token;
			}
		}
		if (!recipe || recipe->runeTokens.empty()) {
			RE::DebugNotification("Runeword: recipe is not set.");
			return;
		}

		const auto blockReason = ResolveRunewordApplyBlockReason(*_runewordSelectedBaseKey, *recipe);
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
			(void)ApplyRunewordResult(*_runewordSelectedBaseKey, *recipe);
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		// Final preflight right before consumption to minimize "consume then fail" cases.
		if (!ResolvePlayerInventoryInstance(*_runewordSelectedBaseKey, entry, xList) || !entry || !entry->object || !xList) {
			RE::DebugNotification("Runeword: selected base became unavailable.");
			return;
		}
		if (!IsLootObjectEligibleForAffixes(entry->object)) {
			RE::DebugNotification("Runeword blocked: selected base is no longer eligible.");
			return;
		}
		const auto preflightBlockReason = ResolveRunewordApplyBlockReason(*_runewordSelectedBaseKey, *recipe);
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
			if (const auto nameIt = _runewordRuneNameByToken.find(token); nameIt != _runewordRuneNameByToken.end()) {
				runeName = nameIt->second;
			}

			const auto owned = GetOwnedRunewordFragmentCount(player, _runewordRuneNameByToken, token);
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

		struct ConsumedRune
		{
			std::uint64_t token = 0u;
			std::uint32_t count = 0u;
			std::string runeName;
		};
		std::vector<ConsumedRune> consumedRunes;
		consumedRunes.reserve(requiredCounts.size);

		auto rollbackConsumedRunes = [&](std::string_view a_reasonTag) {
			bool rollbackFullySucceeded = true;
			for (const auto& consumed : consumedRunes) {
				if (consumed.count == 0u) {
					continue;
				}

				const auto restored = GrantRunewordFragments(
					player,
					_runewordRuneNameByToken,
					consumed.token,
					consumed.count);
				if (restored != consumed.count) {
					rollbackFullySucceeded = false;
					SKSE::log::error(
						"CalamityAffixes: runeword rollback partial (reason={}, runeToken={:016X}, runeName={}, restored={}, expected={}).",
						a_reasonTag,
						consumed.token,
						consumed.runeName,
						restored,
						consumed.count);
				}
			}
			return rollbackFullySucceeded;
		};

		// Consume all required fragments.
		for (std::size_t i = 0; i < requiredCounts.size; ++i) {
			const auto token = requiredCounts.entries[i].token;
			const auto required = requiredCounts.entries[i].count;
			if (token == 0u || required == 0u) {
				continue;
			}

			std::string runeName = "Rune";
			if (const auto nameIt = _runewordRuneNameByToken.find(token); nameIt != _runewordRuneNameByToken.end()) {
				runeName = nameIt->second;
			}

			auto* fragmentItem = LookupRunewordFragmentItem(_runewordRuneNameByToken, token);
			if (!fragmentItem) {
				const bool rollbackOk = rollbackConsumedRunes("fragment-item-missing");
				std::string note = "Runeword fragment item missing: ";
				note.append(runeName);
				if (rollbackOk) {
					note.append(" (rollback restored)");
				}
				RE::DebugNotification(note.c_str());
				SKSE::log::error(
					"CalamityAffixes: runeword fragment item missing (requiredRuneToken={:016X}, runeName={}, rollbackOk={}).",
					token,
					runeName,
					rollbackOk);
				return;
			}

			const auto ownedBeforeSigned = std::max(0, player->GetItemCount(fragmentItem));
			const auto ownedBefore = static_cast<std::uint32_t>(ownedBeforeSigned);
			if (ownedBefore < required) {
				const bool rollbackOk = rollbackConsumedRunes("consume-precheck-owned-too-low");
				std::string note = "Runeword failed to consume fragment: ";
				note.append(runeName);
				if (rollbackOk) {
					note.append(" (rollback restored)");
				}
				RE::DebugNotification(note.c_str());
				SKSE::log::warn(
					"CalamityAffixes: runeword consume precheck failed (runeToken={:016X}, runeName={}, owned={}, required={}, rollbackOk={}).",
					token,
					runeName,
					ownedBefore,
					required,
					rollbackOk);
				return;
			}

			const auto maxRemoveChunk = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
			std::uint32_t remaining = required;
			while (remaining > 0u) {
				const auto removeChunk = std::min(remaining, maxRemoveChunk);
				player->RemoveItem(fragmentItem, static_cast<std::int32_t>(removeChunk), RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				remaining -= removeChunk;
			}

			const auto ownedAfterSigned = std::max(0, player->GetItemCount(fragmentItem));
			const auto ownedAfter = static_cast<std::uint32_t>(ownedAfterSigned);
			const auto consumedCount = (ownedBefore > ownedAfter) ? (ownedBefore - ownedAfter) : 0u;
			if (consumedCount > 0u) {
				consumedRunes.push_back(ConsumedRune{
					.token = token,
					.count = consumedCount,
					.runeName = runeName
				});
			}
			if (consumedCount != required) {
				const bool rollbackOk = rollbackConsumedRunes("consume-postcheck-partial");
				std::string note = "Runeword failed to consume fragment: ";
				note.append(runeName);
				if (rollbackOk) {
					note.append(" (rollback restored)");
				}
				RE::DebugNotification(note.c_str());
				SKSE::log::warn(
					"CalamityAffixes: runeword consume postcheck failed (runeToken={:016X}, runeName={}, consumed={}, required={}, rollbackOk={}).",
					token,
					runeName,
					consumedCount,
					required,
					rollbackOk);
				return;
			}
		}

		std::string applyFailureReason;
		if (!ApplyRunewordResult(*_runewordSelectedBaseKey, *recipe, &applyFailureReason)) {
			const bool rollbackOk = rollbackConsumedRunes("apply-failed");
			if (rollbackOk) {
				RE::DebugNotification("Runeword failed after consume: fragments restored.");
			} else {
				RE::DebugNotification("Runeword failed after consume: fragment rollback partial.");
			}
			SKSE::log::error(
				"CalamityAffixes: runeword apply failed after fragment consume (recipe={}, reason={}, rollbackOk={}).",
				recipe->id,
				applyFailureReason.empty() ? "unknown" : applyFailureReason,
				rollbackOk);
			return;
		}
	}

}
