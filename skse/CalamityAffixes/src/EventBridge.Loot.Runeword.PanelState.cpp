#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RunewordUiPolicy.h"
#include "CalamityAffixes/RunewordUtil.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	EventBridge::RunewordPanelState EventBridge::GetRunewordPanelState()
	{
		const std::scoped_lock lock(_stateMutex);
		RunewordPanelState panelState{};
		if (!_configLoaded) {
			return panelState;
		}

		SanitizeRunewordState();
		if (!_runewordState.selectedBaseKey) {
			return panelState;
		}
		panelState.hasBase = true;

		// Re-transmutation: if the selected base already has a completed runeword,
		// fall through to the normal recipe selection flow instead of blocking.
		// The old runeword is removed when the first rune is inserted
		// (InsertRunewordRuneIntoSelectedBase handles removal).

		const RunewordRecipe* recipe = nullptr;
		std::uint32_t inserted = 0u;

		const auto stateIt = _runewordState.instanceStates.find(*_runewordState.selectedBaseKey);
		if (stateIt != _runewordState.instanceStates.end()) {
			inserted = stateIt->second.insertedRunes;
			recipe = FindRunewordRecipeByToken(stateIt->second.recipeToken);
		}

		if (!recipe) {
			recipe = GetCurrentRunewordRecipe();
		}
		if (!recipe || recipe->runeTokens.empty()) {
			return panelState;
		}

		panelState.hasRecipe = true;
		panelState.recipeName = recipe->displayName;
		panelState.totalRunes = static_cast<std::uint32_t>(recipe->runeTokens.size());
		panelState.insertedRunes = std::min(inserted, panelState.totalRunes);

		const auto applyBlockReason = ResolveRunewordApplyBlockReason(*_runewordState.selectedBaseKey, *recipe);
		const bool canApplyResult = applyBlockReason == RunewordApplyBlockReason::kNone;

		if (panelState.insertedRunes >= panelState.totalRunes) {
			// Legacy: allow finalization only when result can actually be applied.
			panelState.canInsert = CanFinalizeRunewordFromPanel(
				panelState.insertedRunes,
				panelState.totalRunes,
				canApplyResult);
			if (!canApplyResult) {
				panelState.missingSummary = BuildRunewordApplyBlockMessage(applyBlockReason);
			}
			return panelState;
		}

		// Transmute-only UI: require all remaining rune fragments at once.
		const auto requiredCounts = BuildRuneTokenCounts<16>(
			std::span<const std::uint64_t>(recipe->runeTokens.data(), recipe->runeTokens.size()),
			panelState.insertedRunes);

		auto* player = RE::PlayerCharacter::GetSingleton();
		bool ready = true;
		bool firstMissingSet = false;
		std::string missingSummary;

		panelState.requiredRunes.reserve(requiredCounts.size);
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
			panelState.requiredRunes.push_back(RunewordRuneRequirement{
				.runeName = runeName,
				.required = required,
				.owned = owned,
			});
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

			if (!firstMissingSet) {
				firstMissingSet = true;
				panelState.nextRuneName = runeName;
				panelState.nextRuneOwned = owned;
			}
		}

		if (!missingSummary.empty()) {
			panelState.missingSummary = std::move(missingSummary);
		} else if (!canApplyResult) {
			panelState.missingSummary = BuildRunewordApplyBlockMessage(applyBlockReason);
		}

		panelState.canInsert = CanInsertRunewordFromPanel(ready, canApplyResult);
		return panelState;
	}

	std::optional<std::string> EventBridge::GetSelectedRunewordBaseAffixTooltip(int a_uiLanguageMode)
	{
		const std::scoped_lock lock(_stateMutex);
		if (!_configLoaded) {
			return std::nullopt;
		}

		SanitizeRunewordState();
		if (!_runewordState.selectedBaseKey) {
			return std::nullopt;
		}

		RE::InventoryEntryData* entry = nullptr;
		RE::ExtraDataList* xList = nullptr;
		if (!ResolvePlayerInventoryInstance(*_runewordState.selectedBaseKey, entry, xList) || !entry || !entry->object || !xList) {
			return std::nullopt;
		}

		const std::string selectedName = ResolveInventoryDisplayName(entry, xList);
		return GetInstanceAffixTooltip(
			entry,
			selectedName,
			a_uiLanguageMode,
			"inventory",
			0u,
			*_runewordState.selectedBaseKey);
	}

}
