#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RunewordUiPolicy.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	bool EventBridge::SelectRunewordRecipe(std::uint64_t a_recipeToken)
	{
		const std::scoped_lock lock(_stateMutex);
		if (!_configLoaded || a_recipeToken == 0u || _runewordState.recipes.empty()) {
			return false;
		}

		SanitizeRunewordState();
		const auto it = _runewordState.recipeIndexByToken.find(a_recipeToken);
		if (it == _runewordState.recipeIndexByToken.end() || it->second >= _runewordState.recipes.size()) {
			return false;
		}

		const auto idx = it->second;
		const auto& recipe = _runewordState.recipes[idx];
		if (!HasRunewordRuntimeEffect(recipe)) {
			EmitHudNotification("Runeword Recipe: runtime effect not available.");
			return false;
		}
		_runewordState.recipeCycleCursor = static_cast<std::uint32_t>(idx);

		if (_runewordState.selectedBaseKey) {
			const auto selectedKey = *_runewordState.selectedBaseKey;
			const bool completedBase = ResolveCompletedRunewordRecipe(selectedKey) != nullptr;

			if (ShouldClearRunewordInProgressState(completedBase)) {
				_runewordState.instanceStates.erase(selectedKey);
			}

			// Always record the explicit recipe selection so the UI can
			// highlight the newly chosen recipe (even for re-transmutation).
			auto& state = _runewordState.instanceStates[selectedKey];
			if (state.recipeToken != recipe.token) {
				state.recipeToken = recipe.token;
				state.insertedRunes = 0u;
			}
		}

		std::string runes;
		for (std::size_t i = 0; i < recipe.runeIds.size(); ++i) {
			if (i > 0) {
				runes.push_back('-');
			}
			runes.append(recipe.runeIds[i]);
		}

		std::string note = "Runeword Recipe: " + recipe.displayName + " [" + runes + "]";
		if (_runewordState.selectedBaseKey) {
			AppendRunewordSelectionRecommendation(note, recipe, ResolveInstanceLootType(*_runewordState.selectedBaseKey));
		}

		EmitHudNotification(note.c_str());
		return true;
	}
}
