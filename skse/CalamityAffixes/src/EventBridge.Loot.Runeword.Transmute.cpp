#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RunewordUtil.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	void EventBridge::ApplyRunewordResult(std::uint64_t a_instanceKey, const RunewordRecipe& a_recipe)
	{
		const auto blockReason = ResolveRunewordApplyBlockReason(a_instanceKey, a_recipe);
		if (blockReason == RunewordApplyBlockReason::kMissingResultAffix) {
			std::string note = "Runeword failed: missing affix ";
			note.append(a_recipe.displayName);
			RE::DebugNotification(note.c_str());
			SKSE::log::error(
				"CalamityAffixes: runeword result affix missing (recipe={}, resultToken={:016X}).",
				a_recipe.id,
				a_recipe.resultAffixToken);
			return;
		}
		if (blockReason == RunewordApplyBlockReason::kAffixSlotsFull) {
			std::string note = "Runeword failed: base has max affixes";
			RE::DebugNotification(note.c_str());
			SKSE::log::warn(
				"CalamityAffixes: runeword apply blocked (instance={:016X}, recipe={}, reason=max-affixes-full, max={}).",
				a_instanceKey,
				a_recipe.id,
				kMaxAffixesPerItem);
			return;
		}

		const auto affixIt = _affixIndexByToken.find(a_recipe.resultAffixToken);
		if (affixIt == _affixIndexByToken.end() || affixIt->second >= _affixes.size()) {
			SKSE::log::error(
				"CalamityAffixes: runeword result affix became invalid after policy check (recipe={}, resultToken={:016X}).",
				a_recipe.id,
				a_recipe.resultAffixToken);
			return;
		}

		auto& slots = _instanceAffixes[a_instanceKey];
		if (!slots.PromoteTokenToPrimary(a_recipe.resultAffixToken)) {
			SKSE::log::error(
				"CalamityAffixes: runeword promote-to-primary failed after policy check (instance={:016X}, recipe={}, resultToken={:016X}).",
				a_instanceKey,
				a_recipe.id,
				a_recipe.resultAffixToken);
			return;
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
	}

	void EventBridge::InsertRunewordRuneIntoSelectedBase()
	{
		if (!_configLoaded) {
			return;
		}

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
			ApplyRunewordResult(*_runewordSelectedBaseKey, *recipe);
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
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
				std::string note = "Runeword fragment item missing: ";
				note.append(runeName);
				RE::DebugNotification(note.c_str());
				SKSE::log::error(
					"CalamityAffixes: runeword fragment item missing (requiredRuneToken={:016X}, runeName={}).",
					token,
					runeName);
				return;
			}

			for (std::uint32_t n = 0; n < required; ++n) {
				if (!TryConsumeRunewordFragment(player, _runewordRuneNameByToken, token)) {
					std::string note = "Runeword failed to consume fragment: ";
					note.append(runeName);
					RE::DebugNotification(note.c_str());
					SKSE::log::warn(
						"CalamityAffixes: runeword failed to consume fragment (runeToken={:016X}, runeName={}).",
						token,
						runeName);
					return;
				}
			}
		}

		ApplyRunewordResult(*_runewordSelectedBaseKey, *recipe);
	}

}
